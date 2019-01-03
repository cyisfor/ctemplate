cmake_minimum_required(VERSION 3.13)

set(ctemplate "")
#find_package(CTemplate MODULE REQUIRED)
include("${CMAKE_CURRENT_SOURCE_DIR}/FindCTemplate.cmake")
add_ctemplate(test.template.html)
add_executable(example src/example.c)
add_dependencies(example test.template.html)

