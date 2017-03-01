/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "data.h"

#ifndef WIN32
/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#   define GL_GLEXT_PROTOTYPES 1
#   include <GL/gl.h>
#endif

#ifdef HAVE_OPENGL_GL_H
#   include <OpenGL/gl.h>
#else
#   include <GL/gl.h>
#endif

#include "hid_draw.h"
#include "hidgl.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#ifdef WIN32
#   include "hid/common/glext.h"

extern PFNGLUSEPROGRAMPROC         glUseProgram;

extern PFNGLMULTITEXCOORD1FPROC    glMultiTexCoord1f;
extern PFNGLMULTITEXCOORD2FPROC    glMultiTexCoord2f;
extern PFNGLMULTITEXCOORD3FPROC    glMultiTexCoord3f;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC          glUniform1i;
extern PFNGLACTIVETEXTUREARBPROC   glActiveTextureARB;

#endif

static int
compute_offset (int x, int y, int width, int height)
{
  x = (x + width) % width;
  y = (y + height) % height;
  return (y * width + x) * 4;
}

static void
load_texture_from_png (char *filename, bool bumpmap)
{
  GError *error = NULL;
  GdkPixbuf *pixbuf;
  int width;
  int height;
  int rowstride;
  /* int has_alpha; */
  int bits_per_sample;
  int n_channels;
  unsigned char *pixels;
  unsigned char *new_pixels;
  int x, y;

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (pixbuf == NULL) {
    g_error ("%s", error->message);
    g_error_free (error);
    error = NULL;
    return;
  }

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  /* has_alpha = gdk_pixbuf_get_has_alpha (pixbuf); */
  bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  g_warn_if_fail (bits_per_sample == 8);
  g_warn_if_fail (n_channels == 4);
  g_warn_if_fail (rowstride == width * n_channels);

  new_pixels = malloc (width * height * 4);

  /* XXX: Move computing bump map out of the texture loading function */
  if (bumpmap) {
    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++) {
        float r, g, b;
        float lenvec;

        float dzdx = (pixels[compute_offset (x + 1, y, width, height)] - pixels[compute_offset (x - 1, y, width, height)]) / 255.;
        float dzdy = (pixels[compute_offset (x, y + 1, width, height)] - pixels[compute_offset (x, y - 1, width, height)]) / 255.;

#if 0
        dx *= 10.;
        dy *= 10.;
#endif

        /* Basis vectors are (x -> x+1) in X direction , z(x,y), z(x,x)+dzdx in Z direction */
        /*                   (y -> y+1) in Y direction , z(x,y), z(x,y)+dzdy in Z direction */

        /* Normal is cross product of the two basis vectors */
        // | i  j  k    |
        // | 1  0  dzdx |
        // | 0  1  dzdy |

        r = -dzdx;
        g = -dzdy;
        b = 1.;

        lenvec = sqrt (r * r + g * g + b * b);

        r /= lenvec;
        g /= lenvec;
        b /= lenvec;

        new_pixels [compute_offset (x, y, width, height) + 0] =
          (unsigned char)((r / 2. + 0.5) * 255.);
        new_pixels [compute_offset (x, y, width, height) + 1] =
          (unsigned char)((g / 2. + 0.5) * 255.);
        new_pixels [compute_offset (x, y, width, height) + 2] =
          (unsigned char)((b / 2. + 0.5) * 255.);
        new_pixels [compute_offset (x, y, width, height) + 3] = 255;
      }

    memcpy (pixels, new_pixels, width * height * 4);
    gdk_pixbuf_save (pixbuf, "debug_bumps.png", "png", NULL, NULL);
  }

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);

  free (new_pixels);
  g_object_unref (pixbuf);
}

static double
resistor_string_to_value (char *string)
{
  char *c = string;
  double accum_value = 0;
  bool before_decimal_point = true;
  double place_value = 1.;
  double multiplier = 1.;

  while (c != NULL && *c != '\0') {
    if (*c >= '0' && *c <= '9') {
      unsigned char digit_value;
      digit_value = *c - '0';

      if (before_decimal_point) {
        accum_value *= 10.;
        accum_value += digit_value;
      } else {
        place_value /= 10.;
        accum_value += digit_value * place_value;
      }
    } else if (before_decimal_point) {
      switch (*c) {
        case 'R':
        case 'r':
        case '.':
          multiplier = 1.;
          break;

        case 'k':
        case 'K':
          multiplier = 1000.;
          break;

        case 'M':
          multiplier = 1000000.;
          break;

        case 'm':
          multiplier = 1 / 1000.;
          break;

        default:
          /* XXX: Unknown multiplier */
          break;
      }
      before_decimal_point = false;

    } else {
      /* XXX: Unknown input */
    }
    c++;
  }

  accum_value *= multiplier;

  return accum_value;
}

static void
resistor_value_to_stripes (ElementType *element, int no_stripes, int *stripes, int *mul, int *tol)
{
  char *value_string = element->Name[VALUE_INDEX].TextString;
  double value;
  double order_of_magnitude;
  int sigfigs;
  int i;

  /* XXX: Don't know tolerenace, so assume 1% */
  *tol = 2;

  if (value_string == NULL) {
    stripes[0] = 0;
    stripes[1] = 0;
    stripes[2] = 0;
    *mul = 0;
    return;
  }

  value = resistor_string_to_value (value_string);

  order_of_magnitude = log (value) / log (10);
  *mul = (int)order_of_magnitude - no_stripes + 3;
  if (*mul < 0 || *mul > 9) {
    /* Resistor multiplier out of range, e.g. zero ohm links */
    *mul = 2; /* PUT BLACK */
    stripes[0] = 0;
    stripes[1] = 0;
    stripes[2] = 0;
    return;
  }

  /* Round to no_stripes significant figures */
  value /= pow (10., (int)order_of_magnitude - no_stripes + 1);
  sigfigs = (int)(value + 0.5);
  value = round (value);

  /* Start with all the sigfigs in the least significant stripes, then
   * proceed stripe by stripe subtracting anything which can carry over
   * into the next most significant stripe.
   *
   * NB: Stripe 0 is the most significant
   */
  stripes[no_stripes - 1] = sigfigs;

  for (i = no_stripes - 1; i > 0; i--) {
    stripes [i - 1] = stripes[i] / 10;
    stripes [i] -= 10 * stripes[i - 1];

    assert (stripes[i] >= 0 && stripes[i] <= 9);
  }

  assert (stripes[0] >= 0 && stripes[0] <= 9);
}

