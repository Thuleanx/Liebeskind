add_library(engine INTERFACE)

add_subdirectory(src)

target_link_libraries(engine INTERFACE logger)
target_link_libraries(engine INTERFACE low_level_renderer)

target_include_directories(logger PUBLIC "${PROJECT_SOURCE_DIR}/src/engine/include")
target_include_directories(low_level_renderer PUBLIC "${PROJECT_SOURCE_DIR}/src/engine/include")
