add_subdirectory(algo)
add_subdirectory(file_system)
add_subdirectory(logger)
add_subdirectory(math)

add_library(core INTERFACE)

target_link_libraries(core INTERFACE algo file_system logger math)

set(ENGINE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/engine/include")
target_include_directories(algo PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(file_system PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(logger PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(math PUBLIC ${ENGINE_INCLUDE_DIR})
