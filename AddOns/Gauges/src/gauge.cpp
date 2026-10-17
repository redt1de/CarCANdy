#include "gauge.h"
#include <math.h>
#include <string.h>

#define COL_FACE lv_color_hex(0x000000)
#define COL_HUB  lv_color_hex(0x060606)

// How bright the derived shades are relative to the scheme's marking colour.
#define DIM_PERCENT 61
#define HUB_RING_PERCENT 23

// 270 degree sweep opening at the bottom, like the stock tach and speedo.
#define SWEEP_START_DEG 135
#define SWEEP_DEG 270

#define NEEDLE_LEN 190
// Peak marker: a short bar just past the needle tip, reaching out beyond the major ticks.
#define PEAK_MARK_INNER 194
#define PEAK_MARK_OUTER 228
// The lv_scale range is in integer steps; this many across the dial.
#define SCALE_STEPS 10000

static const GaugeSpec *spec;
static struct {
    lv_color_t mark;
    lv_color_t mark_dim;
    lv_color_t needle;
    lv_color_t needle_glow;
    lv_color_t hub_ring;
} pal;
static lv_obj_t *face;
static lv_obj_t *needle_lines[3];
static lv_point_precise_t needle_points[2];
static lv_obj_t *peak_lines[2];
static lv_point_precise_t peak_points[2];
static lv_obj_t *value_label;
static lv_obj_t *peak_label;

static float shown_value;
static bool value_shown;
static float shown_peak;
static bool peak_shown;

static int32_t to_scale(float value)
{
    return (int32_t)lroundf((value - spec->min) / (spec->max - spec->min) * SCALE_STEPS);
}

static lv_color_t shade(uint32_t rgb, uint32_t percent)
{
    return lv_color_make((uint8_t)((rgb >> 16 & 0xff) * percent / 100), (uint8_t)((rgb >> 8 & 0xff) * percent / 100),
                         (uint8_t)((rgb & 0xff) * percent / 100));
}

static lv_obj_t *needle_line_create(lv_obj_t *parent, lv_color_t color, int32_t width, lv_opa_t opa)
{
    lv_obj_t *line = lv_line_create(parent);
    lv_obj_set_style_line_color(line, color, 0);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_opa(line, opa, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    return line;
}

static lv_obj_t *caption_create(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color,
                                int32_t letter_space, int32_t x_ofs, int32_t y_ofs)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_letter_space(label, letter_space, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, x_ofs, y_ofs);
    return label;
}

static void set_number_text(lv_obj_t *label, const char *prefix, float value)
{
    int32_t pow10 = 1;
    for (uint8_t i = 0; i < spec->decimals; i++) pow10 *= 10;
    int32_t scaled = (int32_t)lroundf(value * pow10);
    int32_t mag = LV_ABS(scaled);
    const char *sign = scaled < 0 ? "-" : "";
    if (spec->decimals == 0) lv_label_set_text_fmt(label, "%s%s%d", prefix, sign, (int)mag);
    else lv_label_set_text_fmt(label, "%s%s%d.%0*d", prefix, sign, (int)(mag / pow10), (int)spec->decimals, (int)(mag % pow10));
}

