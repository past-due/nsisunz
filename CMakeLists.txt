cmake_minimum_required(VERSION 3.5)
project(nsisunz)

if(NOT WIN32)
	message( FATAL_ERROR "Only supports Windows" )
endif()

add_subdirectory(Source)

file(GLOB INFO_FILES "*.txt" "*.md")
INSTALL(FILES ${INFO_FILES}
		COMPONENT "Info" DESTINATION ".")
