add_library(engine src/engine.cpp)

add_subdirectory(src)

target_link_libraries(engine INTERFACE logger file_system low_level_renderer scene_graph)
target_link_libraries(engine PRIVATE third_party)

set(ENGINE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/engine/include")

target_include_directories(engine PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(algo PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(file_system PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(cameras PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(source PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(logger PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(low_level_renderer PUBLIC ${ENGINE_INCLUDE_DIR})
target_include_directories(scene_graph PUBLIC ${ENGINE_INCLUDE_DIR})

set(GLSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/test_triangle.frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/test_triangle.vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/test_triangle_instanced.vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/post_processing.vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/post_processing.frag.glsl
)

find_program(GLSL_VALIDATOR "glslangValidator")
mark_as_advanced(FORCE GLSL_VALIDATOR)
if(GLSL_VALIDATOR)
    message(STATUS "glslangValidator found: ${GLSL_VALIDATOR}")
else()
    message(STATUS "glslangValidator not found!")
endif()

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_custom_command(TARGET Shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:source>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:source>/shaders"
        )

add_custom_target(Textures)

# Copy over files under textures
add_custom_command(
    TARGET Textures POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory              
    "${CMAKE_CURRENT_SOURCE_DIR}/textures" "${PROJECT_BINARY_DIR}/textures"
    COMMENT "Copying textures" VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:source>/textures/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_BINARY_DIR}/textures"
    "$<TARGET_FILE_DIR:source>/textures"
)

add_custom_target(Models)

# Copy over files under models
add_custom_command(
    TARGET Textures POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/models/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory              
    "${CMAKE_CURRENT_SOURCE_DIR}/models" "${PROJECT_BINARY_DIR}/models"
    COMMENT "Copying models" VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:source>/models/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_BINARY_DIR}/models"
    "$<TARGET_FILE_DIR:source>/models"
)

add_dependencies(source Shaders)
add_dependencies(source Textures)
add_dependencies(source Models)