// Points a set of stacked lines (a core plus its glow strokes) along the dial
// radius at `value`, from r_in to r_out pixels out from the centre.
static void radial_lines_set(lv_obj_t *const *lines, size_t count, lv_point_precise_t *points, float value,
                             int32_t r_in, int32_t r_out)
{
    float frac = (value - spec->min) / (spec->max - spec->min);
    float angle = (SWEEP_START_DEG + SWEEP_DEG * frac) * (float)M_PI / 180.0f;
    float dx = cosf(angle), dy = sinf(angle);
    int32_t cx = lv_obj_get_width(face) / 2;
    int32_t cy = lv_obj_get_height(face) / 2;
    int32_t x0 = cx + (int32_t)lroundf(dx * r_in), y0 = cy + (int32_t)lroundf(dy * r_in);
    int32_t x1 = cx + (int32_t)lroundf(dx * r_out), y1 = cy + (int32_t)lroundf(dy * r_out);

    // Each line object only spans its own bounding box, so a move redraws
    // that box rather than a whole quadrant of the face.
    int32_t x = LV_MIN(x0, x1);
    int32_t y = LV_MIN(y0, y1);
    lv_point_precise_t p0 = {(lv_value_precise_t)(x0 - x), (lv_value_precise_t)(y0 - y)};
    lv_point_precise_t p1 = {(lv_value_precise_t)(x1 - x), (lv_value_precise_t)(y1 - y)};
    if (p0.x == points[0].x && p0.y == points[0].y && p1.x == points[1].x && p1.y == points[1].y &&
        lv_obj_get_x_aligned(lines[0]) == x && lv_obj_get_y_aligned(lines[0]) == y) {
        return;
    }
    points[0] = p0;
    points[1] = p1;
    for (size_t i = 0; i < count; i++) {
        lv_obj_set_pos(lines[i], x, y);
        lv_line_set_points_mutable(lines[i], points, 2);
    }
}

