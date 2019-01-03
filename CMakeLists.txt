cmake_minimum_required(VERSION 3.13)

set(ctemplate "")
find_package(CTemplate.cmake MODULE REQUIRED)
add_ctemplate(test.template.html)
add_executable(example src/example.c)
add_dependency(example test.template.html)

