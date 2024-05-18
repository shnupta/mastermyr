cmake_minimum_required(VERSION 3.12)

project(mastermyr
    VERSION 0.0.1
		DESCRIPTION "A C++ toolbox from the bog."
    HOMEPAGE_URL "https://github.com/shnupta/mastermyr")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MASTERMYR_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_library(mastermyr INTERFACE)
add_library(mastermyr::mastermyr ALIAS mastermyr)

set(_mastermyr_is_toplevel_project FALSE)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(_mastermyr_is_toplevel_project TRUE)
endif()

target_include_directories(
    mastermyr
    INTERFACE
		$<BUILD_INTERFACE:${MASTERMYR_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(mastermyr INTERFACE "")

# TODO: if i write source
# add_subdirectory(src)
