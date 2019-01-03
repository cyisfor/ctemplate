if(ctemplate)
	# where's the module?
else()
	set(ctemplate "ctemplate")
endif()

file(MAKE_DIRECTORY "${ctemplate}")

if(ctemplate_prefix)
	# generated/ or whatever, where the output goes
else()
	set(ctemplate_prefix "${ctemplate}/gen")
	include_directories("${CMAKE_CURRENT_BINARY_DIR}/${ctemplate_prefix}")
	file(MAKE_DIRECTORY "${ctemplate_prefix}")
endif()

set(gen "${ctemplate}ctpl-generate")

add_executable("${gen}"
	"${ctemplate}/src/generate.c"
	"${ctemplate}/src/generate_main.c")

function (add_ctemplate name)
	set(output "${ctemplate_prefix}/${name}.c")
	set(temp "${output}.temp")
	add_custom_command(
		OUTPUT "${output}"
		MAIN_DEPENDENCY "${name}"
		DEPENDS "${gen}"
		COMMAND "./${gen}" <"${name}" >"${temp}"
		COMMAND mv "${temp}" "${output}")
	add_custom_target(
		"${name}"
		DEPENDS "${output}")
endfunction(add_ctemplate)
