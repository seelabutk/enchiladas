CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(Enchiladas)

SET(CMAKE_CXX_STANDARD 11)

ADD_SUBDIRECTORY("resources/pbnj")

ADD_SUBDIRECTORY("resources/pistache")

INCLUDE_DIRECTORIES(include "resources/pistache/include/")

FILE(GLOB SOURCES "src/*.cpp")

ADD_EXECUTABLE(server ${SOURCES})

TARGET_LINK_LIBRARIES(server pbnj pistache) 

# Copy the index.html code whenever enchiladas gets built
ADD_CUSTOM_COMMAND(TARGET server PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/src/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html)

# Copy the js directory whenever enchiladas gets built
ADD_CUSTOM_COMMAND(TARGET server PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/js ${CMAKE_CURRENT_BINARY_DIR}/js)

# Copy the css directory whenever enchiladas gets built
ADD_CUSTOM_COMMAND(TARGET server PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/css ${CMAKE_CURRENT_BINARY_DIR}/css)
