cmake_minimum_required(VERSION 3.5)
project(battlesnake_c)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-D_DEBUG -DDEBUG -D_CONSOLE -DSTANDALONE_JSON)

if (WIN32)
	add_definitions(-DWIN32)
else ()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif ()

if (APPLE)
endif (APPLE)


if (EMSCRIPTEN)
    add_definitions(-D__EMSCRIPTEN__)
endif (EMSCRIPTEN)

file(GLOB SNAKE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/snakes/*.c
)

set(APP_SOURCE_FILES
    ${SNAKE_SRCS}

    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/snake_c_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/snake_c_api.cpp
)

include_directories(
    ./
    ${CMAKE_CURRENT_SOURCE_DIR}/
)


if (WIN32)
include_directories(
       ${PLATFORM}/windows
 )
endif (WIN32)

add_executable(battlesnake_c ${APP_SOURCE_FILES})

if (WIN32)
# libsodium
target_link_libraries(battlesnake_c ws2_32)
elseif (EMSCRIPTEN)
target_link_libraries(battlesnake_c)
elseif (UNIX)
target_link_libraries(battlesnake_c pthread)
else ()
target_link_libraries(battlesnake_c)
endif ()

