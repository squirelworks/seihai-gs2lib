#
#  CMakeLists.txt
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

cmake_minimum_required(VERSION 3.27)

# Policies.
cmake_policy(SET CMP0042 NEW)	# CMP0042: MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0043 NEW)	# CMP0043: Automatically link to all dependencies found by find_package.
cmake_policy(SET CMP0048 NEW)	# CMP0048: project() command manages VERSION variables.
cmake_policy(SET CMP0054 NEW)	# CMP0054: Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0074 NEW)	# CMP0074: find_package uses PackageName_ROOT variables.
cmake_policy(SET CMP0077 NEW)	# CMP0077: option() honors normal variables.
cmake_policy(SET CMP0091 NEW)	# CMP0091: MSVC runtime library flags are selected by an abstraction.

# vcpkg configuration.
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet")
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/vcpkg_installed")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Project name.
project(gs2lib CXX)

# Directories.
set(CMAKE_DEBUG_POSTFIX _d)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${BIN_DIR})

# Second, for multi-config builds (e.g. msvc)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/lib)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/lib)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/${BIN_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

# Set language.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# Additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# Version number in format X.YY.ZZ
set(VER_X 1)
set(VER_Y 0)
set(VER_Z 0)
set(VER_EXTRA "-beta" CACHE STRING "Extra version")
set(VER_FULL "${VER_X}.${VER_Y}.${VER_Z}${VER_EXTRA}")
set(GSERVER_CREDITS "Joey, Nalin, Codr, and Cadavre")
STRING(REGEX REPLACE " " "-" VER_CPACK ${VER_FULL})

if(APPLE)
	# Set variables for generating the Info.plist file
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${VER_FULL}")
	set(MACOSX_BUNDLE_EXECUTABLE ${PROJECT_NAME})
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.GraalReborn.GS2Lib")
	set(MACOSX_BUNDLE_NSMAIN_NIB_FILE "Application")
	set(MACOSX_BUNDLE_ICON_FILE "carton")
	set(MACOSX_BUNDLE_NAME ${PROJECT_NAME})
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VER_FULL}")
endif()

set(CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}] ")
message(STATUS "Configuring version: ${VER_FULL}")
list(APPEND CMAKE_MESSAGE_INDENT "* ")

option(STATIC "Compile as a static runtime." ON)
message(STATUS "Compile as static runtime:\t\t\t\t\t${STATIC}")
if(NOT STATIC)
	set(BUILD_SHARED_LIBS TRUE CACHE BOOL "-" FORCE)
else()
	add_definitions(-DSTATICLIB)
	set(BUILD_SHARED_LIBS FALSE CACHE BOOL "-" FORCE)
endif()

# Prefer static linkage
if(STATIC)
	set(SUFFIXES_ORIG ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

# Restore library suffixes
if(STATIC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${SUFFIXES_ORIG})
endif()

set(
	SOURCES
	src/CCommon.cpp
	src/CLog.cpp
	src/CSettings.cpp
	src/CString.cpp
	src/CTranslationManager.cpp
	src/IUtil.cpp
	src/md5.cpp
)

set(
	HEADERS
	include/CCommon.h
	include/CLog.h
	include/CSettings.h
	include/CString.h
	include/CTimeout.h
	include/CTranslationManager.h
	include/IEnums.h
	include/IUtil.h
	include/IWin.h
	include/md5.h
)

option(NET "Compile with networking support." ON)
message(STATUS "Compile with networking support:\t\t\t${NET}")
if(NET)
	list(APPEND
		SOURCES
		src/CEncryption.cpp
		src/CFileQueue.cpp
		src/CSocket.cpp
	)

	list(APPEND
		HEADERS
		include/CEncryption.h
		include/CFileQueue.h
		include/CSocket.h
	)
endif()

set(GS2LIB_INCLUDE_DIRECTORY
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_BINARY_DIR}/include"
	PARENT_SCOPE
)

# Set target names for the libraries
set(TARGET_NAME ${PROJECT_NAME})

# Add the library target
add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})

