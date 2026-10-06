add_library(penumbra_icons STATIC
    src/StandardIconBackend.cpp)

target_include_directories(penumbra_icons PUBLIC include)
target_compile_features(penumbra_icons PUBLIC cxx_std_26)
target_link_libraries(penumbra_icons PUBLIC penumbra)
