/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#include "azimuth/util/polygon.h"

#include <assert.h>
#include <math.h> // for INFINITY and isfinite
#include <stdbool.h>
#include <stdlib.h> // for NULL

#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_polygon_contains(az_polygon_t polygon, az_vector_t point) {
  const az_vector_t *vertices = polygon.vertices;
  // We're going to do a simple ray-casting test, where we imagine casting a
  // ray from the point in the +X direction; if we intersect an even number of
  // edges, then we must be outside the polygon.  So we start by assuming that
  // we're outside (inside = false), and invert for each edge we intersect.
  bool inside = false;
  // Iterate over all edges in the polygon.  On each iteration, i is the index
  // of the "primary" vertex, and j is the index of the vertex that comes just
  // after it in the list (wrapping around at the end).
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    // Common case: if the edge is completely above or below the ray, then
    // skip it.
    if ((vertices[i].y > point.y && vertices[j].y > point.y) ||
        (vertices[i].y <= point.y && vertices[j].y <= point.y)) {
      continue;
    }
    // Okay, the edge straddles the X-axis passing through the point.  But if
    // both vertices are to the left of the point, then we can skip this edge.
    if (vertices[i].x < point.x && vertices[j].x < point.x) {
      continue;
    }
    // Conversely, if both vertices are to the right of the point, then we
    // definitely hit the edge.
    if (vertices[i].x > point.x && vertices[j].x > point.x) {
      inside = !inside;
      continue;
    }
    // Otherwise, we can't easily be sure.  Compute the intersection of the
    // edge with the X-axis passing through the point.  If the X-coordinate of
    // the intersection is to the right of the point, then this is an
    // edge-crossing.
    if (point.x <= vertices[i].x + (point.y - vertices[i].y) *
        (vertices[j].x - vertices[i].x) / (vertices[j].y - vertices[i].y)) {
      inside = !inside;
      continue;
    }
  }
  return inside;
}

bool az_convex_polygon_contains(az_polygon_t polygon,
                                az_vector_t point) {
  const az_vector_t *vertices = polygon.vertices;
  // Iterate over all edges in the polygon.  On each iteration, i is the index
  // of the "primary" vertex, and j is the index of the vertex that comes just
  // after it in the list (wrapping around at the end).
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    // Use a cross-product to determine if the point is outside the given edge,
    // using the assumption that vertices come in counter-clockwise order.
    // Since this is a convex polygon, the point is outside the polygon iff it
    // is outside at least once edge.
    if (az_vcross(az_vsub(point, vertices[i]),
                  az_vsub(vertices[j], vertices[i])) >= 0.0) {
      return false;
    }
  }
  return true;
}

static bool az_ray_hits_polygon_internal(
    az_polygon_t polygon, az_vector_t start, az_vector_t delta,
    double *time_out, az_vector_t *normal_out) {
  assert(time_out != NULL);
  const az_vector_t *vertices = polygon.vertices;
  // We're going to iterate through the edges of the polygon, testing each one
  // for an intersection with the ray.  We keep track of the smallest "time"
  // (from 0 to 1) for which the ray hits something.  We also keep track of how
  // many edges we hit at all (at any nonnegative "time"), because if we hit an
  // odd number of edges that means we started inside the polygon, and should
  // actually hit at time zero.
  double best_time = INFINITY;
  az_vector_t best_edge = AZ_VZERO;
  int hits = 0;
  // Iterate over all edges in the polygon.  On each iteration, i is the index
  // of the "primary" vertex, and j is the index of the vertex that comes just
  // after it in the list (wrapping around at the end).
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    const az_vector_t edelta = az_vsub(vertices[j], vertices[i]);
    const double denom = az_vcross(delta, edelta);
    // Make sure the edge isn't parallel to the ray.
    if (denom == 0.0) continue;
    const az_vector_t rel = az_vsub(vertices[i], start);
    // Make sure that the ray hits the edge line between the two vertices.
    const double u = az_vcross(rel, delta) / denom;
    assert(isfinite(u));
    if (u < 0.0 || u >= 1.0) continue;
    // Make sure that the ray hits the edge within the bounds of the ray.
    const double t = az_vcross(rel, edelta) / denom;
    assert(isfinite(t));
    if (t < 0.0) continue;
    ++hits;
    if (t > 1.0) continue;
    if (t < best_time) {
      best_time = t;
      best_edge = edelta;
    }
  }
  // If we started inside the polygon, then we actually hit at time zero.
  if (hits % 2 != 0) {
    best_time = 0.0;
    best_edge = AZ_VZERO;
  }
  // If the ray hits the polygon, store the collision point in point_out, and
  // the normal vector in normal_out.
  const bool did_hit = best_time != INFINITY;
  if (did_hit) {
    assert(hits > 0);
    assert(isfinite(best_time));
    assert(0.0 <= best_time && best_time <= 1.0);
    *time_out = best_time;
    if (normal_out != NULL) {
      if (best_edge.x == 0.0 && best_edge.y == 0.0) {
        *normal_out = az_vneg(delta);
      } else {
        *normal_out = az_vsub(az_vproj(delta, best_edge), delta);
      }
    }
  }
  return did_hit;
}

