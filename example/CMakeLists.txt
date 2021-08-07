cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(AsyncioExample)

# version
execute_process(
    OUTPUT_VARIABLE GIT_BRANCH
    COMMAND git symbolic-ref --short -q HEAD
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(TIMESTAMP TODAY "%Y%m%d%H%I")
set(BUILD_VERSION "${TODAY}")
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/include/version_config.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/version_config.h"
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System Name: ${CMAKE_SYSTEM_NAME}")

message(STATUS "Is the C++ compiler loaded? ${CMAKE_CXX_COMPILER_LOADED}")
if (CMAKE_CXX_COMPILER_LOADED)
    message(STATUS "The C++ compiler ID is: ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "Is the C++ from GNU? ${CMAKE_COMPILER_IS_GNUCXX}")
    message(STATUS "The C++ compiler version is: ${CMAKE_CXX_COMPILER_VERSION}")
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/app/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/app/bin)

if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0501")
else ()
	find_program(CCACHE_FOUND ccache)
	if(CCACHE_FOUND)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
	endif(CCACHE_FOUND)
endif ()

add_subdirectory(${PROJECT_SOURCE_DIR}/1.simple_client)
add_subdirectory(${PROJECT_SOURCE_DIR}/2.simple_server)
add_subdirectory(${PROJECT_SOURCE_DIR}/3.chat_client)
add_subdirectory(${PROJECT_SOURCE_DIR}/4.chat_server)
add_subdirectory(${PROJECT_SOURCE_DIR}/5.load_test_client)
add_subdirectory(${PROJECT_SOURCE_DIR}/6.load_test_server)