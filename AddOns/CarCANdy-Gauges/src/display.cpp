// Panel bring-up for the Waveshare ESP32-S3-Touch-LCD-2.8C: a 480x480 round
// ST7701 panel driven over 16-bit parallel RGB. The controller is configured
// once over 9-bit SPI, with its reset and chip-select lines hanging off a
// TCA9554 I2C expander. Pins, timings and the register sequence come from
// Waveshare's demo for this board.
#include "display.h"
#include <Arduino.h>
#include <Wire.h>
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"

#define LCD_H_RES 480
#define LCD_V_RES 480

#define PIN_I2C_SDA 15
#define PIN_I2C_SCL 7
#define PIN_LCD_SPI_CLK 2
#define PIN_LCD_SPI_MOSI 1
#define PIN_LCD_BACKLIGHT 6
#define PIN_LCD_HSYNC 38
#define PIN_LCD_VSYNC 39
#define PIN_LCD_DE 40
#define PIN_LCD_PCLK 41

#define TCA9554_ADDR 0x20
#define TCA9554_REG_OUTPUT 0x01
#define TCA9554_REG_CONFIG 0x03
// Expander outputs (bit positions)
#define EXIO_LCD_RESET 0
#define EXIO_LCD_CS 2
#define EXIO_BUZZER 7

#define BACKLIGHT_FREQ_HZ 20000
#define BACKLIGHT_RES_BITS 10

#define DRAW_BUF_LINES 60

static esp_lcd_panel_handle_t panel;
static spi_device_handle_t init_spi;
static uint8_t exio_state = 0xff;

static void exio_write_reg(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(TCA9554_ADDR);
    Wire.write(reg);
    Wire.write(value);
    if (Wire.endTransmission() != 0) Serial.println("display: TCA9554 not responding");
}

static void exio_set(uint8_t bit, bool level)
{
    if (level) exio_state |= (1 << bit);
    else exio_state &= ~(1 << bit);
    exio_write_reg(TCA9554_REG_OUTPUT, exio_state);
}

// ST7701 3-wire SPI: one D/C bit followed by 8 data bits.
static void st7701_write(bool is_data, uint8_t value)
{
    spi_transaction_t t = {};
    t.cmd = is_data ? 1 : 0;
    t.addr = value;
    spi_device_transmit(init_spi, &t);
}

#define DELAY_AFTER 0x80 // flag in the length byte: wait 120 ms after the command

// {command, length | flags, data...}
static const uint8_t st7701_init_seq[] = {
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    0xEF, 1, 0x08,
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
    0xC0, 2, 0x3B, 0x00,
    0xC1, 2, 0x10, 0x0C,
    0xC2, 2, 0x07, 0x0A,
    0xC7, 1, 0x00,
    0xCC, 1, 0x10,
    0xCD, 1, 0x08,
    0xB0, 16, 0x05, 0x12, 0x98, 0x0E, 0x0F, 0x07, 0x07, 0x09, 0x09, 0x23, 0x05, 0x52, 0x0F, 0x67, 0x2C, 0x11,
    0xB1, 16, 0x0B, 0x11, 0x97, 0x0C, 0x12, 0x06, 0x06, 0x08, 0x08, 0x22, 0x03, 0x51, 0x11, 0x66, 0x2B, 0x0F,
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x11,
    0xB0, 1, 0x5D,
    0xB1, 1, 0x3E,
    0xB2, 1, 0x81,
    0xB3, 1, 0x80,
    0xB5, 1, 0x4E,
    0xB7, 1, 0x85,
    0xB8, 1, 0x20,
    0xC1, 1, 0x78,
    0xC2, 1, 0x78,
    0xD0, 1, 0x88,
    0xE0, 3, 0x00, 0x00, 0x02,
    0xE1, 11, 0x06, 0x30, 0x08, 0x30, 0x05, 0x30, 0x07, 0x30, 0x00, 0x33, 0x33,
    0xE2, 12, 0x11, 0x11, 0x33, 0x33, 0xF4, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00,
    0xE3, 4, 0x00, 0x00, 0x11, 0x11,
    0xE4, 2, 0x44, 0x44,
    0xE5, 16, 0x0D, 0xF5, 0x30, 0xF0, 0x0F, 0xF7, 0x30, 0xF0, 0x09, 0xF1, 0x30, 0xF0, 0x0B, 0xF3, 0x30, 0xF0,
    0xE6, 4, 0x00, 0x00, 0x11, 0x11,
    0xE7, 2, 0x44, 0x44,
    0xE8, 16, 0x0C, 0xF4, 0x30, 0xF0, 0x0E, 0xF6, 0x30, 0xF0, 0x08, 0xF0, 0x30, 0xF0, 0x0A, 0xF2, 0x30, 0xF0,
    0xE9, 2, 0x36, 0x01,
    0xEB, 7, 0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40,
    0xED, 16, 0xFF, 0x10, 0xAF, 0x76, 0x54, 0x2B, 0xCF, 0xFF, 0xFF, 0xFC, 0xB2, 0x45, 0x67, 0xFA, 0x01, 0xFF,
    0xEF, 6, 0x08, 0x08, 0x08, 0x45, 0x3F, 0x54,
    0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    0x11, DELAY_AFTER,
    0x3A, 1, 0x66,
    0x36, 1, 0x00,
    0x35, 1, 0x00,
    0x29, 0,
};

