project(messenger LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_subdirectory(client)
add_subdirectory(server)
