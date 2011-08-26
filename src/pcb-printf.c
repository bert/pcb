/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2011 Andrew Poelstra
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Andrew Poelstra, 16966 60A Ave, V3S 8X5 Surrey, BC, Canada
 *  asp11@sfu.ca
 *
 */

/*! \file <pcb-printf.c>
 *  \brief Implementation of printf wrapper to output pcb coords and angles
 *  \par Description
 *  For details of all supported specifiers, see the comment at the
 *  top of pcb-printf.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "global.h"

#include "pcb-printf.h"

/* Helper macros for tables */
#define MM_TO_COORD3(a,b,c)		MM_TO_COORD (a), MM_TO_COORD (b), MM_TO_COORD (c)
#define MIL_TO_COORD3(a,b,c)		MIL_TO_COORD (a), MIL_TO_COORD (b), MIL_TO_COORD (c)
#define MM_TO_COORD5(a,b,c,d,e)		MM_TO_COORD (a), MM_TO_COORD (b), MM_TO_COORD (c),	\
                              		MM_TO_COORD (d), MM_TO_COORD (e)
#define MIL_TO_COORD5(a,b,c,d,e)	MIL_TO_COORD (a), MIL_TO_COORD (b), MIL_TO_COORD (c),	\
                               		MIL_TO_COORD (d), MIL_TO_COORD (e)

/* These should be kept in order of smallest scale_factor
 * to largest -- the code uses this ordering when finding
 * the best scale to use for a group of measures */
static Unit Units[] = {
  { 0, "km", NULL, 'k', 0.000001, METRIC, ALLOW_KM, 5,
             0.00005, 0.0005, 0.0025, 0.05, 0.25,
             { "" } },
  { 0, "m",  NULL, 'f', 0.001,    METRIC, ALLOW_M,  5,
             0.0005, 0.005, 0.025, 0.5, 2.5,
             { "" } },
  { 0, "cm", NULL, 'e', 0.1,      METRIC, ALLOW_CM, 5,
             0.005, 0.05, 0.25, 5, 25,
             { "" } },
  { 0, "mm", NULL, 'm', 1,        METRIC, ALLOW_MM, 4,
             0.005, 0.05, 0.25, 5, 25,
             { "" } },
  { 0, "um", NULL, 'u', 1000,     METRIC, ALLOW_UM, 2,
             0.005, 0.05, 0.25, 5, 25,
             { "" } },
  { 0, "nm", NULL, 'n', 1000000,  METRIC, ALLOW_NM, 0,
             5, 50, 2500, 5000, 25000,
             { "" } },

  { 0, "in",   NULL, 'i', 0.001, IMPERIAL, ALLOW_IN,   5,
               0.1, 1.0, 5.0, 25, 100,
               { "inch" } },
  { 0, "mil",  NULL, 'l', 1,     IMPERIAL, ALLOW_MIL,  2,
               0.1, 1.0, 10, 100, 1000,
               { "" } },
  { 0, "cmil", NULL, 'c', 100,   IMPERIAL, ALLOW_CMIL, 0,
               1, 10, 100, 1000, 10000,
               { "pcb" } }
};
#define N_UNITS ((int) (sizeof Units / sizeof Units[0]))
/* \brief Initialize non-static data for pcb-printf
 * \par Function Description
 * Assigns each unit its index for quick access through the
 * main units array, and internationalize the units for GUI
 * display.
 */
void initialize_units()
{
  int i;
  for (i = 0; i < N_UNITS; ++i)
    {
      Units[i].index = i;
      Units[i].in_suffix = _(Units[i].suffix);
    }
}

/* This list -must- contain all printable units from the above list */
/* For now I have just copy/pasted the same values for all metric
 * units and the same values for all imperial ones */
