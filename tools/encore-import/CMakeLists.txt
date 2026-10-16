find_package(OpenSSL REQUIRED)

add_executable(encore-import
    src/main.cpp
    src/disc_reader.cpp
    src/iso9660.cpp
    src/system_cnf.cpp
    src/psx_exe.cpp
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
