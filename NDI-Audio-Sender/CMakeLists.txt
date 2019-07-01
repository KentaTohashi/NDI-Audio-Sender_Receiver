cmake_minimum_required(VERSION 3.14)
project(NDI_Audio_Sender)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NDI REQUIRED libndi)
include_directories(${NDI_INCLUDE_DIRS})
link_directories(${NDI_LIBRARY_DIRS})

add_executable(NDI_Audio_Sender main.cpp NDISender.cpp )

find_package(ALSA REQUIRED)
if (ALSA_FOUND)
    include_directories(${ALSA_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARIES})
endif (ALSA_FOUND)


target_link_libraries(${PROJECT_NAME} ${NDI_LIBRARIES})

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)
