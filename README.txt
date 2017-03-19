This project contains a demonstration of various 2D techniques in computer graphics. Among
them is line and polygon drawing, scan-filling, linear transformations, and linear interpolation.

This was run on linux, though I think this should run on any OS. I used CMake for the build system,
which can be used to generate a make file or any other supported project files.

Libraries:
- GLFW for context creation
- GLAD for extension wrangling (similar to GLEW)
- gltext for rendering text
- stb/stretch_buffer.h, a single header dynamic buffer

Please note the general structure of the program: I used core Opengl 3.3, and so did not have
access to the immediate mode functions. The approach I used to compensate for being able to
directly upload pixels through glDrawPoint was to draw a quad which shows a texture that is
streamed to through a Pixel Buffer Object. The actual pixels on screen are being drawn to this
texture through the functions in draw.c. pixel_display_fill_start() is called to initiate streaming
to the pixel display, which is ended with pixel_display_fill_end().

Files:
- gl_helpers.c contains some helper functions I use for opengl projects, such as compiling shaders
- gl_pixel_display.c contains functions for uploading pixels to the screen texture through a PBO
- draw.c contains the methods involved in drawing to the actual pixel buffer that is displayed on
  the rendered quad. This is the meat of the drawing functions, including the midpoint line algorithm
  and a scanline polyfill algorithm.
- geom.c contains functions for processing geometry, including code for detecting line/polygon
  intersection
- transform.c contains my ad-hoc matrix code. It supports 2x2, and 3x3 matrices as well as 3-vectors
  and 2-vectors.

I realize that this approach is far too complicated for this project, but I wanted to practice
my understanding of modern OpenGL