bool az_ray_hits_polygon(az_polygon_t polygon, az_vector_t start,
                         az_vector_t delta, az_vector_t *point_out,
                         az_vector_t *normal_out) {
  double time = INFINITY;
  if (az_ray_hits_polygon_internal(polygon, start, delta, &time, normal_out)) {
    assert(isfinite(time));
    if (point_out != NULL) {
      *point_out = az_vadd(start, az_vmul(delta, time));
    }
    return true;
  }
  return false;
}

bool az_ray_hits_polygon_trans(az_polygon_t polygon,
                               az_vector_t polygon_position,
                               double polygon_angle, az_vector_t start,
                               az_vector_t delta, az_vector_t *point_out,
                               az_vector_t *normal_out) {
  if (az_ray_hits_polygon(polygon,
                          az_vrelative(start, polygon_position, polygon_angle),
                          az_vrelative(delta, AZ_VZERO, polygon_angle),
                          point_out, normal_out)) {
    if (point_out != NULL) {
      *point_out = az_vadd(az_vrotate(*point_out, polygon_angle),
                           polygon_position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, polygon_angle);
    }
    return true;
  }
  return false;
}

// This function works much the same as az_polygons_collide, but assumes that
// s_polygon is at the origin, unrotated.  az_polygons_collide is then
// implemented in terms of this function.
static bool az_polygons_collide_internal(
    az_polygon_t s_polygon, az_polygon_t m_polygon, az_vector_t position,
    double angle, az_vector_t delta, az_vector_t *pos_out,
    az_vector_t *impact_out, az_vector_t *normal_out) {
  bool did_hit = false;
  double best_time = INFINITY;
  bool mvertex = false;
  int best_vertex = -1;
  az_vector_t best_normal = AZ_VZERO;
  // Test if s_polygon's vertices hit m_polygon:
  for (int i = 0; i < s_polygon.num_vertices; ++i) {
    double time = INFINITY;
    az_vector_t normal = AZ_VZERO;
    if (az_ray_hits_polygon_internal(m_polygon,
                                     az_vrotate(az_vsub(s_polygon.vertices[i],
                                                        position), -angle),
                                     az_vneg(az_vrotate(delta, -angle)),
                                     &time, &normal)) {
      if (time < best_time) {
        did_hit = true;
        best_time = time;
        best_vertex = i;
        best_normal = normal;
      }
    }
  }
  // Test if m_polygon's vertices hit s_polygon:
  for (int i = 0; i < m_polygon.num_vertices; ++i) {
    double time = INFINITY;
    az_vector_t normal = AZ_VZERO;
    if (az_ray_hits_polygon_internal(s_polygon,
                                     az_vadd(az_vrotate(m_polygon.vertices[i],
                                                        angle), position),
                                     delta, &time, &normal)) {
      if (time < best_time) {
        did_hit = true;
        best_time = time;
        mvertex = true;
        best_vertex = i;
        best_normal = normal;
      }
    }
  }
  // If we hit, populate out args:
  if (did_hit) {
    if (pos_out != NULL) {
      *pos_out = az_vadd(position, az_vmul(delta, best_time));
    }
    if (impact_out != NULL) {
      assert(best_vertex >= 0);
      if (mvertex) {
        assert(best_vertex < m_polygon.num_vertices);
        *impact_out =
          az_vadd(az_vadd(az_vrotate(m_polygon.vertices[best_vertex],
                                     angle), position),
                  az_vmul(delta, best_time));
      } else {
        assert(best_vertex < s_polygon.num_vertices);
        *impact_out = s_polygon.vertices[best_vertex];
      }
    }
    if (normal_out != NULL) {
      if (mvertex) {
        *normal_out = best_normal;
      } else {
        *normal_out = az_vrotate(az_vneg(best_normal), angle);
      }
    }
  }
  return did_hit;
}

bool az_polygons_collide(az_polygon_t s_polygon, az_vector_t s_position,
                         double s_angle, az_polygon_t m_polygon,
                         az_vector_t m_position, double m_angle,
                         az_vector_t m_delta, az_vector_t *pos_out,
                         az_vector_t *impact_out, az_vector_t *normal_out) {
  if (az_polygons_collide_internal(
          s_polygon, m_polygon,
          az_vrotate(az_vsub(m_position, s_position), -s_angle),
          m_angle - s_angle,
          az_vrotate(m_delta, -s_angle),
          pos_out, impact_out, normal_out)) {
    if (pos_out != NULL) {
      *pos_out = az_vadd(az_vrotate(*pos_out, s_angle), s_position);
    }
    if (impact_out != NULL) {
      *impact_out = az_vadd(az_vrotate(*impact_out, s_angle), s_position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, s_angle);
    }
    return true;
  }
  return false;
}

/*===========================================================================*/