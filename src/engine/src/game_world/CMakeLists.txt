set(SRC 
    world.cpp
)

add_library(game_world ${SRC})

target_link_libraries(game_world PRIVATE low_level_renderer)
target_link_libraries(game_world PRIVATE logger)