static void
setup_zero_ohm_texture (GLfloat *res_body_color)
{
  GLfloat tex_data[32][3];
  int strip;

  for (strip = 0; strip < sizeof (tex_data) / sizeof (GLfloat[3]); strip++) {
    tex_data[strip][0] = res_body_color[0];
    tex_data[strip][1] = res_body_color[1];
    tex_data[strip][2] = res_body_color[2];
  }

  tex_data[15][0] = 0.;  tex_data[15][1] = 0.;  tex_data[15][2] = 0.;
  tex_data[16][0] = 0.;  tex_data[16][1] = 0.;  tex_data[16][2] = 0.;

  glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB, GL_FLOAT, tex_data);
}

static bool
setup_resistor_texture (ElementType *element, GLfloat *res_body_color, int no_stripes)
{
  GLfloat val_color[10][3] = {{0.00, 0.00, 0.00},
                              {0.52, 0.26, 0.00},
                              {1.00, 0.00, 0.00},
                              {1.00, 0.52, 0.00},
                              {1.00, 1.00, 0.00},
                              {0.00, 0.91, 0.00},
                              {0.00, 0.00, 1.00},
                              {0.68, 0.00, 0.68},
                              {0.65, 0.65, 0.65},
                              {1.00, 1.00, 1.00}};

  GLfloat mul_color[10][3] = {{0.65, 0.65, 0.65},  /* 0.01  */ /* XXX: Silver */
                              {0.72, 0.48, 0.09},  /* 0.1   */ /* XXX: Gold */
                              {0.00, 0.00, 0.00},  /* 1     */
                              {0.52, 0.26, 0.00},  /* 10    */
                              {1.00, 0.00, 0.00},  /* 100   */
                              {1.00, 0.52, 0.00},  /* 1k    */
                              {1.00, 1.00, 0.00},  /* 10k   */
                              {0.00, 0.91, 0.00},  /* 100k  */
                              {0.00, 0.00, 1.00},  /* 1M    */
                              {0.68, 0.00, 0.68}}; /* 10M   */

  GLfloat tol_color[10][3] = {{0.65, 0.65, 0.65},  /* 10%   */ /* XXX: Silver */
                              {0.72, 0.48, 0.09},  /* 5%    */ /* XXX: Gold */
                              {0.52, 0.26, 0.00},  /* 1%    */
                              {1.00, 0.00, 0.00},  /* 2%    */
                              {0.00, 0.91, 0.00},  /* 0.5%  */
                              {0.00, 0.00, 1.00},  /* 0.25% */
                              {0.68, 0.00, 0.68}}; /* 0.1%  */

  int *stripes;
  int mul;
  int tol;

  GLfloat tex_data[32][3];
  int texel;
  int i;
  bool is_zero_ohm;

  if (TEST_FLAG (SELECTEDFLAG, element)) {
    GLfloat selected_color[] = {0.00, 0.70, 0.82};
    float mix_color_ratio = 0.5;
    res_body_color[0] = res_body_color[0] * (1 - mix_color_ratio) + selected_color[0] * mix_color_ratio;
    res_body_color[1] = res_body_color[1] * (1 - mix_color_ratio) + selected_color[1] * mix_color_ratio;
    res_body_color[2] = res_body_color[2] * (1 - mix_color_ratio) + selected_color[2] * mix_color_ratio;
  }

  stripes = malloc (no_stripes * sizeof (int));
  resistor_value_to_stripes (element, no_stripes, stripes, &mul, &tol);

  is_zero_ohm = true;

  for (i = 0; i < no_stripes; i++) {
    if (stripes[i] != 0)
      is_zero_ohm = false;
  }

  if (is_zero_ohm) {
    setup_zero_ohm_texture (res_body_color);
    free (stripes);
    return true;
  }

  for (texel = 0; texel < sizeof (tex_data) / sizeof (GLfloat[3]); texel++) {
    tex_data[texel][0] = res_body_color[0];
    tex_data[texel][1] = res_body_color[1];
    tex_data[texel][2] = res_body_color[2];
  }

  if (no_stripes == 2) {
    tex_data[ 7][0] = val_color[stripes[0]][0];  tex_data[ 7][1] = val_color[stripes[0]][1];  tex_data[ 7][2] = val_color[stripes[0]][2];
    tex_data[10][0] = val_color[stripes[1]][0];  tex_data[10][1] = val_color[stripes[1]][1];  tex_data[10][2] = val_color[stripes[1]][2];
    tex_data[14][0] = mul_color[mul]       [0];  tex_data[14][1] = mul_color[mul]       [1];  tex_data[14][2] = mul_color[mul]       [2];
    tex_data[24][0] = tol_color[tol]       [0];  tex_data[24][1] = tol_color[tol]       [1];  tex_data[24][2] = tol_color[tol]       [2];
  }

  if (no_stripes == 3) {

    tex_data[ 7][0] = val_color[stripes[0]][0];  tex_data[ 7][1] = val_color[stripes[0]][1];  tex_data[ 7][2] = val_color[stripes[0]][2];

    tex_data[10][0] = val_color[stripes[1]][0];  tex_data[10][1] = val_color[stripes[1]][1];  tex_data[10][2] = val_color[stripes[1]][2];
    tex_data[11][0] = val_color[stripes[1]][0];  tex_data[11][1] = val_color[stripes[1]][1];  tex_data[11][2] = val_color[stripes[1]][2];

    tex_data[14][0] = val_color[stripes[2]][0];  tex_data[14][1] = val_color[stripes[2]][1];  tex_data[14][2] = val_color[stripes[2]][2];
    tex_data[15][0] = val_color[stripes[2]][0];  tex_data[15][1] = val_color[stripes[2]][1];  tex_data[15][2] = val_color[stripes[2]][2];

    tex_data[18][0] = mul_color[mul]       [0];  tex_data[18][1] = mul_color[mul]       [1];  tex_data[18][2] = mul_color[mul]       [2];
    tex_data[19][0] = mul_color[mul]       [0];  tex_data[19][1] = mul_color[mul]       [1];  tex_data[19][2] = mul_color[mul]       [2];

    tex_data[24][0] = tol_color[tol]       [0];  tex_data[24][1] = tol_color[tol]       [1];  tex_data[24][2] = tol_color[tol]       [2];
  }


  glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB, GL_FLOAT, tex_data);

  free (stripes);
  return false;
}

