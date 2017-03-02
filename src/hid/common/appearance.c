#include <glib.h>

#include "appearance.h"

appearance
*make_appearance (void)
{
  appearance *appear = g_new0 (appearance, 1);

  return appear;
}

void
destroy_appearance (appearance *appear)
{
  g_free (appear);
}

void
appearance_set_color (appearance *appear, float r, float g, float b)
{
  appear->r = r;
  appear->g = g;
  appear->b = b;
}

void
appearance_set_appearance (appearance *appear, const appearance *from)
{
  *appear = *from;
}
