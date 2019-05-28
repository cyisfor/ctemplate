cmake_minimum_required(VERSION 3.13)

add_executable(ctemplate
	src/generate.c
	src/generate_main.c)

target_include_directories(ctemplate PUBLIC cstuff/src)

function (add_ctemplate name)
	# add_ctemplate(foo.txt) turns (src)/foo.txt into (bin)/foo.txt.c
	# add_ctemplate(foo.txt bar.txt.c) turns (src)/foo.txt into (bin)/bar.txt.c
	# add_ctemplate(name source.c dest.c) turns (bin)/source.c into (bin)/dest.c
	# all three turn their first argument into a target
	# add_ctemplate(name "${CMAKE_CURRENT_SOURCE_DIR}/source.c" dest.c) works
	if(ARGC EQUAL 3)
		set(source "${ARGV1}")
		set(dest "${ARGV2}")
	else()
		if(ARGC EQUAL 2)
			set(source "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV1}")
			set(dest "${ARGV1}.c")
		else()
			set(source "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
			set(dest "${name}.c")
		endif()
	endif()

	get_filename_component(source "${source}" ABSOLUTE
		BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(dest "${dest}" ABSOLUTE
		BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	get_filename_component(dir "${dest}" DIRECTORY)
	file(MAKE_DIRECTORY "${dir}")
	set(temp "${dest}.temp")
	add_custom_command(
		OUTPUT "${dest}"
		MAIN_DEPENDENCY "${source}"
		DEPENDS ctemplate
		COMMAND ctemplate "${source}" "${dest}"
		VERBATIM)
	add_custom_target(
		"${name}"
		DEPENDS "${dest}")
#	include_directories("${CMAKE_CURRENT_BINARY_DIR}")
endfunction(add_ctemplate)

#find_package(CTemplate MODULE REQUIRED)
#include("${CMAKE_CURRENT_SOURCE_DIR}/FindCTemplate.cmake")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_ctemplate(test.template.html)
	add_executable(example src/example.c test.template.html)
	target_include_directories(example PUBLIC
		cstuff/src
		"${CMAKE_CURRENT_BINARY_DIR}")

#	add_dependencies(example test.template.html)
endif()

