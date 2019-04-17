# make the directory and all its parents
function(generate_dest_directory varname file)
	get_filename_component(dir "${dest}" DIRECTORY)
	get_filename_component(dir "${dir}" ABSOLUTE
		BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	add_custom_command(
		OUTPUT "${dir}"
		COMMAND mkdir "${dir}")
	set("${varname}" "${dir}" PARENT_SCOPE)
endfunction(generate_dest_directory)
