set(SRC
    type_id.cpp
)

add_library(algo ${SRC})
target_link_libraries(algo PRIVATE logger)