static void invert_4x4 (float m[4][4], float out[4][4]);

static GLfloat *debug_lines = NULL;
static int no_debug_lines = 0;
static int max_debug_lines = 0;

//#define LENG 1000
#define LENG 0.6
#define STRIDE_FLOATS 6
static void
debug_basis_vector (float x,   float y,   float z,
                    float b1x, float b1y, float b1z,
                    float b2x, float b2y, float b2z,
                    float b3x, float b3y, float b3z)
{
  int comp_count;
  float lenb1, lenb2, lenb3;

  if (no_debug_lines + 3 > max_debug_lines) {
    max_debug_lines += 10;
    debug_lines = realloc (debug_lines, max_debug_lines * sizeof (GLfloat) * 2 * STRIDE_FLOATS);
  }

  lenb1 = sqrt (b1x * b1x + b1y * b1y + b1z * b1z);
  lenb2 = sqrt (b2x * b2x + b2y * b2y + b2z * b2z);
  lenb3 = sqrt (b3x * b3x + b3y * b3y + b3z * b3z);
  b1x /= lenb1;  b1y /= lenb1;  b1z /= lenb1;
  b2x /= lenb2;  b2y /= lenb2;  b2z /= lenb2;
  b3x /= lenb3;  b3y /= lenb3;  b3z /= lenb3;

  comp_count = 0;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = x;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = y;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = z;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 1.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = x + b1x * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = y + b1y * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = z + b1z * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 1.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  no_debug_lines++;

  comp_count = 0;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = x;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = y;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = z;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 1.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = x + b2x * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = y + b2y * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = z + b2z * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 1.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  no_debug_lines++;

  comp_count = 0;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = x;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = y;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = z;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 1.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = x + b3x * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = y + b3y * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = z + b3z * LENG;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 0.;
  debug_lines [no_debug_lines * 2 * STRIDE_FLOATS + comp_count++] = 1.;
  no_debug_lines++;
}

static void
debug_basis_display ()
{
  if (no_debug_lines == 0)
    return;

#if 1
  glPushAttrib (GL_CURRENT_BIT);
  glColor4f (1., 1., 1., 1.);
  glVertexPointer (3, GL_FLOAT, STRIDE_FLOATS * sizeof (GLfloat), &debug_lines[0]);
  glColorPointer (3, GL_FLOAT, STRIDE_FLOATS * sizeof (GLfloat), &debug_lines[3]);
  glEnableClientState (GL_VERTEX_ARRAY);
  glEnableClientState (GL_COLOR_ARRAY);
  glDrawArrays (GL_LINES, 0, no_debug_lines * 2);
  glDisableClientState (GL_COLOR_ARRAY);
  glDisableClientState (GL_VERTEX_ARRAY);
  glPopAttrib ();
#endif

  free (debug_lines);
  debug_lines = NULL;
  no_debug_lines = 0;
  max_debug_lines = 0;
}

