cmake_minimum_required (VERSION 2.6)
project (vri-security)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package( OpenCV REQUIRED )

add_executable(teste "src/main.cpp")
target_link_libraries(teste ${OpenCV_LIBS})
