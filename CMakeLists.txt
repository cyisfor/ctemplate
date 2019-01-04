cmake_minimum_required(VERSION 3.13)

add_executable(ctemplate
	src/generate.c
	src/generate_main.c)

target_include_directories(ctemplate PUBLIC cstuff/src)

# XXX: this should be its own module...
include(generate_dest_directory.cmake)

function (add_ctemplate name)
	if(ARGC EQUAL 3)
		set(source "${ARGV1}")
		set(dest "${ARGV2}")
	else()
		if(ARGC EQUAL 2)
			set(source "${ARGV1}")
		else()
			set(source "${name}")
		endif()
		set(dest "${source}.c")
	endif()

	get_filename_component(source "${source}" ABSOLUTE
		BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(dest "${dest}" ABSOLUTE
		BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

	generate_dest_directory(destdir "${dest}")
	message("uhhhh: '${destdir}'" )
	set(temp "${dest}.temp")
	add_custom_command(
		OUTPUT "${dest}"
		MAIN_DEPENDENCY "${source}"
		DEPENDS ctemplate
		DEPENDS "${destdir}"
		COMMAND ctemplate "${source}" "${dest}"
		VERBATIM)
	add_custom_target(
		"${name}"
		DEPENDS "${dest}")
	include_directories("${CMAKE_CURRENT_BINARY_DIR}")
endfunction(add_ctemplate)

#find_package(CTemplate MODULE REQUIRED)
#include("${CMAKE_CURRENT_SOURCE_DIR}/FindCTemplate.cmake")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_ctemplate(test.template.html)
	add_executable(example src/example.c test.template.html)
	target_include_directories(example PUBLIC cstuff/src)

#	add_dependencies(example test.template.html)
endif()

