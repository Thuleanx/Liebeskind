set(SRC 
    world_loader.cpp
    json_serializer.cpp
)

add_library(save_load ${SRC})

target_link_libraries(save_load PRIVATE game_world)
target_link_libraries(save_load PRIVATE low_level_renderer)
target_link_libraries(save_load PRIVATE logger)