/* b1{x,y,z} is the basis vector along "s" texture space */
/* b2{x,y,z} is the basis vector along "t" texture space */
/* mvm is the model view matrix (transposed in memory) */
static void
compute_light_vector (float b1x, float b1y, float b1z,
                      float b2x, float b2y, float b2z,
                      float *lx, float *ly, float *lz,
                      float *hx, float *hy, float *hz,
                      float x,   float y,   float z,
                      GLfloat *mvm)
{
  float b3x, b3y, b3z;
  float tb1x, tb1y, tb1z;
  float tb2x, tb2y, tb2z;
  float tb3x, tb3y, tb3z;
  /* NB: light_direction is a vector _TOWARDS_ the light source position */
//  float light_direction[] = {-0.5, 1., -1.}; /* XXX: HARDCODEED! */
  float light_direction[] = {0.0, 0.0, 1.0}; /* XXX: HARDCODEED! */
  float half_direction[3];
  float texspace_to_eye[4][4];
  float eye_to_texspace[4][4];
  float lenb1, lenb2, lenb3;
  float len_half;
  float len_light;

  /* Normalise the light vector */
  len_light = sqrt (light_direction[0] * light_direction[0] +
                    light_direction[1] * light_direction[1] +
                    light_direction[2] * light_direction[2]);
  light_direction[0] /= len_light;
  light_direction[1] /= len_light;
  light_direction[2] /= len_light;

  /* Sum with the unit vector towards the viewer */
  half_direction[0] = light_direction[0] + 0.;
  half_direction[1] = light_direction[1] + 0.;
  half_direction[2] = light_direction[2] + 1.;

  if (0)
    debug_basis_vector ((mvm[0] * x + mvm[4] * y + mvm[ 8] * z + mvm[12]) / mvm[15],
                        (mvm[1] * x + mvm[5] * y + mvm[ 9] * z + mvm[13]) / mvm[15],
                        (mvm[2] * x + mvm[6] * y + mvm[10] * z + mvm[14]) / mvm[15],
                        0., 0., 1.,
                        light_direction[0], light_direction[1], light_direction[2],
                        half_direction[0], half_direction[1], half_direction[2]);

  /* Third basis vector is the cross product of tb1 and tb2 */
  b3x = (b2y * b1z - b2z * b1y);
  b3y = (b2z * b1x - b2x * b1z);
  b3z = (b2x * b1y - b2y * b1x);

  /* Transform the S, T texture space bases into eye coordinates */
  tb1x = mvm[0] * b1x + mvm[4] * b1y + mvm[ 8] * b1z;
  tb1y = mvm[1] * b1x + mvm[5] * b1y + mvm[ 9] * b1z;
  tb1z = mvm[2] * b1x + mvm[6] * b1y + mvm[10] * b1z;

  tb2x = mvm[0] * b2x + mvm[4] * b2y + mvm[ 8] * b2z;
  tb2y = mvm[1] * b2x + mvm[5] * b2y + mvm[ 9] * b2z;
  tb2z = mvm[2] * b2x + mvm[6] * b2y + mvm[10] * b2z;

  tb3x = mvm[0] * b3x + mvm[4] * b3y + mvm[ 8] * b3z;
  tb3y = mvm[1] * b3x + mvm[5] * b3y + mvm[ 9] * b3z;
  tb3z = mvm[2] * b3x + mvm[6] * b3y + mvm[10] * b3z;

#if 1
  /* Normalise tb1, tb2 and tb3 */
  lenb1 = sqrt (tb1x * tb1x + tb1y * tb1y + tb1z * tb1z);
  lenb2 = sqrt (tb2x * tb2x + tb2y * tb2y + tb2z * tb2z);
  lenb3 = sqrt (tb3x * tb3x + tb3y * tb3y + tb3z * tb3z);
  tb1x /= lenb1;  tb1y /= lenb1;  tb1z /= lenb1;
  tb2x /= lenb2;  tb2y /= lenb2;  tb2z /= lenb2;
  tb3x /= lenb3;  tb3y /= lenb3;  tb3z /= lenb3;
#endif

  if (0)
    debug_basis_vector ((mvm[0] * x + mvm[4] * y + mvm[ 8] * z + mvm[12]) / mvm[15],
                        (mvm[1] * x + mvm[5] * y + mvm[ 9] * z + mvm[13]) / mvm[15],
                        (mvm[2] * x + mvm[6] * y + mvm[10] * z + mvm[14]) / mvm[15],
                         tb1x, tb1y, tb1z, tb2x, tb2y, tb2z, tb3x, tb3y, tb3z);

  texspace_to_eye[0][0] = tb1x; texspace_to_eye[0][1] = tb2x; texspace_to_eye[0][2] = tb3x;  texspace_to_eye[0][3] = 0.0;
  texspace_to_eye[1][0] = tb1y; texspace_to_eye[1][1] = tb2y; texspace_to_eye[1][2] = tb3y;  texspace_to_eye[1][3] = 0.0;
  texspace_to_eye[2][0] = tb1z; texspace_to_eye[2][1] = tb2z; texspace_to_eye[2][2] = tb3z;  texspace_to_eye[2][3] = 0.0;
  texspace_to_eye[3][0] = 0.0;  texspace_to_eye[3][1] = 0.0;  texspace_to_eye[3][2] = 0.0;   texspace_to_eye[3][3] = 1.0;

  invert_4x4 (texspace_to_eye, eye_to_texspace);

  /* light_direction is in eye space, we need to transform this into texture space */
  *lx = eye_to_texspace[0][0] * light_direction[0] +
        eye_to_texspace[0][1] * light_direction[1] +
        eye_to_texspace[0][2] * light_direction[2];
  *ly = eye_to_texspace[1][0] * light_direction[0] +
        eye_to_texspace[1][1] * light_direction[1] +
        eye_to_texspace[1][2] * light_direction[2];
  *lz = eye_to_texspace[2][0] * light_direction[0] +
        eye_to_texspace[2][1] * light_direction[1] +
        eye_to_texspace[2][2] * light_direction[2];

  /* half_direction is in eye space, we need to transform this into texture space */
  *hx = eye_to_texspace[0][0] * half_direction[0] +
        eye_to_texspace[0][1] * half_direction[1] +
        eye_to_texspace[0][2] * half_direction[2];
  *hy = eye_to_texspace[1][0] * half_direction[0] +
        eye_to_texspace[1][1] * half_direction[1] +
        eye_to_texspace[1][2] * half_direction[2];
  *hz = eye_to_texspace[2][0] * half_direction[0] +
        eye_to_texspace[2][1] * half_direction[1] +
        eye_to_texspace[2][2] * half_direction[2];

  {
    len_light = sqrt (*lx * *lx + *ly * *ly + *lz * *lz);
    *lx /= len_light;
    *ly /= len_light;
    *lz /= len_light;

    *lx = *lx / 2. + 0.5;
    *ly = *ly / 2. + 0.5;
    *lz = *lz / 2. + 0.5;

    len_half = sqrt (*hx * *hx + *hy * *hy + *hz * *hz);
    *hx /= len_half;
    *hy /= len_half;
    *hz /= len_half;

    *hx = *hx / 2. + 0.5;
    *hy = *hy / 2. + 0.5;
    *hz = *hz / 2. + 0.5;
  }
}

static void
emit_vertex (float x,   float y,   float z,
             float b1x, float b1y, float b1z,
             float b2x, float b2y, float b2z,
             float tex0_s, float tex1_s, float tex1_t,
             GLfloat *mvm)
{
  GLfloat lx, ly, lz;
  GLfloat hx, hy, hz;
  compute_light_vector (b1x, b1y, b1z, b2x, b2y, b2z, &lx, &ly, &lz, &hx, &hy, &hz, x, y, z, mvm);
  glColor3f (lx, ly, lz);
  glMultiTexCoord1f (GL_TEXTURE0, tex0_s);
  glMultiTexCoord2f (GL_TEXTURE1, tex1_s, tex1_t);
  glMultiTexCoord3f (GL_TEXTURE2, hx, hy, hz);
  glVertex3f (x, y, z);
}

