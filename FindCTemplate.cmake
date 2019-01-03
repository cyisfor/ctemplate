if(ctemplate)
else()
	set(ctemplate "ctemplate")
endif()
if(ctemplate_prefix)
	# generated/ or whatever
	include_directories("${CMAKE_CURRENT_BINARY_DIR}/${ctemplate_prefix}")
endif()


file(MAKE_DIRECTORY "${ctemplate}")

add_executable("${ctemplate}/generate"
	"${ctemplate}/src/generate.c"
	"${ctemplate}/src/generate_main.c"

function (add_ctemplate name)
	set(output "${ctemplate_prefix}/${name}.c")
	set(temp "${output}.temp")
	add_custom_command(
		OUTPUT "${output}"
		MAIN_DEPENDENCY "${name}"
		DEPENDS "${ctemplate}/generate"
		COMMAND "./${ctemplate}/generate" <"${name}" >"${temp}"
		COMMAND mv "${temp}" "${output}")
	add_custom_target(
		"${name}"
		DEPENDS "${output}")
endfunction(add_ctemplate)
