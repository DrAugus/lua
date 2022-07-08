cmake_minimum_required(VERSION 3.9)
project(lua-cpp)

set(CMAKE_CXX_STANDARD 20)

IF (WIN32)
    message(STATUS "Now is windows.")
    link_libraries("${CMAKE_SOURCE_DIR}/../LuaJIT/src/lua51.lib" "${CMAKE_SOURCE_DIR}/../LuaJIT/src/luajit.lib")
ELSEIF (APPLE)
    message(STATUS "Now is Apple.")
ELSEIF (UNIX)
    message(STATUS "Now is UNIX-like OS's.")
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
    link_libraries("${CMAKE_SOURCE_DIR}/../LuaJIT/src/lua51.so" "${CMAKE_SOURCE_DIR}/../LuaJIT/src/luajit.so")
ELSE ()
    message(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF ()

#
#add_definitions(-D_LUAJIT_ARM_)

# add luajit
include_directories(${CMAKE_SOURCE_DIR}/../LuaJIT/src)

# add ELuna
include_directories(${CMAKE_SOURCE_DIR}/../eluna/eluna)

# add src
file(GLOB APP_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

add_executable(${PROJECT_NAME} ${APP_SOURCES})

#add_library(cpp SHARED ${APP_SOURCES})
#link_directories(${CMAKE_SOURCE_DIR}/../LuaJIT/src/)
#set(LUAJIT_LIBRARIES ${CMAKE_SOURCE_DIR}/../LuaJIT/src/lua51.lib)
#set(_LIBRARIES ${_LIBRARIES} ${LUAJIT_LIBRARIES})
#target_link_libraries(${PROJECT_NAME} lua51.lib luajit.lib)
