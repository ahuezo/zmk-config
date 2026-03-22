if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
  zephyr_library()
  string(FIND "${SHIELD}" "lily58_right" LILY58_RIGHT_SHIELD_INDEX)
  if(NOT LILY58_RIGHT_SHIELD_INDEX EQUAL -1)
    zephyr_compile_definitions(LILY58_OLED_ART_RIGHT=1)
  endif()
  zephyr_library_sources(
    src/lily58_oled_status_screen.c
    src/assets/lily58_fish_bubbles.c
  )
endif()
