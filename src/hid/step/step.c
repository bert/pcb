#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
//#include <stdarg.h> /* not used */
//#include <stdlib.h>
//#include <string.h>
#include <assert.h>
#include <time.h>

#include "global.h"
#include "data.h"
#include "misc.h"
//#include "error.h"
//#include "draw.h"
#include "pcb-printf.h"

#include "hid.h"
#include "hid_draw.h"
#include "../hidint.h"
#include "hid/common/hidnogui.h"
#include "hid/common/draw_helpers.h"
#include "step.h"
#include "hid/common/hidinit.h"
#include "polygon.h"
#include "misc.h"
#include "rtree.h"

#include "hid/common/step_id.h"
#include "hid/common/quad.h"
#include "hid/common/vertex3d.h"
#include "hid/common/contour3d.h"
#include "hid/common/appearance.h"
#include "hid/common/face3d.h"
#include "hid/common/edge3d.h"
#include "hid/common/object3d.h"
#include "object3d_step.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "model.h"
#include "assembly.h"

#define CRASH fprintf(stderr, "HID error: pcb called unimplemented STEP function %s.\n", __FUNCTION__); abort()

#define REVERSED_PCB_CONTOURS 1
//#undef REVERSED_PCB_CONTOURS

#ifdef REVERSED_PCB_CONTOURS
#define COORD_TO_STEP_X(pcb, x) (COORD_TO_MM(                   (x)))
#define COORD_TO_STEP_Y(pcb, y) (COORD_TO_MM((pcb)->MaxHeight - (y)))
#define COORD_TO_STEP_Z(pcb, z) (COORD_TO_MM(                   (z)))
#else
/* XXX: BROKEN UPSIDE DOWN OUTPUT */
#define COORD_TO_STEP_X(pcb, x) (COORD_TO_MM((x)))
#define COORD_TO_STEP_Y(pcb, y) (COORD_TO_MM((y)))
#define COORD_TO_STEP_Z(pcb, z) (COORD_TO_MM((z)))
#endif


static Coord board_thickness = 0;
#define HACK_BOARD_THICKNESS board_thickness
//#define HACK_BOARD_THICKNESS MM_TO_COORD(1.6)
#define HACK_COPPER_THICKNESS MM_TO_COORD(0.035)

HID step_hid;