enum geom_pos {
  FIRST,
  MIDDLE,
  LAST
};

static void
emit_pair (float ang_edge1, float cos_edge1, float sin_edge1,
           float ang_edge2, float cos_edge2, float sin_edge2,
           float prev_r, float prev_z,
           float      r, float      z,
           float next_r, float next_z,
           float tex0_s, float resistor_width,
           enum geom_pos pos,
           GLfloat *mvm)
{
  int repeat;

  tex0_s = z / resistor_width + 0.5;

  for (repeat = 0; repeat < ((pos == FIRST) ? 2 : 1); repeat++)
    emit_vertex (r * cos_edge1, r * sin_edge1, z,
                 sin_edge1, -cos_edge1, 0,
                 cos_edge1 * (next_r - prev_r) / 2., sin_edge1 * (next_r - prev_r) / 2., (next_z - prev_z) / 2.,
                 tex0_s, ang_edge1 / 2. / M_PI, tex0_s, mvm);

  for (repeat = 0; repeat < ((pos == LAST) ? 2 : 1); repeat++)
    emit_vertex (r * cos_edge2, r * sin_edge2, z,
                 sin_edge2, -cos_edge2, 0,
                 cos_edge2 * (next_r - prev_r) / 2., sin_edge2 * (next_r - prev_r) / 2., (next_z - prev_z) / 2.,
                 tex0_s, ang_edge2 / 2. / M_PI, tex0_s, mvm);
}


#define NUM_RESISTOR_STRIPS 100
#define NUM_PIN_RINGS 15

/* XXX: HARDCODED MAGIC */
#define BOARD_THICKNESS MM_TO_COORD (1.6)

void
hidgl_draw_acy_resistor (ElementType *element, float surface_depth, float board_thickness)
{

  float center_x, center_y;
  float angle;
  GLfloat resistor_body_color[] =         {0.31, 0.47, 0.64};
  GLfloat resistor_pin_color[] =          {0.82, 0.82, 0.82};
  GLfloat resistor_warn_pin_color[] =     {0.82, 0.20, 0.20};
  GLfloat resistor_found_pin_color[] =    {0.20, 0.82, 0.20};
  GLfloat resistor_selected_pin_color[] = {0.00, 0.70, 0.82};
  GLfloat *pin_color;

  GLfloat mvm[16];

  int strip;
  int no_strips = NUM_RESISTOR_STRIPS;
  int ring;
  int no_rings = NUM_PIN_RINGS;
  int end;
  bool zero_ohm;

  static bool first_run = true;
  static GLuint texture1;
  static GLuint texture2_resistor;
  static GLuint texture2_zero_ohm;

  GLuint restore_sp;

  /* XXX: Hard-coded magic */
  float resistor_pin_radius = MIL_TO_COORD (12.);
  float resistor_taper1_radius = MIL_TO_COORD (16.);
  float resistor_taper2_radius = MIL_TO_COORD (35.);
  float resistor_bulge_radius = MIL_TO_COORD (43.);
  float resistor_barrel_radius = MIL_TO_COORD (37.);

  float resistor_taper1_offset = MIL_TO_COORD (25.);
  float resistor_taper2_offset = MIL_TO_COORD (33.);
  float resistor_bulge_offset = MIL_TO_COORD (45.);
  float resistor_bulge_width = MIL_TO_COORD (50.);
  float resistor_pin_spacing = MIL_TO_COORD (400.);

  float pin_penetration_depth = MIL_TO_COORD (30.) + board_thickness;

  float resistor_pin_bend_radius = resistor_bulge_radius;
  float resistor_width = resistor_pin_spacing - 2. * resistor_pin_bend_radius;

  PinType *first_pin = element->Pin->data;
  PinType *second_pin = g_list_next (element->Pin)->data;
  PinType *pin;

  Coord pin_delta_x = second_pin->X - first_pin->X;
  Coord pin_delta_y = second_pin->Y - first_pin->Y;

  center_x = first_pin->X + pin_delta_x / 2.;
  center_y = first_pin->Y + pin_delta_y / 2.;
  angle = atan2f (pin_delta_y, pin_delta_x);

  /* TRANSFORM MATRIX */
  glPushMatrix ();
  glTranslatef (center_x, center_y, surface_depth + resistor_pin_bend_radius);
  glRotatef (angle * 180. / M_PI + 90, 0., 0., 1.);
  glRotatef (90, 1., 0., 0.);

  /* Retrieve the resulting modelview matrix for the lighting calculations */
  glGetFloatv (GL_MODELVIEW_MATRIX, (GLfloat *)mvm);

  /* TEXTURE SETUP */
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint*)&restore_sp);
  hidgl_shader_activate (resistor_program);

  {
    GLuint program = hidgl_shader_get_program (resistor_program);
    int tex0_location = glGetUniformLocation (program, "detail_tex");
    int tex1_location = glGetUniformLocation (program, "bump_tex");
    glUniform1i (tex0_location, 0);
    glUniform1i (tex1_location, 1);
  }

  glActiveTextureARB (GL_TEXTURE0_ARB);
//  if (first_run) {
    glGenTextures (1, &texture1);
    glBindTexture (GL_TEXTURE_1D, texture1);
    zero_ohm = setup_resistor_texture (element, resistor_body_color, 3);
