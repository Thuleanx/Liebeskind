set(SRC 
    camera.cpp
    perspective_camera.cpp
    fixed_follow_camera.cpp
)

add_library(cameras ${SRC})

target_link_libraries(cameras PRIVATE third_party)