static Increments increments[] = {
  /* TABLE FORMAT   |  default  |  min  |  max
   *          grid  |           |       |
   *          size  |           |       |
   *          line  |           |       |
   *         clear  |           |       |
   */
  { "km", MM_TO_COORD3 (0.1,  0.01,  1.0),
          MM_TO_COORD3 (0.2,  0.01,  0.5),
          MM_TO_COORD3 (0.1,  0.005, 0.5),
          MM_TO_COORD3 (0.05, 0.005, 0.5) },
  { "m",  MM_TO_COORD3 (0.1,  0.01,  1.0),
          MM_TO_COORD3 (0.2,  0.01,  0.5),
          MM_TO_COORD3 (0.1,  0.005, 0.5),
          MM_TO_COORD3 (0.05, 0.005, 0.5) },
  { "cm", MM_TO_COORD3 (0.1,  0.01,  1.0),
          MM_TO_COORD3 (0.2,  0.01,  0.5),
          MM_TO_COORD3 (0.1,  0.005, 0.5),
          MM_TO_COORD3 (0.05, 0.005, 0.5) },
  { "mm", MM_TO_COORD3 (0.1,  0.01,  1.0),
          MM_TO_COORD3 (0.2,  0.01,  0.5),
          MM_TO_COORD3 (0.1,  0.005, 0.5),
          MM_TO_COORD3 (0.05, 0.005, 0.5) },
  { "um", MM_TO_COORD3 (0.1,  0.01,  1.0),
          MM_TO_COORD3 (0.2,  0.01,  0.5),
          MM_TO_COORD3 (0.1,  0.005, 0.5),
          MM_TO_COORD3 (0.05, 0.005, 0.5) },
  { "nm", MM_TO_COORD3 (0.1,  0.01,  1.0),
          MM_TO_COORD3 (0.2,  0.01,  0.5),
          MM_TO_COORD3 (0.1,  0.005, 0.5),
          MM_TO_COORD3 (0.05, 0.005, 0.5) },

  { "cmil", MIL_TO_COORD3 (5,  1,   25),
            MIL_TO_COORD3 (10, 1,   10),
            MIL_TO_COORD3 (5,  0.5, 10),
            MIL_TO_COORD3 (2,  0.5, 10) },
  { "mil",  MIL_TO_COORD3 (5,  1,   25),
            MIL_TO_COORD3 (10, 1,   10),
            MIL_TO_COORD3 (5,  0.5, 10),
            MIL_TO_COORD3 (2,  0.5, 10) },
  { "in",   MIL_TO_COORD3 (5,  1,   25),
            MIL_TO_COORD3 (10, 1,   10),
            MIL_TO_COORD3 (5,  0.5, 10),
            MIL_TO_COORD3 (2,  0.5, 10) },
};
#define N_INCREMENTS (sizeof increments / sizeof increments[0])

/* \brief Obtain a unit object from its suffix
 * \par Function Description
 * Looks up a given suffix in the main units array. Internationalized
 * unit suffixes are not supported, though pluralized units are, for
 * backward-compatibility.
 *
 * \param [in] const_suffix   The suffix to look up
 *
 * \return A const pointer to the Unit struct, or NULL if none was found
 */
const Unit *get_unit_struct (const char *const_suffix)
{
  int i;
  int s_len = 0;
  /* Turn given suffix into something we can modify... */
  char *m_suffix = g_strdup (const_suffix);
  /* ...and store this in a pointer we can move. */
  char *suffix = m_suffix;

  /* Determine bounds */
  while (isspace (*suffix))
    ++suffix;
  while (isalnum (suffix[s_len]))
    ++s_len;

  /* Also understand plural suffixes: "inches", "mils" */
  if (s_len > 2)
    {
      if (suffix[s_len - 2] == 'e' && suffix[s_len - 1] == 's')
        suffix[s_len - 2] = 0;
      else if (suffix[s_len - 1] == 's')
        suffix[s_len - 1] = 0;
    }

  /* Do lookup */
  if (*suffix)
    for (i = 0; i < N_UNITS; ++i)
      if (strcmp (suffix, Units[i].suffix) == 0 ||
          strcmp (suffix, Units[i].alias[0]) == 0)
        {
          g_free (m_suffix);
          return &Units[i];
        }
  g_free (m_suffix);
  return NULL;
}

