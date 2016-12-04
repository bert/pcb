#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "step_writer.h"

static char *
step_bool (bool expr)
{
  return expr ? ".T." : ".F.";
}

static void
fprint_id_list (FILE *f, step_id_list list)
{
  GList *iter;

  fprintf (f, "(");
  for (iter = list;
       iter != NULL && g_list_next (iter) != NULL;
       iter = g_list_next (iter))
    {
      fprintf (f, "#%i,", GPOINTER_TO_INT (iter->data));
    }
  if (iter == NULL)
    fprintf (f, ")");
  else
    fprintf (f, "#%i)", GPOINTER_TO_INT (iter->data));
}

static void
destroy_step_id_list (step_id_list list)
{
  g_list_free (list);
}

step_id_list
step_id_list_append (step_id_list list, step_id id)
{
  return g_list_append (list, GINT_TO_POINTER (id));
}

/* NB: The GList this produces will leak if not passed back to one of the step_* functions which uses the list and destroys it */
step_id_list
make_step_id_list (int count, ...)
{
  step_id_list list = NULL;
  va_list args;
  int i;

  va_start (args, NULL);

  for (i = 0; i < count; i++)
    {
      step_id id = va_arg (args, step_id);
      list = g_list_append (list, GINT_TO_POINTER (id));
    }

  va_end (args);

  return list;
}

step_file
*step_output_file (FILE *f)
{
  step_file *file;

  file = g_new0 (step_file, 1);
  file->f = f;
  file->next_id = 1;

  return file;
}

void
destroy_step_output_file (step_file *file)
{
  g_free (file);
}

/* XXX: Just reading out boiler-plate at this point */
step_id
make_3d_metric_step_geometric_representation_context (step_file *file)
{
  fprintf (file->f, "#%i=(LENGTH_UNIT()NAMED_UNIT(*)SI_UNIT(.MILLI.,.METRE.));\n", file->next_id);
  fprintf (file->f, "#%i=(NAMED_UNIT(*)PLANE_ANGLE_UNIT()SI_UNIT($,.RADIAN.));\n", file->next_id + 1);
  fprintf (file->f, "#%i=(NAMED_UNIT(*)SI_UNIT($,.STERADIAN.)SOLID_ANGLE_UNIT());\n", file->next_id + 2);
  fprintf (file->f, "#%i=UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.0E-005),#%i,'distance_accuracy_value', 'NONE');\n", file->next_id + 3, file->next_id);

  fprintf (file->f, "#%i=(GEOMETRIC_REPRESENTATION_CONTEXT(3)"
                          "GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#%i))"
                          "GLOBAL_UNIT_ASSIGNED_CONTEXT((#%i,#%i,#%i))"
                          "REPRESENTATION_CONTEXT('NONE','WORKASPACE'));\n",
                    file->next_id + 4, file->next_id + 3, file->next_id, file->next_id + 1, file->next_id + 2);
  file->next_id += 4;

  return file->next_id++;
}

step_id
step_application_context (step_file *file, char *application)
{
  fprintf (file->f, "#%i=APPLICATION_CONTEXT('%s');\n", file->next_id, application);

  return file->next_id++;
}

step_id
step_application_protocol_definition (step_file *file, char *status, char *application_interpreted_model_schema_name,
                                      char *application_protocol_year, step_id application)
{
  fprintf (file->f, "#%i=APPLICATION_PROTOCOL_DEFINITION('%s','%s',%s,#%i);\n",
                    file->next_id, status, application_interpreted_model_schema_name, application_protocol_year, application);

  return file->next_id++;
}

step_id
step_product_context (step_file *file, char *name, step_id frame_of_reference, char *discipline_type)
{
  fprintf (file->f, "#%i=PRODUCT_CONTEXT('%s',#%i,'%s');\n",
                    file->next_id, name, frame_of_reference, discipline_type);

  return file->next_id++;
}

