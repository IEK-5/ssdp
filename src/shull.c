#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "print.h"
#include "shull.h"
#include "error.h"

struct flipdata {
	unsigned int maxflips;
	bool flipped;
};


double sqdist(const sh_point *p, const sh_point *q) {
	return (p->x-q->x)*(p->x-q->x)+(p->y-q->y)*(p->y-q->y);
}
#ifdef _WIN32
sh_point * c_for_win;
int radialcompare(const void *a, const void *b) { 
	double rr = sqdist((const sh_point *)a, c_for_win);
	double ss = sqdist((const sh_point *)b, c_for_win);
	if (rr < ss) {
		return -1;
	}
	else if (rr > ss) {
		return 1;
	}
	else {
		return 0;
	}
} 
void radialsort(sh_point *ps, size_t n, sh_point *q) { 
	c_for_win=q;
	qsort(ps, n, sizeof(sh_point), radialcompare);
} 
#else
int radialcompare(const void *a, const void *b, void *c) { 
	double rr = sqdist((const sh_point *)a, (const sh_point *)c);
	double ss = sqdist((const sh_point *)b, (const sh_point *)c);
	if (rr < ss) {
		return -1;
	}
	else if (rr > ss) {
		return 1;
	}
	else {
		return 0;
	}
} 
void radialsort(sh_point *ps, size_t n, sh_point *q) { 
	qsort_r(ps, n, sizeof(sh_point), radialcompare, q);
} 
#endif /*_WIN32 */
void swap_points(sh_point *p, sh_point *q) { 
	sh_point tmp;
	memcpy(&tmp, p,    sizeof(sh_point));
	memcpy(p,    q,    sizeof(sh_point));
	memcpy(q,    &tmp, sizeof(sh_point));
} 
double plane_cross(const sh_point *a, const sh_point *b, const sh_point *c) { 
	return (b->x-a->x) * (c->y-a->y) - (b->y-a->y) * (c->x-a->x);
} 
double sqcircumradius(const sh_point *a, const sh_point *b, const sh_point *c) { 
	sh_point p = { .x = b->x-a->x, .y = b->y-a->y };
	sh_point q = { .x = c->x-a->x, .y = c->y-a->y };
	double p2 = p.x*p.x + p.y*p.y;
	double q2 = q.x*q.x + q.y*q.y;
	double d = 2*(p.x*q.y - p.y*q.x);
	if (d == 0) {
		return -1;
	}
	double x = (q.y*p2 - p.y*q2)/d;
	double y = (p.x*q2 - q.x*p2)/d;
	return x*x+y*y;
} 
double circumcircle(sh_point *r, const sh_point *a, const sh_point *b, const sh_point *c) { 
	sh_point p = { .x = b->x-a->x, .y = b->y-a->y };
	sh_point q = { .x = c->x-a->x, .y = c->y-a->y };
	double p2 = p.x*p.x + p.y*p.y;
	double q2 = q.x*q.x + q.y*q.y;
	double d = 2*(p.x*q.y - p.y*q.x);
	if (d == 0) {
		return -1;
	}
	double x = (q.y*p2 - p.y*q2)/d;
	double y = (p.x*q2 - q.x*p2)/d;
	if (r != NULL) {
		r->x = a->x + x;
		r->y = a->y + y;
	}
	return x*x+y*y;
} 
sh_triangle *create_triangle(sh_point *p, sh_point *q, sh_point *r) { 
	sh_triangle *t = malloc(sizeof(sh_triangle));
	if (t == NULL)
	{
		// ERRORFLAG MALLOCFAILCREATETRIANGLE  "Error memory allocation failed in creating a triangle"
		AddErr(MALLOCFAILCREATETRIANGLE);
		return NULL;
	}
	t->p[0] = p;
	t->p[1] = q;
	t->p[2] = r;
	t->ccr2 = circumcircle(&t->cc, p, q, r);
	t->e[0] = NULL;
	t->e[1] = NULL;
	t->e[2] = NULL;
	return t;
} 
sh_edge *create_edge(sh_point *p, sh_point *q, sh_triangle *t, sh_triangle *u) { 
	sh_edge *e = malloc(sizeof(sh_edge));
	if (e == NULL)
	{
		// ERRORFLAG MALLOCFAILCREATEEDHE  "Error memory allocation failed in creating an edge"
		AddErr(MALLOCFAILCREATETRIANGLE);
		return NULL;
	}
	e->p[0] = p;
	e->p[1] = q;
	e->t[0] = t;
	e->t[1] = u;
	e->flipcount = 0;
	return e;
} 
void seed_triangulation(sh_triangulation_data *td, sh_point *ps) { 
	sh_triangle *tri;
	sh_edge *e[3];
	
	if ((tri = create_triangle(&ps[0], &ps[1], &ps[2]))==NULL)
		return;
	if ((e[0]=create_edge(&ps[2], &ps[0], tri, NULL))==NULL)
		return;
	if ((e[1]=create_edge(&ps[1], &ps[2], tri, NULL))==NULL)
		return;
	if ((e[2]=create_edge(&ps[0], &ps[1], tri, NULL))==NULL)
		return;
	if (ssdp_error_state)
		return;
	tri->e[0] = e[1];
	tri->e[1] = e[0];
	tri->e[2] = e[2];

	td->triangles = ll_insert_after(NULL, tri);

	td->hull_edges = ll_insert_after(NULL, e[0]);
	ll_glue(td->hull_edges, td->hull_edges); /* make circular */
	ll_insert_after(td->hull_edges, e[1]);
	ll_insert_after(td->hull_edges, e[2]);
	td->internal_edges = NULL;
} 
bool is_visible_to_point(void *a, void *b) { 
	sh_edge *e = (sh_edge *)a;
	sh_point *p = (sh_point *)b;
	double cross = plane_cross(p, e->p[0], e->p[1]);
	if (cross > 0) {
		return true;
	}
	return false;
} 
bool is_not_visible_to_point(void *a, void *b) { 
	sh_edge *e = (sh_edge *)a;
	sh_point *p = (sh_point *)b;
	double cross = plane_cross(p, e->p[0], e->p[1]);
	if (cross <= 0) {
		return true;
	}
	return false;
} 
void add_point_to_hull(sh_triangulation_data *td, sh_point *p) { 
	/*
	 * Find the first hull edge that is visible from the point p
	 * and the last hull edge that is visible from point p.
	 * Those edges, and the edges between, in the hull will be
	 * replaced by two edges, one leading to the point, and one
	 * leading from it.
	 * New triangles will be created from the combination of
	 * each visible edge and the point.
	 */

	ll_node *first_vis;
	ll_node *last_vis;
	ll_node *first_hid;
	ll_node *last_hid;

	if (is_visible_to_point(DATA(td->hull_edges), p)) { 
		first_hid = ll_cfind_r(td->hull_edges, is_not_visible_to_point, p);
		last_hid = ll_crfind_r(td->hull_edges, is_not_visible_to_point, p);
		first_vis = ll_cut_after(last_hid);
		last_vis = ll_cut_before(first_hid);
	}
	else {
		first_vis = ll_cfind_r(td->hull_edges, is_visible_to_point, p);
		last_vis = ll_crfind_r(td->hull_edges, is_visible_to_point, p);
		first_hid = ll_cut_after(last_vis);
		last_hid = ll_cut_before(first_vis);
	} 
	if (first_vis==NULL)
	{
		// point not visible from hull, set has identical points
		// ERRORFLAG TRIANGULATIONIDENTICALP  "Error triangulation point set contains identical points (I think)"
		AddErr(TRIANGULATIONIDENTICALP);
		return;
	}
	sh_edge *e0 = NULL;
	sh_edge *e1 = NULL;
	for (ll_node *n=first_vis; n!=NULL; n=NEXT(n)) { 
		sh_edge *e = (sh_edge *)DATA(n);
		sh_triangle *t = create_triangle(e->p[0], p, e->p[1]);
		td->triangles = ll_insert_before(td->triangles, t);

		e->t[1] = t;
		if (n == first_vis) {
			e0 = create_edge(e->p[0], p, t, NULL);
			
			last_hid = ll_insert_after(last_hid, e0);
		}
		else {
			e0 = e1;
			e0->t[1] = t;
			td->internal_edges = ll_insert_before(td->internal_edges, e0);
		}
		e1 = create_edge(p, e->p[1], t, NULL);

		/* the edge is given the same index as the point it is opposite of in the triangle */
		t->e[0] = e1;
		t->e[1] = e;
		t->e[2] = e0;

		if (n == last_vis) {
			last_hid = ll_insert_after(last_hid, e1);
		}
	} 
	ll_glue(last_vis, td->internal_edges);
	td->internal_edges = first_vis;
	ll_glue(last_hid, first_hid);
	td->hull_edges = first_hid;
} 
int triangulate(sh_triangulation_data *td, sh_point *ps, size_t n) { 

	int p0 = 0;
	int deg=0;
	delaunay_restart:
	if (p0 != 0) { 
		if (p0 == n) {
			// ERRORFLAG SHULLFAIL  "Error the s-hull Delaunay triangulation failed"
			AddErr(SHULLFAIL);
			return false;
		}
		swap_points(&ps[0], &ps[p0]);
	} 
	/*
	 * {{{
	 * 1 Select a seed point p from the set of points.
	 * 2 Sort set of points according to distance to the seed point.
	 * 3 Find the point q closest to p.
	 * 4 Find the point r which yields the smallest circumcircle c for the triangle pqr.
	 * 5 Order the points pqr so that the system is right handed; this is the initial hull h.
	 * 6 Resort the rest of the set of points based on the distance to to the centre of c.
	 * 7 Sequentially add the points s of the set, based on distance, growing h. As points are added
	 *   to h, triangles are created containing s and edges of h visible to s.
	 * 8 When h contains all points, a non-overlapping triangulation has been created.
	 * -- to get Delaunay triangulation:
	 * 9 Adjacent pairs of triangles may require flipping to create a proper Delaunay triangulation
	 *   from the triangulation
	 * }}}
	 */
	/* find starting points */ 
	/*
	 * "Randomly" select the first point, then find the the point closest to it
	 * and put it on index 1.
	 * Then find the point which together with the other two creates the smallest
	 * circumcircle, and put that on index 2.
	 * Order the points so that they become a clockwise ordered triangle, and then
	 * sort all the other points based on closeness to the circumcenter.
	 */

	size_t i_best = 1;
	double r_best = sqdist(&ps[1], &ps[0]);
	for (size_t i=2; i<n; ++i) { 
		double r = sqdist(&ps[i], &ps[0]);
		if (r_best > r) {
			r_best = r;
			i_best = i;
		}
	} 
	swap_points(&ps[1], &ps[i_best]);

	i_best = 2;
	r_best = sqcircumradius(&ps[2], &ps[1], &ps[0]);
	if (r_best == -1) { 
		deg++;
		++p0;
		goto delaunay_restart;
	} 
	for (size_t i=3; i<n; ++i) { 
		double r = sqcircumradius(&ps[i], &ps[1], &ps[0]);
		if (r > -1 && r_best > r) {
			r_best = r;
			i_best = i;
		}
	} 
	swap_points(&ps[2], &ps[i_best]);
	
	/* ensure positively winded starting triangle */ 
	double cross = plane_cross(&ps[0], &ps[1], &ps[2]);
	if (cross > 0) {
		swap_points(&ps[1], &ps[2]);
	}
	else if (cross == 0) {
		++p0;
		deg++;
		goto delaunay_restart;
	}
	
	if (deg)
		Print(WARNING,"Warning: degenerate cases triangulation\n");
	/* calculate circumcircle centre and sort points based on distance */ 
	sh_point cc;
	double radius = circumcircle(&cc, &ps[0], &ps[1], &ps[2]);
	if (radius < 0) {
		++p0;
		goto delaunay_restart;
	}
	if (n > 4) {
		radialsort(ps+3, n-3, &cc);
	}
	
	seed_triangulation(td, ps);
	if (ssdp_error_state)
		return false;
	/* iteratively add points to the hull */ 
	for (size_t i=3; i<n; ++i) {
		add_point_to_hull(td, &ps[i]);
		if (ssdp_error_state)
			return false;
	} 
	return 0;

} 
int find_common_index(const sh_triangle *t, const sh_point *p) { 
	for (size_t i=0; i<3; ++i) {
		if (p == t->p[i]) {
			return i;
		}
	}
	return -1;
} 
void *flip_if_necessary(void *a, void *b) { 
	sh_edge *e = (sh_edge *)a;
	struct flipdata *fd = (struct flipdata *)b;

	if (e->t[0] != NULL && e->t[1] != NULL) {

		/*
		 *                b0  1         0  a1
		 *  c *------------* *           * *------------* b1
		 *    |           / /             \ \           |
		 *    |   T0     / / * b1      c * \ \     T1   |
		 *    |         / / /|           |\ \ \         |
		 *    |        / / / |           | \ \ \        |
		 *    |       / / /  |           |  \ \ \       |
		 *    |      / / /   |           |   \ \ \      |
		 *    |     / / /    |           |    \ \ \     |
		 *    |    / / /     |           |     \ \ \    |
		 *    |   / / /      |           |      \ \ \   |
		 *    |  / / /       |           |       \ \ \  |
		 *    | / / /        |           |        \ \ \ |
		 *    |/ / /         |           |         \ \ \|
		 * a0 * / /     T1   |           |   T0     \ \ * d
		 *     / /           |           |           \ \
		 *    * *------------* d      a0 *------------* *
		 *   0  a1                                   b0  1
		 */

		const int a0 = find_common_index(e->t[0], e->p[0]);
		const int a1 = find_common_index(e->t[1], e->p[0]);
		const int b0 = find_common_index(e->t[0], e->p[1]);
		const int b1 = find_common_index(e->t[1], e->p[1]);
		
		const int c = 3 ^ a0 ^ b0;
		const int d = 3 ^ a1 ^ b1;

		if (a0==-1 || a1==-1 || b0==-1 || b1==-1) { 
			AddErr(SHULLFAIL); 
			return a;
		} 
		/* printf("---\n"); */
		if (
			e->t[0]->ccr2 > sqdist(&e->t[0]->cc, e->t[1]->p[d]) ||
			e->t[1]->ccr2 > sqdist(&e->t[1]->cc, e->t[0]->p[c])) {
			++e->flipcount;
			if (e->flipcount > fd->maxflips) {
				return a;
			}
			fd->flipped = true;
			/*
			 * --- ------------------------------------------------------------
			 * E:  p[0]  is changed so that it points at T0:p[c]
			 *     p[1]  is changed so that it points at T1:p[d]
			 * --- ------------------------------------------------------------
			 * T0: p[a0] stays the same
			 *     p[b0] is changed to point at T1:p[d]
			 *     p[c]  stays the same
			 *
			 *     e[a0] is changed so that it points at the common edge
			 *     e[b0] stays the same
			 *     e[c]  is changed so that it points the pointed at the edge T1:e[b1] and the edge is made to reciprocate
			 * --- ------------------------------------------------------------
			 * T1: p[a1] is changed so that it points at T0:p[c]
			 *     p[b1] stays the same
			 *     p[d]  stays the same
			 *
			 *     e[a1] stays the same
			 *     e[b1] is changed so that it points at the common edge
			 *     e[d]  is changed so that it points at the edge T0:e[a0] and the edge is made to reciprocate
			 * --- ------------------------------------------------------------
			 */
			sh_edge *t1eb1 = e->t[1]->e[b1];
			sh_edge *t0ea0 = e->t[0]->e[a0];

			/* --- ------------------------------------------------------------ */
			e->p[0] = e->t[0]->p[c];
			e->p[1] = e->t[1]->p[d];
			/* --- ------------------------------------------------------------ */
			e->t[0]->p[b0] = e->t[1]->p[d];

			e->t[0]->e[a0] = e;
			e->t[0]->e[c] = t1eb1;
			/* --- ------------------------------------------------------------ */
			e->t[1]->p[a1] = e->t[0]->p[c];

			e->t[1]->e[b1] = e;
			e->t[1]->e[d] = t0ea0;
			/* --- ------------------------------------------------------------ */

			if (t1eb1->t[0] == e->t[1]) { t1eb1->t[0] = e->t[0]; } else { t1eb1->t[1] = e->t[0]; }
			if (t0ea0->t[0] == e->t[0]) { t0ea0->t[0] = e->t[1]; } else { t0ea0->t[1] = e->t[1]; }

			e->t[0]->ccr2 = circumcircle(&e->t[0]->cc, e->t[0]->p[0], e->t[0]->p[1], e->t[0]->p[2]);
			e->t[1]->ccr2 = circumcircle(&e->t[1]->cc, e->t[1]->p[0], e->t[1]->p[1], e->t[1]->p[2]);
		}
	}
	return a;
} 
void *find_highest_flipcount(void *a, void *b) { 
	sh_edge *e = (sh_edge *)a;
	int *flipcount = (int *)b;
	if (*flipcount < e->flipcount) {
		*flipcount = e->flipcount;
	}
	return a;
} 
int make_delaunay(sh_triangulation_data *td) { 
	struct flipdata fd;
	fd.maxflips = ll_length(td->internal_edges);
	fd.maxflips *= fd.maxflips;
	fd.flipped = true;
	while (fd.flipped) {
		fd.flipped = false;
		ll_map_r(td->internal_edges, flip_if_necessary, &fd);
		if (ssdp_error_state)
			return -1;
	}
	int flipcount = 0;
	ll_map_r(td->internal_edges, find_highest_flipcount, &flipcount);
	if (fd.maxflips>flipcount)
		return flipcount;
	else
		Print(WARNING,"Warning: reached maximum flipcount in shull\n");
	return -1;
}
int delaunay(sh_triangulation_data *td, sh_point *ps, size_t n) { 
	int res;
	if (triangulate(td, ps, n) == 0) {
		res=make_delaunay(td);
		if (res>0)
			return res;
	}
	ll_mapdestroy(td->triangles, free);
	ll_mapdestroy(td->hull_edges, free);
	ll_mapdestroy(td->internal_edges, free);
	return -1;
} 
