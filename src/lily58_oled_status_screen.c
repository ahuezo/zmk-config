/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

LV_IMG_DECLARE(lily58_fish_left);
LV_IMG_DECLARE(lily58_fish_right);
LV_IMG_DECLARE(lily58_bubbles_0);
LV_IMG_DECLARE(lily58_bubbles_1);
LV_IMG_DECLARE(lily58_bubbles_2);

struct bubble_animation_state {
    lv_obj_t *bubble_image;
    uint8_t frame_index;
};

static const lv_img_dsc_t *const bubble_frames[] = {
    &lily58_bubbles_0,
    &lily58_bubbles_1,
    &lily58_bubbles_2,
};

static struct bubble_animation_state bubble_animation_state;
static lv_timer_t *bubble_timer;

static void bubble_tick_cb(lv_timer_t *timer) {
    struct bubble_animation_state *state = timer->user_data;

    state->frame_index = (state->frame_index + 1) % ARRAY_SIZE(bubble_frames);
    lv_img_set_src(state->bubble_image, bubble_frames[state->frame_index]);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *fish_image;
    lv_obj_t *bubble_image;
    const lv_img_dsc_t *fish_art;
    bool right_half = IS_ENABLED(LILY58_OLED_ART_RIGHT);

    lv_obj_set_size(screen, 128, 32);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    fish_art = right_half ? &lily58_fish_right : &lily58_fish_left;

    fish_image = lv_img_create(screen);
    lv_img_set_src(fish_image, fish_art);
    lv_obj_set_pos(fish_image, right_half ? 64 : 0, 4);

    bubble_image = lv_img_create(screen);
    lv_img_set_src(bubble_image, bubble_frames[0]);
    lv_obj_set_pos(bubble_image, right_half ? 40 : 48, 0);

    bubble_animation_state.bubble_image = bubble_image;
    bubble_animation_state.frame_index = 0;

    if (bubble_timer != NULL) {
        lv_timer_del(bubble_timer);
    }

    bubble_timer = lv_timer_create(bubble_tick_cb, 500, &bubble_animation_state);

    return screen;
}
