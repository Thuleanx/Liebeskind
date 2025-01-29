set(SRC
    src/game.cpp
)

add_library(game ${SRC})

target_link_libraries(game PRIVATE engine logger third_party)

set(ENGINE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/engine/include")
set(GAME_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/game/include")

target_include_directories(game PUBLIC ${GAME_INCLUDE_DIR} ${ENGINE_INCLUDE_DIR})

