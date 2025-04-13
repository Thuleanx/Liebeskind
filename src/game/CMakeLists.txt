set(SRC
    src/game.cpp
    src/input_management.cpp
    src/cameras/module.cpp
    src/cameras/debug_camera_controller.cpp
)

add_library(game ${SRC})

target_link_libraries(game PRIVATE engine logger third_party)

set(ENGINE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/engine/include")
set(GAME_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/game/include")

target_include_directories(game PUBLIC ${GAME_INCLUDE_DIR} ${ENGINE_INCLUDE_DIR})