step_id
step_product (step_file *file, char *id, char *name, char *description, step_id_list frame_of_reference)
{
  fprintf (file->f, "#%i=PRODUCT('%s','%s','%s',", file->next_id, id, name, description);
  fprint_id_list (file->f, frame_of_reference);
  fprintf (file->f, ");\n");
  destroy_step_id_list (frame_of_reference);

  return file->next_id++;
}

step_id
step_product_related_product_category (step_file *file, char *name, char *description, step_id_list products)
{
  if (description != NULL)
    fprintf (file->f, "#%i=PRODUCT_RELATED_PRODUCT_CATEGORY('%s','%s',", file->next_id, name, description);
  else
    fprintf (file->f, "#%i=PRODUCT_RELATED_PRODUCT_CATEGORY('%s',$,", file->next_id, name);

  fprint_id_list (file->f, products);
  fprintf (file->f, ");\n");
  destroy_step_id_list (products);

  return file->next_id++;
}

step_id
step_product_definition_context (step_file *file, char *name, step_id frame_of_reference, char *life_cycle_stage)
{
  fprintf (file->f, "#%i=PRODUCT_DEFINITION_CONTEXT('%s',#%i,'%s');\n",
                    file->next_id, name, frame_of_reference, life_cycle_stage);

  return file->next_id++;
}

step_id
step_product_definition_formation (step_file *file, char *id, char *description, step_id of_product)
{
  fprintf (file->f, "#%i=PRODUCT_DEFINITION_FORMATION('%s','%s',#%i);\n",
                    file->next_id, id, description, of_product);

  return file->next_id++;
}

step_id
step_product_definition (step_file *file, char *id, char *description, step_id formation, step_id frame_of_reference)
{
  fprintf (file->f, "#%i=PRODUCT_DEFINITION('%s','%s',#%i,#%i);\n",
                    file->next_id, id, description, formation, frame_of_reference);

  return file->next_id++;
}

step_id
step_product_definition_shape (step_file *file, char *name, char *description, step_id definition)
{
  fprintf (file->f, "#%i=PRODUCT_DEFINITION_SHAPE('%s','%s',#%i);\n",
                    file->next_id, name, description, definition);

  return file->next_id++;
}

step_id
step_cartesian_point (step_file *file, char *name, double x, double y, double z)
{
  fprintf (file->f, "#%i=CARTESIAN_POINT('%s',(%f,%f,%f));\n",
                    file->next_id, name, x, y, z);
  return file->next_id++;
}

step_id
step_direction (step_file *file, char *name, double x, double y, double z)
{
  fprintf (file->f, "#%i=DIRECTION('%s',(%f,%f,%f));\n",
                    file->next_id, name, x, y, z);
  return file->next_id++;
}

step_id
step_axis2_placement_3d (step_file *file, char *name, step_id location, step_id axis, step_id ref_direction)
{
  fprintf (file->f, "#%i=AXIS2_PLACEMENT_3D('%s',#%i,#%i,#%i);\n",
                    file->next_id, name, location, axis, ref_direction);
  return file->next_id++;
}

step_id
step_plane (step_file *file, char *name, step_id position)
{
  fprintf (file->f, "#%i=PLANE('%s',#%i);\n",
                    file->next_id, name, position);
  return file->next_id++;
}

step_id
step_cylindrical_surface (step_file *file, char *name, step_id position, double radius)
{
  fprintf (file->f, "#%i=CYLINDRICAL_SURFACE('%s',#%i,%f);\n",
                    file->next_id, name, position, radius);
  return file->next_id++;
}

step_id
step_circle (step_file *file, char *name, step_id position, double radius)
{
  fprintf (file->f, "#%i=CIRCLE('%s',#%i,%f);\n",
                    file->next_id, name, position, radius);
  return file->next_id++;
}

step_id
step_vector (step_file *file, char *name, step_id orientation, double magnitude)
{
  fprintf (file->f, "#%i=VECTOR('%s',#%i,%f);\n",
                    file->next_id, name, orientation, magnitude);
  return file->next_id++;
}

