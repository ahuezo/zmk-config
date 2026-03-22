if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
  zephyr_library()
  zephyr_include_directories(${ZEPHYR_ZMK_MODULE_DIR}/app/include)
  zephyr_library_sources(
    src/lily58_oled_status_screen.c
    src/assets/lily58_fish_bubbles.c
  )
endif()
