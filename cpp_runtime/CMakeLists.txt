cmake_minimum_required(VERSION 3.10)

project(aliensight_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(aliensight_runtime
    src/main.cpp
    src/aliensight_runtime.cpp
)

target_include_directories(aliensight_runtime PRIVATE
    include
)
