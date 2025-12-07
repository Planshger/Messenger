project(messenger LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Test)

add_library(server_lib server/src/server.hpp server/src/server.cpp)
target_link_libraries(server_lib PUBLIC Qt6::Core Qt6::Network)

add_library(client_lib client/src/client.hpp
                       client/src/client.cpp
                       client/src/main.cpp
                       client/src/network/network_client.cpp
                       client/src/network/network_client.hpp

                       client/src/ui/client_widget.cpp
                       client/src/ui/client_widget.hpp)
target_link_libraries(client_lib PUBLIC Qt6::Core Qt6::Widgets Qt6::Network)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server/src ${CMAKE_CURRENT_SOURCE_DIR}/client/src)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(tests)
