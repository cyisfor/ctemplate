cmake_minimum_required(VERSION 3.13)

add_executable(ctemplate
	src/generate.c
	src/generate_main.c)

target_include_directories(ctemplate PUBLIC cstuff/src)

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

	set(temp "${dest}.temp")
	add_custom_command(
		OUTPUT "${dest}"
		MAIN_DEPENDENCY "${source}"
		DEPENDS ctemplate
		COMMAND ctemplate <"${source}" >"${temp}"
		COMMAND mv "${temp}" "${dest}")
	add_custom_target(
		"${name}"
		DEPENDS "${dest}")
	include_directories("${CMAKE_CURRENT_BINARY_DIR}")
endfunction(add_ctemplate)

#find_package(CTemplate MODULE REQUIRED)
#include("${CMAKE_CURRENT_SOURCE_DIR}/FindCTemplate.cmake")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_ctemplate(test.template.html)
	add_executable(example src/example.c)
	add_dependencies(example test.template.html)
endif()

