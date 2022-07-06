cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -pthread)
project(Utilite_yosys_synth)

add_executable(${PROJECT_NAME} Source.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/json/include
)