/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/usb.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

LV_IMG_DECLARE(lily58_matrix_0);
LV_IMG_DECLARE(lily58_matrix_1);
LV_IMG_DECLARE(lily58_matrix_2);
LV_IMG_DECLARE(lily58_matrix_3);

struct lily58_oled_state {
    lv_obj_t *status_root;
    lv_obj_t *anim_root;
    lv_obj_t *matrix_image;
    lv_obj_t *output_label;
    lv_obj_t *battery_label;
    lv_obj_t *layer_label;
    lv_obj_t *wpm_label;
    bool right_half;
    bool show_status;
    uint8_t matrix_frame_index;
};

static const lv_img_dsc_t *const matrix_frames[] = {
    &lily58_matrix_0,
    &lily58_matrix_1,
    &lily58_matrix_2,
    &lily58_matrix_3,
};

static struct lily58_oled_state oled_state;
static lv_timer_t *status_timer;
static lv_timer_t *matrix_timer;
static lv_timer_t *mode_timer;

static lv_obj_t *create_root(lv_obj_t *parent) {
    lv_obj_t *root = lv_obj_create(parent);

    lv_obj_set_size(root, 128, 32);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, LV_PART_MAIN);

    return root;
}

static lv_obj_t *create_status_label(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t width,
                                     lv_text_align_t align) {
    lv_obj_t *label = lv_label_create(parent);

    lv_obj_set_width(label, width);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    lv_label_set_text(label, "");

    return label;
}

static void set_mode(struct lily58_oled_state *state, bool show_status) {
    state->show_status = show_status;

    if (show_status) {
        lv_obj_clear_flag(state->status_root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(state->anim_root, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(state->status_root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(state->anim_root, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_battery_label(struct lily58_oled_state *state) {
    char text[8] = {};
    uint8_t level = bt_bas_get_battery_level();

#if IS_ENABLED(CONFIG_USB)
    if (zmk_usb_is_powered()) {
        strcat(text, LV_SYMBOL_CHARGE);
    }
#endif

    if (level > 95) {
        strcat(text, LV_SYMBOL_BATTERY_FULL);
    } else if (level > 65) {
        strcat(text, LV_SYMBOL_BATTERY_3);
    } else if (level > 35) {
        strcat(text, LV_SYMBOL_BATTERY_2);
    } else if (level > 5) {
        strcat(text, LV_SYMBOL_BATTERY_1);
    } else {
        strcat(text, LV_SYMBOL_BATTERY_EMPTY);
    }

    lv_label_set_text(state->battery_label, text);
}

static void update_output_label(struct lily58_oled_state *state) {
    char text[18] = {};

    if (state->right_half) {
        lv_label_set_text(state->output_label, "PERIPH");
        return;
    }

#if IS_ENABLED(CONFIG_USB)
    if (zmk_usb_is_powered()) {
        snprintf(text, sizeof(text), LV_SYMBOL_USB " USB");
    } else
#endif
    {
        snprintf(text, sizeof(text), LV_SYMBOL_WIFI " BLE");
    }

    lv_label_set_text(state->output_label, text);
}

static void update_layer_label(struct lily58_oled_state *state) {
    if (state->right_half) {
        lv_label_set_text(state->layer_label, "RIGHT");
        return;
    }

    lv_label_set_text(state->layer_label, LV_SYMBOL_KEYBOARD " READY");
}

static void update_wpm_label(struct lily58_oled_state *state) {
    if (state->right_half) {
        lv_label_set_text(state->wpm_label, "SPLIT");
        return;
    }

    lv_label_set_text(state->wpm_label, "MTRX");
}

static void refresh_status(struct lily58_oled_state *state) {
    update_output_label(state);
    update_battery_label(state);
    update_layer_label(state);
    update_wpm_label(state);
}

static void status_tick_cb(lv_timer_t *timer) {
    refresh_status(timer->user_data);
}

static void matrix_tick_cb(lv_timer_t *timer) {
    struct lily58_oled_state *state = timer->user_data;

    state->matrix_frame_index = (state->matrix_frame_index + 1) % ARRAY_SIZE(matrix_frames);
    lv_img_set_src(state->matrix_image, matrix_frames[state->matrix_frame_index]);
}

static void mode_tick_cb(lv_timer_t *timer) {
    struct lily58_oled_state *state = timer->user_data;

    set_mode(state, !state->show_status);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_set_size(screen, 128, 32);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    memset(&oled_state, 0, sizeof(oled_state));
    oled_state.right_half = IS_ENABLED(LILY58_OLED_ART_RIGHT);
    oled_state.show_status = true;

    oled_state.status_root = create_root(screen);
    oled_state.anim_root = create_root(screen);
    oled_state.matrix_image = lv_img_create(oled_state.anim_root);
    lv_img_set_src(oled_state.matrix_image, matrix_frames[0]);
    lv_obj_set_pos(oled_state.matrix_image, 0, 0);

    oled_state.output_label = create_status_label(oled_state.status_root, 0, 0, 80, LV_TEXT_ALIGN_LEFT);
    oled_state.battery_label =
        create_status_label(oled_state.status_root, 94, 0, 34, LV_TEXT_ALIGN_RIGHT);
    oled_state.layer_label = create_status_label(oled_state.status_root, 0, 20, 92, LV_TEXT_ALIGN_LEFT);
    oled_state.wpm_label = create_status_label(oled_state.status_root, 96, 20, 32, LV_TEXT_ALIGN_RIGHT);

    refresh_status(&oled_state);
    set_mode(&oled_state, true);

    if (status_timer != NULL) {
        lv_timer_del(status_timer);
    }

    if (matrix_timer != NULL) {
        lv_timer_del(matrix_timer);
    }

    if (mode_timer != NULL) {
        lv_timer_del(mode_timer);
    }

    status_timer = lv_timer_create(status_tick_cb, 1000, &oled_state);
    matrix_timer = lv_timer_create(matrix_tick_cb, 140, &oled_state);
    mode_timer = lv_timer_create(mode_tick_cb, 8000, &oled_state);

    return screen;
}
