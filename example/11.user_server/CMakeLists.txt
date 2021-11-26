cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME UserServer)
PROJECT(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_UNITY_BUILD yes)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)

if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
	#find_package(asio CONFIG REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wdeprecated-declarations -g -pthread")
endif()

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../common
	)
	
file(GLOB SELF_TEMP_SRC_FILES
	"src/*.cpp"
	"src/*.h"
	)
source_group(src FILES ${SELF_TEMP_SRC_FILES})
list(APPEND SELF_SRC_FILES ${SELF_TEMP_SRC_FILES})

file(GLOB SELF_TEMP_SRC_FILES
	"../../include/*.h"
	)
source_group(include FILES ${SELF_TEMP_SRC_FILES})
list(APPEND SELF_SRC_FILES ${SELF_TEMP_SRC_FILES})

file(GLOB SELF_TEMP_SRC_FILES
	"../../include/asyncio/*.h"
	)
source_group(include\\asyncio FILES ${SELF_TEMP_SRC_FILES})
list(APPEND SELF_SRC_FILES ${SELF_TEMP_SRC_FILES})

file(GLOB SELF_TEMP_SRC_FILES
	"../../include/asyncio/codec/*.h"
	)
source_group(include\\asyncio\\codec FILES ${SELF_TEMP_SRC_FILES})
list(APPEND SELF_SRC_FILES ${SELF_TEMP_SRC_FILES})

add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_NO_DEPRECATED)
add_executable(${PROJECT_NAME} ${SELF_SRC_FILES})

#if (WIN32)
#	target_link_libraries(${PROJECT_NAME} jemalloc)
#else()
#	target_link_libraries(${PROJECT_NAME} 
#		PRIVATE asio asio::asio
#		)
#endif()