void gauge_create(lv_obj_t *parent, const GaugeSpec &new_spec, const GaugeColors &colors)
{
    spec = &new_spec;
    pal.mark = lv_color_hex(colors.markings);
    pal.mark_dim = shade(colors.markings, DIM_PERCENT);
    pal.needle = lv_color_hex(colors.needle);
    pal.needle_glow = pal.mark;
    pal.hub_ring = shade(colors.markings, HUB_RING_PERCENT);
    face = parent;
    value_shown = false;
    peak_shown = false;

    lv_obj_clean(parent);
    lv_obj_set_style_bg_color(parent, COL_FACE, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(parent);

    lv_obj_t *scale = lv_scale_create(parent);
    lv_obj_set_size(scale, 444, 444);
    lv_obj_center(scale);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(scale, 0, SCALE_STEPS);
    lv_scale_set_angle_range(scale, SWEEP_DEG);
    lv_scale_set_rotation(scale, SWEEP_START_DEG);
    lv_scale_set_total_tick_count(scale, spec->tick_count);
    lv_scale_set_major_tick_every(scale, spec->major_every);
    lv_scale_set_label_show(scale, true);
    lv_scale_set_text_src(scale, (const char **)spec->labels);

    lv_obj_set_style_arc_opa(scale, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_line_color(scale, pal.mark, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, 6, LV_PART_INDICATOR);
    lv_obj_set_style_length(scale, 30, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(scale, pal.mark, LV_PART_INDICATOR);
    // Three-digit labels are set a size smaller and further in, so they clear both
    // the ticks at 3 and 9 o'clock and the caption.
    size_t longest_label = 0;
    for (const char *const *label = spec->labels; *label; label++) longest_label = LV_MAX(longest_label, strlen(*label));
    bool wide_labels = longest_label > 2;
    lv_obj_set_style_text_font(scale, wide_labels ? &lv_font_montserrat_28 : &lv_font_montserrat_32, LV_PART_INDICATOR);
    lv_obj_set_style_pad_radial(scale, wide_labels ? 20 : 14, LV_PART_INDICATOR);

    lv_obj_set_style_line_color(scale, pal.mark, LV_PART_ITEMS);
    lv_obj_set_style_line_width(scale, 3, LV_PART_ITEMS);
    lv_obj_set_style_length(scale, 15, LV_PART_ITEMS);

    // The low end of some dials (vacuum, cold) is dimmed so the range that matters reads at a glance.
    if (spec->dim_below > spec->min) {
        static lv_style_t dim_major, dim_minor;
        static bool styles_ready;
        if (!styles_ready) {
            styles_ready = true;
            lv_style_init(&dim_major);
            lv_style_init(&dim_minor);
        }
        lv_style_set_line_color(&dim_major, pal.mark_dim);
        lv_style_set_text_color(&dim_major, pal.mark_dim);
        lv_style_set_line_color(&dim_minor, pal.mark_dim);
        lv_scale_section_t *dim = lv_scale_add_section(scale);
        lv_scale_set_section_range(scale, dim, 0, to_scale(spec->dim_below) - 1);
        lv_scale_set_section_style_indicator(scale, dim, &dim_major);
        lv_scale_set_section_style_items(scale, dim, &dim_minor);
    }

    // Peak marker, like the tell-tale on a mechanical gauge: drawn in the needle's
    // colour so it stands apart from the ticks, and under the needle.
    peak_lines[0] = needle_line_create(parent, pal.needle_glow, 11, LV_OPA_40);
    peak_lines[1] = needle_line_create(parent, pal.needle, 5, LV_OPA_COVER);
    peak_points[0] = peak_points[1] = {0, 0};

    // Needle: a bright core over two translucent wider strokes to fake the backlit glow.
    needle_lines[0] = needle_line_create(parent, pal.needle_glow, 18, LV_OPA_20);
    needle_lines[1] = needle_line_create(parent, pal.needle_glow, 11, LV_OPA_40);
    needle_lines[2] = needle_line_create(parent, pal.needle, 5, LV_OPA_COVER);
    needle_points[0] = needle_points[1] = {0, 0};

    // Thin inner ring echoing the arc around the stock gauge hubs.
    lv_obj_t *ring = lv_arc_create(parent);
    lv_obj_set_size(ring, 176, 176);
    lv_obj_center(ring);
    lv_arc_set_bg_angles(ring, SWEEP_START_DEG, SWEEP_START_DEG + SWEEP_DEG - 360);
    lv_obj_set_style_arc_width(ring, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring, pal.mark_dim, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ring, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_remove_style(ring, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_CLICKABLE);

    caption_create(parent, spec->caption, &lv_font_montserrat_14, pal.mark, 3, 0, -104);
    if (spec->note) {
        caption_create(parent, spec->note, &lv_font_montserrat_14, pal.mark_dim, 2, spec->note_x, spec->note_y);
    }

    value_label = caption_create(parent, "", &lv_font_montserrat_48, pal.mark, 0, 0, 132);
    caption_create(parent, spec->unit, &lv_font_montserrat_20, pal.mark, 3, 0, 174);
    peak_label = caption_create(parent, "", &lv_font_montserrat_14, pal.mark_dim, 2, 0, 204);

    lv_obj_t *hub = lv_obj_create(parent);
    lv_obj_remove_style_all(hub);
    lv_obj_set_size(hub, 92, 92);
    lv_obj_center(hub);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub, COL_HUB, 0);
    lv_obj_set_style_bg_opa(hub, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hub, pal.hub_ring, 0);
    lv_obj_set_style_border_width(hub, 2, 0);

    gauge_set(spec->min);
    gauge_set_peak(spec->min);
}

void gauge_set(float value)
{
    value = LV_CLAMP(spec->min, value, spec->max);
    radial_lines_set(needle_lines, 3, needle_points, value, 0, NEEDLE_LEN);

    // The digits get a deadband so sensor flutter doesn't make them flicker.
    if (!value_shown || fabsf(value - shown_value) >= spec->readout_deadband) {
        value_shown = true;
        shown_value = roundf(value / spec->readout_step) * spec->readout_step;
        set_number_text(value_label, "", shown_value);
    }
}

void gauge_set_peak(float value)
{
    value = LV_CLAMP(spec->min, value, spec->max);
    radial_lines_set(peak_lines, 2, peak_points, value, PEAK_MARK_INNER, PEAK_MARK_OUTER);

    if (!spec->show_peak) return;
    // The printed peak is only worth showing once it is out of the dimmed low range.
    float threshold = LV_MAX(spec->min, spec->dim_below);
    if (value <= threshold) {
        if (peak_shown) lv_label_set_text(peak_label, "");
        peak_shown = false;
    } else if (!peak_shown || fabsf(value - shown_peak) >= spec->readout_step / 2) {
        peak_shown = true;
        shown_peak = value;
        set_number_text(peak_label, "PEAK ", value);
    }
}
