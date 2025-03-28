set(SRC 
    mesh_manager.cpp
    shader_manager.cpp
)

add_library(resource_management ${SRC})

target_link_libraries(resource_management PRIVATE third_party)
target_link_libraries(resource_management PRIVATE logger)
target_link_libraries(resource_management PRIVATE file_system)

