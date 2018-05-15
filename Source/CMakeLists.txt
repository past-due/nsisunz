cmake_minimum_required(VERSION 3.5)

if(NOT WIN32)
	message( FATAL_ERROR "Only supports Windows" )
endif()

OPTION(STATIC_MSVC_RUNTIME "Static MSVC CRT" ON)

macro(configure_msvc_runtime)
  if(MSVC)
    # Set compiler options.
    set(variables
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    if(STATIC_MSVC_RUNTIME)
      message(STATUS
        "MSVC -> forcing use of statically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
      endforeach()
    else()
      message(STATUS
        "MSVC -> forcing use of dynamically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
        endif()
      endforeach()
    endif()
  endif()
endmacro()

configure_msvc_runtime()

set(CMAKE_CXX_STANDARD 11)

###########################
# zlib

add_subdirectory(zlib)

include_directories(zlib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/zlib)


###########################
# miniunzip

add_library(miniunzip STATIC minizip/ioapi.c minizip/iowin32.c minizip/miniunz.c minizip/unzip.c)
target_link_libraries(miniunzip zlibstatic)


###########################
# nsisunz

set(SOURCE_FILES nsisunz.cpp nsis/pluginapi.c)

# ANSI / MBCS DLL
add_library(nsisunz_ansi MODULE ${SOURCE_FILES})
set_target_properties(nsisunz_ansi PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(nsisunz_ansi PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(nsisunz_ansi PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(nsisunz_ansi PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nsisunz_ansi.dll"
		RENAME "nsisunz.dll"
		COMPONENT Plugins DESTINATION "Plugins/ansi")
target_link_libraries(nsisunz_ansi zlibstatic miniunzip)

# Unicode DLL
add_library(nsisunz_unicode MODULE ${SOURCE_FILES})
target_compile_definitions(nsisunz_unicode PRIVATE "UNICODE" "_UNICODE")
set_target_properties(nsisunz_unicode PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(nsisunz_unicode PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(nsisunz_unicode PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(nsisunz_unicode PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nsisunz_unicode.dll"
		RENAME "nsisunz.dll"
		COMPONENT Plugins DESTINATION "Plugins/unicode")
target_link_libraries(nsisunz_unicode zlibstatic miniunzip)


###########################
# Additional Install Rules

# Copy the source files over into the "Source" folder
file(GLOB SRC_FILES "*.txt" "*.cpp" "*.c" "*.h")
INSTALL(FILES ${SRC_FILES}
		COMPONENT "Source" DESTINATION "Source")
INSTALL(DIRECTORY "minizip"
		COMPONENT "Source" DESTINATION "Source")
INSTALL(DIRECTORY "nsis"
		COMPONENT "Source" DESTINATION "Source")
INSTALL(DIRECTORY "zlib"
		COMPONENT "Source" DESTINATION "Source")

