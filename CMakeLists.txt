add_subdirectory(drivers)

zephyr_include_directories(include)

target_sources(app PRIVATE
  src/events/trackpad_status_changed.c
)

# Runtime state get/set. Compiled with the driver (CONFIG_INPUT_AZOTEQ_IQS5XX)
# or the Studio RPC so the RPC links (and round-trips against a static
# fallback state) in device-less builds. Compiling on both split roles lets the
# peripheral apply relayed state.
if(CONFIG_INPUT_AZOTEQ_IQS5XX OR CONFIG_ZMK_TPS43_STUDIO_RPC)
  target_sources(app PRIVATE src/studio/tps43_state.c)
endif()

if(CONFIG_ZMK_TPS43_STUDIO_RPC)
  target_sources(app PRIVATE
    src/studio/tps43_handler.c
    src/studio/tps43_settings.c
  )

  list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
  include(nanopb)
  set(NANOPB_GENERATE_CPP_APPEND_PATH TRUE)
  set(NANOPB_GENERATE_CPP_STANDALONE OFF)

  zephyr_library()
  file(GLOB_RECURSE PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
  nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES})
  target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${proto_srcs} ${proto_hdrs})

  target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/proto)
  target_link_libraries(app PRIVATE ${ZEPHYR_CURRENT_LIBRARY})
endif()

if(CONFIG_ZMK_SPLIT_RELAY_EVENT AND CONFIG_INPUT_AZOTEQ_IQS5XX)
  target_sources(app PRIVATE src/split/tps43_relay.c)
endif()
