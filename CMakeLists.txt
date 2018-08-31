cmake_minimum_required (VERSION 2.6)
project (vri-security)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package( OpenCV REQUIRED )

add_executable(vri-record "src/main.cpp")
target_link_libraries(vri-record ${OpenCV_LIBS})
install(TARGETS vri-record DESTINATION bin)
