find_package(OpenSSL REQUIRED)

add_executable(encore-import
    src/main.cpp
)

target_link_libraries(encore-import
    PRIVATE
        OpenSSL::Crypto
)

target_compile_options(encore-import
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