/* ACCESSORS */
/* \brief Returns the master unit list. This may not be modified. */
const Unit *get_unit_list (void)
{
	return Units;
}
/* \brief Returns the length of the master unit list. */
int get_n_units (void)
{
	return N_UNITS;
}

/* \brief Obtain an increment object from its suffix
 * \par Function Description
 * Looks up a given suffix in the main increments array. Internationalized
 * unit suffixes are not supported, nor are pluralized units.
 *
 * \param [in] suffix   The suffix to look up
 *
 * \return A const pointer to the Increments struct, or NULL if none was found
 */
Increments *get_increments_struct (const char *suffix)
{
  int i;
  /* Do lookup */
  for (i = 0; i < N_INCREMENTS; ++i)
    if (strcmp (suffix, increments[i].suffix) == 0)
      return &increments[i];
  return NULL;
}

/* \brief Convert a pcb coord to the given unit
 *
 * \param [in] unit  The unit to convert to
 * \param [in] x     The quantity to convert
 *
 * \return The converted measure
 */
double coord_to_unit (const Unit *unit, Coord x)
{
  double base;
  if (unit == NULL)
    return -1;
  base = unit->family == METRIC
           ? COORD_TO_MM (1)
           : COORD_TO_MIL (1);
  return x * unit->scale_factor * base;
}

/* \brief Convert a given unit to pcb coords
 *
 * \param [in] unit  The unit to convert from
 * \param [in] x     The quantity to convert
 *
 * \return The converted measure
 */
Coord unit_to_coord (const Unit *unit, double x)
{
  return x / coord_to_unit (unit, 1);
}

static int min_sig_figs(double d)
{
  char buf[50];
  int rv;

  if(d == 0) return 0;

  /* Normalize to x.xxxx... form */
  if(d < 0)      d *= -1;
  while(d >= 10) d /= 10;
  while(d < 1)   d *= 10;

  rv = sprintf(buf, "%g", d);
  return rv;
}

/* \brief Internal coord-to-string converter for pcb-printf
 * \par Function Description
 * Converts a (group of) measurement(s) to a comma-deliminated
 * string, with appropriate units. If more than one coord is
 * given, the list is enclosed in parens to make the scope of
 * the unit suffix clear.
 *
 * \param [in] coord        Array of coords to convert
 * \param [in] n_coords     Number of coords in array
 * \param [in] printf_spec  printf sub-specifier to use with %f
 * \param [in] e_allow      Bitmap of units the function may use
 * \param [in] suffix_type  Whether to add a suffix
 *
 * \return A string containing the formatted coords. Must be freed with g_free.
 */
