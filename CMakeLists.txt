cmake_minimum_required(VERSION 3.13)

add_executable(ctemplate
	src/generate.c
	src/generate_main.c)

set(ctpl_needs_this_location "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)

function (add_ctemplate name)
	if(ARGC EQUAL 2)
		set(source "${ARGV1}")
	else()
		set(source "${name}")
	endif()
	set(output "${source}.c")
	set(temp "${output}.temp")
	add_custom_command(
		OUTPUT "${output}"
		MAIN_DEPENDENCY "${source}"
		DEPENDS ctemplate
		COMMAND "${ctpl_needs_this_location}/ctemplate" <"${source}" >"${temp}"
		COMMAND mv "${temp}" "${output}")
	add_custom_target(
		"${name}"
		DEPENDS "${output}")
	include_directories("${CMAKE_CURRENT_BINARY_DIR}")
endfunction(add_ctemplate)

#find_package(CTemplate MODULE REQUIRED)
#include("${CMAKE_CURRENT_SOURCE_DIR}/FindCTemplate.cmake")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_ctemplate(test.template.html)
	add_executable(example src/example.c)
	add_dependencies(example test.template.html)
endif()

