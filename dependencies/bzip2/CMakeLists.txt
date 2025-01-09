#
#  dependencies/bzip2/CMakeLists.txt
#
#  Copyright 2019 死体
#
#  This file is part of GS2Emu.
#
#  GS2Emu is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  GS2Emu is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GS2Emu.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.0.0)

file(GLOB SOURCES
    src/blocksort.c
    src/huffman.c
    src/crctable.c
    src/randtable.c
    src/compress.c
    src/decompress.c
    src/bzlib.c
)

file(GLOB HEADERS src/*.h)

project(bzip2 C)

set(TARGET_NAME ${PROJECT_NAME})

set(CMAKE_DEBUG_POSTFIX _d)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${BIN_DIR})

message(${PROJECT_SOURCE_DIR}/lib)

if(APPLE)
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})

    # Enable ARC (automatic reference counting) for OS X build
    set_property(
            TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
    )
else()
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
endif()

if(MSVC)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DSTDERR_FILENO=2)
endif()