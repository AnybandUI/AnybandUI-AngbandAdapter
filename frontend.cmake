# Included by Angband's external frontend build entry point.
set(ADAPTER_ROOT "${CMAKE_CURRENT_LIST_DIR}")
add_library(anybandui_json STATIC "${ADAPTER_ROOT}/vendor/cjson/cJSON.c")
target_include_directories(anybandui_json PUBLIC "${ADAPTER_ROOT}/vendor/cjson")
target_sources(${ANGBAND_FRONTEND_TARGET} PRIVATE
    "${ADAPTER_ROOT}/src/main-anybandui.c")
target_link_libraries(${ANGBAND_FRONTEND_TARGET} PRIVATE anybandui_json)
set_target_properties(${ANGBAND_FRONTEND_TARGET} PROPERTIES
    OUTPUT_NAME angband-anybandui C_STANDARD 99)
if(WIN32)
    target_compile_definitions(${ANGBAND_CORE_TARGET} PRIVATE
        WINDOWS _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${ANGBAND_FRONTEND_TARGET} PRIVATE
        WINDOWS _CRT_SECURE_NO_WARNINGS)
endif()
configure_file("${ADAPTER_ROOT}/engine.anyband.json.in"
    "${CMAKE_BINARY_DIR}/game/engine.anyband.json" @ONLY)
add_custom_command(TARGET ${ANGBAND_FRONTEND_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/lib"
        "$<TARGET_FILE_DIR:${ANGBAND_FRONTEND_TARGET}>/lib"
    COMMENT "Staging engine game data")

# In-process rendering checks: no child engine or protocol test driver.
add_executable(anybandui-map-tests EXCLUDE_FROM_ALL
    "${ADAPTER_ROOT}/tests/map.c"
    $<TARGET_OBJECTS:OurUnitTestLib> $<TARGET_OBJECTS:OurCoreLib>)
target_include_directories(anybandui-map-tests PRIVATE
    ${ANGBAND_CORE_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/src/tests")
target_link_libraries(anybandui-map-tests PRIVATE ${ANGBAND_CORE_LINK_LIBRARIES})
set_target_properties(anybandui-map-tests PROPERTIES C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/game")