static void st7701_configure(void)
{
    spi_bus_config_t bus = {};
    bus.mosi_io_num = PIN_LCD_SPI_MOSI;
    bus.miso_io_num = -1;
    bus.sclk_io_num = PIN_LCD_SPI_CLK;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 64;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED));

    spi_device_interface_config_t dev = {};
    dev.command_bits = 1;
    dev.address_bits = 8;
    dev.mode = 0;
    dev.clock_speed_hz = 4 * 1000 * 1000;
    dev.spics_io_num = -1;
    dev.queue_size = 1;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &init_spi));

    exio_set(EXIO_LCD_RESET, false);
    delay(10);
    exio_set(EXIO_LCD_RESET, true);
    delay(50);

    exio_set(EXIO_LCD_CS, false);
    delay(10);
    for (size_t i = 0; i < sizeof(st7701_init_seq);) {
        uint8_t cmd = st7701_init_seq[i++];
        uint8_t len = st7701_init_seq[i++];
        st7701_write(false, cmd);
        for (uint8_t n = len & ~DELAY_AFTER; n > 0; n--) st7701_write(true, st7701_init_seq[i++]);
        if (len & DELAY_AFTER) delay(120);
    }
    exio_set(EXIO_LCD_CS, true);
    delay(10);

    // These two pins are shared with the SD card slot, so hand the bus back.
    spi_bus_remove_device(init_spi);
    spi_bus_free(SPI2_HOST);
}

static void rgb_panel_start(void)
{
    esp_lcd_rgb_panel_config_t cfg = {};
    cfg.clk_src = LCD_CLK_SRC_PLL240M;
    // cfg.timings.pclk_hz = 30 * 1000 * 1000;
    cfg.timings.pclk_hz = 16 * 1000 * 1000;  //<<<<<<<<<<<<<<<<<<<<<< jumpy fixy
    cfg.timings.h_res = LCD_H_RES;
    cfg.timings.v_res = LCD_V_RES;
    cfg.timings.hsync_pulse_width = 8;
    cfg.timings.hsync_back_porch = 10;
    cfg.timings.hsync_front_porch = 50;
    cfg.timings.vsync_pulse_width = 2;
    cfg.timings.vsync_back_porch = 18;
    cfg.timings.vsync_front_porch = 8;
    cfg.timings.flags.pclk_active_neg = 0;
    cfg.data_width = 16;
    cfg.bits_per_pixel = 16;
    cfg.num_fbs = 1;
    // Bounce buffer keeps the picture from drifting while the PSRAM frame buffer is busy.
    // cfg.bounce_buffer_size_px = 10 * LCD_H_RES;
     cfg.bounce_buffer_size_px = 20 * LCD_H_RES; //<<<<<<<<<<<<<<<<<<<<<< jumpy fixy


    cfg.hsync_gpio_num = PIN_LCD_HSYNC;
    cfg.vsync_gpio_num = PIN_LCD_VSYNC;
    cfg.de_gpio_num = PIN_LCD_DE;
    cfg.pclk_gpio_num = PIN_LCD_PCLK;
    cfg.disp_gpio_num = -1;
    static const int data_pins[16] = {5, 45, 48, 47, 21, 14, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17};
    for (int i = 0; i < 16; i++) cfg.data_gpio_nums[i] = (gpio_num_t)data_pins[i];
    cfg.flags.fb_in_psram = true;

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(disp);
}

void display_set_backlight(uint8_t percent)
{
    if (percent > 100) percent = 100;
    ledcWrite(PIN_LCD_BACKLIGHT, ((1u << BACKLIGHT_RES_BITS) * percent) / 100);
}

lv_display_t *display_init(void)
{
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    // Latch output levels before switching the expander pins to outputs: buzzer off, everything else idle high.
    exio_state = 0xff & ~(1 << EXIO_BUZZER);
    exio_write_reg(TCA9554_REG_OUTPUT, exio_state);
    exio_write_reg(TCA9554_REG_CONFIG, 0x00);

    st7701_configure();
    rgb_panel_start();

    size_t buf_size = LCD_H_RES * DRAW_BUF_LINES * sizeof(uint16_t);
    void *buf = heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf) buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    assert(buf);

    lv_display_t *disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, buf, NULL, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    ledcAttach(PIN_LCD_BACKLIGHT, BACKLIGHT_FREQ_HZ, BACKLIGHT_RES_BITS);
    display_set_backlight(80);
    return disp;
}
