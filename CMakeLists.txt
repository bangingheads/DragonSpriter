cmake_minimum_required(VERSION 3.8)
project(dragonspriter)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(ADD_SRC folder inc src)
	source_group(${folder} FILES ${inc} ${src})
	set(DSPRI_SOURCES ${DSPRI_SOURCES} ${inc} ${src} PARENT_SCOPE)
endfunction(ADD_SRC)

# Local includes
include_directories("ext")

ADD_SRC(App "" "main.cpp")

add_executable(dragonspriter ${DSPRI_SOURCES})
set_target_properties(dragonspriter PROPERTIES 
					# VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
					CXX_STANDARD 17
					CXX_EXTENSIONS OFF)
