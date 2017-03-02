/* Public type and operator definitions for the quad-edge data structure. */

/* See
**
**   "Primitives for the Manipulation of General Subdivisions
**   and the Computation of Voronoi Diagrams"
**
**   L. Guibas, J. Stolfi, ACM TOG, April 1985
**
** Originally written by Jim Roth (DEC CADM Advanced Group) on May 1986.
** Modified by J. Stolfi on April 1993.
** See the copyright notice at the end of this file.
*/
/*
** Modified to support 64-bit or 32-bit systems, and data-fields extended by Peter Clifton
*/

#ifndef QUAD_H
#define QUAD_H

#include <stdint.h>

/* Edge records: */

typedef uintptr_t edge_ref;

typedef struct {
    edge_ref next[4];
    void *data[4];
    unsigned mark;
    int id;
    void *undir_data[2];
  } edge_struct;

/* Edge orientation operators: */

#define ROT(e) (((e) & ~(uintptr_t)3)+(((e) + 1) & 3))
#define SYM(e) (((e) & ~(uintptr_t)3)+(((e) + 2) & 3))
#define TOR(e) (((e) & ~(uintptr_t)3)+(((e) + 3) & 3))

/* Vertex/face walking operators: */

#define ONEXT(e)    ((edge_struct *)((e) & ~(uintptr_t)3u))->next[((e) + 0) & 3u]
#define ROTRNEXT(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->next[((e) + 1) & 3u]
#define SYMDNEXT(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->next[((e) + 2) & 3u]
#define TORLNEXT(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->next[((e) + 3) & 3u]

#define RNEXT(e) (TOR(ROTRNEXT(e)))
#define DNEXT(e) (SYM(SYMDNEXT(e)))
#define LNEXT(e) (ROT(TORLNEXT(e)))

#define OPREV(e) (ROT(ROTRNEXT(e)))
#define DPREV(e) (TOR(TORLNEXT(e)))
#define RPREV(e) (SYMDNEXT(e))
#define LPREV(e) (SYM(ONEXT(e)))

/* Data pointers: */

#define ODATA(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->data[((e) + 0) & 3u]
#define RDATA(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->data[((e) + 1) & 3u]
#define DDATA(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->data[((e) + 2) & 3u]
#define LDATA(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->data[((e) + 3) & 3u]

#define ID(e)         ((edge_struct *)((e) & ~(uintptr_t)3u))->id
#define UNDIR_DATA(e) ((edge_struct *)((e) & ~(uintptr_t)3u))->undir_data[(e) & 1u]

edge_ref make_edge(void);

void destroy_edge(edge_ref e);

void splice(edge_ref a, edge_ref b);

void quad_enum(
    edge_ref a,
    void visit_proc(edge_ref e, void *closure),
    void *closure
  );
  /*
    Enumerates undirected primal edges reachable from $a$.

    Calls visit_proc(e, closure) for every edge $e$ that can be reached from
    edge $a$ by a chain of SYM and ONEXT calls; except that exactly one
    of $e$ and SYM(e) is visited. */

#endif

/*
** Copyright notice:
**
** Copyright 1996 Institute of Computing, Unicamp.
**
** Permission to use this software for any purpose is hereby granted,
** provided that any substantial copy or mechanically derived version
** of this file that is made available to other parties is accompanied
** by this copyright notice in full, and is distributed under these same
** terms.
**
** DISCLAIMER: This software is provided "as is" with no explicit or
** implicit warranty of any kind.  Neither the authors nor their
** employers can be held responsible for any losses or damages
** that might be attributed to its use.
**
** End of copyright notice.
*/
