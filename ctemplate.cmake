if(ctemplate)
else()
	set(ctemplate "ctemplate")
endif()
file(MAKE_DIRECTORY "${ctemplate}")

add_executable("${ctemplate}/generate"
	"${ctemplate}/src/generate.c"
	"${ctemplate}/src/generate_main.c"

function (add_ctemplate name)
	add_custom_command(
		OUTPUT "${name}.c"
		MAIN_DEPENDENCY "${name}"
		DEPENDS "${ctemplate}/generate"
		COMMAND "./${ctemplate}/generate" <"${name}" >"${name}.c.temp"
		COMMAND mv "${name}.c.temp" "${name}.c")
	add_custom_target(
		"${name}"
		DEPENDS "${name}.c")
endfunction(add_ctemplate)
