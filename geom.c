#include <stdbool.h>
#include <stb/stretchy_buffer.h>

#include "geom.h"
#include "draw.h"

void create_polygon(polygon_t* poly)
{
  poly->points = NULL;
  poly->num_points = 0;
  poly->num_edges = 0;
  poly->complex = false;
  poly->closed = false;
}

void polygon_add_point(polygon_t* poly, point_t point)
{
  sb_push(poly->points, point);
  poly->num_edges = poly->num_points;
  poly->num_points++;
  poly->complex = poly_self_intersect(poly);
}

void polygon_close(polygon_t* poly, point_t point)
{
  sb_push(poly->points, point);
  poly->num_edges += 2;
  poly->num_points++;
  poly->closed = true;
  poly->complex = poly_self_intersect(poly);
}

void delete_polygon(polygon_t* poly)
{
  if (poly->points)
    sb_free(poly->points);
  poly->num_points = 0;
}

int line_coefficient(point_t p, point_t l1, point_t l2)
{
  return ((p.x - l1.x) * (l2.y - l1.y)) - ((p.y - l1.y) * (l2.x - l1.x));
}

bool lines_intersect(point_t u1, point_t u2,
                     point_t p1, point_t p2)
{
  int u1_c = line_coefficient(u1, p1, p2);
  int u2_c = line_coefficient(u2, p1, p2);

  bool u_result = (u1_c > 0 && u2_c < 0) || (u2_c > 0 && u1_c < 0);
  
  if (!u_result)
    return false;
  
  int p1_c = line_coefficient(p1, u1, u2);
  int p2_c = line_coefficient(p2, u1, u2);

  bool p_result = (p1_c > 0 && p2_c < 0) || (p2_c > 0 && p1_c < 0);

  if (!p_result)
    return false;  

  return true;
}

bool line_poly_intersect(point_t l1, point_t l2,
                         polygon_t* p)
{
  for (int j = 0; j < p->num_edges; j++)
  {
    // edge 1
    point_t* u1 = p->points + j;
    point_t* u2 = p->points + ((j +  1) % p->num_points);
    
    if (lines_intersect(*u1, *u2, l1, l2))
    {
      return true;
    }
  }
  return false;
}

bool poly_self_intersect(polygon_t* p)
{
  for (int j = 0; j < p->num_edges; j++)
  {
    // edge 1
    point_t* u1 = p->points + j;
    point_t* u2 = p->points + ((j +  1) % p->num_points);
    
    if (line_poly_intersect(*u1, *u2, p))
    {
      return true;
    }
  }
  return false;
}