//  } else {
//    glBindTexture (GL_TEXTURE_1D, texture1);
//  }
  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameterf (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glEnable (GL_TEXTURE_1D);

  glActiveTextureARB (GL_TEXTURE1_ARB);
  if (first_run) {
    glGenTextures (1, &texture2_resistor);
    glBindTexture (GL_TEXTURE_2D, texture2_resistor);
    load_texture_from_png ("resistor_bump.png", true);

    glGenTextures (1, &texture2_zero_ohm);
    glBindTexture (GL_TEXTURE_2D, texture2_zero_ohm);
    load_texture_from_png ("zero_ohm_bump.png", true);
  }
  if (zero_ohm)
    glBindTexture (GL_TEXTURE_2D, texture2_zero_ohm);
  else
    glBindTexture (GL_TEXTURE_2D, texture2_resistor);

  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glEnable (GL_TEXTURE_2D);
  glActiveTextureARB (GL_TEXTURE0_ARB);

  /* COLOR / MATERIAL SETUP */
//  glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
//  glEnable (GL_COLOR_MATERIAL);

  glPushAttrib (GL_CURRENT_BIT);
//  glColor4f (1., 1., 1., 1.);
  glColor4f (0., 0., 1., 0.);

  glDisable (GL_LIGHTING);

  if (1) {
    GLfloat emission[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat specular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat shininess = 20.;
    glMaterialfv (GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, &shininess);
  }

#if 1
  glBegin (GL_TRIANGLE_STRIP);

  for (strip = 0; strip < no_strips; strip++) {

    int ring;
    int no_rings;
    float angle_edge1 = strip * 2. * M_PI / no_strips;
    float angle_edge2 = (strip + 1) * 2. * M_PI / no_strips;

    float cos_edge1 = cosf (angle_edge1);
    float sin_edge1 = sinf (angle_edge1);
    float cos_edge2 = cosf (angle_edge2);
    float sin_edge2 = sinf (angle_edge2);

    struct strip_item {
      GLfloat z;
      GLfloat r;
      GLfloat tex0_s;
    } strip_data[] = {
      {-resistor_width / 2. - 1.,                                                     resistor_pin_radius,     0.}, /* DUMMY */
      {-resistor_width / 2.,                                                          resistor_pin_radius,     0.},
      {-resistor_width / 2. + resistor_taper1_offset,                                 resistor_taper1_radius,  0.},
      {-resistor_width / 2. + resistor_taper2_offset,                                 resistor_taper2_radius,  0.},
      {-resistor_width / 2. + resistor_bulge_offset                              ,   resistor_bulge_radius, 0.},
      {-resistor_width / 2. + resistor_bulge_offset + resistor_bulge_width * 3. / 4.,   resistor_bulge_radius, 0.},
      {-resistor_width / 2. + resistor_bulge_offset + resistor_bulge_width,           resistor_barrel_radius,  0.},
//      {-resistor_width / 2. + resistor_bulge_offset + resistor_bulge_width,           resistor_barrel_radius,  0.},
                                                                                      /*********************/  
      { 0,                                                                            resistor_barrel_radius,  0.5},
                                                                                      /*********************/
//      { resistor_width / 2. - resistor_bulge_offset - resistor_bulge_width,           resistor_barrel_radius,  1.},
      { resistor_width / 2. - resistor_bulge_offset - resistor_bulge_width,           resistor_barrel_radius,  1.},
      { resistor_width / 2. - resistor_bulge_offset - resistor_bulge_width * 3. / 4.,   resistor_bulge_radius, 1.},
      { resistor_width / 2. - resistor_bulge_offset,                                  resistor_bulge_radius, 1.},
      { resistor_width / 2. - resistor_taper2_offset,                                 resistor_taper2_radius,  1.},
      { resistor_width / 2. - resistor_taper1_offset,                                 resistor_taper1_radius,  1.},
      { resistor_width / 2.,                                                          resistor_pin_radius,     1.},
      { resistor_width / 2. + 1.,                                                     resistor_pin_radius,     1.}, /* DUMMY */
    };

    no_rings = sizeof (strip_data) / sizeof (struct strip_item);
    for (ring = 1; ring < no_rings - 1; ring++) {
      enum geom_pos pos = MIDDLE;
      if (ring == 1)            pos = FIRST;
      if (ring == no_rings - 2) pos = LAST;

      emit_pair (angle_edge1, cos_edge1, sin_edge1,
                 angle_edge2, cos_edge2, sin_edge2,
                 strip_data[ring - 1].r, strip_data[ring - 1].z,
                 strip_data[ring    ].r, strip_data[ring    ].z,
                 strip_data[ring + 1].r, strip_data[ring + 1].z,
                 strip_data[ring    ].tex0_s, resistor_width, pos, mvm);
    }
  }
#endif

  glEnd ();

  glActiveTextureARB (GL_TEXTURE1_ARB);
  glDisable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, 0);

  glActiveTextureARB (GL_TEXTURE0_ARB);
  glDisable (GL_TEXTURE_1D);
  glBindTexture (GL_TEXTURE_1D, 0);
  glDeleteTextures (1, &texture1);

  glEnable (GL_LIGHTING);

  glUseProgram (0);

  /* COLOR / MATERIAL SETUP */
  glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable (GL_COLOR_MATERIAL);

  if (1) {
//    GLfloat ambient[] = {0.0, 0.0, 0.0, 1.0};
    GLfloat specular[] = {0.5, 0.5, 0.5, 1.0};
    GLfloat shininess = 120.;
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, &shininess);
  }

  for (end = 0; end < 2; end++) {
    float end_sign = (end == 0) ? 1. : -1.;

    pin = (end == 1) ? first_pin : second_pin;

    if (TEST_FLAG (WARNFLAG, pin))
      pin_color = resistor_warn_pin_color;
    else if (TEST_FLAG (SELECTEDFLAG, pin))
      pin_color = resistor_selected_pin_color;
    else if (TEST_FLAG (FOUNDFLAG, pin))
      pin_color = resistor_found_pin_color;
    else
      pin_color = resistor_pin_color;

    glColor3f (pin_color[0] / 1.5,
               pin_color[1] / 1.5,
               pin_color[2] / 1.5);

    for (ring = 0; ring < no_rings; ring++) {

      float angle_ring_edge1 = ring * M_PI / 2. / no_rings + ((end == 0) ? 0. : -M_PI / 2.);
      float angle_ring_edge2 = (ring + 1) * M_PI / 2. / no_rings + ((end == 0) ? 0. : -M_PI / 2.);
      float y_strip_edge1 = cosf (angle_ring_edge1);
      float z_strip_edge1 = sinf (angle_ring_edge1);
      float y_strip_edge2 = cosf (angle_ring_edge2);
      float z_strip_edge2 = sinf (angle_ring_edge2);
      float r = resistor_pin_bend_radius;

      glBegin (GL_TRIANGLE_STRIP);

      /* NB: We wrap back around to complete the last segment, so in effect
       *     we draw no_strips + 1 strips.
       */
      for (strip = 0; strip < no_strips + 1; strip++) {
        float strip_angle = strip * 2. * M_PI / no_strips;

        float x1 = resistor_pin_radius * cos (strip_angle);
        float y1 = resistor_pin_radius * sin (strip_angle) * y_strip_edge1 + r * y_strip_edge1 - r;
        float z1 = resistor_pin_radius * sin (strip_angle) * z_strip_edge1 + r * z_strip_edge1 + resistor_width / 2. * end_sign;

        float x2 = resistor_pin_radius * cos (strip_angle);
        float y2 = resistor_pin_radius * sin (strip_angle) * y_strip_edge2 + r * y_strip_edge2 - r;
        float z2 = resistor_pin_radius * sin (strip_angle) * z_strip_edge2 + r * z_strip_edge2 + resistor_width / 2. * end_sign;

        glNormal3f (cos (strip_angle), sin (strip_angle) * y_strip_edge1, sin (strip_angle) * z_strip_edge1);
        glVertex3f (x1, y1, z1);
        glNormal3f (cos (strip_angle), sin (strip_angle) * y_strip_edge2, sin (strip_angle) * z_strip_edge2);
        glVertex3f (x2, y2, z2);
      }
      glEnd ();
    }

    if (1) {
      float r = resistor_pin_bend_radius;
      glBegin (GL_TRIANGLE_STRIP);

      /* NB: We wrap back around to complete the last segment, so in effect
       *     we draw no_strips + 1 strips.
       */
      for (strip = 0; strip < no_strips + 1; strip++) {
        float strip_angle = strip * 2. * M_PI / no_strips;

        float x1 = resistor_pin_radius * cos (strip_angle);
        float y1 = -r;
        float z1 = resistor_pin_radius * sin (strip_angle) + (r + resistor_width / 2.) * end_sign;

        float x2 = resistor_pin_radius * cos (strip_angle);
        float y2 = -r - pin_penetration_depth;
        float z2 = resistor_pin_radius * sin (strip_angle) + (r + resistor_width / 2.) * end_sign;

        glNormal3f (cos (strip_angle), 0., sin (strip_angle));
        glVertex3f (x1, y1, z1);
        glNormal3f (cos (strip_angle), 0., sin (strip_angle));
        glVertex3f (x2, y2, z2);
      }
      glEnd ();
    }

    if (1) {
      float r = resistor_pin_bend_radius;
      glBegin (GL_TRIANGLE_FAN);

      glNormal3f (0, 0., -1.);
      glVertex3f (0, -r - pin_penetration_depth - resistor_pin_radius / 2., (r + resistor_width / 2.) * end_sign);

      /* NB: We wrap back around to complete the last segment, so in effect
       *     we draw no_strips + 1 strips.
       */
      for (strip = no_strips + 1; strip > 0; strip--) {
        float strip_angle = strip * 2. * M_PI / no_strips;

        float x = resistor_pin_radius * cos (strip_angle);
        float y = -r - pin_penetration_depth;
        float z = resistor_pin_radius * sin (strip_angle) + (r + resistor_width / 2.) * end_sign;

        glNormal3f (cos (strip_angle), 0., sin (strip_angle));
        glVertex3f (x, y, z);
      }
      glEnd ();
    }
  }

  glDisable (GL_COLOR_MATERIAL);
  glPopAttrib ();

  glDisable (GL_LIGHTING);
