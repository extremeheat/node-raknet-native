cmake_minimum_required(VERSION 3.10)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
project( node-raknet )

# RakNet
set(RAKNET_ENABLE_SAMPLES FALSE)
set(RAKNET_ENABLE_DLL FALSE)
set(RAKNET_ENABLE_SAMPLES FALSE)

add_subdirectory( raknet )

include_directories("raknet/Source"})

include_directories(${CMAKE_JS_INC})
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Include N-API wrappers
include_directories(
    ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api 
    ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api/src
    ${CMAKE_JS_INC}
)

message(STATUS "Include dirs")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()


# define NPI_VERSION
add_definitions(-DNAPI_VERSION=6)

target_link_libraries( node-raknet RakNetLibStatic )