# Set the include directories for the target
target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Visual C++ Compiler options
if(MSVC)
	# Suppress secure string function warnings
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)

	if(MSVC_VERSION GREATER_EQUAL 1910)
		target_compile_options(${PROJECT_NAME} PUBLIC "/permissive-")
	endif()
	if(MSVC_VERSION GREATER_EQUAL 1914)
		target_compile_options(${PROJECT_NAME} PUBLIC "/Zc:__cplusplus")
	endif()
	if(MSVC_VERSION GREATER_EQUAL 1925)
		target_compile_options(${PROJECT_NAME} PUBLIC "/Zc:preprocessor")
	endif()

	target_compile_options(${PROJECT_NAME} PUBLIC
		"$<$<CONFIG:Release>:/guard:cf>"			# Control Flow Guard
		"$<$<CONFIG:Release>:/Qspectre>"			# Spectre Mitigation
		"$<$<CONFIG:RelWithDebInfo>:/dynamicdeopt>"	# Dynamic Deoptimization
	)
	target_link_options(${PROJECT_NAME} PUBLIC
		"$<$<CONFIG:RelWithDebInfo>:/dynamicdeopt>"	# Dynamic Deoptimization
	)
endif()

# GCC ignore attribute warnings.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wno-attributes
		-Wno-narrowing
		-Wno-switch
	)
endif()

# GCC static compile.
if(STATIC AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	target_compile_options(${PROJECT_NAME} PUBLIC -static-libstdc++)
endif()

# MinGW links.
if(MINGW)
	target_compile_options(${PROJECT_NAME} PUBLIC "-mthreads")
	target_link_options(${PROJECT_NAME} PUBLIC "-mthreads")
endif()

# Dependency: bzip2.
find_package(BZip2 REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE BZip2::BZip2)

# Dependency: zlib.
find_package(ZLIB REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE ZLIB::ZLIB)

# Dependency: WolfSSL.
option(WOLFSSL "Compile with WolfSSL support." OFF)
message(STATUS "Compile with WolfSSL support:\t\t\t\t${WOLFSSL}")
if(WOLFSSL)
	find_package(wolfssl CONFIG REQUIRED)
	target_link_libraries(${TARGET_NAME} PRIVATE wolfssl::wolfssl)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
	target_link_libraries(${TARGET_NAME} PRIVATE ws2_32 wsock32 iphlpapi)
endif()

# Seems to be fixed in later versions of mingw
#if(MINGW)
#	option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
#	add_subdirectory(dependencies/mingw_stdthreads)
#	target_link_libraries(${TARGET_NAME} mingw_stdthreads)
#	target_compile_definitions(mingw_stdthreads INTERFACE MINGW_STDTHREAD_REDUNDANCY_WARNING)
#endif()

# Packaging
if(APPLE)
	set(CPACK_GENERATOR DragNDrop)
	set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME} ${VER_FULL}")
	set(
		CPACK_DMG_DS_STORE_SETUP_SCRIPT
		${PROJECT_SOURCE_DIR}/resources/packaging/osx/DMGSetup.scpt
	)
	set(
		CPACK_DMG_BACKGROUND_IMAGE
		${PROJECT_SOURCE_DIR}/resources/packaging/osx/DMGBackground.tif
	)
elseif(WIN32)
	set(CPACK_GENERATOR "ZIP")
else()
	set(CPACK_GENERATOR TGZ)
endif()

if(NOT CPack_CMake_INCLUDED)
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		message(STATUS "Enabling packaging")

		set(CPACK_PACKAGE_NAME ${PROJECT_NAME_LOWER})
		set(CPACK_PACKAGE_VENDOR "Preagonal Team")
		set(CPACK_PACKAGE_CONTACT "preagonal.net")
		set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VER_CPACK}")
		set(CPACK_PACKAGE_VERSION_MAJOR ${VER_X})
		set(CPACK_PACKAGE_VERSION_MINOR ${VER_Y})
		set(CPACK_PACKAGE_VERSION_PATCH ${VER_Z}${VER_EXTRA})
		set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GS2Lib")
		set(CPACK_PACKAGE_DESCRIPTION_FILE ${PROJECT_SOURCE_DIR}/README.md)
		set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
		set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE.md)
		set(CPACK_SOURCE_GENERATOR TGZ)
		set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VER_CPACK}-src")
		set(CPACK_SOURCE_IGNORE_FILES "/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}")

		include(CPack)

		# Install the library
		install(TARGETS ${TARGET_NAME}
				DESTINATION ${CMAKE_INSTALL_LIBDIR}
				EXPORT "${TARGET_NAME}-targets"
				LIBRARY)

		# Install the headers
		install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
				DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
				FILES_MATCHING PATTERN "*.h")
	endif()
endif()
set(GS2LIB_LOADED ON PARENT_SCOPE)
