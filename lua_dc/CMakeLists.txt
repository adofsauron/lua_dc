cmake_minimum_required(VERSION 2.8)

set(PROJNAME lua_dc)
project(${PROJNAME})

set (CMAKE_DEBUG_POSTFIX "_d")

set (srcs
    src/branch.h
    src/def.h
    src/encrypt.h
    src/log.cpp
    src/log.h
    src/lua_wrap.cpp
    src/lua_wrap.h
    src/seria.cpp
    src/seria.h
    src/seria_build.cpp
    src/seria_build.h
    src/seria_decode.cpp
    src/seria_decode.h
    src/seria_encode.cpp
    src/seria_encode.h
    src/seria_helper.cpp
    src/seria_helper.h
    src/tools.h
    src/buffer/CRC32.cpp
    src/buffer/NetInStream.h
    src/buffer/NetBuffer.h
    src/buffer/CRC32.h
    src/buffer/ByteStream.h
    src/buffer/NetOutStream.h
)

add_definitions(-fPIC -std=c++11 -D__LINUX__ -DBRANCH_SERVER)

add_library(${PROJNAME} ${srcs})
target_include_directories(${PROJNAME} PUBLIC ../..)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR} )

# 给linux使用
install(TARGETS ${PROJNAME}
	ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib")
    
