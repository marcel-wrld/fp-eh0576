cmake_minimum_required(VERSION 3.31)

project(fp-eh0576)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

add_executable(fp-eh0576 eh0576.h eh0576.c main.c)

target_link_libraries(fp-eh0576 ${LIBUSB_LIBRARIES} m)
