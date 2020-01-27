cmake_minimum_required(VERSION 3.13)
project(ctemplate VERSION 0.1)

add_executable(ctemplate
	src/die.c
	src/generate.c
	src/generate_main.c)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/add_module")
include(AddModule)

add_module(cstuff
  GIT 874ae5456e63ea82ddba9cca149600a71be06daf
  file://$ENV{HOME}/code/cstuff
  file://$ENV{HOME}/repo/cstuff
  git@github.com:cyisfor/cstuff.git
  https://github.com/cyisfor/cstuff.git)
add_cstuff(mmapfile record)

target_link_libraries(ctemplate PRIVATE cstuff)

add_module(str_to_enum_trie
  GIT 47df9a09a1029c1ddcdf4b4e00672f4386f24c48
  file://$ENV{HOME}/code/str_to_enum_trie
  file://$ENV{HOME}/repo/str_to_enum_trie
  git@github.com:cyisfor/str_to_enum_trie.git
  https://github.com/cyisfor/str_to_enum_trie.git)
str_to_enum_trie(processing_type src/processing_types.list)

add_executable(derp_ctemplate
  processing_type.c
  src/internal_output.c
  src/c_generate.c
  src/c_generate_main.c)
target_include_directories(derp_ctemplate PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(derp_ctemplate PRIVATE cstuff)

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
	if(O_OPEN)
		list(APPEND ENV "open=${O_OPEN}")
	endif()
	if(O_CLOSE)
		list(APPEND ENV "close=${O_CLOSE}")
	endif()
#	list(JOIN ENV "\n" ENV)
	add_custom_command(
		OUTPUT "${dest}"
		MAIN_DEPENDENCY "${source}"
		DEPENDS ctemplate
		COMMAND
		env ${ENV}
		$<TARGET_FILE:ctemplate> "${source}" "${dest}"
		VERBATIM)
	add_custom_target(
		"${name}"
		DEPENDS "${dest}")
#	include_directories("${CMAKE_CURRENT_BINARY_DIR}")
endfunction(add_ctemplate)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_ctemplate(test.template.html)
	add_executable(example src/example.c test.template.html)
	target_include_directories(example PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
	target_link_libraries(example PRIVATE cstuff)

	add_ctemplate(example2tmpl src/example2.c example2.c.c
		OPEN ".|"
		CLOSE "|.")
	add_executable(example2 example2.c.c)
	add_dependencies(example2 example2tmpl)
	target_include_directories(example2 PRIVATE src)
	target_link_libraries(example2 PRIVATE cstuff)
	add_custom_command(
		OUTPUT example2.c.c.c
		COMMAND
		example2 > example2.c.c.c
		DEPENDS
		example2)
	add_executable(example2.2
		example2.c.c.c)
#	add_dependencies(example test.template.html)
endif()
