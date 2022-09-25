cmake_minimum_required(VERSION 3.10)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
project( node-raknet )

# RakNet
set(RAKNET_ENABLE_SAMPLES false CACHE BOOL "Enable samples" FORCE )
set(RAKNET_ENABLE_DLL false CACHE BOOL "Enable DLL" FORCE)

add_subdirectory( raknet )

set (CMAKE_CXX_STANDARD 14)
add_compile_definitions(MAXIMUM_NUMBER_OF_INTERNAL_IDS=20)
include_directories("raknet/Source")

include_directories(${CMAKE_JS_INC})
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Include N-API wrappers
execute_process(
  COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
message(STATUS "NAPI DIR='${NODE_ADDON_API_DIR}'")
include_directories(${NODE_ADDON_API_DIR})

message(STATUS "Include dirs")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()


# define NPI_VERSION
add_compile_definitions(DNAPI_VERSION=6)

target_link_libraries( node-raknet RakNetLibStatic )
