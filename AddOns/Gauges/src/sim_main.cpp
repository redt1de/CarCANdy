// Headless PC build: runs the gauge against a fake clock and writes PNG frames.
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "app.h"

#define HOR_RES 480
#define VER_RES 480
#define STEP_MS 16

static uint32_t fake_ms;
static uint16_t framebuffer[HOR_RES * VER_RES];
static uint8_t draw_buf[HOR_RES * 60 * 2];

static uint32_t tick_cb(void)
{
    return fake_ms;
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t w = lv_area_get_width(area);
    for (int32_t y = area->y1; y <= area->y2; y++) {
        memcpy(&framebuffer[y * HOR_RES + area->x1], px_map, w * 2);
        px_map += w * 2;
    }
    lv_display_flush_ready(disp);
}

static void put_be32(std::vector<uint8_t> &out, uint32_t v)
{
    for (int shift = 24; shift >= 0; shift -= 8) out.push_back((uint8_t)(v >> shift));
}

static uint32_t crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xedb88320 & -(crc & 1));
    }
    return ~crc;
}

static void put_chunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &body)
{
    put_be32(out, (uint32_t)body.size());
    size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), body.begin(), body.end());
    put_be32(out, crc32(&out[start], out.size() - start));
}

// Minimal PNG writer (uncompressed deflate) so the sim needs no image library.
static bool save_png(const std::string &path)
{
    std::vector<uint8_t> raw;
    raw.reserve((HOR_RES * 3 + 1) * VER_RES);
    for (int y = 0; y < VER_RES; y++) {
        raw.push_back(0);
        for (int x = 0; x < HOR_RES; x++) {
            // Grey out what the round panel can't show.
            int dx = 2 * x - (HOR_RES - 1), dy = 2 * y - (VER_RES - 1);
            if (dx * dx + dy * dy > HOR_RES * HOR_RES) {
                raw.insert(raw.end(), {0x30, 0x30, 0x30});
                continue;
            }
            uint16_t px = framebuffer[y * HOR_RES + x];
            uint8_t r = (px >> 11) & 0x1f, g = (px >> 5) & 0x3f, b = px & 0x1f;
            raw.push_back((uint8_t)(r << 3 | r >> 2));
            raw.push_back((uint8_t)(g << 2 | g >> 4));
            raw.push_back((uint8_t)(b << 3 | b >> 2));
        }
    }

    std::vector<uint8_t> idat = {0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    for (size_t pos = 0; pos < raw.size(); pos += 65535) {
        size_t len = raw.size() - pos < 65535 ? raw.size() - pos : 65535;
        idat.push_back(pos + len == raw.size());
        idat.push_back(len & 0xff);
        idat.push_back(len >> 8);
        idat.push_back(~len & 0xff);
        idat.push_back((~len >> 8) & 0xff);
        idat.insert(idat.end(), raw.begin() + pos, raw.begin() + pos + len);
    }
    put_be32(idat, b << 16 | a);

    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    std::vector<uint8_t> ihdr;
    put_be32(ihdr, HOR_RES);
    put_be32(ihdr, VER_RES);
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
    put_chunk(png, "IHDR", ihdr);
    put_chunk(png, "IDAT", idat);
    put_chunk(png, "IEND", {});

    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(png.data(), 1, png.size(), f) == png.size();
    fclose(f);
    return ok;
}

int main(int argc, char **argv)
{
    std::string out_dir = argc > 1 ? argv[1] : "sim_out";
    uint32_t duration_ms = argc > 2 ? (uint32_t)atoi(argv[2]) : 20000;
    uint32_t frame_every_ms = argc > 3 ? (uint32_t)atoi(argv[3]) : 1000;
    if (system(("mkdir -p '" + out_dir + "'").c_str()) != 0) return 1;

    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *disp = lv_display_create(HOR_RES, VER_RES);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    app_init();
    // Optional 4th and 5th arguments pick a gauge (BOOST, AFR, ...) and a colour
    // scheme (RED, BLUE, ...) by name, overriding config.h.
    if (argc > 4) {
        for (int t = 0; t < GAUGE_TYPE_COUNT; t++) {
            if (strcmp(argv[4], gauge_spec((GaugeType)t).name) == 0) app_set_gauge((GaugeType)t);
        }
    }
    if (argc > 5) {
        for (int c = 0; c < GAUGE_COLOR_COUNT; c++) {
            if (strcmp(argv[5], gauge_colors((GaugeColor)c).name) == 0) app_set_color((GaugeColor)c);
        }
    }

    for (fake_ms = 0; fake_ms <= duration_ms; fake_ms += STEP_MS) {
        lv_timer_handler();
        if (fake_ms % frame_every_ms < STEP_MS) {
            char name[32];
            snprintf(name, sizeof(name), "/frame_%06u.png", (unsigned)fake_ms);
            if (!save_png(out_dir + name)) {
                fprintf(stderr, "cannot write %s%s\n", out_dir.c_str(), name);
                return 1;
            }
        }
    }
    printf("frames written to %s/\n", out_dir.c_str());
    return 0;
}
