/*!
 * \file src/ascii-printf.c
 *
 * \brief printf() replacements which write ASCII text and numbers, regardless
 *        of the current locale (LC_ALL, LC_NUMBERIC, ...).
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 2015 Markus "Traumflug" Hitter <mah@jump-ing.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ascii-printf.h"

#include <string.h>
#include <glib/gprintf.h>


/*!
 * \brief Main ascii-printf function. All variants derive from this.
 *
 * \param str     String buffer for holding the result. NULL if result should
 *                not be written to a string.
 *
 * \param size    The maximum number of bytes to write to this buffer.
 *                Ignored when writing to a stream.
 *
 * \param stream  File stream for writing the result. If NULL, result isn't
 *                written to a stream.
 *
 * \param format  Format string for the result. Same syntax as formatting
 *                strings for printf(), but limited support (see below).
 *
 * \param args    Argument list matching the formatting string.
 *
 * To avoid being caught by locale quirks, like commas instead of dots for
 * decimal separators, we replace formatting floats with g_ascii_formatd()
 * from Glib. Unfortunately Glib provides no g_ascii_printf() its self.
 *
 * Target of this function are exporters, which write formatted strings to
 * standards compliant files. Usually such file format standards are picky
 * about decimal separators and similar stuff, so localisation of such
 * strings is bad, bad, bad.
 *
 * Supported formats:
 *
 *  - All double formats (%e, %E, %f, %F, %g and %G) full featured.
 *
 *  - Strings (%s) without length specifier.
 *
 * That's just what's currently needed, extending to more formats shouldn't
 * be too complicated when more needs arise.
 *
 * \note printf()s not formatting floats don't need this, integers have no
 *       decimal separator and can use the standard printf() just as well.
 */
static int ascii_universal_printf(char *str, size_t size, FILE *stream,
                                 const char *format, va_list args) {
  int written = 0, len;
  char float_format[16], float_str[128];
  const char *format_start = NULL;

  if (str) {
    fprintf(stderr, "ascii_universal_printf(): printing to a string is "
                    "currently not supported.\n");
    return 0;
  }

  while (*format) {
    if (*format == '%') {

      format_start = format;

      while (*format && format_start) {
        switch (*format) {
          case 'e':
          case 'E':
          case 'f':
          case 'F':
          case 'g':
          case 'G':
            len = format - format_start + 1;
            if (len > 15) len = 15;
            strncpy(float_format, format_start, len);
            float_format[len] = '\0';
            g_ascii_formatd(float_str, 128, float_format, va_arg(args, double));
            written += strlen(float_str);
            if (stream) fputs(float_str, stream);
            format_start = NULL;
            break;
          case 's':
            // variable format_start abused :-)
            format_start = va_arg(args, char *);
            written += strlen(format_start);
            if (stream) fputs(format_start, stream);
            format_start = NULL;
            break;
        }
        format++;
      }
    }

    if (stream) fputc(*format, stream);
    written++;
    format++;
  }

  return written;
}

/*!
 * \brief Locale-protected version of fprintf().
 *
 * \param stream  File stream for writing the result. If NULL, result isn't
 *                written to a stream.
 *
 * \param format  Format string for the result. Same syntax as formatting
 *                strings for printf(), but limited support. For details, see
 *                ascii_universal_printf().
 *
 * \param ...     Arguments matching the formatting string.
 */
int ascii_fprintf(FILE *stream, const char *format, ...) {
  int length;

  va_list args;
  va_start(args, format);

  length = ascii_universal_printf(NULL, 0, stream, format, args);

  va_end(args);

  return length;
}

/*!
 * \brief Locale-protected version of snprintf().
 *
 * \param str     String buffer for holding the result.
 *
 * \param size    The maximum number of bytes to write to this buffer.
 *
 * \param format  Format string for the result. Same syntax as formatting
 *                strings for printf(), but limited support. For details, see
 *                ascii_universal_printf().
 *
 * \param ...     Arguments matching the formatting string.
 */
int ascii_snprintf(char *str, size_t size, const char *format, ...) {
  int length;

  va_list args;
  va_start(args, format);

  length = ascii_universal_printf(str, size, 0, format, args);

  va_end(args);

  return length;
}


#ifdef PCB_UNIT_TEST

void
ascii_printf_register_tests() {
  g_test_add_func("/ascii-printf/test-printf", ascii_printf_test_printf);
}

void
ascii_printf_test_printf() {
  /**
   * Testing ascii_fprintf() isn't possible here, as this writes to a file.
   * However, several of the exportes regression tests in tests/ check
   * this functionality.
   */
}

#endif
