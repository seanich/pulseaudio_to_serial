cmake_minimum_required(VERSION 3.8)
project(pulseaudio_to_serial)

set(CMAKE_C_STANDARD 11)

link_directories(/usr/lib)

set(SOURCE_FILES main.c)
add_executable(pulseaudio_to_serial ${SOURCE_FILES})
target_link_libraries(pulseaudio_to_serial pulse pulse-simple)