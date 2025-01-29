set(SRC 
    scene_drawer.cpp
)

add_library(scene_graph ${SRC})

target_link_libraries(scene_graph PRIVATE cameras)
target_link_libraries(scene_graph PRIVATE low_level_renderer)
target_link_libraries(scene_graph PRIVATE resource_management)
target_link_libraries(scene_graph PRIVATE third_party)
