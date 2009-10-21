#include "global.h"
#include "hid.h"
#include "polygon.h"

static void fill_contour (hidGC gc, PLINE *pl)
{
  int *x, *y, n, i = 0;
  VNODE *v;

  n = pl->Count;
  x = malloc (n * sizeof (int));
  y = malloc (n * sizeof (int));

  for (v = &pl->head; i < n; v = v->next)
    {
      x[i] = v->point[0];
      y[i++] = v->point[1];
    }

  gui->fill_polygon (gc, n, x, y);

  free (x);
  free (y);
}

static void thindraw_contour (hidGC gc, PLINE *pl)
{
  VNODE *v;
  int last_x, last_y;
  int this_x, this_y;

  /* Need at least two points in the contour */
  if (pl->head.next == NULL)
    return;

  gui->set_line_width (gc, 0);
  gui->set_line_cap (gc, Round_Cap);

  last_x = pl->head.point[0];
  last_y = pl->head.point[1];
  v = pl->head.next;

  do
    {
      this_x = v->point[0];
      this_y = v->point[1];

      gui->draw_line (gc, last_x, last_y, this_x, this_y);
      // gui->fill_circle (gc, this_x, this_y, 30);

      last_x = this_x;
      last_y = this_y;
    }
  while ((v = v->next) != pl->head.next);
}

static void fill_contour_cb (PLINE *pl, void *user_data)
{
  hidGC gc = user_data;
  PLINE *local_pl = pl;

  fill_contour (gc, pl);
  poly_FreeContours (&local_pl);
}

void common_fill_pcb_polygon (hidGC gc, PolygonType *poly,
                              const BoxType *clip_box)
{
  /* FIXME: We aren't checking the gui's dicer flag..
            we are dicing for every case. Some GUIs
            rely on this, and need their flags fixing. */

  NoHolesPolygonDicer (poly, clip_box, fill_contour_cb, gc);

  /* Draw other parts of the polygon if fullpoly flag is set */
  if (TEST_FLAG (FULLPOLYFLAG, poly))
    {
      PolygonType p = *poly;

      for (p.Clipped = poly->Clipped->f;
           p.Clipped != poly->Clipped;
           p.Clipped = p.Clipped->f)
        NoHolesPolygonDicer (&p, clip_box, fill_contour_cb, gc);
    }

}

static int thindraw_hole_cb (PLINE *pl, void *user_data)
{
  hidGC gc = user_data;
  thindraw_contour (gc, pl);
  return 0;
}

void common_thindraw_pcb_polygon (hidGC gc, PolygonType *poly,
                                  const BoxType *clip_box)
{
  thindraw_contour (gc, poly->Clipped->contours);
  PolygonHoles (poly, clip_box, thindraw_hole_cb, gc);
}