static gchar *CoordsToString(Coord coord[], int n_coords, const char *printf_spec, enum e_allow allow, enum e_suffix suffix_type)
{
  GString *buff;
  gchar *printf_buff;
  gchar filemode_buff[G_ASCII_DTOSTR_BUF_SIZE];
  enum e_family family;
  double *value;
  const char *suffix;
  int i, n;

  value = malloc (n_coords * sizeof *value);
  buff  = g_string_new ("");

  /* Sanity checks */
  if (buff == NULL || value == NULL)
    return NULL;
  if (allow == 0)
    allow = ALLOW_ALL;
  if (printf_spec == NULL)
    printf_spec = "";

  /* Check our freedom in choosing units */
  if ((allow & ALLOW_IMPERIAL) == 0)
    family = METRIC;
  else if ((allow & ALLOW_METRIC) == 0)
    family = IMPERIAL;
  else
    {
      int met_votes = 0,
          imp_votes = 0;

      for (i = 0; i < n_coords; ++i)
        if(min_sig_figs(COORD_TO_MIL(coord[i])) < min_sig_figs(COORD_TO_MM(coord[i])))
          ++imp_votes;
        else
          ++met_votes;

      if (imp_votes > met_votes)
        family = IMPERIAL;
      else
        family = METRIC;
    }

  /* Set base unit */
  for (i = 0; i < n_coords; ++i)
    {
      switch (family)
        {
        case METRIC:   value[i] = COORD_TO_MM (coord[i]); break;
        case IMPERIAL: value[i] = COORD_TO_MIL (coord[i]); break;
        }
    }

  /* Determine scale factor -- find smallest unit that brings
   * the whole group above unity */
  for (n = 0; n < N_UNITS; ++n)
    {
      if ((Units[n].allow & allow) != 0 && (Units[n].family == family))
        {
          int n_above_one = 0;
    
          for (i = 0; i < n_coords; ++i)
            if (fabs(value[i] * Units[n].scale_factor) > 1)
              ++n_above_one;
          if (n_above_one == n_coords)
            break;
        }
    }
  /* If nothing worked, wind back to the smallest allowable unit */
  if (n == N_UNITS)
    {
      do {
        --n;
      } while ((Units[n].allow & allow) == 0 || Units[n].family != family);
    }

  /* Apply scale factor */
  suffix = Units[n].suffix;
  for (i = 0; i < n_coords; ++i)
    value[i] = value[i] * Units[n].scale_factor;

  /* Create sprintf specifier, using default_prec no preciscion is given */
  i = 0;
  while (printf_spec[i] == '%' || isdigit(printf_spec[i]) ||
         printf_spec[i] == '-' || printf_spec[i] == '+' ||
         printf_spec[i] == '#' || printf_spec[i] == '0')
    ++i;
  if (printf_spec[i] == '.')
    printf_buff = g_strdup_printf (", %sf", printf_spec);
  else
    printf_buff = g_strdup_printf (", %s.%df", printf_spec, Units[n].default_prec);

  /* Actually sprintf the values in place
   *  (+ 2 skips the ", " for first value) */
  if (n_coords > 1)
    g_string_append_c (buff, '(');
  if (suffix_type == FILE_MODE)
    {
      g_ascii_formatd (filemode_buff, sizeof filemode_buff, printf_buff + 2, value[0]);
      g_string_append_printf (buff, "%s", filemode_buff);
    }
  else
    g_string_append_printf (buff, printf_buff + 2, value[0]);
  for (i = 1; i < n_coords; ++i)
    {
      if (suffix_type == FILE_MODE)
        {
          g_ascii_formatd (filemode_buff, sizeof filemode_buff, printf_buff, value[i]);
          g_string_append_printf (buff, "%s", filemode_buff);
        }
      else
        g_string_append_printf (buff, printf_buff, value[i]);
    }
  if (n_coords > 1)
    g_string_append_c (buff, ')');
  /* Append suffix */
  if (value[0] != 0 || n_coords > 1)
    {
      switch (suffix_type)
        {
        case NO_SUFFIX:
          break;
        case SUFFIX:
          g_string_append_printf (buff, " %s", suffix);
          break;
        case FILE_MODE:
          g_string_append_printf (buff, "%s", suffix);
          break;
        }
    }

  g_free (printf_buff);
  free (value);
  /* Return just the gchar* part of our string */
  return g_string_free (buff, FALSE);
}

/* \brief Main pcb-printf function
 * \par Function Description
 * This is a printf wrapper that accepts new format specifiers to
 * output pcb coords as various units. See the comment at the top
 * of pcb-printf.h for full details.
 *
 * \param [in] fmt    Format specifier
 * \param [in] args   Arguments to specifier
 *
 * \return A formatted string. Must be freed with g_free.
 */
