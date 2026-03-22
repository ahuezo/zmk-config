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
LV_IMG_DECLARE(lily58_fish_left_alt);
LV_IMG_DECLARE(lily58_fish_right_alt);
LV_IMG_DECLARE(lily58_bubbles_0);
LV_IMG_DECLARE(lily58_bubbles_1);
LV_IMG_DECLARE(lily58_bubbles_2);

struct scene_config {
    const lv_img_dsc_t *fish_art;
    lv_coord_t fish_x;
    lv_coord_t fish_y;
    lv_coord_t bubble_x;
    lv_coord_t bubble_y;
};

struct bubble_animation_state {
    lv_obj_t *bubble_image;
    uint8_t frame_index;
};

struct scene_cycle_state {
    lv_obj_t *fish_image;
    lv_obj_t *bubble_image;
    lv_obj_t *panel_box;
    lv_obj_t *panel_title;
    lv_obj_t *panel_line1;
    lv_obj_t *panel_line2;
    uint8_t scene_index;
    bool right_half;
};

static const lv_img_dsc_t *const bubble_frames[] = {
    &lily58_bubbles_0,
    &lily58_bubbles_1,
    &lily58_bubbles_2,
};

static const struct scene_config left_scenes[] = {
    {.fish_art = &lily58_fish_left, .fish_x = 0, .fish_y = 4, .bubble_x = 48, .bubble_y = 0},
    {.fish_art = &lily58_fish_left_alt, .fish_x = 14, .fish_y = 8, .bubble_x = 38, .bubble_y = 2},
    {.fish_art = &lily58_fish_left_alt, .fish_x = 22, .fish_y = 6, .bubble_x = 46, .bubble_y = 5},
};

static const struct scene_config right_scenes[] = {
    {.fish_art = &lily58_fish_right, .fish_x = 64, .fish_y = 4, .bubble_x = 40, .bubble_y = 0},
    {.fish_art = &lily58_fish_right_alt, .fish_x = 82, .fish_y = 8, .bubble_x = 74, .bubble_y = 2},
    {.fish_art = &lily58_fish_right_alt, .fish_x = 74, .fish_y = 6, .bubble_x = 66, .bubble_y = 5},
};

static struct bubble_animation_state bubble_animation_state;
static struct scene_cycle_state scene_cycle_state;
static lv_timer_t *bubble_timer;
static lv_timer_t *scene_timer;

static lv_obj_t *create_panel_box(lv_obj_t *parent, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *box = lv_obj_create(parent);

    lv_obj_set_size(box, 48, 24);
    lv_obj_set_pos(box, x, y);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(box, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box, 1, LV_PART_MAIN);

    return box;
}

static lv_obj_t *create_panel_label(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(label, x, y);

    return label;
}

static void set_panel_hidden(struct scene_cycle_state *state, bool hidden) {
    if (hidden) {
        lv_obj_add_flag(state->panel_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(state->panel_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(state->panel_line1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(state->panel_line2, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(state->panel_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(state->panel_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(state->panel_line1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(state->panel_line2, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_panel(struct scene_cycle_state *state) {
    bool compact_scene = state->scene_index != 0;
    lv_coord_t panel_x = state->right_half ? 4 : 76;

    set_panel_hidden(state, !compact_scene);
    if (!compact_scene) {
        return;
    }

    lv_obj_set_pos(state->panel_box, panel_x, 4);
    lv_obj_set_pos(state->panel_title, panel_x + 4, 5);
    lv_obj_set_pos(state->panel_line1, panel_x + 4, 12);
    lv_obj_set_pos(state->panel_line2, panel_x + 4, 19);

    lv_label_set_text(state->panel_title, state->right_half ? "RIGHT" : "LEFT");
    if (state->scene_index == 1) {
        lv_label_set_text(state->panel_line1, "FISH");
        lv_label_set_text(state->panel_line2, "MINI");
    } else {
        lv_label_set_text(state->panel_line1, "BUB");
        lv_label_set_text(state->panel_line2, "FLOW");
    }
}

static void bubble_tick_cb(lv_timer_t *timer) {
    struct bubble_animation_state *state = timer->user_data;

    state->frame_index = (state->frame_index + 1) % ARRAY_SIZE(bubble_frames);
    lv_img_set_src(state->bubble_image, bubble_frames[state->frame_index]);
}

static void scene_tick_cb(lv_timer_t *timer) {
    struct scene_cycle_state *state = timer->user_data;
    const struct scene_config *scenes = state->right_half ? right_scenes : left_scenes;
    const struct scene_config *scene;

    state->scene_index = (state->scene_index + 1) % ARRAY_SIZE(left_scenes);
    scene = &scenes[state->scene_index];

    lv_img_set_src(state->fish_image, scene->fish_art);
    lv_obj_set_pos(state->fish_image, scene->fish_x, scene->fish_y);
    lv_obj_set_pos(state->bubble_image, scene->bubble_x, scene->bubble_y);
    update_panel(state);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *fish_image;
    lv_obj_t *bubble_image;
    const struct scene_config *scenes;
    const struct scene_config *scene;
    bool right_half = IS_ENABLED(LILY58_OLED_ART_RIGHT);

    lv_obj_set_size(screen, 128, 32);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    scenes = right_half ? right_scenes : left_scenes;
    scene = &scenes[0];

    fish_image = lv_img_create(screen);
    lv_img_set_src(fish_image, scene->fish_art);
    lv_obj_set_pos(fish_image, scene->fish_x, scene->fish_y);

    bubble_image = lv_img_create(screen);
    lv_img_set_src(bubble_image, bubble_frames[0]);
    lv_obj_set_pos(bubble_image, scene->bubble_x, scene->bubble_y);

    bubble_animation_state.bubble_image = bubble_image;
    bubble_animation_state.frame_index = 0;
    scene_cycle_state.fish_image = fish_image;
    scene_cycle_state.bubble_image = bubble_image;
    scene_cycle_state.panel_box = create_panel_box(screen, right_half ? 4 : 76, 4);
    scene_cycle_state.panel_title = create_panel_label(screen, "", 0, 0);
    scene_cycle_state.panel_line1 = create_panel_label(screen, "", 0, 0);
    scene_cycle_state.panel_line2 = create_panel_label(screen, "", 0, 0);
    scene_cycle_state.scene_index = 0;
    scene_cycle_state.right_half = right_half;
    update_panel(&scene_cycle_state);

    if (bubble_timer != NULL) {
        lv_timer_del(bubble_timer);
    }

    bubble_timer = lv_timer_create(bubble_tick_cb, 500, &bubble_animation_state);

    if (scene_timer != NULL) {
        lv_timer_del(scene_timer);
    }

    scene_timer = lv_timer_create(scene_tick_cb, 6000, &scene_cycle_state);

    return screen;
}
