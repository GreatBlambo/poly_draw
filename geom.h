#pragma once
#include <stdlib.h>
#include <stdbool.h>

typedef struct point_t 
{
  float x;
  float y;
} point_t;

typedef struct
{
  point_t* points;
  size_t num_points;
  size_t num_edges;
  bool complex;
  bool closed;
} polygon_t;

void create_polygon(polygon_t* poly);

void polygon_add_point(polygon_t* poly, point_t point);

void polygon_close(polygon_t* poly, point_t point);

void delete_polygon(polygon_t* poly);

bool lines_intersect(point_t u1, point_t u2,
                     point_t p1, point_t p2);

bool line_poly_intersect(point_t l1, point_t l2,
                         polygon_t* p);
  
bool poly_self_intersect(polygon_t* p);
