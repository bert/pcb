/* Implementation of the Guibas-Stolfi quad-edge structure. */

/*
** Written by J. Stolfi on april 1993, based on an original
** implementation by Jim Roth (DEC CADM Advanced Group, May 1986).
** See the copyright notice at the end of this file.
*/
/*
** Modified to support 64-bit or 32-bit systems, and data-fields extended by Peter Clifton
*/

#include "quad.h"
#include <memory.h>
#include <malloc.h>

#define MARK(e)  ((edge_struct *)((e) & ~(uintptr_t)3u))->mark

/* Make a new edge: */

static int global_edge_count = 0;

edge_ref make_edge(void)
  {
    edge_ref e;

    e = (edge_ref) malloc(sizeof(edge_struct));
    ONEXT(e) = e;
    SYMDNEXT(e) = SYM(e);
    ROTRNEXT(e) = TOR(e);
    TORLNEXT(e) = ROT(e);
    MARK(e) = 0;
    ID(e) = global_edge_count++;
    UNDIR_DATA(e) = NULL;
    UNDIR_DATA(ROT(e)) = NULL;
    return e;
  }

/* Delete an edge: */

void destroy_edge(edge_ref e)
  {
    edge_ref f = SYM(e);
    if (ONEXT(e) != e) splice(e, OPREV(e));
    if (ONEXT(f) != f) splice(f, OPREV(f));
    free((char *) ((e) & ~(uintptr_t)3u));
  }

/* Splice primitive: */

void splice(edge_ref a, edge_ref b)
  {
    edge_ref ta, tb;
    edge_ref alpha = ROT(ONEXT(a));
    edge_ref beta = ROT(ONEXT(b));

    ta = ONEXT(a);
    tb = ONEXT(b);
    ONEXT(a) = tb;
    ONEXT(b) = ta;
    ta = ONEXT(alpha);
    tb = ONEXT(beta);
    ONEXT(alpha) = tb;
    ONEXT(beta) = ta;
  }

/* Enumerate edge quads */

void quad_do_enum (
    edge_ref a,
    void visit_proc(edge_ref e, void *closure),
    void *closure,
    unsigned mark
  );

unsigned next_mark = 1;

void quad_enum(
    edge_ref a,
    void visit_proc(edge_ref e, void *closure),
    void *closure
  )
  {
    unsigned mark = next_mark;
    next_mark++;
    if (next_mark == 0) next_mark = 1;
    quad_do_enum(a, visit_proc, closure, mark);
  }

void quad_do_enum (
    edge_ref e,
    void visit_proc(edge_ref e, void *closure),
    void *closure,
    unsigned mark
  )
  {
    while (MARK(e) != mark)
      {
        visit_proc(e, closure);
        MARK(e) = mark;
        quad_do_enum (ONEXT(SYM(e)), visit_proc, closure, mark);
        e = ONEXT(e);
      }
  }

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
** NOTE: this copyright notice does not claim to supersede any copyrights
** that may apply to the original DEC implementation of the quad-edge
** data structure.
**
** DISCLAIMER: This software is provided "as is" with no explicit or
** implicit warranty of any kind.  Neither the authors nor their
** employers can be held responsible for any losses or damages
** that might be attributed to its use.
**
** End of copyright notice.
*/
