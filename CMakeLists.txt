project(messenger LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(tests)

add_library(server_lib server/src/server.hpp server/src/server.cpp)
target_link_libraries(server_lib PRIVATE Qt6::Core Qt6::Network)

add_library(client_lib client/src/client.hpp client/src/client.cpp)
target_link_libraries(client_lib PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)