gchar *pcb_vprintf(const char *fmt, va_list args)
{
  GString *string = g_string_new ("");
  GString *spec   = g_string_new ("");

  enum e_allow mask = ALLOW_ALL;

  if (string == NULL || spec == NULL)
    return NULL;

  while(*fmt)
    {
      enum e_suffix suffix = NO_SUFFIX;

      if(*fmt == '%')
        {
          gchar *unit_str = NULL;
          const char *ext_unit = "";
          Coord value[10];
          int count, i;

          g_string_assign (spec, "");

          /* Get printf sub-specifiers */
          g_string_append_c (spec, *fmt++);
          while(isdigit(*fmt) || *fmt == '.' || *fmt == ' ' || *fmt == '*'
                              || *fmt == '#' || *fmt == 'l' || *fmt == 'L'
                              || *fmt == 'h' || *fmt == '+' || *fmt == '-')
          {
            if (*fmt == '*')
              {
                g_string_append_printf (spec, "%d", va_arg (args, int));
                fmt++;
              }
            else
              g_string_append_c (spec, *fmt++);
          }
          /* Get our sub-specifiers */
          if(*fmt == '#')
            {
              mask = ALLOW_CMIL;  /* This must be pcb's base unit */
              fmt++;
            }
          if(*fmt == '$')
            {
              suffix = SUFFIX;
              fmt++;
            }
          /* Tack full specifier onto specifier */
          if (*fmt != 'm')
            g_string_append_c (spec, *fmt);
          switch(*fmt)
            {
            /* Printf specs */
            case 'o': case 'i': case 'd':
            case 'u': case 'x': case 'X':
              if(spec->str[1] == 'l')
                {
                  if(spec->str[2] == 'l')
                    unit_str = g_strdup_printf (spec->str, va_arg(args, long long));
                  else
                    unit_str = g_strdup_printf (spec->str, va_arg(args, long));
                }
              else
                {
                  unit_str = g_strdup_printf (spec->str, va_arg(args, int));
                }
              break;
            case 'e': case 'E': case 'f':
            case 'g': case 'G':
              if (strchr (spec->str, '*'))
                {
                  int prec = va_arg(args, int);
                  unit_str = g_strdup_printf (spec->str, va_arg(args, double), prec);
                }
              else
                unit_str = g_strdup_printf (spec->str, va_arg(args, double));
              break;
            case 'c':
              if(spec->str[1] == 'l' && sizeof(int) <= sizeof(wchar_t))
                unit_str = g_strdup_printf (spec->str, va_arg(args, wchar_t));
              else
                unit_str = g_strdup_printf (spec->str, va_arg(args, int));
              break;
            case 's':
              if(spec->str[0] == 'l')
                unit_str = g_strdup_printf (spec->str, va_arg(args, wchar_t *));
              else
                unit_str = g_strdup_printf (spec->str, va_arg(args, char *));
              break;
            case 'n':
              /* Depending on gcc settings, this will probably break with
               *  some silly "can't put %n in writeable data space" message */
              unit_str = g_strdup_printf (spec->str, va_arg(args, int *));
              break;
            case 'p':
              unit_str = g_strdup_printf (spec->str, va_arg(args, void *));
              break;
            case '%':
              g_string_append_c (string, '%');
              break;
            /* Our specs */
            case 'm':
              ++fmt;
              if (*fmt == '*')
                ext_unit = va_arg(args, const char *);
              if (*fmt != '+' && *fmt != 'a')
                value[0] = va_arg(args, Coord);
              count = 1;
              switch(*fmt)
                {
                case 's': unit_str = CoordsToString(value, 1, spec->str, ALLOW_MM | ALLOW_MIL, suffix); break;
                case 'S': unit_str = CoordsToString(value, 1, spec->str, mask & ALLOW_ALL, suffix); break;
                case 'M': unit_str = CoordsToString(value, 1, spec->str, mask & ALLOW_METRIC, suffix); break;
                case 'L': unit_str = CoordsToString(value, 1, spec->str, mask & ALLOW_IMPERIAL, suffix); break;
#if 0
                case 'r': unit_str = CoordsToString(value, 1, spec->str, ALLOW_READABLE, FILE_MODE); break;
#else
                case 'r': unit_str = CoordsToString(value, 1, spec->str, ALLOW_READABLE, NO_SUFFIX); break;
#endif
                /* All these fallthroughs are deliberate */
                case '9': value[count++] = va_arg(args, Coord);
                case '8': value[count++] = va_arg(args, Coord);
                case '7': value[count++] = va_arg(args, Coord);
                case '6': value[count++] = va_arg(args, Coord);
                case '5': value[count++] = va_arg(args, Coord);
                case '4': value[count++] = va_arg(args, Coord);
                case '3': value[count++] = va_arg(args, Coord);
                case '2':
                case 'D':
                  value[count++] = va_arg(args, Coord);
                  unit_str = CoordsToString(value, count, spec->str, mask & ALLOW_ALL, suffix);
                  break;
                case 'd':
                  value[1] = va_arg(args, Coord);
                  unit_str = CoordsToString(value, 2, spec->str, ALLOW_MM | ALLOW_MIL, suffix);
                  break;
                case '*':
                  for (i = 0; i < N_UNITS; ++i)
                    if (strcmp (ext_unit, Units[i].suffix) == 0)
                      unit_str = CoordsToString(value, 1, spec->str, Units[i].allow, suffix);
                  if (unit_str == NULL)
                    unit_str = CoordsToString(value, 1, spec->str, mask & ALLOW_ALL, suffix);
                  break;
                case 'a':
                  g_string_append (spec, ".0f");
                  if (suffix == SUFFIX)
                    g_string_append (spec, " deg");
                  unit_str = g_strdup_printf (spec->str, (double) va_arg(args, Angle));
                  break;
                case '+':
                  mask = va_arg(args, enum e_allow);
                  break;
                default:
                  for (i = 0; i < N_UNITS; ++i)
                    if (*fmt == Units[i].printf_code)
                      unit_str = CoordsToString(value, 1, spec->str, Units[i].allow, suffix);
                  if (unit_str == NULL)
                    unit_str = CoordsToString(value, 1, spec->str, ALLOW_ALL, suffix);
                  break;
                }
              break;
            }
          if (unit_str != NULL)
            {
              g_string_append (string, unit_str);
              g_free (unit_str);
            }
        }
      else
        g_string_append_c (string, *fmt);
      ++fmt;
    }
  g_string_free (spec, TRUE);
  /* Return just the gchar* part of our string */
  return g_string_free (string, FALSE);
}


