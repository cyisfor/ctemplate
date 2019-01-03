set(ctemplate "")
include(ctemplate.cmake)
add_ctemplate(test.template.html)
add_executable(example src/example.c)
add_dependency(example test.template.html)

