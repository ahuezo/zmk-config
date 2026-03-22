if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
  zephyr_library()
  zephyr_library_sources(
    src/lily58_oled_status_screen.c
    src/assets/lily58_matrix_frames.c
  )
endif()