//  glDisable (GL_DEPTH_TEST);

  glPushMatrix ();
  glLoadIdentity ();
  debug_basis_display ();
  glPopMatrix ();
//  glEnable (GL_DEPTH_TEST);

  glPopMatrix ();
  glUseProgram (restore_sp);

  first_run = false;
}

static float
determinant_4x4 (float m[4][4])
{
  float det;
  det = m[0][3] * m[1][2] * m[2][1] * m[3][0]-m[0][2] * m[1][3] * m[2][1] * m[3][0] -
        m[0][3] * m[1][1] * m[2][2] * m[3][0]+m[0][1] * m[1][3] * m[2][2] * m[3][0] +
        m[0][2] * m[1][1] * m[2][3] * m[3][0]-m[0][1] * m[1][2] * m[2][3] * m[3][0] -
        m[0][3] * m[1][2] * m[2][0] * m[3][1]+m[0][2] * m[1][3] * m[2][0] * m[3][1] +
        m[0][3] * m[1][0] * m[2][2] * m[3][1]-m[0][0] * m[1][3] * m[2][2] * m[3][1] -
        m[0][2] * m[1][0] * m[2][3] * m[3][1]+m[0][0] * m[1][2] * m[2][3] * m[3][1] +
        m[0][3] * m[1][1] * m[2][0] * m[3][2]-m[0][1] * m[1][3] * m[2][0] * m[3][2] -
        m[0][3] * m[1][0] * m[2][1] * m[3][2]+m[0][0] * m[1][3] * m[2][1] * m[3][2] +
        m[0][1] * m[1][0] * m[2][3] * m[3][2]-m[0][0] * m[1][1] * m[2][3] * m[3][2] -
        m[0][2] * m[1][1] * m[2][0] * m[3][3]+m[0][1] * m[1][2] * m[2][0] * m[3][3] +
        m[0][2] * m[1][0] * m[2][1] * m[3][3]-m[0][0] * m[1][2] * m[2][1] * m[3][3] -
        m[0][1] * m[1][0] * m[2][2] * m[3][3]+m[0][0] * m[1][1] * m[2][2] * m[3][3];
   return det;
}

