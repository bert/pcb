/*!
 * \file src/polyarea.h
 *
 * \brief poly_Boolean: a polygon clip library.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * Copyright (C) 1997  Alexey Nikitin, Michael Leonov
 *
 * leonov@propro.iis.nsk.su
 *
 * Copyright (C) 1997 Klamer Schutte (minor patches)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef	PCB_POLYAREA_H
#define	PCB_POLYAREA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int BOOLp;

#ifndef FALSE
enum {
	FALSE = 0,
	TRUE  = 1
};
#endif

#define PLF_DIR 1
#define PLF_INV 0
#define PLF_MARK 1

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif


typedef Coord vertex[2]; /*!< longing point representation of coordinates. */
typedef vertex Vector;

#define VertexEqu(a,b) (memcmp((a),(b),sizeof(Vector))==0)
#define VertexCpy(a,b) memcpy((a),(b),sizeof(Vector))


extern Vector vect_zero;

enum
{
    err_no_memory = 2,
    err_bad_parm = 3,
    err_ok = 0
};


typedef struct CVCList CVCList;
typedef struct VNODE VNODE;
struct CVCList
{
    double angle;
    VNODE *parent;
    CVCList *prev, *next, *head;
    char poly, side;
};
struct VNODE
{
    VNODE *next, *prev, *shared;
    struct {
      unsigned int status:3;
      unsigned int mark:1;
    } Flags;
    CVCList *cvc_prev;
    CVCList *cvc_next;
    Vector point;
};

typedef struct PLINE PLINE;
struct PLINE
{
    Coord xmin, ymin, xmax, ymax;
    PLINE *next;
    VNODE head;
    unsigned int Count;
    double area;
    rtree_t *tree;
    bool is_round;
    Coord cx, cy;
    Coord radius;
    struct {
      unsigned int status:3;
      unsigned int orient:1;
    } Flags;
};

PLINE *poly_NewContour(Vector v);

void poly_IniContour(PLINE *  c);
void poly_ClrContour(PLINE *  c);  /* clears list of vertices */
void poly_DelContour(PLINE ** c);

BOOLp poly_CopyContour(PLINE ** dst, PLINE * src);

void poly_PreContour(PLINE * c, BOOLp optimize); /* prepare contour */
void poly_InvContour(PLINE * c);  /* invert contour */

VNODE *poly_CreateNode(Vector v);

void poly_InclVertex(VNODE * after, VNODE * node);
void poly_ExclVertex(VNODE * node);


typedef struct POLYAREA POLYAREA;
struct POLYAREA
{
    POLYAREA *f, *b;
    PLINE *contours;
    rtree_t *contour_tree;
};

BOOLp poly_M_Copy0(POLYAREA ** dst, const POLYAREA * srcfst);
void poly_M_Incl(POLYAREA **list, POLYAREA *a);

BOOLp poly_Copy0(POLYAREA **dst, const POLYAREA *src);
BOOLp poly_Copy1(POLYAREA  *dst, const POLYAREA *src);

BOOLp poly_InclContour(POLYAREA * p, PLINE * c);
BOOLp poly_ExclContour(POLYAREA * p, PLINE * c);


BOOLp poly_ChkContour(PLINE * a);

BOOLp poly_CheckInside(POLYAREA * c, Vector v0);
BOOLp Touching(POLYAREA *p1, POLYAREA *p2);

/* tools for clipping */

/*!
 * \brief Checks whether point lies within contour independently of its
 * orientation.
 */
int poly_InsideContour(PLINE *c, Vector v);
int poly_ContourInContour(PLINE * poly, PLINE * inner);
POLYAREA *poly_Create(void);

void poly_Free(POLYAREA **p);
void poly_Init(POLYAREA  *p);
void poly_FreeContours(PLINE **pl);
BOOLp poly_Valid(POLYAREA *p);

enum PolygonBooleanOperation {
	PBO_UNITE,
	PBO_ISECT,
	PBO_SUB,
	PBO_XOR
};

double vect_dist2 (Vector v1, Vector v2);
double vect_det2 (Vector v1, Vector v2);
double vect_len2 (Vector v1);

int vect_inters2 (Vector A, Vector B, Vector C, Vector D, Vector S1,
                  Vector S2);

int poly_Boolean(const POLYAREA * a, const POLYAREA * b, POLYAREA ** res, int action);
int poly_Boolean_free(POLYAREA * a, POLYAREA * b, POLYAREA ** res, int action);
int poly_AndSubtract_free(POLYAREA * a, POLYAREA * b, POLYAREA ** aandb, POLYAREA ** aminusb);
int SavePOLYAREA( POLYAREA *PA, char * fname);
#ifdef __cplusplus
}
#endif

#endif /* PCB_POLYAREA_H */
