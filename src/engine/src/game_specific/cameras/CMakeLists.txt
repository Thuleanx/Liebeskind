set(SRC 
    camera.cpp
    module.cpp
    perspective_camera.cpp
)

add_library(cameras ${SRC})

target_link_libraries(cameras PUBLIC third_party)
