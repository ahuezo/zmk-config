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

#define OLED_PAGE_COUNT 3

struct bubble_animation_state {
    lv_obj_t *bubble_images[2];
    uint8_t frame_index;
};

struct page_cycle_state {
    lv_obj_t *pages[OLED_PAGE_COUNT];
    uint8_t page_index;
};

static const lv_img_dsc_t *const bubble_frames[] = {
    &lily58_bubbles_0,
    &lily58_bubbles_1,
    &lily58_bubbles_2,
};

static struct bubble_animation_state bubble_animation_state;
static struct page_cycle_state page_cycle_state;
static lv_timer_t *bubble_timer;
static lv_timer_t *page_timer;

static void style_page(lv_obj_t *page) {
    lv_obj_set_size(page, 128, 32);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
}

static lv_obj_t *create_label(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y,
                              lv_text_align_t align) {
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    lv_obj_set_pos(label, x, y);

    return label;
}

static lv_obj_t *create_box(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *box = lv_obj_create(parent);

    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, x, y);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(box, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box, 2, LV_PART_MAIN);

    return box;
}

static lv_obj_t *create_animation_page(lv_obj_t *parent, const lv_img_dsc_t *fish_art, bool right_half,
                                       lv_obj_t **bubble_out) {
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_t *fish_image;
    lv_obj_t *bubble_image;

    style_page(page);

    fish_image = lv_img_create(page);
    lv_img_set_src(fish_image, fish_art);
    lv_obj_set_pos(fish_image, right_half ? 64 : 0, 4);

    bubble_image = lv_img_create(page);
    lv_img_set_src(bubble_image, bubble_frames[0]);
    lv_obj_set_pos(bubble_image, right_half ? 40 : 48, 0);

    *bubble_out = bubble_image;
    return page;
}

static lv_obj_t *create_mixed_page(lv_obj_t *parent, const lv_img_dsc_t *fish_art, bool right_half,
                                   lv_obj_t **bubble_out) {
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_t *fish_image;
    lv_obj_t *bubble_image;
    lv_obj_t *info_box;

    style_page(page);

    fish_image = lv_img_create(page);
    lv_img_set_src(fish_image, fish_art);
    lv_obj_set_pos(fish_image, right_half ? 58 : 6, 4);

    bubble_image = lv_img_create(page);
    lv_img_set_src(bubble_image, bubble_frames[0]);
    lv_obj_set_pos(bubble_image, right_half ? 34 : 54, 0);

    info_box = create_box(page, right_half ? 4 : 74, 3, 50, 26);
    create_label(info_box, right_half ? "RIGHT" : "LEFT", 2, 0, LV_TEXT_ALIGN_LEFT);
    create_label(info_box, "LILY58", 2, 10, LV_TEXT_ALIGN_LEFT);
    create_label(info_box, "FISH", 2, 20, LV_TEXT_ALIGN_LEFT);

    *bubble_out = bubble_image;
    return page;
}

static lv_obj_t *create_status_page(lv_obj_t *parent, bool right_half) {
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_t *title_box;
    lv_obj_t *line1_box;
    lv_obj_t *line2_box;

    style_page(page);

    title_box = create_box(page, 4, 3, 120, 10);
    create_label(title_box, right_half ? "OLED STATUS / RIGHT HALF" : "OLED STATUS / LEFT HALF", 2, -1,
                 LV_TEXT_ALIGN_LEFT);

    line1_box = create_box(page, 4, 16, 58, 12);
    create_label(line1_box, "ANIM  FISH+BUB", 2, 0, LV_TEXT_ALIGN_LEFT);

    line2_box = create_box(page, 66, 16, 58, 12);
    create_label(line2_box, right_half ? "MODE  PERIPH" : "MODE  CENTRAL", 2, 0, LV_TEXT_ALIGN_LEFT);
    return page;
}

static void bubble_tick_cb(lv_timer_t *timer) {
    struct bubble_animation_state *state = timer->user_data;
    size_t i;

    state->frame_index = (state->frame_index + 1) % ARRAY_SIZE(bubble_frames);
    for (i = 0; i < ARRAY_SIZE(state->bubble_images); i++) {
        if (state->bubble_images[i] != NULL) {
            lv_img_set_src(state->bubble_images[i], bubble_frames[state->frame_index]);
        }
    }
}

static void page_tick_cb(lv_timer_t *timer) {
    struct page_cycle_state *state = timer->user_data;

    lv_obj_add_flag(state->pages[state->page_index], LV_OBJ_FLAG_HIDDEN);
    state->page_index = (state->page_index + 1) % OLED_PAGE_COUNT;
    lv_obj_clear_flag(state->pages[state->page_index], LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    const lv_img_dsc_t *fish_art;
    bool right_half = IS_ENABLED(LILY58_OLED_ART_RIGHT);
    uint8_t i;

    lv_obj_set_size(screen, 128, 32);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    fish_art = right_half ? &lily58_fish_right : &lily58_fish_left;

    for (i = 0; i < ARRAY_SIZE(bubble_animation_state.bubble_images); i++) {
        bubble_animation_state.bubble_images[i] = NULL;
    }
    bubble_animation_state.frame_index = 0;
    page_cycle_state.page_index = 0;

    page_cycle_state.pages[0] =
        create_animation_page(screen, fish_art, right_half, &bubble_animation_state.bubble_images[0]);

    page_cycle_state.pages[1] =
        create_mixed_page(screen, fish_art, right_half, &bubble_animation_state.bubble_images[1]);
    lv_obj_add_flag(page_cycle_state.pages[1], LV_OBJ_FLAG_HIDDEN);

    page_cycle_state.pages[2] = create_status_page(screen, right_half);
    lv_obj_add_flag(page_cycle_state.pages[2], LV_OBJ_FLAG_HIDDEN);

    if (bubble_timer != NULL) {
        lv_timer_del(bubble_timer);
    }

    bubble_timer = lv_timer_create(bubble_tick_cb, 500, &bubble_animation_state);

    if (page_timer != NULL) {
        lv_timer_del(page_timer);
    }

    page_timer = lv_timer_create(page_tick_cb, 4000, &page_cycle_state);

    return screen;
}
