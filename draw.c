#include <stdlib.h>

#include "draw.h"

void draw_point(pixel_display_t* display, pixel_t color, int x, int y, unsigned int radius)
{
  if ((x < 0 || x > display->w) || (y < 0 || y > display->h))
    return;
  
  for (int i = x - radius; i <= x + radius; i++)
  {
    for (int j = y - radius; j <= y + radius; j++)
    {
      if ((i > 0 && i < display->w) && (j > 0 && j < display->h))
        display->buf[i + (j * display->w)] = color;
    }
  }
}

void clear_display(pixel_display_t* display, pixel_t color)
{
  for (int i = 0; i < display->w * display->h; i++)
  {
    display->buf[i] = color;
  }
}

int sign(int x)
{
  return (x > 0) - (x < 0);
}

int swap(int* x, int* y)
{
  int tmp = *x;
  *x = *y;
  *y = *x;
}

void draw_line(pixel_display_t* display, pixel_t color,
               int x1, int y1, int x2, int y2)
{
  size_t w = display->w;
  size_t h = display->h;
  
  int dx = x2 - x1;
  int dy = y2 - y1;

  int x_derr = 2 * abs(dx);
  int y_derr = 2 * abs(dy);

  int x_incr = sign(dx);
  int y_incr = sign(dy);

  if (abs(dx) > abs(dy))
  {
    int d = (2 * abs(dy)) - abs(dx);
    int y = y1;
    for (int x = x1; x != x2; x += x_incr)
    {
      draw_point(display, color, x, y, 0);
      
      if (d > 0)
      {
        y += y_incr;
        d -= x_derr;
      }
      d += y_derr;
    }
  }
  else
  {
    int d = (2 * abs(dx)) - abs(dy);
    int x = x1;
    for (int y = y1; y != y2; y += y_incr)
    {
      draw_point(display, color, x, y, 0);
      
      if (d > 0)
      {
        x += x_incr;
        d -= y_derr;
      }
      d += x_derr;
    }
  }
}

void draw_polygon_bounds(pixel_display_t* display, pixel_t color, polygon_t* p)
{
  for (int i = 0; i < p->num_edges; i++)
  {
    point_t* point = p->points + i;
    point_t* next = p->points + ((i +  1) % p->num_points);
    
    draw_line(display, color,
              point->x, point->y,
              next->x, next->y);
  }
}

void draw_polygon_points(pixel_display_t* display, pixel_t color, polygon_t* p, unsigned int radius)
{
  for (int i = 0; i < p->num_points; i++)
  {
    point_t* point = p->points + i;
    draw_point(display, color,
               point->x, point->y,
               radius);
  }
}


typedef struct
{
  int x;
  int n;
  int vert_index;
} x_entry_t;

int x_entry_comparator(const void* p_x1, const void* p_x2)
{
  x_entry_t* x1 = (x_entry_t*) p_x1;
  x_entry_t* x2 = (x_entry_t*) p_x2;
  
  if (x1->x < x2->x)
    return -1;
  else if (x1->x == x2->x)
    return 0;
  else
    return 1;
}

void scan_fill(pixel_display_t* display, pixel_t color, polygon_t* p)
{  
  if (p->complex)
    return;
  if (p->num_points < 3)
    return;
  if (!p->closed)
    return;

  int max_y = 0;
  int min_y = display->h;
  for (int i = 0; i < p->num_points; i++)
  {
    point_t* point = p->points + i;
    if (point->y > max_y)
      max_y = point->y;
    if (point->y < min_y)
      min_y = point->y;
  }

  x_entry_t* x_intersections = (x_entry_t*) malloc(sizeof(x_entry_t) * p->num_edges);
  
  for (int y = min_y;  y < max_y; y++)
  {   
    point_t p1;
    p1.x = -display->w;
    p1.y = y;
    
    point_t p2;
    p2.x = 2 * display->w;
    p2.y = y;

    size_t num_x = 0;
    for (int i = 0; i < p->num_edges; i++)
    {
      // edge 1
      point_t* u0 = p->points + ((i + p->num_points - 1) % p->num_points); // last point
      point_t* u1 = p->points + i;
      point_t* u2 = p->points + ((i + 1) % p->num_points);

      // horizontal edges
      if (u1->y == u2->y)
      {
        if (u1->y == y)
        {
          x_entry_t new_entry;
          new_entry.vert_index = i;
          new_entry.x = u1->x;

          x_intersections[num_x] = new_entry;
          num_x += 1;
        }
        continue;
      }
      
      if (!((u1->y < y && u2->y > y)
            || (u1->y > y && u2->y < y)
            || u1->y == y))
        continue;
      
      // Find intersection
      x_entry_t new_entry;
      new_entry.vert_index = -1;
      int x = (int) (((double) ((y - u1->y) * (u2->x - u1->x)) / (double) (u2->y - u1->y)) + u1->x);
      new_entry.x = x + 1;
        
      if (u1->x == x
          && u1->y == y)
      {
        new_entry.vert_index = i;
      }
      x_intersections[num_x] = new_entry;
      num_x++;
    }
    
    qsort((void*) x_intersections, num_x, sizeof(x_entry_t), x_entry_comparator);

    int last_diff_1;
    int n = 0;
    for (int i = 0; i < num_x; i++)
    {
      if (i == num_x -1)
        continue;      
      if (x_intersections[i].vert_index >= 0)
      {
        int v = x_intersections[i].vert_index;
        point_t* u0 = p->points + ((v + p->num_points - 1) % p->num_points);
        point_t* u1 = p->points + v;
        point_t* u2 = p->points + ((v + p->num_points + 1) % p->num_points);
      
        int diff_1 = sign(u1->y - u0->y);
        int diff_2 = sign(u1->y - u2->y);
        
        if (i == 0)
          last_diff_1 = diff_1;

        if (diff_1 == 0) // If the vertex tails a flat edge, use the diff from the last edge
          diff_1 = last_diff_1;
        
        if (diff_1 == diff_2
            || diff_2 == 0) // If the vertex is a local maximum or leads a flat edge, do not change the parity
        {
          n += 2;
        }
        else
        {
          n += 1;
        }
        
        last_diff_1 = diff_1;
      }
      else
      {
        n++;
      }
      
      if (n % 2)
        draw_line(display, color, x_intersections[i].x, y, x_intersections[i + 1].x, y);
    }
  }

  free(x_intersections);
}
