cmake_minimum_required(VERSION 2.6...4.1.1)
project(CMoe-Counter C)
SET(CMAKE_BUILD_TYPE "Release")

include_directories("/usr/local/include")
link_directories("/usr/local/lib")

add_executable(cmoe cmoe.c)
add_executable(cmoeditor editor.c)

target_link_libraries(cmoe spb)
target_link_libraries(cmoeditor spb)

if (BUILD STREQUAL "docker")
    INSTALL(TARGETS cmoe RUNTIME DESTINATION bin)
endif ()
INSTALL(TARGETS cmoeditor RUNTIME DESTINATION bin)
