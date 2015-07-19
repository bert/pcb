#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "hid.h"

/* otherwise homeless function, refactored out of the five export HIDs */
void
hc_util_derive_default_filename(const char *pcbfile, HID_Attribute *filename_attrib,
                                const char *suffix, char **memory)
{
  char *buf;
  char *pf;

  if (pcbfile == NULL)
    pf = strdup ("unknown.pcb");
  else
    pf = strdup (pcbfile);

  if (!pf || (memory && filename_attrib->default_val.str_value != *memory)) {
    return;
  }

  buf = (char *)malloc (strlen (pf) + strlen(suffix) + 1);

  if (memory) {
    *memory = buf;
  }

  if (buf) {

    size_t bl;
    strcpy (buf, pf);
    bl = strlen(buf);

    if (bl > 4 && strcmp (buf + bl - 4, ".pcb") == 0) {
      buf[bl - 4] = 0;
    }

    strcat(buf, suffix);

    if (filename_attrib->default_val.str_value) {
      free ((void *) filename_attrib->default_val.str_value);
    }

    filename_attrib->default_val.str_value = buf;
  }

  free (pf);
}
