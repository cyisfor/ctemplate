set(ctemplate "")
include(ctemplate.cmake)
add_ctemplate(
add_executable(generate src/generate_main.c)
add_library(generate src/generate.c)