step_id
step_line (step_file *file, char *name, step_id pnt, step_id dir)
{
  fprintf (file->f, "#%i=LINE('%s',#%i,#%i);\n",
                    file->next_id, name, pnt, dir);
  return file->next_id++;
}

step_id
step_vertex_point (step_file *file, char *name, step_id pnt)
{
  fprintf (file->f, "#%i=VERTEX_POINT('%s',#%i);\n",
                    file->next_id, name, pnt);
  return file->next_id++;
}

step_id
step_edge_curve (step_file *file, char *name, step_id edge_start, step_id edge_end, step_id edge_geometry, bool same_sense)
{
  fprintf (file->f, "\n#%i=EDGE_CURVE('%s',#%i,#%i,#%i,%s);",
                    file->next_id, name, edge_start, edge_end, edge_geometry, step_bool (same_sense));
  return file->next_id++;
}

step_id
step_oriented_edge (step_file *file, char *name, step_id edge_element, bool orientation)
{
  fprintf (file->f, "#%i=ORIENTED_EDGE('%s',*,*,#%i,%s);",
                    file->next_id, name, edge_element, step_bool (orientation));
  return file->next_id++;
}

step_id
step_edge_loop (step_file *file, char *name, step_id_list edge_list)
{
  fprintf (file->f, "#%i=EDGE_LOOP('%s',", file->next_id, name);
  fprint_id_list (file->f, edge_list);
  fprintf (file->f, ");");
  destroy_step_id_list (edge_list);

  return file->next_id++;
}

step_id
step_face_bound (step_file *file, char *name, step_id bound, bool orientation)
{
  fprintf (file->f, "#%i=FACE_BOUND('%s',#%i,%s);",
                    file->next_id, name, bound, step_bool (orientation));

  return file->next_id++;
}

step_id
step_face_outer_bound (step_file *file, char *name, step_id bound, bool orientation)
{
  fprintf (file->f, "#%i=FACE_OUTER_BOUND('%s',#%i,%s);",
                    file->next_id, name, bound, step_bool (orientation));

  return file->next_id++;
}

step_id
step_advanced_face (step_file *file, char *name, step_id_list bounds, step_id face_geometry, bool same_sense)
{
  fprintf (file->f, "#%i=ADVANCED_FACE('%s',", file->next_id, name);
  fprint_id_list (file->f, bounds);
  fprintf (file->f, ",#%i,%s);\n", face_geometry, step_bool (same_sense));
  destroy_step_id_list (bounds);

  return file->next_id++;
}

step_id
step_closed_shell (step_file *file, char *name, step_id_list cfs_faces)
{
  fprintf (file->f, "#%i=CLOSED_SHELL('%s',", file->next_id, name);
  fprint_id_list (file->f, cfs_faces);
  fprintf (file->f, ");\n");
  destroy_step_id_list (cfs_faces);

  return file->next_id++;
}

step_id
step_manifold_solid_brep (step_file *file, char *name, step_id outer)
{
  fprintf (file->f, "#%i=MANIFOLD_SOLID_BREP('%s',#%i);\n", file->next_id, name, outer);

  return file->next_id++;
}

step_id
step_advanced_brep_shape_representation (step_file *file, char *name, step_id_list items, step_id context_of_items)
{
  fprintf (file->f, "#%i=ADVANCED_BREP_SHAPE_REPRESENTATION('%s',", file->next_id, name);
  fprint_id_list (file->f, items);
  fprintf (file->f, ",#%i);\n", context_of_items);
  destroy_step_id_list (items);

  return file->next_id++;
}

step_id
step_shape_definition_representation (step_file *file, step_id definition, step_id used_representation)
{
  fprintf (file->f, "#%i=SHAPE_DEFINITION_REPRESENTATION(#%i,#%i);\n", file->next_id, definition, used_representation);

  return file->next_id++;
}

