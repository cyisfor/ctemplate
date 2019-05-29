cmake_minimum_required(VERSION 3.13)

add_executable(ctemplate
	src/die.c
	src/generate.c
	src/generate_main.c)

target_include_directories(ctemplate PUBLIC cstuff/src)

function (add_ctemplate name)
	set(OPTIONS)
	set(ONEVALS OPEN CLOSE)
	set(MULTIVALS)
	cmake_parse_arguments(PARSE_ARGV 1 O
		"${OPTIONS}" "${ONEVALS}" "${MULTIVALS}")
	# add_ctemplate(foo.txt) turns (src)/foo.txt into (bin)/foo.txt.c
	# add_ctemplate(foo.txt bar.txt.c) turns (src)/foo.txt into (bin)/bar.txt.c
	# add_ctemplate(name source.c dest.c) turns (bin)/source.c into (bin)/dest.c
	# all three turn their first argument into a target
	# add_ctemplate(... OPEN "((" CLOSE "))") changes the open and close delimiters

	list(LENGTH O_UNPARSED_ARGUMENTS narg)
	if("${narg}" EQUAL 2)
		# add_ctemplate(name source dest ...)
		list(GET O_UNPARSED_ARGUMENTS 0 source)
		list(GET O_UNPARSED_ARGUMENTS 1 dest)
	elseif("${narg}" EQUAL 1)
		# add_ctemplate(name source ...)
		list(GET O_UNPARSED_ARGUMENTS 0 source)
		set(dest "${source}.c")
	else()
		# add_ctemplate(name ...)
		set(source "${name}")
		set(dest "${name}.c")
	endif()

	get_filename_component(source "${source}" ABSOLUTE
		BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(dest "${dest}" ABSOLUTE
		BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	get_filename_component(dir "${dest}" DIRECTORY)
	file(MAKE_DIRECTORY "${dir}")
	add_custom_command(
		OUTPUT "${dest}"
		MAIN_DEPENDENCY "${source}"
		DEPENDS ctemplate
		COMMAND
		$<IF:O_OPEN,open='${O_OPEN}',>
		$<IF:O_CLOSE,close='${O_CLOSE}',>
		$<TARGET_FILE:ctemplate> "${source}" "${dest}"
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

