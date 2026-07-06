#ifndef WORLEY_H
#define WORLEY_H

#include <stdlib.h>
#include <math.h>

#include "/home/codeleaded/System/Static/Library/Math.h"
#include "/home/codeleaded/System/Static/Library/Graphics.h"
#include "/home/codeleaded/System/Static/Library/Random.h"
#include "/home/codeleaded/System/Static/Library/TransformedView.h"

typedef struct Worley_Intersection {
    Vec2 p;
	Line* l0;
	Line* l1;
	Line* l2;
} Worley_Intersection;

typedef struct Worley_Barrier {
    Line l;
	Vec2 p0;
	Vec2 p1;
} Worley_Barrier;

typedef struct Worley {
    Vector points;
    Vector barriers;
    Vector iss;
	TransformedView tv;
	Rect area;
} Worley;

typedef struct {
    float a, b, c;        // ax + by + c = 0
    int site1, site2;
} VLine;                    // Unendliche Mittelsenkrechte

typedef struct {
    Vec2 start;
    Vec2 end;
    int site1, site2;
    int is_ray;            // 0 = normales Segment, 1 = Ray ins Unendliche
} VoronoiSegment;

typedef struct {
    int site_index;
    Vec2* vertices;
    int num_vertices;
} VoronoiCell;

// Hilfsfunktionen (VLine-Operationen)
float line_value(const VLine* l, float x, float y) {
    return l->a * x + l->b * y + l->c;
}

int lines_intersect(const VLine* l1, const VLine* l2, Vec2* p) {
    float det = l1->a * l2->b - l2->a * l1->b;
    if (fabs(det) < 1e-9) return 0;

    p->x = (l2->b * l1->c - l1->b * l2->c) / det;
    p->y = (l1->a * l2->c - l2->a * l1->c) / det;
    return 1;
}

// Distanz zu einem Punkt
float dist_sq(const Vec2* p, float x, float y) {
    float dx = p->x - x;
    float dy = p->y - y;
    return dx*dx + dy*dy;
}

// Prüft, ob ein Punkt ein gültiger Voronoi-Vertex ist
int is_voronoi_vertex(const Vec2* v, const Vec2* sites, int num_sites, int s1, int s2) {
    float d = dist_sq(&sites[s1], v->x, v->y);
    for (int i = 0; i < num_sites; ++i) {
        if (i == s1 || i == s2) continue;
        if (dist_sq(&sites[i], v->x, v->y) < d - 1e-6) return 0;  // näher an einem anderen Punkt
    }
    return 1;
}

static int build_bisector(const Vec2* sites, int site_a, int site_b, VLine* out) {
    if (!out || site_a < 0 || site_b < 0 || site_a == site_b) {
        return 0;
    }

    float mx = (sites[site_a].x + sites[site_b].x) * 0.5f;
    float my = (sites[site_a].y + sites[site_b].y) * 0.5f;
    float dx = sites[site_b].x - sites[site_a].x;
    float dy = sites[site_b].y - sites[site_a].y;

    out->site1 = site_a;
    out->site2 = site_b;

    if (fabsf(dx) < 1e-9f) {
        out->a = 1.0f;
        out->b = 0.0f;
        out->c = -mx;
    } else if (fabsf(dy) < 1e-9f) {
        out->a = 0.0f;
        out->b = 1.0f;
        out->c = -my;
    } else {
        out->a = dy;
        out->b = -dx;
        out->c = -out->a * mx - out->b * my;
    }

    return 1;
}

static int clip_segment_to_box(
    const Vec2* a, const Vec2* b,
    float minx, float miny, float maxx, float maxy,
    Vec2* out0, Vec2* out1
) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float p[4] = { -dx, dx, -dy, dy };
    float q[4] = { a->x - minx, maxx - a->x, a->y - miny, maxy - a->y };
    float u1 = 0.0f;
    float u2 = 1.0f;

    for (int i = 0; i < 4; ++i) {
        if (fabsf(p[i]) < 1e-9f) {
            if (q[i] < 0.0f) {
                return 0;
            }
            continue;
        }

        float r = q[i] / p[i];
        if (p[i] < 0.0f) {
            if (r > u1) u1 = r;
        } else {
            if (r < u2) u2 = r;
        }

        if (u1 > u2 + 1e-7f) {
            return 0;
        }
    }

    out0->x = a->x + dx * u1;
    out0->y = a->y + dy * u1;
    out1->x = a->x + dx * u2;
    out1->y = a->y + dy * u2;
    return 1;
}