static void
invert_4x4 (float m[4][4], float out[4][4])
{
  float scale = 1 / determinant_4x4 (m);

  out[0][0] = (m[1][2] * m[2][3] * m[3][1] - m[1][3] * m[2][2] * m[3][1] +
               m[1][3] * m[2][1] * m[3][2] - m[1][1] * m[2][3] * m[3][2] -
               m[1][2] * m[2][1] * m[3][3] + m[1][1] * m[2][2] * m[3][3]) * scale;
  out[0][1] = (m[0][3] * m[2][2] * m[3][1] - m[0][2] * m[2][3] * m[3][1] -
               m[0][3] * m[2][1] * m[3][2] + m[0][1] * m[2][3] * m[3][2] +
               m[0][2] * m[2][1] * m[3][3] - m[0][1] * m[2][2] * m[3][3]) * scale;
  out[0][2] = (m[0][2] * m[1][3] * m[3][1] - m[0][3] * m[1][2] * m[3][1] +
               m[0][3] * m[1][1] * m[3][2] - m[0][1] * m[1][3] * m[3][2] -
               m[0][2] * m[1][1] * m[3][3] + m[0][1] * m[1][2] * m[3][3]) * scale;
  out[0][3] = (m[0][3] * m[1][2] * m[2][1] - m[0][2] * m[1][3] * m[2][1] -
               m[0][3] * m[1][1] * m[2][2] + m[0][1] * m[1][3] * m[2][2] +
               m[0][2] * m[1][1] * m[2][3] - m[0][1] * m[1][2] * m[2][3]) * scale;
  out[1][0] = (m[1][3] * m[2][2] * m[3][0] - m[1][2] * m[2][3] * m[3][0] -
               m[1][3] * m[2][0] * m[3][2] + m[1][0] * m[2][3] * m[3][2] +
               m[1][2] * m[2][0] * m[3][3] - m[1][0] * m[2][2] * m[3][3]) * scale;
  out[1][1] = (m[0][2] * m[2][3] * m[3][0] - m[0][3] * m[2][2] * m[3][0] +
               m[0][3] * m[2][0] * m[3][2] - m[0][0] * m[2][3] * m[3][2] -
               m[0][2] * m[2][0] * m[3][3] + m[0][0] * m[2][2] * m[3][3]) * scale;
  out[1][2] = (m[0][3] * m[1][2] * m[3][0] - m[0][2] * m[1][3] * m[3][0] -
               m[0][3] * m[1][0] * m[3][2] + m[0][0] * m[1][3] * m[3][2] +
               m[0][2] * m[1][0] * m[3][3] - m[0][0] * m[1][2] * m[3][3]) * scale;
  out[1][3] = (m[0][2] * m[1][3] * m[2][0] - m[0][3] * m[1][2] * m[2][0] +
               m[0][3] * m[1][0] * m[2][2] - m[0][0] * m[1][3] * m[2][2] -
               m[0][2] * m[1][0] * m[2][3] + m[0][0] * m[1][2] * m[2][3]) * scale;
  out[2][0] = (m[1][1] * m[2][3] * m[3][0] - m[1][3] * m[2][1] * m[3][0] +
               m[1][3] * m[2][0] * m[3][1] - m[1][0] * m[2][3] * m[3][1] -
               m[1][1] * m[2][0] * m[3][3] + m[1][0] * m[2][1] * m[3][3]) * scale;
  out[2][1] = (m[0][3] * m[2][1] * m[3][0] - m[0][1] * m[2][3] * m[3][0] -
               m[0][3] * m[2][0] * m[3][1] + m[0][0] * m[2][3] * m[3][1] +
               m[0][1] * m[2][0] * m[3][3] - m[0][0] * m[2][1] * m[3][3]) * scale;
  out[2][2] = (m[0][1] * m[1][3] * m[3][0] - m[0][3] * m[1][1] * m[3][0] +
               m[0][3] * m[1][0] * m[3][1] - m[0][0] * m[1][3] * m[3][1] -
               m[0][1] * m[1][0] * m[3][3] + m[0][0] * m[1][1] * m[3][3]) * scale;
  out[2][3] = (m[0][3] * m[1][1] * m[2][0] - m[0][1] * m[1][3] * m[2][0] -
               m[0][3] * m[1][0] * m[2][1] + m[0][0] * m[1][3] * m[2][1] +
               m[0][1] * m[1][0] * m[2][3] - m[0][0] * m[1][1] * m[2][3]) * scale;
  out[3][0] = (m[1][2] * m[2][1] * m[3][0] - m[1][1] * m[2][2] * m[3][0] -
               m[1][2] * m[2][0] * m[3][1] + m[1][0] * m[2][2] * m[3][1] +
               m[1][1] * m[2][0] * m[3][2] - m[1][0] * m[2][1] * m[3][2]) * scale;
  out[3][1] = (m[0][1] * m[2][2] * m[3][0] - m[0][2] * m[2][1] * m[3][0] +
               m[0][2] * m[2][0] * m[3][1] - m[0][0] * m[2][2] * m[3][1] -
               m[0][1] * m[2][0] * m[3][2] + m[0][0] * m[2][1] * m[3][2]) * scale;
  out[3][2] = (m[0][2] * m[1][1] * m[3][0] - m[0][1] * m[1][2] * m[3][0] -
               m[0][2] * m[1][0] * m[3][1] + m[0][0] * m[1][2] * m[3][1] +
               m[0][1] * m[1][0] * m[3][2] - m[0][0] * m[1][1] * m[3][2]) * scale;
  out[3][3] = (m[0][1] * m[1][2] * m[2][0] - m[0][2] * m[1][1] * m[2][0] +
               m[0][2] * m[1][0] * m[2][1] - m[0][0] * m[1][2] * m[2][1] -
               m[0][1] * m[1][0] * m[2][2] + m[0][0] * m[1][1] * m[2][2]) * scale;
}
