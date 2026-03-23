/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

LV_IMG_DECLARE(lily58_matrix_0);
LV_IMG_DECLARE(lily58_matrix_1);
LV_IMG_DECLARE(lily58_matrix_2);
LV_IMG_DECLARE(lily58_matrix_3);

struct matrix_animation_state {
    lv_obj_t *image;
    uint8_t frame_index;
};

static const lv_img_dsc_t *const matrix_frames[] = {
    &lily58_matrix_0,
    &lily58_matrix_1,
    &lily58_matrix_2,
    &lily58_matrix_3,
};

static struct matrix_animation_state matrix_animation_state;
static lv_timer_t *matrix_timer;

static void matrix_tick_cb(lv_timer_t *timer) {
    struct matrix_animation_state *state = timer->user_data;

    state->frame_index = (state->frame_index + 1) % ARRAY_SIZE(matrix_frames);
    lv_img_set_src(state->image, matrix_frames[state->frame_index]);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *matrix_image;

    lv_obj_set_size(screen, 128, 32);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    matrix_image = lv_img_create(screen);
    lv_img_set_src(matrix_image, matrix_frames[0]);
    lv_obj_set_pos(matrix_image, 0, 0);

    matrix_animation_state.image = matrix_image;
    matrix_animation_state.frame_index = 0;

    if (matrix_timer != NULL) {
        lv_timer_del(matrix_timer);
    }

    matrix_timer = lv_timer_create(matrix_tick_cb, 280, &matrix_animation_state);

    return screen;
}
