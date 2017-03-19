cmake_minimum_required(VERSION 3.2)

set(APP_NAME polydraw)

set(SOURCES main.c
            gl_helpers.c
	    gl_pixel_display.c
            draw.c
            geom.c
            transform.c)

## Third party libs

include_directories(thirdparty)

# GLEW

add_library(glad STATIC
                 thirdparty/glad/src/glad.c
                 thirdparty/glad/include
)
include_directories(thirdparty/glad/include)

# GLFW

# Building only the GLFW lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

add_subdirectory(thirdparty/glfw)

find_package(OpenGL REQUIRED)

##

add_executable(${APP_NAME} ${SOURCES})
target_link_libraries(${APP_NAME} ${OPENGL_LIBRARIES} glfw glad)