static float point_to_segment_t(const Vec2* a, const Vec2* b, const Vec2* p) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float len2 = dx * dx + dy * dy;
    if (len2 < 1e-9f) {
        return 0.0f;
    }
    return ((p->x - a->x) * dx + (p->y - a->y) * dy) / len2;
}

static int point_is_on_segment(const Vec2* p, const Vec2* a, const Vec2* b, float eps) {
    float t = point_to_segment_t(a, b, p);
    return t >= -eps && t <= 1.0f + eps;
}

static int point_in_rect(const Vec2* p, float minx, float miny, float maxx, float maxy) {
    return p->x >= minx - 1e-6f && p->x <= maxx + 1e-6f && p->y >= miny - 1e-6f && p->y <= maxy + 1e-6f;
}

static int line_intersect_rect(const VLine* line, float minx, float miny, float maxx, float maxy, Vec2* out_points, int* out_count) {
    if (!line || !out_points || !out_count) {
        return 0;
    }

    int count = 0;
    Vec2 points[8];

    if (fabsf(line->b) > 1e-9f) {
        float y = (-line->a * minx - line->c) / line->b;
        if (y >= miny - 1e-6f && y <= maxy + 1e-6f) {
            points[count++] = (Vec2){ minx, y };
        }

        y = (-line->a * maxx - line->c) / line->b;
        if (y >= miny - 1e-6f && y <= maxy + 1e-6f) {
            points[count++] = (Vec2){ maxx, y };
        }
    }

    if (fabsf(line->a) > 1e-9f) {
        float x = (-line->b * miny - line->c) / line->a;
        if (x >= minx - 1e-6f && x <= maxx + 1e-6f) {
            points[count++] = (Vec2){ x, miny };
        }

        x = (-line->b * maxy - line->c) / line->a;
        if (x >= minx - 1e-6f && x <= maxx + 1e-6f) {
            points[count++] = (Vec2){ x, maxy };
        }
    }

    for (int i = 0; i < count; ++i) {
        int duplicate = 0;
        for (int j = 0; j < i; ++j) {
            float d = hypotf(points[i].x - points[j].x, points[i].y - points[j].y);
            if (d < 1e-4f) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) {
            out_points[(*out_count)++] = points[i];
        }
    }

    return *out_count > 0;
}

static int point_is_on_voronoi_edge(
    const Vec2* p,
    const Vec2* sites,
    int num_sites,
    int site_a,
    int site_b,
    float eps
) {
    if (site_a < 0 || site_b < 0 || site_a == site_b || site_a >= num_sites || site_b >= num_sites) {
        return 0;
    }

    float da = dist_sq(&sites[site_a], p->x, p->y);
    float db = dist_sq(&sites[site_b], p->x, p->y);
    if (fabsf(da - db) > 2.0f * eps) {
        return 0;
    }

    for (int i = 0; i < num_sites; ++i) {
        if (i == site_a || i == site_b) {
            continue;
        }

        float d = dist_sq(&sites[i], p->x, p->y);
        if (d < fminf(da, db) - eps) {
            return 0;
        }
    }

    return 1;
}

static int add_voronoi_candidate(
    Vec2* candidates,
    float* candidate_t,
    int* num_candidates,
    int max_candidates,
    const Vec2* p,
    const Vec2* line_point0,
    const Vec2* line_point1,
    const Vec2* sites,
    int num_sites,
    int site_a,
    int site_b,
    float eps
) {
    if (!p || !candidates || !candidate_t || !num_candidates) {
        return 0;
    }

    if (!point_is_on_voronoi_edge(p, sites, num_sites, site_a, site_b, eps)) {
        return 0;
    }

    if (!point_is_on_segment(p, line_point0, line_point1, eps)) {
        return 0;
    }

    for (int c = 0; c < *num_candidates; ++c) {
        float d = hypotf(p->x - candidates[c].x, p->y - candidates[c].y);
        if (d < 1e-4f) {
            return 0;
        }
    }

    if (*num_candidates >= max_candidates) {
        return 0;
    }

    candidates[*num_candidates] = *p;
    candidate_t[*num_candidates] = point_to_segment_t(line_point0, line_point1, p);
    ++(*num_candidates);
    return 1;
}