/* \brief Wrapper for pcb_vprintf that outputs to a string
 *
 * \param [in] string  Pointer to string to output into
 * \param [in] fmt     Format specifier
 *
 * \return The length of the written string
 */
int pcb_sprintf(char *string, const char *fmt, ...)
{
  gchar *tmp;

  va_list args;
  va_start(args, fmt);

  tmp = pcb_vprintf (fmt, args);
  strcpy (string, tmp);
  g_free (tmp);
  
  va_end(args);
  return strlen (string);
}

/* \brief Wrapper for pcb_vprintf that outputs to a file
 *
 * \param [in] fh   File to output to
 * \param [in] fmt  Format specifier
 *
 * \return The length of the written string
 */
int pcb_fprintf(FILE *fh, const char *fmt, ...)
{
  int rv;
  gchar *tmp;

  va_list args;
  va_start(args, fmt);

  if (fh == NULL)
    rv = -1;
  else
    {
      tmp = pcb_vprintf (fmt, args);
      rv = fprintf (fh, "%s", tmp);
      g_free (tmp);
    }
  
  va_end(args);
  return rv;
}

/* \brief Wrapper for pcb_vprintf that outputs to stdout
 *
 * \param [in] fmt  Format specifier
 *
 * \return The length of the written string
 */
int pcb_printf(const char *fmt, ...)
{
  int rv;
  gchar *tmp;

  va_list args;
  va_start(args, fmt);

  tmp = pcb_vprintf (fmt, args);
  rv = printf ("%s", tmp);
  g_free (tmp);
  
  va_end(args);
  return rv;
}

/* \brief Wrapper for pcb_vprintf that outputs to a newly allocated string
 *
 * \param [in] fmt  Format specifier
 *
 * \return The newly allocated string. Must be freed with g_free.
 */
char *pcb_g_strdup_printf(const char *fmt, ...)
{
  gchar *tmp;

  va_list args;
  va_start(args, fmt);
  tmp = pcb_vprintf (fmt, args);
  va_end(args);
  return tmp;
}

