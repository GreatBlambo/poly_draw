#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <gltext/gltext.h>
#include <stb/stretchy_buffer.h>

#include "gl_helpers.h"
#include "gl_pixel_display.h"
#include "draw.h"
#include "geom.h"
#include "transform.h"

#define WIDTH 800
#define HEIGHT 600

static void error_callback(int error, const char *description)
{
  fprintf(stderr, description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

enum mode_t
{
  NIL,
  DRAW,
  DEFORM,
  TRANSFORM,
  MORPH
};
    
typedef struct
{
  GLTtext* header;
  GLTtext* instructions;
  GLTtext* warning;
  GLTtext* transform_mode;
} ui_t;

void ui_init(ui_t* ui)
{
  gltInit();
  
  ui->header = gltCreateText();
  gltSetText(ui->header,
             "Press 1-4 for different modes:\n"
             "1: DRAW, 2: DEFORM, 3: TRANSFORM, 4: MORPH, R: Reset");
  ui->instructions = gltCreateText();
  ui->warning = gltCreateText();
  ui->transform_mode = gltCreateText();
}

void ui_warn_intersection(ui_t* ui)
{
  gltSetText(ui->warning, "Warning: a polygon is self-intersecting.");
}

void ui_draw(ui_t* ui, mode_t mode)
{
  static int yoffset = 32;
  static mode_t current_mode = NIL;
  if (current_mode != mode)
  {
    switch (mode)
    {
     case DRAW:
       gltSetText(ui->instructions,
                  "Draw mode: Left click to introduce points, right click to complete\n"
                  "the polygon.");
       break;
     case DEFORM:
       gltSetText(ui->instructions,
                  "Deform mode: Left click to select points nearby, drag to move.");
       break;
     case TRANSFORM:
       gltSetText(ui->instructions,
                  "Transform mode: Left click to select a polygon, then right click to set a local \n origin\n"
                  "Press: a) translate s) rotate, d) scale, \n f) shear, g) reflect, h) select another polygon\n"
                  "Drag the left mouse button to perform the transformation, press enter to end");
       break;
     case MORPH:
       gltSetText(ui->instructions,
                  "Morph mode: Drag vertices into new positions to establish correspondence. Press enter to morph");
       break;
     default:
       gltSetText(ui->instructions,
                  "");
       break;
    }
    current_mode = mode;
  }
  
  gltColor(1.0f, 1.0f, 1.0f, 1.0f);
  gltDrawText2DAligned(ui->transform_mode, WIDTH, HEIGHT, 1, GLT_RIGHT, GLT_BOTTOM);
  gltDrawText2D(ui->header, 0, 0, 1);
  gltColor(1.0f, 0.5f, 1.0f, 1.0f);
  gltDrawText2D(ui->instructions, 0, yoffset, 1);
  gltColor(1.0f, 0.0f, 0.0f, 1.0f);
  gltDrawText2D(ui->warning, 0, HEIGHT - yoffset, 1);
  
  gltSetText(ui->warning, "");
  gltSetText(ui->transform_mode, "");
}

void ui_destroy(ui_t* ui)
{
  gltDeleteText(ui->header);
  gltDeleteText(ui->instructions);
  gltDeleteText(ui->warning);
  gltDeleteText(ui->transform_mode);
  gltTerminate();
}

// Modes

static int g_last_mouse_l_state = GLFW_RELEASE;
static int g_last_mouse_r_state = GLFW_RELEASE;

void draw_mode(pixel_display_t* display, ui_t* ui, GLFWwindow* window, polygon_t** polygons)
{
  static pixel_t line_color = {.r = 255, .g = 0, .b = 0, .a = 255};
  
  if (!*polygons)
  {
    polygon_t new_poly;
    create_polygon(&new_poly);
    sb_push(*polygons, new_poly);
  }

  double x, y;
  glfwGetCursorPos(window, &x, &y);

  point_t new_point;
        
  new_point.x = (int) x;
  new_point.y = (int) y;
  
  polygon_t* current_polygon = &sb_last(*polygons);
  
  if (current_polygon->points)
  {
    point_t last_point;
    last_point.x = sb_last(current_polygon->points).x;
    last_point.y = sb_last(current_polygon->points).y;
    
    draw_line(display, line_color, new_point.x, new_point.y, last_point.x, last_point.y);

    if (line_poly_intersect(new_point, last_point, current_polygon))
      ui_warn_intersection(ui);
  }
  
  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  if (state == GLFW_PRESS && g_last_mouse_l_state != GLFW_PRESS)
  {
    polygon_add_point(current_polygon, new_point); // new point
  }
  g_last_mouse_l_state = state;
  state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
  if (state == GLFW_PRESS && g_last_mouse_r_state != GLFW_PRESS)
  {    
    if (current_polygon->points && current_polygon->num_points >= 2)
    {
      polygon_close(current_polygon, new_point);
          
      polygon_t new_polygon;
      create_polygon(&new_polygon);
      sb_push(*polygons, new_polygon);
    }
  }
  g_last_mouse_r_state = state;
}

point_t* closest_point(int x, int y, polygon_t** polygons, double min_d)
{
  int d = 0;
  point_t* closest_point = NULL;
  for (int i = 0; i < sb_count(*polygons); i++)
  {
    polygon_t* p = *polygons + i;
    for (int j = 0; j < sb_count(p->points); j++)
    {
      point_t* point = p->points + j;

      double dx = (double) abs(x - point->x);
      double dy = (double) abs(y - point->y);
      double new_d = sqrt(dx * dx + dy * dy);
      if (!closest_point
          || new_d < d)
      {
        closest_point = point;
        d = new_d;
      }
    }
  }
  if (d > min_d)
    return NULL;
  return closest_point;
}

void deform_mode(pixel_display_t* display, GLFWwindow* window, polygon_t** polygons)
{
  static pixel_t point_color = {.r = 255, .g = 0, .b = 0, .a = 255};
  static point_t* dragged_point = NULL;
  
  if (!*polygons)
    return;

  double x, y;
  glfwGetCursorPos(window, &x, &y);

  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

  if (state == GLFW_RELEASE)
    dragged_point = closest_point((int) x, (int) y, polygons, 10);

  if (!dragged_point)
    return;
  
  if (state == GLFW_PRESS)
  {
    dragged_point->x = (int) x;
    dragged_point->y = (int) y;
  }

  draw_point(display, point_color, dragged_point->x, dragged_point->y, 5); 
}

polygon_t* closest_polygon(int x, int y, polygon_t** polygons, double min_d)
{
  int d = 0;
  polygon_t* closest_p = NULL;
  for (int i = 0; i < sb_count(*polygons); i++)
  {
    polygon_t* p = *polygons + i;
    for (int j = 0; j < sb_count(p->points); j++)
    {
      point_t* point = p->points + j;

      double dx = (double) abs(x - point->x);
      double dy = (double) abs(y - point->y);
      double new_d = sqrt(dx * dx + dy * dy);
      
      if (!closest_p
          || new_d < d)
      {
        closest_p = p;
        d = new_d;
      }
    }
  }
  
  if (d > min_d)
    return NULL;
  return closest_p;
}
  
static pixel_t red   = {.r = 255, .g = 0, .b = 0, .a = 255};
static pixel_t blue  = {.r = 0, .g = 0, .b = 255, .a = 255};
static pixel_t green = {.r = 0, .g = 255, .b = 0, .a = 255};
  
static int last_l_mouse_state = GLFW_RELEASE;
static int last_r_mouse_state = GLFW_RELEASE;

void transform_mode(pixel_display_t* display, ui_t* ui, GLFWwindow* window, polygon_t** polygons)
{  
  typedef enum
  {
    SELECT,
    TRANSLATE,
    ROTATE,
    SCALE,
    SHEAR,
    REFLECT
  } transform_mode_t;
  
  static transform_mode_t mode = SELECT;
  static int poly_index = -1;

  static int cached_num_polys = -1;
  
  static point_t origin = {.x = 0, .y = 0};

  if (!*polygons)
    return;

  if (sb_count(*polygons) < cached_num_polys)
  {
    mode = SELECT;
    poly_index = -1;
  }
  cached_num_polys = sb_count(*polygons);

  static mat3_t transform_mat = {
    .vals = {
      1, 0, 0,
      0, 1, 0,
      0, 0, 1
    }
  };
    
  // set transform mode
  if (poly_index >= 0)
  {
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      mode = TRANSLATE;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      mode = ROTATE;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      mode = SCALE;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
      mode = SHEAR;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
      mode = REFLECT;
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
      mode = SELECT;
  }
  
  // perform transforms
  {    
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    
    int l_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int r_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    
    if (poly_index >= 0)
    {      
      polygon_t* polygon = (*polygons) + poly_index;
      draw_polygon_points(display, blue, polygon, 5);
      
      vec3_t target;
      target.vals[0] = polygon->points[0].x - origin.x;
      target.vals[1] = polygon->points[0].y - origin.y;
      target.vals[2] = 1;

      target = mat3_vec3_mult(&transform_mat, &target);

      point_t transformed_point;
      transformed_point.x = target.vals[0] + origin.x;
      transformed_point.y = target.vals[1] + origin.y;

      vec3_t transformed_vec;
      transformed_vec.vals[0] = transformed_point.x - origin.x;
      transformed_vec.vals[1] = transformed_point.y - origin.y;
      transformed_vec.vals[2] = 1;
      
      if (r_mouse_state == GLFW_PRESS)
      {
        origin.x = x;
        origin.y = y;
      }
      
      draw_point(display, red, origin.x, origin.y, 5);
      draw_line(display, red, origin.x, origin.y, origin.x, 0);
      draw_line(display, red, origin.x, origin.y, origin.x, display->h);
      draw_line(display, red, origin.x, origin.y, 0, origin.y);
      draw_line(display, red, origin.x, origin.y, display->w, origin.y);
      
      draw_point(display, blue, transformed_point.x, transformed_point.y, 5);
      draw_line(display, blue, origin.x, origin.y, transformed_point.x, transformed_point.y);
        
      if (mode == TRANSLATE)
      {
        gltSetText(ui->transform_mode, "TRANSLATE: Click where you want to translate\n");
        static mat3_t translate_mat;
        if (l_mouse_state == GLFW_PRESS)
        {
          point_t trans_point;
          trans_point.x = (int) x;
          trans_point.y = (int) y;

          mat3_translation(trans_point.x - transformed_point.x, trans_point.y - transformed_point.y, &translate_mat);

          // Draw preview line          
          draw_point(display, green, trans_point.x, trans_point.y, 5);
          draw_line(display, green, transformed_point.x, transformed_point.y, trans_point.x, trans_point.y);
        }

        if (l_mouse_state == GLFW_RELEASE
            && last_l_mouse_state == GLFW_PRESS)
        {
          mat3_mult(&transform_mat, &translate_mat, &transform_mat);
        }
      }
      else if (mode == ROTATE)
      {
        gltSetText(ui->transform_mode, "ROTATE: Click and drag to rotate the point\n");
        static mat3_t rotate_mat;
        if (l_mouse_state == GLFW_PRESS)
        {
          // Get angle between user point and some point on polygon
          vec2_t dest_pos;
          dest_pos.vals[0] = x - origin.x;
          dest_pos.vals[1] = y - origin.y;
          
          vec2_t vert_pos;
          vert_pos.vals[0] = transformed_point.x - origin.x;
          vert_pos.vals[1] = transformed_point.y - origin.y;
          
          float theta;
          theta = atan2((vert_pos.vals[0] * dest_pos.vals[1]) - (vert_pos.vals[1] * dest_pos.vals[0]),
                          vec2_mult(&dest_pos, &vert_pos));

          // Produce rotation matrix
          mat3_rotation(theta, &rotate_mat);

          // Draw preview line
          vec3_t preview_vec = mat3_vec3_mult(&rotate_mat, &transformed_vec);
          draw_point(display, green, preview_vec.vals[0] + origin.x, preview_vec.vals[1] + origin.y, 5);
          draw_line(display, green, origin.x, origin.y, preview_vec.vals[0] + origin.x, preview_vec.vals[1] + origin.y);
        }

        if (l_mouse_state == GLFW_RELEASE
            && last_l_mouse_state == GLFW_PRESS)
        {
          mat3_mult(&transform_mat, &rotate_mat, &transform_mat);
        }
      }
      else if (mode == SCALE)
      {
        gltSetText(ui->transform_mode, "SCALE: Click along the axes to scale in that direction\n");
        static mat3_t scale_mat;
        if (l_mouse_state == GLFW_PRESS)
        {          
          if (transformed_vec.vals[0] != 0
              && transformed_vec.vals[1] != 0)
          {          
            float hx = (x - origin.x) / transformed_vec.vals[0];
            float hy = (y - origin.y) / transformed_vec.vals[1];
            
            mat3_scale(hx, hy, &scale_mat);

            // Draw preview line
            draw_point(display, green, x, y, 5);
            draw_line(display, green, origin.x, origin.y, x, y);
          }
        }

        if (l_mouse_state == GLFW_RELEASE
            && last_l_mouse_state == GLFW_PRESS)
        {
          mat3_mult(&transform_mat, &scale_mat, &transform_mat);
        }
      }
      else if (mode == SHEAR)
      {
        gltSetText(ui->transform_mode, "SHEAR: Click along the axes to shear in that direction\n");
        static mat3_t shear_mat;
        if (l_mouse_state == GLFW_PRESS)
        {          
          float hx = -(x - origin.x) / (display->w);
          float hy = (y - origin.y) / (display->h);
            
          mat3_shear(hx, hy, &shear_mat);

          // Draw preview line
          draw_point(display, green, x, y, 5);
          draw_line(display, green, origin.x, origin.y, x, y);
        }

        if (l_mouse_state == GLFW_RELEASE
            && last_l_mouse_state == GLFW_PRESS)
        {
          mat3_mult(&transform_mat, &shear_mat, &transform_mat);
        }
      }
      else if (mode == REFLECT)
      {
        gltSetText(ui->transform_mode, "REFLECT: Click to reflect on y = -x\n");
        static mat3_t reflect_mat;
        if (l_mouse_state == GLFW_RELEASE
            && last_l_mouse_state == GLFW_PRESS)
        {
          mat3_reflect(&reflect_mat);
          mat3_mult(&transform_mat, &reflect_mat, &transform_mat);
        }
      }

      // Commit transformation
      if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
      {
        // Transform polygon
        for (int i = 0; i < polygon->num_points; i++)
        {
          vec3_t pos;
          pos.vals[0] = polygon->points[i].x - origin.x;
          pos.vals[1] = polygon->points[i].y - origin.y;
          pos.vals[2] = 1;
          
          pos = mat3_vec3_mult(&transform_mat, &pos);

          polygon->points[i].x = pos.vals[0] + origin.x;
          polygon->points[i].y = pos.vals[1] + origin.y;
        }
        mat3_identity(&transform_mat);
      }
    }
    
    if (mode == SELECT)
    {
      polygon_t* closest_poly = closest_polygon((int) x, (int) y, polygons, 20);
      if (closest_poly
          && closest_poly->closed
          && !closest_poly->complex)
      {
        draw_polygon_points(display, red, closest_poly, 5);
      
        if (l_mouse_state == GLFW_PRESS
            && last_l_mouse_state == GLFW_RELEASE)
        {
          poly_index = (closest_poly - *polygons);
          mat3_identity(&transform_mat);
        }
      }
    }
    
    last_l_mouse_state = l_mouse_state;
    last_r_mouse_state = r_mouse_state;
  }
}

point_t* closest_point_in_poly(int x, int y, polygon_t* p, double min_d)
{
  int d = 0;
  point_t* closest_point = NULL;
  for (int j = 0; j < sb_count(p->points); j++)
  {
    point_t* point = p->points + j;
    
    double dx = (double) abs(x - point->x);
    double dy = (double) abs(y - point->y);
    double new_d = sqrt(dx * dx + dy * dy);
    if (!closest_point
        || new_d < d)
    {
      closest_point = point;
      d = new_d;
    }
  }
  if (d > min_d)
    return NULL;
  return closest_point;
}

void morph_mode(pixel_display_t* display, ui_t* ui, GLFWwindow* window, polygon_t** polygons)
{
  static int poly_index = -1;
  static int point_index = -1;
  static point_t* points = NULL;
  static bool animating = false;

  if (!*polygons)
  {
    poly_index = -1;
    point_index = -1;
    if (points)
      free(points);
    points = NULL;
    animating = false;
    return;
  }
  
  // if animating, lerp between each point
  if (animating)
  {
    //static float t = 0;
    
    polygon_t* p = *polygons + poly_index;
    int n = 0;
    for (int i = 0; i < p->num_points; i++)
    {
      vec2_t p1;
      p1.vals[0] = p->points[i].x;
      p1.vals[1] = p->points[i].y;
      
      vec2_t p2;
      p2.vals[0] = points[i].x;
      p2.vals[1] = points[i].y;

      //t += 0.01;
      
      vec2_t frac = vec2_lerp(&p1, &p2, 0.2);
      p->points[i].x = frac.vals[0];
      p->points[i].y = frac.vals[1];

      vec2_t diff;
      diff.vals[0] = points[i].x - p->points[i].x;
      diff.vals[1] = points[i].y - p->points[i].y;

      float len = vec2_len(&diff);
      if (len < 0.5)
      {
        n++;
      }
    }

    if (n == p->num_points)
    {
      animating = false;
    
      poly_index = -1;
      point_index = -1;
      points = NULL;
    }
    return;
  }
  
  double x, y;
  glfwGetCursorPos(window, &x, &y);
    
  int l_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

  if (poly_index == -1
      && points)
    free(points);

  if (l_mouse_state == GLFW_PRESS)
  {
    if (poly_index == -1)
    {
      polygon_t* closest_poly = closest_polygon((int) x, (int) y, polygons, 20);
      
      if (closest_poly
          && closest_poly->closed
          && !closest_poly->complex)
      {
        draw_polygon_points(display, red, closest_poly, 5);
      
        if (l_mouse_state == GLFW_PRESS
            && last_l_mouse_state == GLFW_RELEASE)
        {
          poly_index = (closest_poly - *polygons);
        }
      
        point_t* point = closest_point_in_poly(x, y, closest_poly, 20);
        point_index = point - closest_poly->points;
        points = malloc(sizeof(point_t) * closest_poly->num_points);
        
        for (int i = 0; i < closest_poly->num_points; i++)
        {
          points[i] = closest_poly->points[i];
        }
      }
    }
    else
    {
      if (point_index != -1)
      {
        points[point_index].x = x;
        points[point_index].y = y;
      }
      else
      {
        polygon_t* p = *polygons + poly_index;
        point_t* point = closest_point_in_poly(x, y, p, 20);
        if (point)
        {
          point_index = point - p->points;
        }
      }
    }
  }
  else
  {
    if (point_index != -1)
    {
      point_index = -1;
    }
  }

  if (poly_index != -1)
  {
    polygon_t* p = *polygons + poly_index;
    draw_polygon_points(display, blue, p, 5);
    for (int i = 0; i < p->num_points; i++)
    {
      draw_line(display, red, p->points[i].x, p->points[i].y, points[i].x, points[i].y);
      draw_point(display, red, points[i].x, points[i].y, 5);
    }
  }
  else
  {
    for (int i = 0; i < sb_count(*polygons); i++)
      draw_polygon_points(display, red, (*polygons) + i, 5);
  }

  if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
  {
    animating = true;
  }
}

int main(void) {

  // Context setup

  GLFWwindow *window;
  
  if (!glfwInit())
    exit(EXIT_FAILURE);
  
  glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  
  glfwMakeContextCurrent(window);
  
  gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

  glfwSetKeyCallback(window, key_callback);

  // Setup quad

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  
  static const GLfloat quad[] =
    {
      -1.0f, -1.0f, 0.0f, 1.0f,
      1.0f, -1.0f, 1.0f, 1.0f,
      -1.0f,  1.0f, 0.0f, 0.0f,
      -1.0f,  1.0f, 0.0f, 0.0f,
      1.0f, -1.0f, 1.0f, 1.0f,
      1.0f,  1.0f, 1.0f, 0.0f
    };

  vert_attrib_t attribs[2];
  create_vert_attrib(&attribs[0], "position", 0, 2, GL_FLOAT, false, 4 * sizeof(GL_FLOAT), 0);
  create_vert_attrib(&attribs[1], "uv", 1, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), 2 * sizeof(GLfloat));

  vert_spec_t vert_spec;
  create_vertex_spec(&vert_spec, attribs, 2);

  GLuint quad_vbo = create_buffer(quad, sizeof(quad), GL_ARRAY_BUFFER, GL_STATIC_DRAW);

  // Setup shaders

  static const char vert_source[] =
    "#version 330 core\n"
    "in vec2 position;"
    "in vec2 uv;"
    "out vec2 frag_uv;"
    "void main()"
    "{"
    "  frag_uv = uv;"
    "  gl_Position = vec4(position, 0.0, 1.0);"
    "}";

  static const char frag_source[] =
    "#version 330 core\n"
    "in vec2 frag_uv;"
    "out vec4 color;"
    "uniform sampler2D tex;"
    "void main()"
    "{"
    "  color = texture(tex, frag_uv);"
    "}";

  GLuint shaders[2];  
  shaders[0] = compile_shader(vert_source, sizeof(vert_source), GL_VERTEX_SHADER);
  shaders[1] = compile_shader(frag_source, sizeof(frag_source), GL_FRAGMENT_SHADER);

  GLuint program = link_shader_program(shaders, 2, &vert_spec);
  detach_shaders(program, shaders, 2);

  destroy_shader(shaders[0]);
  destroy_shader(shaders[1]);
  
  //
  
  pixel_display_t display;
  create_pixel_display(&display, WIDTH, HEIGHT);

  pixel_t bg_color;
  bg_color.r = 200;
  bg_color.g = 255;
  bg_color.b = 200;
  bg_color.a = 255;

  pixel_t line_color;
  line_color.r = 0;
  line_color.g = 0;
  line_color.b = 0;
  line_color.a = 0;

  pixel_t poly_color;
  poly_color.r = 255;
  poly_color.g = 255;
  poly_color.b = 255;
  poly_color.a = 255;

  mode_t mode;
  mode = DRAW;
  
#define MAX_POLYS 10

  ui_t ui;
  ui_init(&ui);

  polygon_t* polygons = NULL;
  
  while (!glfwWindowShouldClose(window))
  {
    // Input
    glfwPollEvents();

    // Draw
    glClear(GL_COLOR_BUFFER_BIT);
    
    pixel_display_fill_start(&display);
    clear_display(&display, bg_color);
    
    bool intersect_warn = false;

    // Check intersections
    for (int i = 0; i < sb_count(polygons); i++)
    {
      polygon_t* p = polygons + i;
      if (p->complex = poly_self_intersect(p))
        intersect_warn = true;
    }

    if (intersect_warn)
      ui_warn_intersection(&ui);

    // Draw scan fill polys
    for (int i = 0; i < sb_count(polygons); i++)
    {
      polygon_t* p = polygons + i;
      if (!p->complex)
        scan_fill(&display, poly_color, p);
      draw_polygon_bounds(&display, line_color, polygons + i);
    }

    // Modes
    // Get key
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
      mode = DRAW;
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
      mode = DEFORM;
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
      mode = TRANSFORM;
    else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
      mode = MORPH;
    else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
      for (int i = 0; i < sb_count(polygons); i++)
      {
        delete_polygon(polygons + i);
      }
      sb_free(polygons);
      polygons = NULL;
    }
    // Draw to pixel buffer

    if (mode == DRAW)
    {
      draw_mode(&display, &ui, window, &polygons);
    }
    else if (mode == DEFORM)
    {
      deform_mode(&display, window, &polygons);
    }
    else if (mode == TRANSFORM)
    {
      transform_mode(&display, &ui, window, &polygons);
    }
    else if (mode == MORPH)
    {
      morph_mode(&display, &ui, window, &polygons);
    }
    
    pixel_display_fill_end(&display);
    
    // Draw pixel buffer
    glBindVertexArray(vao);
    glViewport(0, 0, display.w, display.h);
    glUseProgram(program);
    
    // Draw pixel buffer quad
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    set_vertex_spec(&vert_spec);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, display.tex);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw text
    ui_draw(&ui, mode);
    
    // Swap backbuffer
    glfwSwapBuffers(window);
  }

  // clean up polygons
  for (int i = 0; i < sb_count(polygons); i++)
  {
    delete_polygon(polygons + i);
  }
  sb_free(polygons);
  
  ui_destroy(&ui);
    
  glDeleteVertexArrays(1, &vao);
    
  delete_pixel_display(&display);

  delete_buffer(quad_vbo);

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}