static int add_unique_point(Vec2* points, int* count, int max_count, const Vec2* p, float eps) {
    if (!points || !count || !p) {
        return 0;
    }

    for (int i = 0; i < *count; ++i) {
        float d = hypotf(points[i].x - p->x, points[i].y - p->y);
        if (d < eps) {
            return 0;
        }
    }

    if (*count >= max_count) {
        return 0;
    }

    points[*count] = *p;
    ++(*count);
    return 1;
}

static int compare_points_by_angle(const void* a, const void* b, void* context) {
    const Vec2* pa = (const Vec2*)a;
    const Vec2* pb = (const Vec2*)b;
    const Vec2* center = (const Vec2*)context;

    float da = atan2f(pa->y - center->y, pa->x - center->x);
    float db = atan2f(pb->y - center->y, pb->x - center->x);
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

typedef struct {
    int a;
    int b;
    int c;
    int alive;
} DelaunayTriangle;

typedef struct {
    int a;
    int b;
    int t1;
    int t2;
} DelaunayEdge;

static void sort_edge_endpoints(int* a, int* b) {
    if (*a > *b) {
        int tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

static int add_delaunay_edge(
    DelaunayEdge* edges,
    int* edge_count,
    int max_edges,
    int a,
    int b,
    int triangle_index
) {
    if (!edges || !edge_count || max_edges <= 0) {
        return 0;
    }

    sort_edge_endpoints(&a, &b);

    for (int i = 0; i < *edge_count; ++i) {
        if (edges[i].a == a && edges[i].b == b) {
            if (edges[i].t1 < 0) {
                edges[i].t1 = triangle_index;
            } else if (edges[i].t2 < 0) {
                edges[i].t2 = triangle_index;
            }
            return 1;
        }
    }

    if (*edge_count >= max_edges) {
        return 0;
    }

    edges[*edge_count].a = a;
    edges[*edge_count].b = b;
    edges[*edge_count].t1 = triangle_index;
    edges[*edge_count].t2 = -1;
    ++(*edge_count);
    return 1;
}

static int triangle_circumcenter(const Vec2* vertices, const DelaunayTriangle* tri, Vec2* out) {
    if (!vertices || !tri || !out) {
        return 0;
    }

    const Vec2* p0 = &vertices[tri->a];
    const Vec2* p1 = &vertices[tri->b];
    const Vec2* p2 = &vertices[tri->c];

    float d = 2.0f * ((p1->x - p0->x) * (p2->y - p0->y) - (p1->y - p0->y) * (p2->x - p0->x));
    if (fabsf(d) < 1e-12f) {
        return 0;
    }

    float ux = ((p0->x * p0->x + p0->y * p0->y) * (p2->y - p1->y) +
                (p1->x * p1->x + p1->y * p1->y) * (p0->y - p2->y) +
                (p2->x * p2->x + p2->y * p2->y) * (p1->y - p0->y)) / d;
    float uy = ((p0->x * p0->x + p0->y * p0->y) * (p1->x - p2->x) +
                (p1->x * p1->x + p1->y * p1->y) * (p2->x - p0->x) +
                (p2->x * p2->x + p2->y * p2->y) * (p0->x - p1->x)) / d;

    out->x = ux;
    out->y = uy;
    return 1;
}

static int circumcircle_contains(const Vec2* vertices, const DelaunayTriangle* tri, const Vec2* p) {
    if (!vertices || !tri || !p) {
        return 0;
    }

    Vec2 center;
    if (!triangle_circumcenter(vertices, tri, &center)) {
        return 0;
    }

    float dx = p->x - center.x;
    float dy = p->y - center.y;
    float radius_sq = (vertices[tri->a].x - center.x) * (vertices[tri->a].x - center.x) +
                      (vertices[tri->a].y - center.y) * (vertices[tri->a].y - center.y);
    return dx * dx + dy * dy <= radius_sq + 1e-9f;
}

VoronoiSegment* compute_voronoi_segments(
    const Vec2* sites, int num_sites,
    int* out_num_segments,
    float minx, float miny, float maxx, float maxy
) {
    if (!out_num_segments || !sites || num_sites < 2) {
        return NULL;
    }

    *out_num_segments = 0;

    const float eps = 1e-4f;
    const int capacity = num_sites * 16 + 64;
    DelaunayTriangle* triangles = malloc(sizeof(DelaunayTriangle) * capacity);
    if (!triangles) {
        return NULL;
    }

    Vec2* vertices = malloc(sizeof(Vec2) * (num_sites + 3));
    if (!vertices) {
        free(triangles);
        return NULL;
    }

    for (int i = 0; i < num_sites; ++i) {
        vertices[i] = sites[i];
    }

    float min_x = sites[0].x;
    float max_x = sites[0].x;
    float min_y = sites[0].y;
    float max_y = sites[0].y;
    for (int i = 1; i < num_sites; ++i) {
        min_x = fminf(min_x, sites[i].x);
        max_x = fmaxf(max_x, sites[i].x);
        min_y = fminf(min_y, sites[i].y);
        max_y = fmaxf(max_y, sites[i].y);
    }

    float span = fmaxf(max_x - min_x, max_y - min_y);
    if (span < 1.0f) {
        span = 1.0f;
    }

    vertices[num_sites + 0] = (Vec2){ min_x - span * 2.0f, min_y - span * 2.0f };
    vertices[num_sites + 1] = (Vec2){ max_x + span * 2.0f, min_y - span * 2.0f };
    vertices[num_sites + 2] = (Vec2){ min_x - span * 2.0f, max_y + span * 2.0f };

    int triangle_count = 0;
    triangles[triangle_count++] = (DelaunayTriangle){ num_sites + 0, num_sites + 1, num_sites + 2, 1 };

    for (int point_index = 0; point_index < num_sites; ++point_index) {
        int* bad = malloc(sizeof(int) * triangle_count);
        if (!bad) {
            free(vertices);
            free(triangles);
            return NULL;
        }

        int bad_count = 0;
        for (int t = 0; t < triangle_count; ++t) {
            if (!triangles[t].alive) {
                continue;
            }
            if (circumcircle_contains(vertices, &triangles[t], &vertices[point_index])) {
                bad[bad_count++] = t;
            }
        }

        if (bad_count == 0) {
            free(bad);
            continue;
        }

        for (int t = 0; t < bad_count; ++t) {
            triangles[bad[t]].alive = 0;
        }

        DelaunayEdge* edges = malloc(sizeof(DelaunayEdge) * (bad_count * 3 + 16));
        if (!edges) {
            free(bad);
            free(vertices);
            free(triangles);
            return NULL;
        }

        int edge_count = 0;
        for (int t = 0; t < bad_count; ++t) {
            int tri_index = bad[t];
            add_delaunay_edge(edges, &edge_count, bad_count * 3 + 16, triangles[tri_index].a, triangles[tri_index].b, tri_index);
            add_delaunay_edge(edges, &edge_count, bad_count * 3 + 16, triangles[tri_index].b, triangles[tri_index].c, tri_index);
            add_delaunay_edge(edges, &edge_count, bad_count * 3 + 16, triangles[tri_index].c, triangles[tri_index].a, tri_index);
        }

        for (int e = 0; e < edge_count; ++e) {
            if (edges[e].t2 >= 0) {
                continue;
            }
            if (triangle_count >= capacity) {
                free(edges);
                free(bad);
                free(vertices);
                free(triangles);
                return NULL;
            }
            triangles[triangle_count++] = (DelaunayTriangle){ edges[e].a, edges[e].b, point_index, 1 };
        }

        free(edges);
        free(bad);
    }

    DelaunayTriangle* filtered = malloc(sizeof(DelaunayTriangle) * triangle_count);
    if (!filtered) {
        free(vertices);
        free(triangles);
        return NULL;
    }

    int filtered_count = 0;
    for (int t = 0; t < triangle_count; ++t) {
        if (!triangles[t].alive) {
            continue;
        }
        if (triangles[t].a >= num_sites || triangles[t].b >= num_sites || triangles[t].c >= num_sites) {
            continue;
        }
        filtered[filtered_count++] = triangles[t];
    }

    free(triangles);
    triangles = filtered;
    triangle_count = filtered_count;

    int max_segments = num_sites * 4 + 32;
    VoronoiSegment* segments = malloc(sizeof(VoronoiSegment) * max_segments);
    if (!segments) {
        free(vertices);
        free(triangles);
        return NULL;
    }

    int num_seg = 0;
    DelaunayEdge* delaunay_edges = malloc(sizeof(DelaunayEdge) * (triangle_count * 3 + 16));
    if (!delaunay_edges) {
        free(vertices);
        free(triangles);
        free(segments);
        return NULL;
    }

    int edge_count = 0;
    for (int t = 0; t < triangle_count; ++t) {
        add_delaunay_edge(delaunay_edges, &edge_count, triangle_count * 3 + 16, triangles[t].a, triangles[t].b, t);
        add_delaunay_edge(delaunay_edges, &edge_count, triangle_count * 3 + 16, triangles[t].b, triangles[t].c, t);
        add_delaunay_edge(delaunay_edges, &edge_count, triangle_count * 3 + 16, triangles[t].c, triangles[t].a, t);
    }

    for (int e = 0; e < edge_count; ++e) {
        if (delaunay_edges[e].a >= num_sites || delaunay_edges[e].b >= num_sites) {
            continue;
        }
        if (delaunay_edges[e].t2 < 0) {
            VLine bisector;
            if (!build_bisector(sites, delaunay_edges[e].a, delaunay_edges[e].b, &bisector)) {
                continue;
            }

            Vec2 midpoint = (Vec2){ (sites[delaunay_edges[e].a].x + sites[delaunay_edges[e].b].x) * 0.5f,
                                    (sites[delaunay_edges[e].a].y + sites[delaunay_edges[e].b].y) * 0.5f };
            Vec2 dir = (Vec2){ -bisector.b, bisector.a };
            float len = hypotf(dir.x, dir.y);
            if (len > 1e-9f) {
                dir.x /= len;
                dir.y /= len;
            }

            Vec2 p0 = (Vec2){ midpoint.x - dir.x * 1000.0f, midpoint.y - dir.y * 1000.0f };
            Vec2 p1 = (Vec2){ midpoint.x + dir.x * 1000.0f, midpoint.y + dir.y * 1000.0f };
            Vec2 clipped0;
            Vec2 clipped1;
            if (!clip_segment_to_box(&p0, &p1, minx, miny, maxx, maxy, &clipped0, &clipped1)) {
                continue;
            }

            if (hypotf(clipped1.x - clipped0.x, clipped1.y - clipped0.y) < 1e-3f) {
                continue;
            }

            segments[num_seg].site1 = delaunay_edges[e].a;
            segments[num_seg].site2 = delaunay_edges[e].b;
            segments[num_seg].is_ray = 0;
            segments[num_seg].start = clipped0;
            segments[num_seg].end = clipped1;
            ++num_seg;
        } else {
            Vec2 c0;
            Vec2 c1;
            if (!triangle_circumcenter(vertices, &triangles[delaunay_edges[e].t1], &c0) ||
                !triangle_circumcenter(vertices, &triangles[delaunay_edges[e].t2], &c1)) {
                continue;
            }

            Vec2 clipped0;
            Vec2 clipped1;
            if (!clip_segment_to_box(&c0, &c1, minx, miny, maxx, maxy, &clipped0, &clipped1)) {
                continue;
            }

            if (hypotf(clipped1.x - clipped0.x, clipped1.y - clipped0.y) < 1e-3f) {
                continue;
            }

            segments[num_seg].site1 = delaunay_edges[e].a;
            segments[num_seg].site2 = delaunay_edges[e].b;
            segments[num_seg].is_ray = 0;
            segments[num_seg].start = clipped0;
            segments[num_seg].end = clipped1;
            ++num_seg;
        }
    }

    free(delaunay_edges);
    free(vertices);
    free(triangles);

    *out_num_segments = num_seg;
    return segments;
}

VoronoiCell* compute_voronoi_cells(
    const Vec2* sites, int num_sites,
    int* out_num_cells,
    float minx, float miny, float maxx, float maxy
) {
    if (!out_num_cells || !sites || num_sites <= 0) {
        return NULL;
    }

    *out_num_cells = 0;
    VoronoiCell* cells = calloc((size_t)num_sites, sizeof(VoronoiCell));
    if (!cells) {
        return NULL;
    }

    const float eps = 1e-4f;
    const int max_edges = num_sites * (num_sites - 1) / 2 + 16;
    typedef struct {
        int site1;
        int site2;
        Vec2 p0;
        Vec2 p1;
    } VoronoiEdge;

    VoronoiEdge* edges = malloc(sizeof(VoronoiEdge) * max_edges);
    if (!edges) {
        free(cells);
        return NULL;
    }

    int num_edges = 0;

    for (int i = 0; i < num_sites; ++i) {
        for (int j = i + 1; j < num_sites; ++j) {
            VLine line;
            if (!build_bisector(sites, i, j, &line)) {
                continue;
            }

            Vec2 box_points[8];
            int box_count = 0;
            if (!line_intersect_rect(&line, minx, miny, maxx, maxy, box_points, &box_count)) {
                continue;
            }

            Vec2 candidates[64];
            int num_candidates = 0;
            for (int c = 0; c < box_count; ++c) {
                add_unique_point(candidates, &num_candidates, 64, &box_points[c], 1e-4f);
            }

            for (int k = 0; k < num_sites; ++k) {
                if (k == i || k == j) {
                    continue;
                }

                VLine ik;
                VLine jk;
                if (build_bisector(sites, i, k, &ik)) {
                    Vec2 inter;
                    if (lines_intersect(&line, &ik, &inter) && point_in_rect(&inter, minx, miny, maxx, maxy)) {
                        add_unique_point(candidates, &num_candidates, 64, &inter, 1e-4f);
                    }
                }

                if (build_bisector(sites, j, k, &jk)) {
                    Vec2 inter;
                    if (lines_intersect(&line, &jk, &inter) && point_in_rect(&inter, minx, miny, maxx, maxy)) {
                        add_unique_point(candidates, &num_candidates, 64, &inter, 1e-4f);
                    }
                }
            }

            if (num_candidates < 2) {
                continue;
            }

            Vec2 dir = (Vec2){ -line.b, line.a };
            float len = hypotf(dir.x, dir.y);
            if (len > 1e-9f) {
                dir.x /= len;
                dir.y /= len;
            }

            for (int c = 0; c < num_candidates; ++c) {
                float t = candidates[c].x * dir.x + candidates[c].y * dir.y;
                for (int d = c + 1; d < num_candidates; ++d) {
                    float tt = candidates[d].x * dir.x + candidates[d].y * dir.y;
                    if (tt < t) {
                        Vec2 tmp = candidates[c];
                        candidates[c] = candidates[d];
                        candidates[d] = tmp;
                        t = tt;
                    }
                }
            }

            for (int c = 0; c + 1 < num_candidates; ++c) {
                if (num_edges >= max_edges) {
                    break;
                }
                edges[num_edges].site1 = i;
                edges[num_edges].site2 = j;
                edges[num_edges].p0 = candidates[c];
                edges[num_edges].p1 = candidates[c + 1];
                ++num_edges;
            }
        }
    }

    for (int site = 0; site < num_sites; ++site) {
        Vec2* raw_vertices = malloc(sizeof(Vec2) * (num_edges * 2 + 64));
        if (!raw_vertices) {
            continue;
        }

        int raw_count = 0;

        for (int e = 0; e < num_edges; ++e) {
            if (edges[e].site1 == site || edges[e].site2 == site) {
                add_unique_point(raw_vertices, &raw_count, num_edges * 2 + 64, &edges[e].p0, eps);
                add_unique_point(raw_vertices, &raw_count, num_edges * 2 + 64, &edges[e].p1, eps);
            }
        }

        for (int s = 0; s <= 16; ++s) {
            float t = (float)s / 16.0f;
            Vec2 p0 = (Vec2){ minx, miny + (maxy - miny) * t };
            Vec2 p1 = (Vec2){ maxx, miny + (maxy - miny) * t };
            Vec2 p2 = (Vec2){ minx + (maxx - minx) * t, miny };
            Vec2 p3 = (Vec2){ minx + (maxx - minx) * t, maxy };
            Vec2 samples[4] = { p0, p1, p2, p3 };
            for (int s2 = 0; s2 < 4; ++s2) {
                int nearest = -1;
                float nearest_d = 1e30f;
                for (int k = 0; k < num_sites; ++k) {
                    float d = (sites[k].x - samples[s2].x) * (sites[k].x - samples[s2].x) + (sites[k].y - samples[s2].y) * (sites[k].y - samples[s2].y);
                    if (d < nearest_d) {
                        nearest_d = d;
                        nearest = k;
                    }
                }
                if (nearest == site) {
                    add_unique_point(raw_vertices, &raw_count, num_edges * 2 + 64, &samples[s2], eps);
                }
            }
        }

        if (raw_count < 3) {
            Vec2 center = sites[site];
            Vec2 fallback[4] = {
                (Vec2){ center.x - 0.5f, center.y - 0.5f },
                (Vec2){ center.x + 0.5f, center.y - 0.5f },
                (Vec2){ center.x + 0.5f, center.y + 0.5f },
                (Vec2){ center.x - 0.5f, center.y + 0.5f }
            };
            for (int i = 0; i < 4; ++i) {
                add_unique_point(raw_vertices, &raw_count, num_edges * 2 + 64, &fallback[i], eps);
            }
        }

        if (raw_count > 0) {
            for (int i = 0; i < raw_count; ++i) {
                for (int j = i + 1; j < raw_count; ++j) {
                    if (atan2f(raw_vertices[j].y - sites[site].y, raw_vertices[j].x - sites[site].x) < atan2f(raw_vertices[i].y - sites[site].y, raw_vertices[i].x - sites[site].x)) {
                        Vec2 tmp = raw_vertices[i];
                        raw_vertices[i] = raw_vertices[j];
                        raw_vertices[j] = tmp;
                    }
                }
            }

            cells[site].site_index = site;
            cells[site].num_vertices = raw_count;
            cells[site].vertices = malloc(sizeof(Vec2) * raw_count);
            if (cells[site].vertices) {
                for (int i = 0; i < raw_count; ++i) {
                    cells[site].vertices[i] = raw_vertices[i];
                }
            }
        }

        free(raw_vertices);
    }

    free(edges);
    *out_num_cells = num_sites;
    return cells;
}

Worley Worley_New(float dx,float dy,unsigned int count){
    Worley wy;
    wy.points = Vector_New(sizeof(Vec2));
    wy.barriers = Vector_New(sizeof(Worley_Barrier));
    wy.iss = Vector_New(sizeof(Line));
	wy.tv = TransformedView_Make(
        (Vec2){ 0.0f,0.0f },
        (Vec2){ 0.0f,0.0f },
        (Vec2){ 0.01f,0.01f },
        0.0f
    );
	wy.area = Rect_New(
		(Vec2){ 0.0f,0.0f },
        (Vec2){ dx,dy }
    );

	for(int i = 0;i<count;i++){
		const Vec2 p = {
			Random_f64_MinMax(wy.area.p.x,wy.area.p.x + wy.area.d.x),
			Random_f64_MinMax(wy.area.p.y,wy.area.p.y + wy.area.d.y)
		};
		Vector_Push(&wy.points,(Vec2[]){{ p.x,p.y }});
	}
	return wy;
}
void Worley_Update(Worley* wy){
    
}
void Worley_Render(Worley* wy,unsigned int* Target,int Width,int Height){
    TransformedView_Output(&wy->tv,(Vec2){ Width,Height });
	wy->tv.AspectRatio = (float)Width / (float)Height;

	Vector_Clear(&wy->barriers);
	for(int i = 0;i<wy->points.size;i++){
		const Vec2 p = *(Vec2*)Vector_Get(&wy->points,i);
		
		for(int j = i + 1;j<wy->points.size;j++){
			const Vec2 tp = *(Vec2*)Vector_Get(&wy->points,j);
			const Line l = Line_NewSepBorder(p,tp,wy->area.p,wy->area.d);
			const Vec2 mp = Line_MP(Line_New(p,tp));

			const Vec2 sp = TransformedView_WorldScreenPos(&wy->tv,mp);
			const Vec2 sd = TransformedView_WorldScreenLength(&wy->tv,(Vec2){ 0.1f,0.1f });
			Circle_R_RenderWire(Target,Width,Height,sp,sd,GREEN,1.0f);
			
			Vector_Push(&wy->barriers,(Worley_Barrier[]){{
				.l = l,
				.p0 = p,
				.p1 = tp
			}});
		}
	}

    Vector_Clear(&wy->iss);
	for(int i = 0;i<wy->barriers.size;i++){
		Worley_Barrier* l0 = (Worley_Barrier*)Vector_Get(&wy->barriers,i);
		
		for(int j = i + 1;j<wy->barriers.size;j++){
			Worley_Barrier* l1 = (Worley_Barrier*)Vector_Get(&wy->barriers,j);
			const Vec2 i01 = Line_Intersect(l0->l,l1->l);
			
			for(int k = j + 1;k<wy->barriers.size;k++){
				Worley_Barrier* l2 = (Worley_Barrier*)Vector_Get(&wy->barriers,k);
				const Vec2 i02 = Line_Intersect(l0->l,l2->l);
				
				if(Vec2_Cmp(i01,i02)){
					//Vector_Push(&wy->iss,(Worley_Intersection[]){{
					//	.p = i01,
					//	.l0 = l0,
					//	.l1 = l1,
					//	.l2 = l2,
					//}});

					l0->l = Line_NewChopBorder(
						&l0->l,l0->p0,l0->p1,
						&l1->l,l1->p0,l1->p1,
						&l2->l,l2->p0,l2->p1
					);
					l1->l = Line_NewChopBorder(
						&l1->l,l1->p0,l1->p1,
						&l2->l,l2->p0,l2->p1,
						&l0->l,l0->p0,l0->p1
					);
					l2->l = Line_NewChopBorder(
						&l2->l,l2->p0,l2->p1,
						&l0->l,l0->p0,l0->p1,
						&l1->l,l1->p0,l1->p1
					);

					// if(Line_Contains(nl00,l0->mp)) 		l0->l = nl00;
					// else								l0->l = nl01;
					
					// if(Line_Contains(nl10,l1->mp)) 		l1->l = nl10;
					// else								l1->l = nl11;
					
					// if(Line_Contains(nl20,l2->mp)) 		l2->l = nl20;
					// else								l2->l = nl21;
				}
			}
		}
	}

	const Rect sarea = TransformedView_WorldScreenRect(&wy->tv,wy->area);
	Rect_RenderWire(Target,Width,Height,sarea,WHITE,1.0f);

	for(int i = 0;i<wy->points.size;i++){
		const Vec2 p = *(Vec2*)Vector_Get(&wy->points,i);
		const Vec2 sp = TransformedView_WorldScreenPos(&wy->tv,p);
		const Vec2 sd = TransformedView_WorldScreenLength(&wy->tv,(Vec2){ 1.0f,1.0f });
		Circle_R_RenderWire(Target,Width,Height,sp,sd,WHITE,1.0f);
	}

	int num_seg = 0;
    VoronoiSegment* segs = compute_voronoi_segments((Vec2*)wy->points.Memory, wy->points.size, &num_seg,wy->area.p.x,wy->area.p.y,wy->area.p.x + wy->area.d.x,wy->area.p.y + wy->area.d.y);

	for(int i = 0;i<num_seg;i++){
		if(!segs || !isfinite(segs[i].start.x) || !isfinite(segs[i].start.y) || !isfinite(segs[i].end.x) || !isfinite(segs[i].end.y)){
			continue;
		}

		const Vec2 ss = TransformedView_WorldScreenPos(&wy->tv,(Vec2){ segs[i].start.x, segs[i].start.y });
		const Vec2 se = TransformedView_WorldScreenPos(&wy->tv,(Vec2){ segs[i].end.x, segs[i].end.y });
		const float dist = hypotf(ss.x - se.x, ss.y - se.y);
		if(dist < 1e-3f){
			continue;
		}
		Line_RenderX(Target,Width,Height,ss,se,RED,1.0f);
	}

    free(segs);
}
void Worley_Free(Worley* wy){
	Vector_Free(&wy->points);
	Vector_Free(&wy->barriers);
	Vector_Free(&wy->iss);
}

#endif // !WORLEY_H