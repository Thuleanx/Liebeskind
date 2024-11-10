add_library(file_system file.cpp)

target_compile_features(file_system PUBLIC cxx_std_17)
target_link_libraries(file_system PRIVATE third_party)

target_link_libraries(file_system PRIVATE logger)
