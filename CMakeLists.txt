cmake_minimum_required(VERSION 3.10)

project(launcher CXX)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_subdirectory(mdl)

add_library(${CMAKE_PROJECT_NAME} SHARED src/launcher.cpp src/main.cpp)

if(WIN32)
	target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/win32.cpp)
else()
	target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/unix.cpp)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} locenv)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -static-libstdc++)
endif()