HID_Attribute step_attribute_list[] = {
  /* other HIDs expect this to be first.  */

/* %start-doc options "91 STEP Export"
@ftable @code
@cindex stepfile
@item --stepfile <string>
Name of the STEP output file. Can contain a path.
@end ftable
%end-doc
*/
  {"stepfile", "STEP output file",
   HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_stepfile 0

  /* %start-doc options "91 STEP Export"
   @ftable @code
   @cindex copper
   @item --copper
   Export copper tracking
   @end ftable
   %end-doc
   */
    {"copper", N_("Export copper objects"),
         HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_copper 1

  /* %start-doc options "91 STEP Export"
   @ftable @code
   @cindex solder-mask
   @item --solder-mask
   Export solder mask
   @end ftable
   %end-doc
   */
    {"soldermask", N_("Export soldermask"),
         HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_soldermask 2

  /* %start-doc options "91 STEP Export"
   @ftable @code
   @cindex silk
   @item --silk
   Export silkscreen
   @end ftable
   %end-doc
   */
    {"silk", N_("Export silk"),
         HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_silk 3

  /* %start-doc options "91 STEP Export"
   @ftable @code
   @cindex models
   @item --models
   Export component models
   @end ftable
   %end-doc
   */
    {"models", N_("Export component models"),
         HID_Boolean, 0, 0, {1, 0, 0}, 0, 0},
#define HA_models 4

  /* %start-doc options "91 STEP Export"
   @ftable @code
   @cindex thickness
   @item --thickness
   Board thickness
   @end ftable
   %end-doc
   */
    {"thickness", N_("Board thickness"),
         HID_Coord, 0, MM_TO_COORD(100), {0, 0, 0, MM_TO_COORD (1.6)}, 0, 0}, /* XXX: Arbitrary limit of 100mm thick PCB */
#define HA_thickness 5
};

#define NUM_OPTIONS (sizeof(step_attribute_list)/sizeof(step_attribute_list[0]))

REGISTER_ATTRIBUTES (step_attribute_list)

static HID_Attr_Val step_option_values[NUM_OPTIONS];

static HID_Attribute *
step_get_export_options (int *n)
{
  static char *last_made_filename = 0;
  if (PCB)
    derive_default_filename(PCB->Filename, &step_attribute_list[HA_stepfile], ".step", &last_made_filename);

  if (n)
    *n = NUM_OPTIONS;
  return step_attribute_list;
}

/* NB: Result is in mm */
static void
parse_cartesian_point_3d_string (const char *str, double *x, double *y, double *z)
{
  *x = 0.0, *y = 0.0, *z = 0.0;
}

/* NB: Result is in mm */
static void
parse_direction_3d_string (const char *str, double *x, double *y, double *z)
{
  *x = 0.0, *y = 0.0, *z = 0.0;
}

/* NB: Result is in degrees */
static void
parse_rotation_string (const char *str, double *rotation)
{
  *rotation = 0.0;
}

static void
parse_position_attribute (ElementType *element, char *attr_name, double *res)
{
  const char *attr_value = AttributeGet (element, attr_name);
  bool absolute;

  *res = 0.0;
  if (attr_value == NULL)
    return;

  *res = COORD_TO_MM (GetValueEx (attr_value, NULL, &absolute, NULL, "cmil"));
}

static void
parse_numeric_attribute (ElementType *element, char *attr_name, double *res)
{
  const char *attr_value = AttributeGet (element, attr_name);
  bool absolute;

  *res = 0.0;
  if (attr_value == NULL)
    return;

  *res = COORD_TO_MM (GetValueEx (attr_value, NULL, &absolute, NULL, "mm")); /* KLUDGE */
}

static GList *loaded_models = NULL;

void
step_load_models(Coord board_thickness_)
{
  int i;
  const char *filename;
  struct assembly_model *model;
  struct assembly_model_instance *instance;
  const char *attribute;

  board_thickness = board_thickness_;

  ELEMENT_LOOP (PCB->Data);
    {
      bool on_solder = TEST_FLAG (ONSOLDERFLAG, element);
      double on_solder_negate = on_solder ? -1.0 : 1.0;
      const char *model_filename;
      double ox, oy, oz;
      double ax, ay, az;
      double rx, ry, rz;
      double rotation;
      double cos_rot;
      double sin_rot;
      GList *model_iter;

      /* Skip if the component doesn't have a STEP-AP214 3d_model */
      attribute = AttributeGet (element, "PCB::3d_model::type");
      if (attribute == NULL || strcmp (attribute, "STEP-AP214") != 0)
        continue;

      attribute = AttributeGet (element, "PCB::3d_model::filename");
      if (attribute == NULL)
        continue;
      model_filename = attribute;

#if 0   /* Rather than write a parser for three floats in a string, separate X, Y, Z explicitly for quicker testing */

      attribute = AttributeGet (element, "PCB::3d_model::origin");
      if (attribute == NULL)
        continue;
      parse_cartesian_point_3d_string (attribute, &ox, &oy, &oz);

      attribute = AttributeGet (element, "PCB::3d_model::axis");
      if (attribute == NULL)
        continue;
      parse_direction_3d_string (attribute, &ax, &ay, &az);
      ax = 0.0, ay = 0.0, az = 1.0;

      attribute = AttributeGet (element, "PCB::3d_model::ref_dir");
      if (attribute == NULL)
        continue;
      parse_direction_3d_string (attribute, &rx, &ry, &rz);
      rx = 1.0, ry = 0.0, rz = 0.0;
#endif

      /* XXX: Should parse a unit suffix, e.g. "degrees" */
      attribute = AttributeGet (element, "PCB::rotation");
      if (attribute == NULL)
        continue;
      parse_rotation_string (attribute, &rotation);

      /* XXX: QUICKER TO CODE INDIVIDULAL VALUES NOT SPACE SEPARATED */
      parse_position_attribute (element, "PCB::3d_model::origin::X", &ox);
      parse_position_attribute (element, "PCB::3d_model::origin::Y", &oy);
      parse_position_attribute (element, "PCB::3d_model::origin::Z", &oz);
      parse_numeric_attribute (element, "PCB::3d_model::axis::X", &ax);
      parse_numeric_attribute (element, "PCB::3d_model::axis::Y", &ay);
      parse_numeric_attribute (element, "PCB::3d_model::axis::Z", &az);
      parse_numeric_attribute (element, "PCB::3d_model::ref_dir::X", &rx);
      parse_numeric_attribute (element, "PCB::3d_model::ref_dir::Y", &ry);
      parse_numeric_attribute (element, "PCB::3d_model::ref_dir::Z", &rz);
      parse_numeric_attribute (element, "PCB::rotation", &rotation);

#if 1  /* Write the intended final syntax attributes */
      if (1)
        {
          GString *value = g_string_new (NULL);

          attribute = AttributeGet (element, "PCB::3d_model::origin::X");
          g_string_printf (value, "%s", attribute != NULL ? attribute : "0.0mm");
          attribute = AttributeGet (element, "PCB::3d_model::origin::Y");
          g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0mm");
          attribute = AttributeGet (element, "PCB::3d_model::origin::Z");
          g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0mm");
          AttributePutToList (&element->Attributes, "PCB::3d_model::origin", value->str, true);

          attribute = AttributeGet (element, "PCB::3d_model::axis::X");
          g_string_printf (value, "%s", attribute != NULL ? attribute : "0.0");
          attribute = AttributeGet (element, "PCB::3d_model::axis::Y");
          g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
          attribute = AttributeGet (element, "PCB::3d_model::axis::Z");
          g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
          AttributePutToList (&element->Attributes, "PCB::3d_model::axis", value->str, true);

          attribute = AttributeGet (element, "PCB::3d_model::ref_dir::X");
          g_string_printf (value, "%s", attribute != NULL ? attribute : "0.0");
          attribute = AttributeGet (element, "PCB::3d_model::ref_dir::Y");
          g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
          attribute = AttributeGet (element, "PCB::3d_model::ref_dir::Z");
          g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
          AttributePutToList (&element->Attributes, "PCB::3d_model::ref_dir", value->str, true);

          g_string_free (value, true);
        }
#endif

      instance = element->assembly_model_instance;

      if (instance != NULL)
        {
          if (strcmp (instance->model->filename, model_filename) == 0)
            {
              /* Same model */
            }
          else
            {
              /* Different model, delete the old instance */

              struct assembly_model *old_model;

              old_model = instance->model;
              old_model->instances = g_list_remove (old_model->instances, instance);
              g_free (instance);
              instance = NULL;

              /* Keep things clean, don't leave this pointers dangling - even temporarily */
              element->assembly_model_instance = NULL;

              if (old_model->instances == NULL)
                {
                  /* Assume unused, so unload */
                  if (old_model->step_model != NULL)
                    step_model_free (old_model->step_model);
                  g_free (old_model);
                  loaded_models = g_list_remove (loaded_models, old_model);
                }
            }
        }

      if (instance == NULL)
        {
          /* No model loaded yet */
          model = NULL;

          /* Look for prior usage of this model */
          for (model_iter = loaded_models;
               model_iter != NULL;
               model_iter = g_list_next (model_iter))
            {
              struct assembly_model *possible_model;
              possible_model = model_iter->data;
              if (strcmp (possible_model->filename, model_filename) == 0)
                {
                  model = possible_model;
                  break;
                }
            }

          /* If we didn't find this model used already, add it to the list */
          if (model == NULL)
            {
              model = g_new0 (struct assembly_model, 1);
              model->filename = model_filename;
              model->step_model = step_model_to_shape_master (model_filename);
              loaded_models = g_list_append (loaded_models, model);
            }
          instance = g_new0 (struct assembly_model_instance, 1);
          instance->model = model;
          model->instances = g_list_append (model->instances, instance);
          element->assembly_model_instance = instance;
        }

      cos_rot = cos (rotation * M_PI / 180.);
      sin_rot = sin (rotation * M_PI / 180.);

      // Rotation of part on board
      // (NB: Y flipped from normal right handed convention)
      //[cos -sin   0] [x] = [xcos - ysin]
      //[sin  cos   0] [y]   [xsin + ycos]
      //[  0    0   1] [z]   [z          ]

      // Flip of part to backside of board
      // [  1   0   0] [x] = [ x]
      // [  0  -1   0] [y] = [-y]
      // [  0   0  -1] [z] = [-z]


      instance->name = NAMEONPCB_NAME (element);
#ifdef REVERSED_PCB_CONTOURS
      instance->ox =                     ( ox * cos_rot + oy * sin_rot);
      instance->oy = -on_solder_negate * (-ox * sin_rot + oy * cos_rot);
      instance->oz = -on_solder_negate * oz; /* <--- ????: -ve on on_solder_negative seems inconsistent w.r.t. others! */
      instance->ax =                     ( ax * cos_rot + ay * sin_rot);
      instance->ay = -on_solder_negate * (-ax * sin_rot + ay * cos_rot);
      instance->az = -on_solder_negate * az;
      instance->rx =                     ( rx * cos_rot + ry * sin_rot);
      instance->ry = -on_solder_negate * (-rx * sin_rot + ry * cos_rot);
      instance->rz = -on_solder_negate * rz;
#else
      instance->ox =                     ( ox * cos_rot + oy * sin_rot);
      instance->oy =  on_solder_negate * (-ox * sin_rot + oy * cos_rot);
      instance->oz =  on_solder_negate * oz;
      instance->ax =                     ( ax * cos_rot + ay * sin_rot);
      instance->ay =  on_solder_negate * (-ax * sin_rot + ay * cos_rot);
      instance->az =  on_solder_negate * az;
      instance->rx =                     ( rx * cos_rot + ry * sin_rot);
      instance->ry =  on_solder_negate * (-rx * sin_rot + ry * cos_rot);
      instance->rz =  on_solder_negate * rz;
#endif

      instance->ox += COORD_TO_STEP_X (PCB, element->MarkX);
      instance->oy += COORD_TO_STEP_Y (PCB, element->MarkY);
#ifdef REVERSED_PCB_CONTOURS
      instance->oz += COORD_TO_STEP_Z (PCB, on_solder ? -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS : HACK_COPPER_THICKNESS);
#else
      instance->oz += COORD_TO_STEP_Z (PCB, on_solder_negate * (-HACK_BOARD_THICKNESS / 2 -HACK_COPPER_THICKNESS));
#endif

    }
  END_LOOP;
}

static void
step_do_export (HID_Attr_Val * options)
{
  int i;
  const char *filename;
  const char *temp_pcb_filename = "_pcb.step";
  GList *board_outline_list;
  POLYAREA *board_outline;
  POLYAREA *piece;

  if (!options)
    {
      step_get_export_options (0);
      for (i = 0; i < NUM_OPTIONS; i++)
        step_option_values[i] = step_attribute_list[i].default_val;
      options = step_option_values;
    }

  board_thickness = options[HA_thickness].coord_value;
  object3d_set_board_thickness (board_thickness);

  filename = options[HA_stepfile].str_value;
  if (filename == NULL)
    filename = "pcb-out.step";

  board_outline_list = object3d_from_board_outline ();

  board_outline = board_outline_poly (true);
  piece = board_outline;
  do {
    GList *silk_objects;
    GList *mask_objects;
    GList *copper_layer_objects;
    PLINE *curc;
    PLINE *next;
    PLINE **prev_next;

    // Remove any complete internal contours due to vias, so we may
    // more realistically show tented vias without loosing the ability
    // to split the soldermask body with a contour partly formed of vias.
    //
    // Should have the semantics that via holes in the mask are only due
    // to the clearance size, not the drill size - IFF they are on the
    // interior of a board body piece.
    //
    // If the via wall forms part of the board piece outside contour, the
    // soldermask will be the maximum of the drilling hole, or the clearance;
    // via drill-hole walls are not removed from the piece outside contour.

    prev_next = &piece->contours;
    for (curc = piece->contours; curc != NULL; curc = next)
      {
        next = curc->next;

        /* XXX: Insufficient test for via contour.. really need to KNOW this was a pin/via,
         *      as we may start using round tagged contours for circular cutouts etc...
         */
        if (!curc->is_round)
          {
            prev_next = &curc->next;
            continue;
          }

        /* Remove contour... */
        assert (*prev_next == curc);
        *prev_next = curc->next;
        curc->next = NULL;

        r_delete_entry (piece->contour_tree, (BoxType *) curc);
        poly_DelContour (&curc);
      }

#if 1
    if (options[HA_silk].int_value)
      {
        silk_objects = object3d_from_silk_within_area (piece, TOP_SIDE);
        board_outline_list = g_list_concat (board_outline_list, silk_objects);

        silk_objects = object3d_from_silk_within_area (piece, BOTTOM_SIDE);
        board_outline_list = g_list_concat (board_outline_list, silk_objects);
      }
#endif

#if 1
    if (options[HA_soldermask].int_value)
      {
        mask_objects = object3d_from_soldermask_within_area (piece, TOP_SIDE);
        board_outline_list = g_list_concat (board_outline_list, mask_objects);

        mask_objects = object3d_from_soldermask_within_area (piece, BOTTOM_SIDE);
        board_outline_list = g_list_concat (board_outline_list, mask_objects);
      }
#endif

#if 1
    if (options[HA_copper].int_value)
      {
        copper_layer_objects = object3d_from_copper_layers_within_area (piece);
        board_outline_list = g_list_concat (board_outline_list, copper_layer_objects);
      }
#endif

  } while ((piece = piece->f) != board_outline);
  poly_Free (&board_outline);

//  object3d_list_export_to_step_part (board_outline_list, temp_pcb_filename);
  object3d_list_export_to_step_assy (board_outline_list, temp_pcb_filename);
  g_list_free_full (board_outline_list, (GDestroyNotify)destroy_object3d);

  if (1) {
    GList *models = NULL;
    struct assembly_model *model;
    struct assembly_model_instance *instance;
    const char *attribute;

    model = g_new0 (struct assembly_model, 1);
    model->filename = temp_pcb_filename;
    models = g_list_append (models, model);

    instance = g_new0 (struct assembly_model_instance, 1);
    instance->model = model;
    instance->name = "PCB";
    instance->ox = 0.0,  instance->oy = 0.0,  instance->oz = 0.0;
    instance->ax = 0.0,  instance->ay = 0.0,  instance->az = 1.0;
    instance->rx = 1.0,  instance->ry = 0.0,  instance->rz = 0.0;
    model->instances = g_list_append (model->instances, instance);

    if (options[HA_models].int_value)
      {

        ELEMENT_LOOP (PCB->Data);
          {
            bool on_solder = TEST_FLAG (ONSOLDERFLAG, element);
            double on_solder_negate = on_solder ? -1.0 : 1.0;
            const char *model_filename;
            double ox, oy, oz;
            double ax, ay, az;
            double rx, ry, rz;
            double rotation;
            double cos_rot;
            double sin_rot;
            GList *model_iter;

            /* Skip if the component doesn't have a STEP-AP214 3d_model */
            attribute = AttributeGet (element, "PCB::3d_model::type");
            if (attribute == NULL || strcmp (attribute, "STEP-AP214") != 0)
              continue;

            attribute = AttributeGet (element, "PCB::3d_model::filename");
            if (attribute == NULL)
              continue;
            model_filename = attribute;

#if 0   /* Rather than write a parser for three floats in a string, separate X, Y, Z explicitly for quicker testing */

            attribute = AttributeGet (element, "PCB::3d_model::origin");
            if (attribute == NULL)
              continue;
            parse_cartesian_point_3d_string (attribute, &ox, &oy, &oz);

            attribute = AttributeGet (element, "PCB::3d_model::axis");
            if (attribute == NULL)
              continue;
            parse_direction_3d_string (attribute, &ax, &ay, &az);
            ax = 0.0, ay = 0.0, az = 1.0;

            attribute = AttributeGet (element, "PCB::3d_model::ref_dir");
            if (attribute == NULL)
              continue;
            parse_direction_3d_string (attribute, &rx, &ry, &rz);
            rx = 1.0, ry = 0.0, rz = 0.0;
#endif

            /* XXX: Should parse a unit suffix, e.g. "degrees" */
            attribute = AttributeGet (element, "PCB::rotation");
            if (attribute == NULL)
              continue;
            parse_rotation_string (attribute, &rotation);

            /* XXX: QUICKER TO CODE INDIVIDULAL VALUES NOT SPACE SEPARATED */
            parse_position_attribute (element, "PCB::3d_model::origin::X", &ox);
            parse_position_attribute (element, "PCB::3d_model::origin::Y", &oy);
            parse_position_attribute (element, "PCB::3d_model::origin::Z", &oz);
            parse_numeric_attribute (element, "PCB::3d_model::axis::X", &ax);
            parse_numeric_attribute (element, "PCB::3d_model::axis::Y", &ay);
            parse_numeric_attribute (element, "PCB::3d_model::axis::Z", &az);
            parse_numeric_attribute (element, "PCB::3d_model::ref_dir::X", &rx);
            parse_numeric_attribute (element, "PCB::3d_model::ref_dir::Y", &ry);
            parse_numeric_attribute (element, "PCB::3d_model::ref_dir::Z", &rz);
            parse_numeric_attribute (element, "PCB::rotation", &rotation);

#if 1  /* Write the intended final syntax attributes */
            if (1)
              {
                GString *value = g_string_new (NULL);

                attribute = AttributeGet (element, "PCB::3d_model::origin::X");
                g_string_printf (value, "%s", attribute != NULL ? attribute : "0.0mm");
                attribute = AttributeGet (element, "PCB::3d_model::origin::Y");
                g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0mm");
                attribute = AttributeGet (element, "PCB::3d_model::origin::Z");
                g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0mm");
                AttributePutToList (&element->Attributes, "PCB::3d_model::origin", value->str, true);

                attribute = AttributeGet (element, "PCB::3d_model::axis::X");
                g_string_printf (value, "%s", attribute != NULL ? attribute : "0.0");
                attribute = AttributeGet (element, "PCB::3d_model::axis::Y");
                g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
                attribute = AttributeGet (element, "PCB::3d_model::axis::Z");
                g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
                AttributePutToList (&element->Attributes, "PCB::3d_model::axis", value->str, true);

                attribute = AttributeGet (element, "PCB::3d_model::ref_dir::X");
                g_string_printf (value, "%s", attribute != NULL ? attribute : "0.0");
                attribute = AttributeGet (element, "PCB::3d_model::ref_dir::Y");
                g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
                attribute = AttributeGet (element, "PCB::3d_model::ref_dir::Z");
                g_string_append_printf (value, " %s", attribute != NULL ? attribute : "0.0");
                AttributePutToList (&element->Attributes, "PCB::3d_model::ref_dir", value->str, true);

                g_string_free (value, true);
              }
#endif

            printf ("Transform (%f, %f, %f), (%f, %f, %f), (%f, %f, %f). Rotation of part is %f\n", ox, oy, oz, ax, ay, az, rx, ry, rz, rotation);

            model = NULL;

            /* Look for prior usage of this model */
            for (model_iter = models;
                 model_iter != NULL;
                 model_iter = g_list_next (model_iter))
              {
                struct assembly_model *possible_model;
                possible_model = model_iter->data;
                if (strcmp (possible_model->filename, model_filename) == 0)
                  {
                    model = possible_model;
                    break;
                  }
              }

            /* If we didn't find this model used already, add it to the list */
            if (model == NULL)
              {
                model = g_new0 (struct assembly_model, 1);
                model->filename = model_filename;
                model->step_model = NULL; /* We don't use this for assembly export */
                models = g_list_append (models, model);
              }

            cos_rot = cos (rotation * M_PI / 180.);
            sin_rot = sin (rotation * M_PI / 180.);

            // Rotation of part on board
            // (NB: Y flipped from normal right handed convention)
            //[cos -sin   0] [x] = [xcos - ysin]
            //[sin  cos   0] [y]   [xsin + ycos]
            //[  0    0   1] [z]   [z          ]

            // Flip of part to backside of board
            // [  1   0   0] [x] = [ x]
            // [  0  -1   0] [y] = [-y]
            // [  0   0  -1] [z] = [-z]

            instance = g_new0 (struct assembly_model_instance, 1);
            instance->model = model;
            instance->name = NAMEONPCB_NAME (element);
#ifdef REVERSED_PCB_CONTOURS
            instance->ox =                     ( ox * cos_rot + oy * sin_rot);
            instance->oy = -on_solder_negate * (-ox * sin_rot + oy * cos_rot);
            instance->oz = -on_solder_negate * oz; /* <--- ????: -ve on on_solder_negative seems inconsistent w.r.t. others! */
            instance->ax =                     ( ax * cos_rot + ay * sin_rot);
            instance->ay = -on_solder_negate * (-ax * sin_rot + ay * cos_rot);
            instance->az = -on_solder_negate * az;
            instance->rx =                     ( rx * cos_rot + ry * sin_rot);
            instance->ry = -on_solder_negate * (-rx * sin_rot + ry * cos_rot);
            instance->rz = -on_solder_negate * rz;
#else
            instance->ox =                     ( ox * cos_rot + oy * sin_rot);
            instance->oy =  on_solder_negate * (-ox * sin_rot + oy * cos_rot);
            instance->oz =  on_solder_negate * oz;
            instance->ax =                     ( ax * cos_rot + ay * sin_rot);
            instance->ay =  on_solder_negate * (-ax * sin_rot + ay * cos_rot);
            instance->az =  on_solder_negate * az;
            instance->rx =                     ( rx * cos_rot + ry * sin_rot);
            instance->ry =  on_solder_negate * (-rx * sin_rot + ry * cos_rot);
            instance->rz =  on_solder_negate * rz;
#endif

            instance->ox += COORD_TO_STEP_X (PCB, element->MarkX);
            instance->oy += COORD_TO_STEP_Y (PCB, element->MarkY);
#ifdef REVERSED_PCB_CONTOURS
            instance->oz += COORD_TO_STEP_Z (PCB, on_solder ? -HACK_BOARD_THICKNESS - HACK_COPPER_THICKNESS : HACK_COPPER_THICKNESS);
#else
            instance->oz += COORD_TO_STEP_Z (PCB, on_solder_negate * (-HACK_BOARD_THICKNESS / 2 -HACK_COPPER_THICKNESS));
#endif

            model->instances = g_list_append (model->instances, instance);

          }
        END_LOOP;
      }

    export_step_assembly (filename, models);

    /* XXX: LEAK ALL THE MODEL DATA.. BEING LAZY RIGHT NOW */
  }
}

static void
step_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (step_attribute_list, NUM_OPTIONS);
  hid_parse_command_line (argc, argv);
}

#include "dolists.h"

void step_step_init (HID *hid)
{
  hid->get_export_options = step_get_export_options;
  hid->do_export          = step_do_export;
  hid->parse_arguments    = step_parse_arguments;
}

void
hid_step_init ()
{
  memset (&step_hid, 0, sizeof (HID));

  common_nogui_init (&step_hid);
  step_step_init (&step_hid);

  step_hid.struct_size        = sizeof (HID);
  step_hid.name               = "step";
  step_hid.description        = "STEP AP214 export";
  step_hid.exporter           = 1;

  hid_register_hid (&step_hid);

#include "step_lists.h"
}