step_id
step_colour_rgb (step_file *file, char *name, double red, double green, double blue)
{
  fprintf (file->f, "#%i=COLOUR_RGB('%s',%f,%f,%f);\n",
                    file->next_id, name, red, green, blue);
  return file->next_id++;
}

step_id
step_fill_area_style_colour (step_file *file, char *name, step_id fill_colour)
{
  fprintf (file->f, "#%i=FILL_AREA_STYLE_COLOUR('%s',#%i);\n",
                    file->next_id, name, fill_colour);

  return file->next_id++;
}

step_id
step_fill_area_style (step_file *file, char *name, step_id_list fill_styles)
{
  fprintf (file->f, "#%i=FILL_AREA_STYLE('%s',", file->next_id, name);
  fprint_id_list (file->f, fill_styles);
  fprintf (file->f, ");\n");
  destroy_step_id_list (fill_styles);

  return file->next_id++;
}

step_id
step_surface_style_fill_area (step_file *file, step_id fill_area)
{
  fprintf (file->f, "#%i=SURFACE_STYLE_FILL_AREA(#%i);\n",
                    file->next_id, fill_area);

  return file->next_id++;
}

step_id
step_surface_side_style (step_file *file, char *name, step_id_list styles)
{
  fprintf (file->f, "#%i=SURFACE_SIDE_STYLE('%s',", file->next_id, name);
  fprint_id_list (file->f, styles);
  fprintf (file->f, ");\n");
  destroy_step_id_list (styles);

  return file->next_id++;
}

/* XXX: surface_side should be an enum ".POSITIVE.", ".NEGATIVE." or ".BOTH." */
step_id
step_surface_style_usage (step_file *file, char *surface_side, step_id style)
{
  fprintf (file->f, "#%i=SURFACE_STYLE_USAGE(.%s.,#%i);\n",
                    file->next_id, surface_side, style);

  return file->next_id++;
}

step_id
step_presentation_style_assignment (step_file *file, step_id_list styles)
{
  fprintf (file->f, "#%i=PRESENTATION_STYLE_ASSIGNMENT(", file->next_id);
  fprint_id_list (file->f, styles);
  fprintf (file->f, ");\n");
  destroy_step_id_list (styles);

  return file->next_id++;
}

step_id
step_styled_item (step_file *file, char *name, step_id_list styles, step_id item)
{
  fprintf (file->f, "#%i=STYLED_ITEM('%s',", file->next_id, name);
  fprint_id_list (file->f, styles);
  fprintf (file->f, ",#%i);\n", item);
  destroy_step_id_list (styles);

  return file->next_id++;
}

step_id
step_over_riding_styled_item (step_file *file, char *name, step_id_list styles, step_id item, step_id over_ridden_style)
{
  fprintf (file->f, "#%i=OVER_RIDING_STYLED_ITEM('%s',", file->next_id, name);
  fprint_id_list (file->f, styles);
  fprintf (file->f, ",#%i,#%i);\n", item, over_ridden_style);
  destroy_step_id_list (styles);

  return file->next_id++;
}

step_id
step_presentation_layer_assignment (step_file *file, char *name, char *description, step_id_list assigned_items)
{
  fprintf (file->f, "#%i=PRESENTATION_LAYER_ASSIGNMENT('%s','%s',", file->next_id, name, description);
  fprint_id_list (file->f, assigned_items);
  fprintf (file->f, ");\n");
  destroy_step_id_list (assigned_items);

  return file->next_id++;
}

step_id
step_mechanical_design_geometric_presentation_representation (step_file *file, char *name, step_id_list items, step_id context_of_items)
{
  fprintf (file->f, "#%i=MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION('%s',", file->next_id, name);
  fprint_id_list (file->f, items);
  fprintf (file->f, ",#%i);\n", context_of_items);
  destroy_step_id_list (items);

  return file->next_id++;
}
