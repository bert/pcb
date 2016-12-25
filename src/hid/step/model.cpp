/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2015 Peter Clifton
 *  Copyright (C) 2015 PCB Contributors (see ChangeLog for details)
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */


#include <STEPfile.h>
#include <sdai.h>
#include <STEPattribute.h>
#include <ExpDict.h>
#include <Registry.h>
#include <errordesc.h>

#include <STEPcomplex.h>
#include <SdaiHeaderSchema.h>

#include "schema.h"

#include <SdaiAUTOMOTIVE_DESIGN.h>

#include "utils.h"
#include "string.h"

extern "C" {
#include <glib.h>
/* XXX: Sdai and PCB clash.. both define MarkType */
#include "global.h"
#include "hid/common/appearance.h"
#include "hid/common/step_id.h"
#include "hid/common/quad.h"
#include "hid/common/edge3d.h"
#include "hid/common/contour3d.h"
#include "hid/common/face3d.h"
#include "hid/common/vertex3d.h"
#include "hid/common/object3d.h"
}

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if 0
#  define DEBUG_PRODUCT_DEFINITION_SEARCH
#  define DEBUG_CHILD_REMOVAL
#  define DEBUG_PRODUCT_DEFINITION
#  define DEBUG_B_SPLINE_CURVES
#  define DEBUG_ASSEMBLY_TRAVERSAL
#  define DEBUG_TRANSFORMS
#else
#  undef DEBUG_PRODUCT_DEFINITION_SEARCH
#  undef DEBUG_CHILD_REMOVAL
#  undef DEBUG_PRODUCT_DEFINITION
#  undef DEBUG_B_SPLINE_CURVES
#  undef DEBUG_ASSEMBLY_TRAVERSAL
#  undef DEBUG_TRANSFORMS
#endif

#if 1
#  define DEBUG_NOT_IMPLEMENTED
#else
#  undef DEBUG_NOT_IMPLEMENTED
#endif

#include <glib.h>

extern "C" {
#include "model.h"
}


typedef std::list<SDAI_Application_instance *> ai_list;
typedef std::list<SdaiManifold_solid_brep *> msb_list;
typedef std::list<SdaiMapped_item *> mi_list;


SdaiProduct_definition *
read_model_from_file (Registry *registry,
                      InstMgr *instance_list,
                      const char *filename)
{
  STEPfile sfile = STEPfile (*registry, *instance_list, "", false);

  try
    {
      sfile.ReadExchangeFile (filename);
    }
  catch (...)
    {
      std::cout << "ERROR: Caught exception when attempting to read from file '" << filename << "' (does the file exist?)" << std::endl;
      return NULL;
    }

  Severity severity = sfile.Error().severity();
  if (severity != SEVERITY_NULL)
    {
      sfile.Error().PrintContents (std::cout);
      std::cout << "WARNING: Error reading from file '" << filename << "'" << std::endl;
//      return NULL;
// XXX: HANDLE OTHER ERRORS BETTER?
    }

  pd_list pd_list;

  // Find all PRODUCT_DEFINITION entities with a SHAPE_DEFINITION_REPRESETNATION
  find_all_pd_with_sdr (instance_list, &pd_list, 0);

  /*  Try to determine the root product */
  find_and_remove_child_pd (instance_list, &pd_list, 0, "Next_assembly_usage_occurrence"); // Remove any PD which are children of another via NAUO
  find_and_remove_child_pd (instance_list, &pd_list, 0, "Assembly_component_usage");       // Remove any PD which are children of another via ACU
  find_and_remove_child_pd_mi_rm_sr (instance_list, &pd_list, 0); // Remove any PD which are children of another via MAPPED_ITEM->REPRESENTATION_MAP->SHAPE_REPRESENTATION


#ifdef DEBUG_PRODUCT_DEFINITION_SEARCH
  std::cout << "Hopefully left with the root product definition" << std::endl;
  for (pd_list::iterator iter = pd_list.begin(); iter != pd_list.end(); iter++)
    std::cout << "Product definition list item #" << (*iter)->StepFileId () << std::endl;
  std::cout << std::endl;
#endif

  // If we didn't find a suitable PD, give up now
  if (pd_list.size() == 0)
    {
      std::cout << "ERROR: Did not find a PRODUCT_DEFINITION (with associated SHAPE_DEFINITION_REPRESENTATION)" << std::endl;
      return NULL;
    }

  if (pd_list.size() > 1)
    std::cout << "WARNING: Found more than one PRODUCT_DEFINITION that might be the root" << std::endl;

  // Use the first PD meeting the criterion. Hopefully there should just be one, but if not, we pick the first.
  return *pd_list.begin();
}

static void
find_manifold_solid_brep_possible_voids (SdaiShape_representation *sr,
                                         msb_list *msb_list)
{
  SingleLinkNode *iter = sr->items_ ()->GetHead ();

  while (iter != NULL)
    {
      SDAI_Application_instance *node = ((EntityNode *)iter)->node;

      if (strcmp (node->EntityName (), "Manifold_Solid_Brep") == 0 ||
          strcmp (node->EntityName (), "Brep_With_Voids") == 0)

        msb_list->push_back ((SdaiManifold_solid_brep *)node);

      iter = iter->NextNode ();
    }
}

static void
find_mapped_item (SdaiShape_representation *sr,
                  mi_list *mi_list)
{
  SingleLinkNode *iter = sr->items_ ()->GetHead ();

  while (iter != NULL)
    {
      SDAI_Application_instance *node = ((EntityNode *)iter)->node;

      if (strcmp (node->EntityName (), "Mapped_Item") == 0)
        mi_list->push_back ((SdaiMapped_item *)node);

      iter = iter->NextNode ();
    }
}

/* XXX: This doesn't really follow the correct STEP approach here,
 *      but we just start by looking whether we can find a styled_item
 *      or overriding_styled_item pointing towards the item in question,
 *      then retrieve its styling.
 */

static GHashTable *
build_hash_of_styled_items (InstMgr *instance_list)
{
  /* NB: NULLs give g_direct_hash and g_direct_equal */
  GHashTable *table = g_hash_table_new (NULL, NULL);

  MgrNode * mnode = NULL;
  int search_index;
  SDAI_Application_instance *entity;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  while (ENTITY_NULL != (entity = instance_list->GetApplication_instance ("Styled_Item", search_index)))
    {
      SdaiStyled_item *si = static_cast<SdaiStyled_item *>(entity);

      g_hash_table_insert (table, si->item_ (), (void *)si);

      int id = entity->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }

  mnode = NULL;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  while (ENTITY_NULL != (entity = instance_list->GetApplication_instance ("Over_Riding_Styled_Item", search_index)))
    {
      SdaiStyled_item *si = static_cast<SdaiStyled_item *>(entity);

      g_hash_table_insert (table, si->item_ (), (void *)si);

      int id = entity->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }

  return table;
}

static EntityAggregate *
find_styles_for_item (GHashTable *table, SDAI_Application_instance *item)
{
  /* When reading a STYLED_ITEM with item pointing to an ABSR entity (as Solidworks
   * likes to emit, for product-wide colouring, STEPcode reads the item entity
   * pointer as NULL_ENTITY, since ABSR doesn't match the type expected by
   * the item property.
   *
   * We take a guess, and assume this is the only one - so if we look up NULL_ENTITY
   * in the hash table, we find the correct corresponding STYLED_ITEM.
   *
   * XXX: This might not be the case unfortunately, e.g. an assembly with multiple
   *      ABSR with errant STYLED_ITEM pointers associated. (Need to teach STEPcode
   *      to store the incorrect pointer for us, or to be more lenient with this case.
   */
  if (!strcmp (item->EntityName (), "Advanced_Brep_Shape_Representation"))
    {
      item = NULL_ENTITY;
    }

  auto si = static_cast<SdaiStyled_item *>(g_hash_table_lookup (table, item));

  if (si == NULL)
    return NULL;

  return si->styles_ ();
}

/* XXX: Does not pay paticularly much attention to specifics of how the STEP style
 *      is defined to apply. Surface side, edge/face, etc... all we are really
 *      interested in is extracting the required colour in the majority of cases.
 */
static appearance *
make_appearance_for_styles (EntityAggregate *styles)
{
  /* Try to find path through from styles:
   *
   * SdaiPresentation_style_assignment *psa = styles[i];
   * SdaiPresentation_style_select *pss = psa->styles[j];
   * (One of:
   *    pre_defined_presentation_style
   *    point_style
   *    curve_style
   *    surface_style_usage          <--- Likely to find this one first
   *    symbol_style
   *    fill_area_style              <--- Eventually looking for this one?
   *    text_style,
   *    approximation_tolerance
   *    externally_defined_style
   *    null_style
   * )
   *
   * SdaiSurface_style_usage *ssu = *pss;
   * Ignore side specification given by ssu->side;
   * SdaiSurface_side_style_select *ssss = ssu->style;
   * SdaiSurface_side_style *sss = *ssss;
   * SdaiSurface_style_element_select *sses = sss->styles[k]; 1-7 parts
   * (One of:
   *    surface_style_fill_area          <--- Looking for this one
   *    surface_style_boundary
   *    surface_style_silhouette
   *    surface_style_segmentation_curve
   *    surface_style_control_grid
   *    surface_style_parameter_line
   *    surface_style_rendering
   * )
   * SdaiSurface_style_fill_area *ssfa = *sses;
   * SdaiFill_area_style *fas = ssfa->fill_area;   <--- WOOT.. NEARLY THERE!!
   * SdaiFill_style_select *fss = fas->fill_styles[l];
   * (One of:
   *    fill_area_style_colour               <--- Looking for this one
   *    externally_defined_tile_style
   *    fill_area_style_tiles
   *    externally_defined_hatch_style
   *    fill_area_style_hatching
   * )
   * SdaiFill_area_style_colour *fasc = *fss;
   * SdaiColour *fc = fasc->fill_colour;
   * SdaiColour_Rgb *scr = dynamic_cast<SdaiColour_Rgb *>(fc);
   * r = scr->red;
   * g = scr->green;
   * b = scr->blue;
   */

  for (EntityNode * psa_iter = static_cast<EntityNode *>(styles->GetHead ());
       psa_iter != NULL;
       psa_iter = static_cast<EntityNode *>(psa_iter->NextNode ()))
    {
      auto *psa = dynamic_cast<SdaiPresentation_style_assignment *>(psa_iter->node);

      if (psa == NULL)
        {
          printf ("psa == NULL\n");
          continue;
        }

      for (SelectNode * pss_iter = static_cast<SelectNode *>(psa->styles_ ()->GetHead ());
           pss_iter != NULL;
           pss_iter = static_cast<SelectNode *>(pss_iter->NextNode ()))
        {
          auto *pss = static_cast<SdaiPresentation_style_select *>(pss_iter->node);

          if (pss == NULL)
            {
              printf ("pss == NULL\n");
              continue;
            }

          if (pss->IsFill_area_style ())
            {
              printf ("Found a FILL_AREA_STYLE example... investigate!\n");
            }
          else if (!pss->IsSurface_style_usage ())
            {
              continue;
            }

          SdaiSurface_style_usage *ssu = *pss;

//          printf ("Found a SSU.. (Side = %i)\n", (int)ssu->side_ ());

          SdaiSurface_side_style_select *ssss = ssu->style_ ();
          SdaiSurface_side_style *sss = *ssss;

          for (SelectNode * sses_iter = static_cast<SelectNode *>(sss->styles_ ()->GetHead ());
               sses_iter != NULL;
               sses_iter = static_cast<SelectNode *>(sses_iter->NextNode ()))
            {
              auto *sses = static_cast<SdaiSurface_style_element_select *>(sses_iter->node);

              if (!sses->IsSurface_style_fill_area ())
                {
                  continue;
                }

              SdaiSurface_style_fill_area *ssfa = *sses;
              SdaiFill_area_style *fas = ssfa->fill_area_ ();

              for (SelectNode * fss_iter = static_cast<SelectNode *>(fas->fill_styles_ ()->GetHead ());
                   fss_iter != NULL;
                   fss_iter = static_cast<SelectNode *>(fss_iter->NextNode ()))
                {
                  auto *fss = static_cast<SdaiFill_style_select *>(fss_iter->node);

                  if (!fss->IsFill_area_style_colour ())
                    {
                      continue;
                    }

                  SdaiFill_area_style_colour *fasc = *fss;
                  SdaiColour *fc = fasc->fill_colour_ ();
                  SdaiColour_rgb *scr = dynamic_cast<SdaiColour_rgb *>(fc);

                  if (scr == NULL)
                    {
                      printf ("Colour appears not to be COLOUR_RGB\n");
                      continue;
                    }

                  float r = scr->red_ ();
                  float g = scr->green_ ();
                  float b = scr->blue_ ();

                  appearance *appear = make_appearance ();
                  appearance_set_color (appear, r, g, b);
                  return appear;
                }
            }
        }
    }

  return NULL;
}

static void
unpack_axis_geom (SdaiAxis2_placement_3d *axis,
                  double *ox, double *oy, double *oz,
                  double *ax, double *ay, double *az,
                  double *rx, double *ry, double *rz)
{
  *ox = ((RealNode *)axis->location_ ()->coordinates_ ()->GetHead ())->value;
  *oy = ((RealNode *)axis->location_ ()->coordinates_ ()->GetHead ()->NextNode ())->value;
  *oz = ((RealNode *)axis->location_ ()->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;
  *ax = ((RealNode *)axis->axis_ ()->direction_ratios_ ()->GetHead ())->value;
  *ay = ((RealNode *)axis->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
  *az = ((RealNode *)axis->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
  *rx = ((RealNode *)axis->ref_direction_ ()->direction_ratios_ ()->GetHead ())->value;
  *ry = ((RealNode *)axis->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
  *rz = ((RealNode *)axis->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
}

typedef struct process_step_info {
  /* Hash / list of SR -> step_model */
  object3d *object;
  face3d *current_face;
  contour3d *current_contour;
  double current_transform[4][4];
  GHashTable *styled_items;

} process_step_info;

void
copy_4x4 (double from[4][4],
          double to[4][4])
{
  memcpy (to, from, sizeof(double[4][4]));
}

void
identity_4x4 (double m[4][4])
{
  int i, j;

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      m[i][j] = 0.0;

  m[0][0] = 1.0;
  m[1][1] = 1.0;
  m[2][2] = 1.0;
  m[3][3] = 1.0;
}

/* NB: Column major */
/* matrix[column][row] */
void
mult_4x4 (double a[4][4], double b[4][4], double to[4][4])
{
  int i, j;

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      to[i][j] = a[0][j] * b[i][0] +
                 a[1][j] * b[i][1] +
                 a[2][j] * b[i][2] +
                 a[3][j] * b[i][3];
}

/* NB: Column major */
void
translate_origin (double m[4][4], double x, double y, double z)
{
  m[3][0] += x;
  m[3][1] += y;
  m[3][2] += z;
}

/* NB: Column major */
void
rotate_basis (double m[4][4], double ax, double ay, double az,
                              double rx, double ry, double rz)
{
  double basis[4][4];
  double old[4][4];
  double ox, oy, oz;

  ox = ay * rz - az * ry;
  oy = az * rx - ax * rz;
  oz = ax * ry - ay * rx;

  basis[0][0] = rx;  basis[1][0] = ox;  basis[2][0] = ax;  basis[3][0] = 0.0;
  basis[0][1] = ry;  basis[1][1] = oy;  basis[2][1] = ay;  basis[3][1] = 0.0;
  basis[0][2] = rz;  basis[1][2] = oz;  basis[2][2] = az;  basis[3][2] = 0.0;
  basis[0][3] = 0.0; basis[1][3] = 0.0; basis[2][3] = 0.0; basis[3][3] = 1.0;

  copy_4x4 (m, old);
  //mult_4x4 (old, basis, m);
  mult_4x4 (basis, old, m);
}

/* NB: Row major, or transosed column major. Since Matrix will be orthogonal, this is equal to its inverse */
void
rotate_basis_inverted (double m[4][4], double ax, double ay, double az,
                                       double rx, double ry, double rz)
{
  double basis[4][4];
  double old[4][4];
  double ox, oy, oz;

  ox = ay * rz - az * ry;
  oy = az * rx - ax * rz;
  oz = ax * ry - ay * rx;

  basis[0][0] = rx;  basis[0][1] = ox;  basis[0][2] = ax;  basis[0][3] = 0.0;
  basis[1][0] = ry;  basis[1][1] = oy;  basis[1][2] = ay;  basis[1][3] = 0.0;
  basis[2][0] = rz;  basis[2][1] = oz;  basis[2][2] = az;  basis[2][3] = 0.0;
  basis[3][0] = 0.0; basis[3][1] = 0.0; basis[3][2] = 0.0; basis[3][3] = 1.0;

  copy_4x4 (m, old);
  //mult_4x4 (old, basis, m);
  mult_4x4 (basis, old, m);
}

/* NB: Column major */
void
transform_vertex (double m[4][4], double *x, double *y, double *z)
{
  double new_x, new_y, new_z; //, new_w;

  new_x = m[0][0] * *x +
          m[1][0] * *y +
          m[2][0] * *z +
          m[3][0] * 1.0;

  new_y = m[0][1] * *x +
          m[1][1] * *y +
          m[2][1] * *z +
          m[3][1] * 1.0;

  new_z = m[0][2] * *x +
          m[1][2] * *y +
          m[2][2] * *z +
          m[3][2] * 1.0;

#if 0
  new_w = m[0][3] * *x +
          m[1][3] * *y +
          m[2][3] * *z +
          m[3][3] * 1.0;

  new_x /= new_w;
  new_y /= new_w;
  new_z /= new_w;
#endif

  *x = new_x;
  *y = new_y;
  *z = new_z;
}

/* NB: Column major */
void
transform_vector (double m[4][4], double *x, double *y, double *z)
{
  double new_x, new_y, new_z; //, new_w;

  new_x = m[0][0] * *x +
          m[1][0] * *y +
          m[2][0] * *z;

  new_y = m[0][1] * *x +
          m[1][1] * *y +
          m[2][1] * *z;

  new_z = m[0][2] * *x +
          m[1][2] * *y +
          m[2][2] * *z;

#if 0
  new_w = m[0][3] * *x +
          m[1][3] * *y +
          m[2][3] * *z +
          m[3][3] * 1.0;

  new_x /= new_w;
  new_y /= new_w;
  new_z /= new_w;
#endif

  *x = new_x;
  *y = new_y;
  *z = new_z;
}

static double
distance (double a[3], double b[3])
{
  return hypot (hypot (a[0] - b[0], a[1] - b[1]), a[2] - b[2]);
}

static void
process_bscwk (SDAI_Application_instance *start_entity, edge_ref our_edge, process_step_info *info, bool orientation)
{
  /* Code using lazy binding approach, since many of the B_SPLINE_* types we will encounter
   * are used in complex entities.. no sense witing code to handle them twice
   */

  edge_info *our_edge_info = (edge_info *)UNDIR_DATA(our_edge);
//  our_edge_info->is_placeholder = true; /* Highlight for now */

  SDAI_Application_instance *entity = start_entity;
  STEPcomplex *stepcomplex = NULL;
  STEPattribute *attr;

  bool is_complex;
  bool found_bounded_curve = false;
  bool found_b_spline_curve = false;
  bool found_b_spline_curve_with_knots = false;
  bool found_curve = false;
  bool found_geometric_representation_item = false;
  bool found_rational_b_spline_curve = false;
  bool found_representation_item = false;

  /* Potential Complex pieces:
   *
   * BOUNDED_CURVE
   * B_SPLINE_CURVE
   * B_SPLINE_CURVE_WITH_KNOTS
   * CURVE
   * GEOMETRIC_REPRESENTATION_ITEM
   * RATIONAL_B_SPLINE_CURVE
   * REPRESENTATION_ITEM
   */

  /* Things we take note of for futher processing.. */

  /* B_SPLINE_CURVE */
  int b_spline_curve_degree = 0;
  EntityAggregate *control_points = NULL;
  int num_control_points; /* (Convenience) */

  /* B_SPLINE_CURVE_WITH_KNOTS */
  IntAggregate *knot_multiplicities = NULL;
  RealAggregate *knots = NULL;
  int num_knots; /* (Calculated) */
  int num_knot_multiplicities; /* (Convenience) */

  /* RATIONAL_B_SPLINE_CURVE */
  RealAggregate *weights = NULL;

  /* Iterators, counters etc.. */
  int i;
  int j;
  int k;
  EntityNode *cp_iter;
  IntNode *km_iter;
  RealNode *k_iter;
  RealNode *w_iter;

  is_complex = entity->IsComplex();

  if (is_complex)
    {
      stepcomplex = dynamic_cast<STEPcomplex *>(entity)->head;
      entity = stepcomplex;
    }

  while (entity)
    {
      if (entity->EntityName() == NULL)
        {
          std::cout << "ERROR: NULL whilst traversing complex / entity" << std::endl;
          return;
        }
      else if (!strcmp (entity->EntityName (), "Bounded_Curve"))
        {
          found_bounded_curve = true;
        }
      else if (!strcmp (entity->EntityName (), "B_Spline_Curve"))
        {
          found_b_spline_curve = true;
        }
      else if (!strcmp (entity->EntityName (), "B_Spline_Curve_With_Knots"))
        {
          found_b_spline_curve_with_knots = true;
        }
      else if (!strcmp (entity->EntityName (), "Curve"))
        {
          found_curve = true;
        }
      else if (!strcmp (entity->EntityName (), "Geometric_Representation_Item"))
        {
          found_geometric_representation_item = true;
        }
      else if (!strcmp (entity->EntityName (), "Rational_B_Spline_Curve"))
        {
          found_rational_b_spline_curve = true;
        }
      else if (!strcmp (entity->EntityName (), "Representation_Item"))
        {
          found_representation_item = true;
        }
      else
        {
          printf ("INFO: Unchecked entity name in complex, \"%s\"\n", entity->EntityName ());
        }

      entity->ResetAttributes ();
      while ((attr = entity->NextAttribute()) != NULL)
        {
          if (!strcmp (attr->Name (), "name"))
            {
            }
          else if (!strcmp (attr->Name (), "degree"))
            {
              b_spline_curve_degree = *attr->Integer ();
            }
          else if (!strcmp (attr->Name (), "control_points_list"))
            {
              control_points = dynamic_cast<EntityAggregate *>(attr->Aggregate ());
            }
          else if (!strcmp (attr->Name (), "curve_form"))
            {
            }
          else if (!strcmp (attr->Name (), "closed_curve"))
            {
            }
          else if (!strcmp (attr->Name (), "self_intersect"))
            {
            }
          else if (!strcmp (attr->Name (), "knot_multiplicities"))
            {
              knot_multiplicities = dynamic_cast<IntAggregate *>(attr->Aggregate ());
            }
          else if (!strcmp (attr->Name (), "knots"))
            {
              knots = dynamic_cast<RealAggregate *>(attr->Aggregate ());
            }
          else if (!strcmp (attr->Name (), "knot_spec"))
            {
            }
          else if (!strcmp (attr->Name (), "weights_data"))
            {
              weights = dynamic_cast<RealAggregate *>(attr->Aggregate ());
            }
          else
            {
              printf ("INFO: Unchecked attribute name in entity, \"%s\"\n", attr->Name ());
            }
        }

      if (stepcomplex != NULL)
        stepcomplex = stepcomplex->sc;

      entity = stepcomplex;
    }

#ifdef DEBUG_B_SPLINE_CURVES
  /* Now we see what we found .... */
  printf ("------\n");
  printf ("b_spline_curve_degree = %i\n",b_spline_curve_degree);
  printf ("control_points = %p\n",control_points);
  printf ("knot_multiplicities = %p\n", knot_multiplicities);
  printf ("knots = %p\n", knots);
  printf ("weights = %p\n", weights);
  printf ("is_complex = %s\n",                          is_complex                          ? "true" : "false");
  printf ("found_bounded_curve = %s\n",                 found_bounded_curve                 ? "true" : "false");
  printf ("found_b_spline_curve = %s\n",                found_b_spline_curve                ? "true" : "false");
  printf ("found_b_spline_curve_with_knots = %s\n",     found_b_spline_curve_with_knots     ? "true" : "false");
  printf ("found_curve = %s\n",                         found_curve                         ? "true" : "false");
  printf ("found_geometric_representation_item = %s\n", found_geometric_representation_item ? "true" : "false");
  printf ("found_rational_b_spline_curve = %s\n",       found_rational_b_spline_curve       ? "true" : "false");
  printf ("found_representation_item = %s\n",           found_representation_item           ? "true" : "false");

  if (control_points != NULL)
    printf ("Number of control points = %i\n", control_points->EntryCount ());

  if (knot_multiplicities != NULL)
    printf ("Number of knot_multiplicities = %i\n", knot_multiplicities->EntryCount ());

  if (knots != NULL) {
    printf ("Number of knots = %i\n", knots->EntryCount ());
  }
#endif

  if ((knot_multiplicities == NULL) !=
      (knots == NULL))
    {
      printf ("ERROR: Have one of knots, knot_multiplicities but not both!\n");
      return;
    }

  if (knot_multiplicities != NULL &&
      knots != NULL &&
      knot_multiplicities->EntryCount () != knots->EntryCount ())
    {
      printf ("ERROR: Different length of knot and knot multiplicity lists\n");
      return;
    }

  if (control_points == NULL)
    {
      printf ("ERROR: control points == NULL\n");
      return;
    }

  num_control_points = control_points->EntryCount ();

  if (weights != NULL) {
#ifdef DEBUG_B_SPLINE_CURVES
    printf ("Number of weights = %i\n", weights->EntryCount ());
#endif

    if (weights->EntryCount () != num_control_points)
      {
        printf ("ERROR: Weights not null, but entry length doesn't equal the number of control points\n");
      }
  }

  num_knots = num_control_points + b_spline_curve_degree + 1;

#ifdef DEBUG_B_SPLINE_CURVES
  printf ("------\n");
#endif

  our_edge_info->is_bspline = true;
  our_edge_info->degree = b_spline_curve_degree;
  our_edge_info->num_control_points = num_control_points;

  our_edge_info->control_points = g_new(double, num_control_points * 3);
  our_edge_info->weights =        g_new(double, num_control_points);
  our_edge_info->knots =          g_new(double, num_knots);

  /* Control points */

  for (i = 0, cp_iter = dynamic_cast<EntityNode *>(control_points->GetHead ());
       i < num_control_points * 3;
       i += 3, cp_iter = dynamic_cast<EntityNode *>(cp_iter->NextNode ()))
    {
      SdaiCartesian_point *cp = dynamic_cast<SdaiCartesian_point *>(cp_iter->node);

      our_edge_info->control_points[i + 0] = ((RealNode *)cp->coordinates_ ()->GetHead ())->value;
      our_edge_info->control_points[i + 1] = ((RealNode *)cp->coordinates_ ()->GetHead ()->NextNode ())->value;
      our_edge_info->control_points[i + 2] = ((RealNode *)cp->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;

      transform_vertex (info->current_transform,
                        &our_edge_info->control_points[i + 0],
                        &our_edge_info->control_points[i + 1],
                        &our_edge_info->control_points[i + 2]);
    }

  /* Weights */

  if (weights == NULL)
    {
      for (i = 0; i < num_control_points; i++)
        our_edge_info->weights[i] = 1.0;
    }
  else
    {
      for (i = 0, w_iter = dynamic_cast<RealNode *>(weights->GetHead ());
           i < num_control_points;
           i++, w_iter = dynamic_cast<RealNode *>(w_iter->NextNode ()))
        {
          our_edge_info->weights[i] = w_iter->value;
        }
    }

  /* Knots & multiplicities */

  if (knot_multiplicities == NULL)
    {
      int start_knot;

      /* Examples for d=3:
      *
       *        Uniform has: KNOTS -d, -d+1, ... 0 ... num_cp - 1, num_cp
       *   Quasi unirom has: KNOTS 0, 0, 0, 0, 1, 2, 3, ..., n, n, n, n                    (example for d = 3, n = num_cp - 2 * d (?))
       *  Simple bezier has: KNOTS 0, 0, 0, 0, 1, 1, 1, 1                                  (NB: a special case of quasi uniform)
       * General bezier has: KNOTS 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, ..., n, n, n, n
       */

#warning INVOKE THE PROPER CASE!!
      if (1/* UNIFORM CASE */)
        start_knot = -b_spline_curve_degree;
      else if (0/* QUASI UNIFORM CASE */)
        start_knot = 0;
      else if (0/* PIECEWISE BEZIER CASE */)
        start_knot = 0;
      else
        return; /* ERROR */

      k = 0;
      for (i = 0; i < num_knots; i++)
        {
          int multiplicity;
          bool first_or_last = (k == 0) || (k = num_knots - b_spline_curve_degree - 1);

#warning INVOKE THE PROPER CASE!!
          if (1/* UNIFORM CASE */)
            multiplicity = 1;
          else if (0/* QUASI UNIFORM CASE */)
            multiplicity = first_or_last ? b_spline_curve_degree + 1 : 1;
          else if (0/* PIECEWISE BEZIER CASE */)
            multiplicity = first_or_last ? b_spline_curve_degree + 1 : b_spline_curve_degree;
          else
            return; /* ERROR */

          for (j = 0; j < multiplicity; j++, k++)
            our_edge_info->knots[k] = start_knot;

          /* XXX: Not expanded the multiplicities here */
        }
    }
  else
    {
      /* NB: From checks above, we kow if know_multiplicities != NULL, so is knots */

      num_knot_multiplicities = knot_multiplicities->EntryCount ();

      k = 0;
      for (i = 0, k_iter = dynamic_cast<RealNode *>(knots->GetHead ()), km_iter = dynamic_cast<IntNode *>(knot_multiplicities->GetHead ());
           i < num_knot_multiplicities;
           i++, k_iter = dynamic_cast<RealNode *>(k_iter->NextNode ()), km_iter = dynamic_cast<IntNode *>(km_iter->NextNode ()))
        {
          for (j = 0; j < km_iter->value; j++, k++)
            our_edge_info->knots[k] = k_iter->value;
        }
    }


  double v1_[3];
  double v2_[3];

  vertex3d *v1;
  vertex3d *v2;

  if (orientation)
    {
      v1 = (vertex3d *)ODATA(our_edge);
      v2 = (vertex3d *)DDATA(our_edge);
    }
  else
    {
      v2 = (vertex3d *)ODATA(our_edge);
      v1 = (vertex3d *)DDATA(our_edge);
    }

  v1_[0] = v1->x;
  v1_[1] = v1->y;
  v1_[2] = v1->z;

  v2_[0] = v2->x;
  v2_[1] = v2->y;
  v2_[2] = v2->z;

  double dist1 = distance(v1_, &our_edge_info->control_points[0]);
  double dist2 = distance(v2_, &our_edge_info->control_points[(num_control_points - 1)*3]);

  if (dist1 > 0.01 || dist2 > 0.02)
    {
      printf ("Entity #%i end point to first control point distances %f and %f\n",
              start_entity->StepFileId (), dist1, dist2);
    }
}

static void
process_edge_geometry (SdaiEdge *edge, bool orientation, edge_ref our_edge, process_step_info *info)
{
  vertex3d *vertex;
  double x1, y1, z1;
  double x2, y2, z2;
  edge_info *our_edge_info = (edge_info *)UNDIR_DATA(our_edge);

  if (strcmp (edge->edge_start_ ()->EntityName (), "Vertex_Point") != 0 ||
      strcmp (edge->edge_end_   ()->EntityName (), "Vertex_Point") != 0)
    {
      printf ("WARNING: Edge start and/or end vertices are not specified as VERTEX_POINT\n");
      return;
    }

  // NB: Assuming edge points to an EDGE, or one of its subtypes that does not make edge_start and edge_end derived attributes.
  //     In practice, edge should point to an EDGE_CURVE sub-type
  SdaiVertex_point *edge_start = (SdaiVertex_point *) (orientation ? edge->edge_start_ () : edge->edge_end_ ());
  SdaiVertex_point *edge_end =  (SdaiVertex_point *) (!orientation ? edge->edge_start_ () : edge->edge_end_ ());

  // NB: XXX: SdaiVertex_point multiply inherits from vertex and geometric_representation_item

  SdaiPoint *edge_start_point = edge_start->vertex_geometry_ ();
  SdaiPoint *edge_end_point = edge_end->vertex_geometry_ ();

  if (strcmp (edge_start_point->EntityName (), "Cartesian_Point") == 0)
    {
      /* HAPPY WITH THIS TYPE */
    }
  else
    {
      // XXX: point_on_curve, point_on_surface, point_replica, degenerate_pcurve
      printf ("WARNING: Got Edge start point as unhandled point type (%s)\n", edge_start_point->EntityName ());
      return;
    }

  if (strcmp (edge_end_point->EntityName (), "Cartesian_Point") == 0)
    {
      /* HAPPY WITH THIS TYPE */
    }
  else
    {
      // XXX: point_on_curve, point_on_surface, point_replica, degenerate_pcurve
      printf ("WARNING: Got Edge end point as unhandled point type (%s)\n", edge_end_point->EntityName ());
      return;
    }

  SdaiCartesian_point *edge_start_cp = (SdaiCartesian_point *)edge_start_point;
  SdaiCartesian_point *edge_end_cp = (SdaiCartesian_point *)edge_end_point;

  x1 = ((RealNode *)edge_start_cp->coordinates_ ()->GetHead ())->value;
  y1 = ((RealNode *)edge_start_cp->coordinates_ ()->GetHead ()->NextNode ())->value;
  z1 = ((RealNode *)edge_start_cp->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;
  x2 = ((RealNode *)edge_end_cp->coordinates_ ()->GetHead ())->value;
  y2 = ((RealNode *)edge_end_cp->coordinates_ ()->GetHead ()->NextNode ())->value;
  z2 = ((RealNode *)edge_end_cp->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;

#if 0
  printf ("    Edge #%i starts at (%f, %f, %f) and ends at (%f, %f, %f)\n",
          edge->StepFileId (), x1, y1, z1, x2, y2, z2);
#endif

  if (strcmp (edge->EntityName (), "Edge_Curve") == 0)
    {
      SdaiEdge_curve *ec = (SdaiEdge_curve *)edge;

      SdaiCurve *curve = ec->edge_geometry_ ();
      bool same_sense = ec->same_sense_ ();

      our_edge_info->same_sense = same_sense;

#if 0
      printf ("         underlying curve is %s #%i, same_sense is %s\n", curve->EntityName (), curve->StepFileId(), same_sense ? "True" : "False");
#endif

      if (strcmp (curve->EntityName (), "Surface_Curve") == 0)
        {
          SdaiSurface_curve *sc = (SdaiSurface_curve *)curve;

          /* Hopefully we can just point curve at the (presumed) 3D curve */
          curve = sc->curve_3d_ ();
        }

      if (strcmp (curve->EntityName (), "Line") == 0)
        {
          transform_vertex (info->current_transform, &x1, &y1, &z1);
          transform_vertex (info->current_transform, &x2, &y2, &z2);

          object3d_add_edge (info->object, our_edge);
          vertex = make_vertex3d (x1, y1, z1);
          ODATA(our_edge) = vertex;
          vertex = make_vertex3d (x2, y2, z2);
          DDATA(our_edge) = vertex;

//              printf ("WARNING: Underlying curve geometry type Line is not supported yet\n");
//              return;
        }
      else if (strcmp (curve->EntityName (), "Circle") == 0)
        {
          SdaiCircle *circle = (SdaiCircle *)curve;
          double cx, cy, cz;
          double nx, ny, nz;
          double rx, ry, rz;

          SdaiAxis2_placement_3d *placement = *circle->position_ ();
          unpack_axis_geom (placement, &cx, &cy, &cz, &nx, &ny, &nz, &rx, &ry, &rz);
#if 0
          double cx = ((RealNode *)circle->position_ ()->location_ ()->coordinates_ ()->GetHead ())->value;
          double cy = ((RealNode *)circle->position_ ()->location_ ()->coordinates_ ()->GetHead ()->NextNode ())->value;
          double cz = ((RealNode *)circle->position_ ()->location_ ()->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;
          double nx = ((RealNode *)circle->position_ ()->axis_ ()->direction_ratios_ ()->GetHead ())->value;
          double ny = ((RealNode *)circle->position_ ()->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
          double nz = ((RealNode *)circle->position_ ()->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
#endif

          double radius = circle->radius_();

          transform_vertex (info->current_transform, &cx, &cy, &cz);
          transform_vertex (info->current_transform, &x1, &y1, &z1);
          transform_vertex (info->current_transform, &x2, &y2, &z2);

          transform_vector (info->current_transform, &nx, &ny, &nz);

          if (orientation) // NOT REQUIRED, SINCE WE ADDED same_sense to the edge info ----> (orientation == same_sense)
            {
              edge_info_set_round2 (our_edge_info, cx, cy, cz, nx, ny, nz, rx, ry, rz, radius);
            }
          else
            {
              edge_info_set_round2 (our_edge_info, cx, cy, cz, -nx, -ny, -nz, rx, ry, rz, radius);
            }

          object3d_add_edge (info->object, our_edge);
          vertex = make_vertex3d (x1, y1, z1);
          ODATA(our_edge) = vertex;
          vertex = make_vertex3d (x2, y2, z2);
          DDATA(our_edge) = vertex;

        }
      else if (strcmp (curve->EntityName (), "Ellipse") == 0)
        {
          SdaiEllipse *ellipse = (SdaiEllipse *)curve;
          double cx, cy, cz;
          double nx, ny, nz;
          double rx, ry, rz;

          SdaiAxis2_placement_3d *placement = *ellipse->position_ ();
          unpack_axis_geom (placement, &cx, &cy, &cz, &nx, &ny, &nz, &rx, &ry, &rz);
#if 0
          double cx = ((RealNode *)ellipse->position_ ()->location_ ()->coordinates_ ()->GetHead ())->value;
          double cy = ((RealNode *)ellipse->position_ ()->location_ ()->coordinates_ ()->GetHead ()->NextNode ())->value;
          double cz = ((RealNode *)ellipse->position_ ()->location_ ()->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;
          double nx = ((RealNode *)ellipse->position_ ()->axis_ ()->direction_ratios_ ()->GetHead ())->value;
          double ny = ((RealNode *)ellipse->position_ ()->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
          double nz = ((RealNode *)ellipse->position_ ()->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
#endif

          transform_vertex (info->current_transform, &x1, &y1, &z1);
          transform_vertex (info->current_transform, &x2, &y2, &z2);

          our_edge_info->is_ellipse = true;

          double radius1 = ellipse->semi_axis_1_ ();
          double radius2 = ellipse->semi_axis_2_ ();

          transform_vertex (info->current_transform, &cx, &cy, &cz);

          transform_vector (info->current_transform, &nx, &ny, &nz);

          if (orientation) // NOT REQUIRED, SINCE WE ADDED same_sense to the edge info ----> (orientation == same_sense)
            {
              edge_info_set_round2 (our_edge_info, cx, cy, cz, nx, ny, nz, rx, ry, rz, radius1);
            }
          else
            {
              edge_info_set_round2 (our_edge_info, cx, cy, cz, -nx, -ny, -nz, rx, ry, rz, radius1);
            }
          our_edge_info->radius2 = radius2;

          object3d_add_edge (info->object, our_edge);
          vertex = make_vertex3d (x1, y1, z1);
          ODATA(our_edge) = vertex;
          vertex = make_vertex3d (x2, y2, z2);
          DDATA(our_edge) = vertex;

        }
      else if (curve->IsComplex() || /* This is a guess - assuming complex curves are likely to be B_SPLINE_* complexes */
               strcmp (curve->EntityName (), "B_Spline_Curve_With_Knots") == 0)
        {
          transform_vertex (info->current_transform, &x1, &y1, &z1);
          transform_vertex (info->current_transform, &x2, &y2, &z2);

          object3d_add_edge (info->object, our_edge);
          vertex = make_vertex3d (x1, y1, z1);
          ODATA(our_edge) = vertex;
          vertex = make_vertex3d (x2, y2, z2);
          DDATA(our_edge) = vertex;

          process_bscwk (curve, our_edge, info, orientation);
        }
      else
        {
#ifdef DEBUG_NOT_IMPLEMENTED
          printf ("WARNING: Unhandled curve geometry type (%s), #%i\n", curve->EntityName (), curve->StepFileId ());
          if (curve->IsComplex())
            {
              printf ("CURVE IS COMPLEX\n");
            }
#endif
          // XXX: line, conic, pcurve, surface_curve, offset_curve_2d, offset_curve_3d, curve_replica
          // XXX: Various derived types of the above, e.g.:
          //      conic is a supertype of: circle, ellipse, hyperbola, parabola

          transform_vertex (info->current_transform, &x1, &y1, &z1);
          transform_vertex (info->current_transform, &x2, &y2, &z2);

          our_edge_info->is_placeholder = true;

          object3d_add_edge (info->object, our_edge);
          vertex = make_vertex3d (x1, y1, z1);
          ODATA(our_edge) = vertex;
          vertex = make_vertex3d (x2, y2, z2);
          DDATA(our_edge) = vertex;

          return;
        }

    }
  else
    {
      printf ("WARNING: found unknown edge type (%s)\n", edge->EntityName ());
      return;
    }
}

static step_model *
process_sr_or_subtype(InstMgr *instance_list, SdaiShape_representation *sr, process_step_info *info);

static step_model *
process_shape_representation(InstMgr *instance_list, SdaiShape_representation *sr, process_step_info *info)
{
  step_model *step_model;
//  std::cout << "INFO: Processing raw SR" << std::endl;

  step_model = g_new0(struct step_model, 1);
//  step_model->filename = g_strdup(filename);
//  step_model->instances = NULL;    /* ??? */

#if 0
  SdaiAxis2_placement_3d *part_origin = find_axis2_placement_3d_in_sr (sr);
  if (part_origin == NULL)
    std::cout << "WARNING: Could not find AXIS2_PLACEMENT_3D entity in SHAPE_REPRESENTATION" << std::endl;

  if (part_origin != NULL)
    {
      step_model->ox = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ())->value;
      step_model->oy = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ()->NextNode ())->value;
      step_model->oz = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;
      step_model->ax = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ())->value;
      step_model->ay = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
      step_model->az = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
      step_model->rx = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ())->value;
      step_model->ry = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
      step_model->rz = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
    }
#endif

  /* We need to find "Shape_representation_relation" linking this SR to another.
   * The SRR could be on its own, or (for assemblies), is likely to be in a complex with
   * "Representation_relationship_with_transformation", in which case the
   * "Representation_Relationship" supertype of Shape_representation_relation is also
   * explicitly in the complex.
   */

  srr_list srr_list;

  // Find all SHAPE_REPRESENTATION_RELATIONSHIP with rep_1 = sr
  find_all_srr_with_rep_1_or_2 (instance_list, &srr_list, 0, sr);

  bool processed_any = false;

  for (srr_list::iterator iter = srr_list.begin (); iter != srr_list.end (); iter++)
    {
      SdaiShape_representation_relationship *srr = (*iter);
//      std::cout << "Found SRR; processing" << std::endl;

      SdaiShape_representation *child_sr = dynamic_cast<SdaiShape_representation *>(srr->rep_2_ ());

      /* Catch the silly case where the model confuses rep_1 and rep_2 */
      if (child_sr == sr)
        child_sr = dynamic_cast<SdaiShape_representation *>(srr->rep_1_ ());

      /* XXX: Actually only want to "process" the SR once per SR, then create _instances_ of it */
      /* XXX: Do something with the result */
      // Leave existing transformation
      process_sr_or_subtype (instance_list, child_sr, info);
      processed_any = true;
    }

  /* Kludge... don't look for the complex transformed relationships if we already found a simple one. */
  if (processed_any)
    return step_model;

  srr_rrwt_list srr_rrwt_list;

  // Find all SHAPE_REPRESENTATION_RELATIONSHIP with rep_1 = sr
  find_all_srr_rrwt_with_rep_1_or_2 (instance_list, &srr_rrwt_list, 0, sr);

  /* XXX: Encountered some models where the child was rep1, the parent rep2??.
   *      E.g. from Samtec, ERM5-075-02.0-L-DV-TR.stp
   */

  /* XXX: Should probably use the NAUO stuff to figure out and follow the assembly structure,
   *      not just guess at what to include based upon finding SHAPE_REPRESENTATION_RELATIONSHIPs.
   */

  for (srr_rrwt_list::iterator iter = srr_rrwt_list.begin (); iter != srr_rrwt_list.end (); iter++)
    {
      double backup_transform[4][4];
      double ox, oy, oz;
      double ax, ay, az;
      double rx, ry, rz;
      SdaiAxis2_placement_3d *parent_axis;
      SdaiAxis2_placement_3d *child_axis;

      srr_rrwt *item = (*iter);
//      std::cout << "Found SRR + RRWT; processing" << std::endl;

      SdaiShape_representation *child_sr = dynamic_cast<SdaiShape_representation *>(item->rep_2);
      SdaiItem_defined_transformation *idt = item->idt;

//      std::cout << "  child SR: #" << child_sr->StepFileId() << " IDT: #" << idt->StepFileId() << std::endl;

      copy_4x4 (info->current_transform, backup_transform);

      if (item->forwards)
        {
          child_axis = dynamic_cast<SdaiAxis2_placement_3d *>(idt->transform_item_1_());
          parent_axis = dynamic_cast<SdaiAxis2_placement_3d *>(idt->transform_item_2_());
        }
      else
        {
          child_axis = dynamic_cast<SdaiAxis2_placement_3d *>(idt->transform_item_2_());
          parent_axis = dynamic_cast<SdaiAxis2_placement_3d *>(idt->transform_item_1_());
        }

      if (parent_axis == NULL ||
          child_axis == NULL)
        {
          std::cout << "ERROR: Got NULL in one of the axis placements for IDT" << std::endl;
          continue;
        }

      unpack_axis_geom (child_axis, &ox, &oy, &oz, &ax, &ay, &az, &rx, &ry, &rz);

#ifdef DEBUG_TRANSFORMS
      printf ("child axis o: (%f, %f, %f)\n"
              "           a: (%f, %f, %f)\n"
              "           r: (%f, %f, %f)\n",
              ox, oy, oz,
              ax, ay, az,
              rx, ry, rz);
#endif

      /* XXX: Looking only at the target vector.. need to find some examples where the parent transform coordinate system is not unity to get this correct */
      rotate_basis (info->current_transform, ax, ay, az, rx, ry, rz);

      /* Is this in the correct order? */
      translate_origin (info->current_transform, ox, oy, oz);

      unpack_axis_geom (parent_axis, &ox, &oy, &oz, &ax, &ay, &az, &rx, &ry, &rz);

#ifdef DEBUG_TRANSFORMS
      printf ("parent axis o: (%f, %f, %f)\n"
              "            a: (%f, %f, %f)\n"
              "            r: (%f, %f, %f)\n",
              ox, oy, oz,
              ax, ay, az,
              rx, ry, rz);
      printf ("\n");
#endif

      rotate_basis_inverted (info->current_transform, ax, ay, az, rx, ry, rz);
      translate_origin (info->current_transform, -ox, -oy, -oz);

      /* XXX: Actually only want to "process" the SR once per SR, then create _instances_ of it */
      /* XXX: Origin offset etc..? */
      /* XXX: Do something with the result */
      process_sr_or_subtype (instance_list, child_sr, info);

      // Revert the transformation
      copy_4x4 (backup_transform, info->current_transform);
    }


  // Find all SRR where RR.rep_1 = sr
  //              let   child_sr = RR.rep_2

  // If SRR node is complex, and also RRWT, extract transform as follows:
  // SdaiTransformation(SdaiSelect) transformation = RRWT.transformation_operator
  // Check transformation.UnderlyingTypeName () == "Item_defined_transformation" - if not, error (don't know how to do Functionally defined tranformation
  // item1 = transform.transform_item_1_() // Axis in parent (use dynamic cast to ensure it is the correct type?)
  // item2 = transform.transform_item_2_() // Axis in child (use dynamic cast to ensure it is the correct type?)
  // If item1 or item2 is NULL, then drop this child?

  // If SRR node was not complex, insert child with 1:1 tranformation

  return step_model;
}

static void
debug_edge (edge_ref edge, const char *message)
{
  edge_ref e = edge;
  edge_info *info;

  printf ("%s\n", message);
  info = (edge_info *)UNDIR_DATA(e);
  printf ("e: %p (%i%s)\n", (void *)e, info->edge_identifier, ((e & 2) == 2) ? "R" : "");
  while ((e = ONEXT(e)) != edge)
    {
      info = (edge_info *)UNDIR_DATA(e);
      printf ("next: %p (%i%s)\n", (void *)e, info->edge_identifier, ((e & 2) == 2) ? "R" : "");
    }
}

static int edge_no = 0;

static step_model *
process_sr_or_subtype(InstMgr *instance_list, SdaiShape_representation *sr, process_step_info *info)
{
  step_model *step_model;
//  object3d *object;
  GHashTable *edges_hash_set;
  int face_count = 0;

  // If sr is an exact match for the step entity SHAPE_REPRESENTATION (not a subclass), call the specific hander
  if (strcmp (sr->EntityName (), "Shape_Representation") == 0)
    {
      return process_shape_representation (instance_list, sr, info);
    }

  if (strcmp (sr->EntityName (), "Advanced_Brep_Shape_Representation") != 0)
    {
      printf ("step_model_to_shape_master: Looking for Advanced_Brep_Shape_Representation, but found %s (which we don't support yet)\n", sr->EntityName ());
      return NULL;
    }

  /* XXX: Solidworks seems to attach a STYLED_ITEM entity to the ABSR, however this is NOT legal AP214,
   *      so STEPcode may not correctly read the STYLED_ITEM instance. (It does type checking of the item
   *      pointed to in the STYLED_ITEM.
   */
  EntityAggregate *illegal_absr_styles = find_styles_for_item (info->styled_items, sr);
  appearance *illegal_absr_appear = NULL;
  if (illegal_absr_styles != NULL)
    {
      printf ("Found style list for ABSR (illegal, but Solidworks does this))\n");
//      destroy_appearance (info->object->appear);
      illegal_absr_appear = make_appearance_for_styles (illegal_absr_styles);
    }

//  object = make_object3d ((char *)"Test");

  step_model = g_new0(struct step_model, 1);
//  step_model->filename = g_strdup(filename);
//  step_model->instances = NULL;    /* ??? */

#if 0
  SdaiAxis2_placement_3d *part_origin = find_axis2_placement_3d_in_sr (sr);
  if (part_origin == NULL)
    std::cout << "WARNING: Could not find AXIS2_PLACEMENT_3D entity in SHAPE_REPRESENTATION" << std::endl;

  if (part_origin != NULL)
    {
      step_model->ox = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ())->value;
      step_model->oy = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ()->NextNode ())->value;
      step_model->oz = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;
      step_model->ax = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ())->value;
      step_model->ay = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
      step_model->az = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
      step_model->rx = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ())->value;
      step_model->ry = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
      step_model->rz = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
    }
#endif

  msb_list msb_list;
  find_manifold_solid_brep_possible_voids (sr, &msb_list);

  for (msb_list::iterator iter = msb_list.begin (); iter != msb_list.end (); iter++)
    {

      /* XXX: Default to the illegal ABSR appearance (Thanks Solidworks!) */
      info->object->appear = illegal_absr_appear;

      EntityAggregate *solid_styles = find_styles_for_item (info->styled_items, (*iter));
      if (solid_styles != NULL)
        {
//          printf ("Found style list for solid\n");
//          destroy_appearance (info->object->appear);
          info->object->appear = make_appearance_for_styles (solid_styles);
#if 0
          printf ("Colour %f, %f, %f\n",
                  (double)info->object->appear->r,
                  (double)info->object->appear->g,
                  (double)info->object->appear->b);
#endif
        }


//      std::cout << "Found MANIFOLD_SOLID_BREP; processing" << std::endl;
      SdaiClosed_shell *cs = (*iter)->outer_ ();

      /* XXX: Need to check if msb is actually an instance of BREP_WITH_VOIDS, whereupon we also need to iterate over the void shell(s) */

//      std::cout << "Closed shell is " << cs << std::endl;

      EntityAggregate *shell_styles = find_styles_for_item (info->styled_items, cs);
      if (shell_styles != NULL)
        {
//          printf ("Found style list for shell\n");
//          destroy_appearance (info->object->appear);
          info->object->appear = make_appearance_for_styles (shell_styles);
#if 0
          printf ("Colour %f, %f, %f\n",
                  (double)info->object->appear->r,
                  (double)info->object->appear->g,
                  (double)info->object->appear->b);
#endif
        }


      /* NB: NULLs give g_direct_hash and g_direct_equal */
      edges_hash_set = g_hash_table_new (NULL, NULL);

      for (SingleLinkNode *iter = cs->cfs_faces_ ()->GetHead ();
           iter != NULL;
           iter = iter->NextNode ())
        {
          SdaiFace *face = (SdaiFace *)((EntityNode *)iter)->node;
          EntityAggregate *face_styles = find_styles_for_item (info->styled_items, face);

          /* XXX: Do we look for specific types of face at this point? (Expect ADVANCED_FACE usually?) */
          if (strcmp (face->EntityName (), "Advanced_Face") != 0)
            {
              printf ("WARNING: Found face of type %s (which we don't support yet)\n", face->EntityName ());
              continue;
            }

          /* NB: ADVANCED_FACE is a FACE_SURFACE, which has SdaiSurface *face_geometry_ (), and Boolean same_sense_ () */
          //SdaiAdvanced_face *af = (SdaiAdvanced_face *) face;
          /* NB: FACE_SURFACE is a FACE, which has EntityAggreate bounds_ (), whos' members are SdaiFace_bound *  */
          SdaiFace_surface *fs = (SdaiFace_surface *) face;

          SdaiSurface *surface = fs->face_geometry_ ();
#if 0
          std::cout << "Face " << face->name_ ().c_str () << " has surface of type " << surface->EntityName () << " and same_sense = " << fs->same_sense_ () << std::endl;
#endif

          info->current_face = make_face3d ((char *)"");
          info->current_face->face_identifier = fs->StepFileId ();
          object3d_add_face (info->object, info->current_face);

          if (face_styles != NULL)
            {
//              printf ("Found style list for face\n");
//              destroy_appearance (info->current_face->appear);
              info->current_face->appear = make_appearance_for_styles (face_styles);
#if 0
              printf ("Colour %f, %f, %f\n",
                      (double)info->current_face->appear->r,
                      (double)info->current_face->appear->g,
                      (double)info->current_face->appear->b);
#endif
               }

          /* XXX: KLUDGE TO FIX THE FACT WE DON'T SUPPORT MULTIPLE BODIES, HENCE KEEP
           *      OVER-WRITING THE VARIOUS PARENT STYLES IN THE object3d EACH TIME WE
           *      ENCOUNTER A NEW SOLID BODY, OR SUB-ASSEMBLY PART
           */
          if (info->current_face->appear == NULL)
            {
//              printf ("Defauting face to solid appearance\n");
              info->current_face->appear = info->object->appear;
#if 0
              printf ("Colour %f, %f, %f\n",
                      (double)info->current_face->appear->r,
                      (double)info->current_face->appear->g,
                      (double)info->current_face->appear->b);
#endif
            }


          if (surface->IsComplex ())
            {
#ifdef DEBUG_NOT_IMPLEMENTED
              printf ("WARNING: Found a STEP Complex entity for our surface (which we don't support yet). Probably a B_SPLINE surface?\n");
#endif
            }
          else if (strcmp (surface->EntityName (), "Plane") == 0)
            {
              SdaiPlane *plane = dynamic_cast<SdaiPlane *>(surface);

//              printf ("WARNING: planar surfaces are not supported yet\n");

              unpack_axis_geom (plane->position_ (),
                                &info->current_face->ox,
                                &info->current_face->oy,
                                &info->current_face->oz,
                                &info->current_face->ax,
                                &info->current_face->ay,
                                &info->current_face->az,
                                &info->current_face->rx,
                                &info->current_face->ry,
                                &info->current_face->rz);

              transform_vertex (info->current_transform,
                                &info->current_face->ox,
                                &info->current_face->oy,
                                &info->current_face->oz);

              transform_vector (info->current_transform,
                                &info->current_face->ax,
                                &info->current_face->ay,
                                &info->current_face->az);

              transform_vector (info->current_transform,
                                &info->current_face->rx,
                                &info->current_face->ry,
                                &info->current_face->rz);

              info->current_face->is_planar = true;

              info->current_face->nx = info->current_face->ax;
              info->current_face->ny = info->current_face->ay;
              info->current_face->nz = info->current_face->az;

              if (!fs->same_sense_ ())
                {
                  info->current_face->nx = -info->current_face->nx;
                  info->current_face->ny = -info->current_face->ny;
                  info->current_face->nz = -info->current_face->nz;

                  /* XXX: Could use face->surface_orientation_reversed ? */
//                  printf ("Not same sense, flipping normal\n");
                }

              info->current_face->surface_orientation_reversed = false;
            }
          else if (strcmp (surface->EntityName (), "Cylindrical_Surface") == 0)
            {
              SdaiCylindrical_surface *cylinder = dynamic_cast<SdaiCylindrical_surface *>(surface);
//              printf ("WARNING: cylindrical surfaces are not supported yet\n");

              unpack_axis_geom (cylinder->position_ (),
                                &info->current_face->ox,
                                &info->current_face->oy,
                                &info->current_face->oz,
                                &info->current_face->ax,
                                &info->current_face->ay,
                                &info->current_face->az,
                                &info->current_face->rx,
                                &info->current_face->ry,
                                &info->current_face->rz);

              transform_vertex (info->current_transform,
                                &info->current_face->ox,
                                &info->current_face->oy,
                                &info->current_face->oz);

              transform_vector (info->current_transform,
                                &info->current_face->ax,
                                &info->current_face->ay,
                                &info->current_face->az);

              transform_vector (info->current_transform,
                                &info->current_face->rx,
                                &info->current_face->ry,
                                &info->current_face->rz);

              info->current_face->is_cylindrical = true;
              info->current_face->radius = cylinder->radius_ ();

              if (fs->same_sense_ ())
                {
                  info->current_face->surface_orientation_reversed = false;
                }
              else
                {
                  info->current_face->surface_orientation_reversed = true;
                }

            }
          else if (strcmp (surface->EntityName (), "Conical_Surface") == 0)
            {
//              printf ("WARNING: conical surfaces are not supported yet\n");
            }
          else if (strcmp (surface->EntityName (), "Toroidal_Surface") == 0)
            {
//              printf ("WARNING: toroidal surfaces are not supported yet\n");
            }
          else if (strcmp (surface->EntityName (), "Spherical_Surface") == 0)
            {
//              printf ("WARNING: spherical surfaces are not supported yet\n");
            }
          else
            {
#ifdef DEBUG_NOT_IMPLEMENTED
              printf ("ERROR: Found an unknown surface type (which we obviously don't support). Surface name is %s\n", surface->EntityName ());
#endif
            }

          for (SingleLinkNode *iter = fs->bounds_ ()->GetHead ();
               iter != NULL;
               iter = iter->NextNode ())
            {
              SdaiFace_bound *fb = (SdaiFace_bound *)((EntityNode *)iter)->node;
              bool face_bound_orientation = fb->orientation_ ();

#if 0
              bool is_outer_bound = (strcmp (fb->EntityName (), "Face_Outer_Bound") == 0);

              if (is_outer_bound)
                std::cout << "  Outer bounds of face include ";
              else
                std::cout << "  Bounds of face include ";
#endif

              // NB: SdaiFace_bound has SdaiLoop *bound_ (), and Boolean orientation_ ()
              // NB: SdaiLoop is a SdaiTopological_representation_item, which is a SdaiRepresentation_item, which has a name_ ().
              // NB: Expect bounds_ () may return a SUBTYPE of SdaiLoop, such as, but not necessarily: SdaiEdge_loop
              SdaiLoop *loop = fb->bound_ ();

#if 0
              std::cout << "loop #" << loop->StepFileId () << ", of type " << loop->EntityName () << ":" << std::endl;
#endif
//              printf ("FACE LOOP\n");
              if (strcmp (loop->EntityName (), "Edge_Loop") == 0)
                {
                  SdaiEdge_loop *el = (SdaiEdge_loop *)loop;
                  edge_ref first_edge_of_contour = 0;
                  edge_ref previous_edge_of_contour = 0;

                  // NB: EDGE_LOOP uses multiple inheritance from LOOP and PATH, thus needs special handling to
                  //     access the elements belonging to PATH, such as edge_list ...
                  //     (Not sure if this is a bug in STEPcode, as the SdaiEdge_loop class DOES define
                  //     an accessor edge_list_ (), yet it appears to return an empty aggregate.

                  char path_entity_name[] = "Path"; /* SDAI_Application_instance::GetMiEntity() should take const char *, but doesn't */
                  SdaiPath *path = (SdaiPath *)el->GetMiEntity (path_entity_name);

                  for (SingleLinkNode *iter = path->edge_list_ ()->GetHead ();
                       iter != NULL;
                       iter = iter->NextNode ())
                    {
                      SdaiOriented_edge *oe = (SdaiOriented_edge *)((EntityNode *)iter)->node;
                      /* XXX: Will it _always?_ be an SdaiOriented_edge? */

                      edge_ref our_edge;

                      // NB: Stepcode does not compute derived attributes, so we need to look at the EDGE
                      //     "edge_element" referred to by the ORIENTED_EDGE, to find the start and end vertices

                      SdaiEdge *edge = oe->edge_element_ ();
                      bool orientation = oe->orientation_ ();

                      if (strcmp (edge->edge_start_ ()->EntityName (), "Vertex_Point") != 0 ||
                          strcmp (edge->edge_end_   ()->EntityName (), "Vertex_Point") != 0)
                        {
                          printf ("WARNING: Edge start and/or end vertices are not specified as VERTEX_POINT\n");
                          continue;
                        }

                      // NB: Assuming edge points to an EDGE, or one of its subtypes that does not make edge_start and edge_end derived attributes.
                      //     In practice, edge should point to an EDGE_CURVE sub-type
                      SdaiVertex_point *edge_start = (SdaiVertex_point *) (orientation ? edge->edge_start_ () : edge->edge_end_ ());
                      SdaiVertex_point *edge_end =  (SdaiVertex_point *) (!orientation ? edge->edge_start_ () : edge->edge_end_ ());

                      // NB: XXX: SdaiVertex_point multiply inherits from vertex and geometric_representation_item

                      SdaiPoint *edge_start_point = edge_start->vertex_geometry_ ();
                      SdaiPoint *edge_end_point = edge_end->vertex_geometry_ ();

                      if (strcmp (edge_start_point->EntityName (), "Cartesian_Point") == 0)
                        {
                          /* HAPPY WITH THIS TYPE */
                        }
                      else
                        {
                          // XXX: point_on_curve, point_on_surface, point_replica, degenerate_pcurve
                          printf ("WARNING: Got Edge start point as unhandled point type (%s)\n", edge_start_point->EntityName ());
                          continue;
                        }

                      if (strcmp (edge_end_point->EntityName (), "Cartesian_Point") == 0)
                        {
                          /* HAPPY WITH THIS TYPE */
                        }
                      else
                        {
                          // XXX: point_on_curve, point_on_surface, point_replica, degenerate_pcurve
                          printf ("WARNING: Got Edge end point as unhandled point type (%s)\n", edge_end_point->EntityName ());
                          continue;
                        }

                      our_edge = (edge_ref)g_hash_table_lookup (edges_hash_set, edge);

                      if (our_edge != 0)
                        {
                          /* Already processed this edge (but hopefully in the other direction!) */
//                          our_edge = SYM(our_edge);
                        }
                      else
                        {
                          our_edge = make_edge ();

                          /* Temporary debug hack */
                          edge_info *our_edge_info = make_edge_info ();
                          our_edge_info->edge_identifier = ++edge_no;
                          UNDIR_DATA(our_edge) = our_edge_info;

                          /* Populate edge geometry
                           *
                           * NB: Forcing orientation to true, so we create the "forward" direction oriented edge
                           *     with the non'SYM'd edge pointer. This lets us spot the reversed edges correctly
                           *     when processing / rendering. (Which we do by spotting the poitner manipulation
                           *     done by SYM on the original make_edge() pointer
                           */
                          process_edge_geometry (edge, true /*orientation*/, our_edge, info);
//                          process_edge_geometry (edge, orientation, our_edge, info);

                          g_hash_table_insert (edges_hash_set, edge, (void *)our_edge);
                        }

#if 1
                      if (!orientation)
                        our_edge = SYM(our_edge);
#endif

                      if (first_edge_of_contour == 0)
                        {
                          if (face_bound_orientation)
                            info->current_contour = make_contour3d (our_edge);
                          else
                            info->current_contour = make_contour3d (SYM(our_edge));
                          face3d_add_contour (info->current_face, info->current_contour);
                          first_edge_of_contour = our_edge;
                        }

#if 0
                      printf ("EDGE: (%f, %f, %f)-(%f, %f, %f)\n",
                              ((vertex3d *)ODATA(our_edge))->x,
                              ((vertex3d *)ODATA(our_edge))->y,
                              ((vertex3d *)ODATA(our_edge))->x,
                              ((vertex3d *)DDATA(our_edge))->x,
                              ((vertex3d *)DDATA(our_edge))->y,
                              ((vertex3d *)DDATA(our_edge))->x);
#endif

                      if (previous_edge_of_contour != 0)
                        {
                          /* Link up the edges around this face contour */
                          if (face_bound_orientation)
                            splice (our_edge, OPREV(SYM(previous_edge_of_contour)));
                          else
                            splice (OPREV(our_edge), SYM(previous_edge_of_contour));
                        }

                      /* Stash reference to this edge for linking next time */
                      previous_edge_of_contour = our_edge;
                    }

                  /* Link up the edges around this face contour */
                  if (face_bound_orientation)
                    splice (first_edge_of_contour, OPREV(SYM(previous_edge_of_contour)));
                  else
                    splice (OPREV(first_edge_of_contour), SYM(previous_edge_of_contour));
                }
              else
                {
                  printf ("WARNING: Face is bounded by an unhandled loop type (%s)\n", loop->EntityName ());
                  continue;
                }
            }
          face_count ++;
        }

        /* Deal with edges hash set */
        g_hash_table_destroy (edges_hash_set);
    }

  mi_list mi_list;
  find_mapped_item (sr, &mi_list);

  for (mi_list::iterator iter = mi_list.begin (); iter != mi_list.end (); iter++)
    {
#ifdef DEBUG_ASSEMBLY_TRAVERSAL
      std::cout << "Found MAPPED_ITEM; processing" << std::endl;
#endif
      SdaiMapped_item *mi = (*iter);

      SdaiRepresentation_map *rm = dynamic_cast<SdaiRepresentation_map *>(mi->mapping_source_());
      SdaiAxis2_placement_3d *mi_axis = dynamic_cast<SdaiAxis2_placement_3d *>(mi->mapping_target_());

      if (rm == NULL)
        {
          std::cout << "ERROR: Could not find REPRESENTATION_ITEM referred to by MAPPED_ITEM" << std::endl;
          continue;
        }

      if (mi_axis == NULL)
        {
          std::cout << "ERROR: Could not find AXIS2_PLACEMENT_3D referred to by MAPPED_ITEM" << std::endl;
          continue;
        }

      SdaiAxis2_placement_3d *rm_axis = dynamic_cast<SdaiAxis2_placement_3d *>(rm->mapping_origin_());
      SdaiShape_representation *child_sr = dynamic_cast<SdaiShape_representation *>(rm->mapped_representation_());

      if (rm_axis == NULL)
        {
          std::cout << "ERROR: Could not find AXIS2_PLACEMENT_3D referred to by REPRESENTATION_ITEM" << std::endl;
          continue;
        }

      if (child_sr == NULL)
        {
          std::cout << "ERROR: Could not find SHAPE_REPRESENTATION referred to by MAPPED_ITEM" << std::endl;
          continue;
        }

#if 0
      /* XXX: Actually only want to "process" the SR once per SR, then create _instances_ of it */
      /* XXX: Origin offset etc..? */
      /* XXX: Do something with the result */
      process_sr_or_subtype (instance_list, child_sr, info);
#endif

      double backup_transform[4][4];
      double ox, oy, oz;
      double ax, ay, az;
      double rx, ry, rz;
      SdaiAxis2_placement_3d *parent_axis = rm_axis;
      SdaiAxis2_placement_3d *child_axis = mi_axis;

      copy_4x4 (info->current_transform, backup_transform);

      if (parent_axis == NULL ||
          child_axis == NULL)
        {
          std::cout << "ERROR: Got NULL in one of the axis placements for IDT" << std::endl;
          continue;
        }

      unpack_axis_geom (child_axis, &ox, &oy, &oz, &ax, &ay, &az, &rx, &ry, &rz);

      /* XXX: Looking only at the target vector.. need to find some examples where the parent transform coordinate system is not unity to get this correct */
      rotate_basis (info->current_transform, ax, ay, az, rx, ry, rz);
      translate_origin (info->current_transform, ox, oy, oz);

      unpack_axis_geom (parent_axis, &ox, &oy, &oz, &ax, &ay, &az, &rx, &ry, &rz);

      translate_origin (info->current_transform, -ox, -oy, -oz);
      rotate_basis_inverted (info->current_transform, ax, ay, az, rx, ry, rz);

      /* XXX: Actually only want to "process" the SR once per SR, then create _instances_ of it */
      /* XXX: Origin offset etc..? */
      /* XXX: Do something with the result */
      process_sr_or_subtype (instance_list, child_sr, info);

      // Revert the transformation
      copy_4x4 (backup_transform, info->current_transform);
    }

  step_model->object = info->object;

  return step_model;
}

extern "C" struct step_model *
step_model_to_shape_master (const char *filename)
{
  step_model *step_model;
  process_step_info info;

//  printf ("step_model_to_shape_master(\"%s\")\n", filename);

  Registry * registry = new Registry (SchemaInit);
  InstMgr * instance_list = new InstMgr (/* ownsInstance = */1);

  // Increment FileId so entities start at #1 instead of #0.
  instance_list->NextFileId();

  SdaiProduct_definition *pd = read_model_from_file (registry, instance_list, filename);
  if (pd == NULL)
    {
      printf ("ERROR Loading STEP model from file '%s'", filename);
      return NULL;
    }

  SdaiShape_definition_representation *sdr = find_sdr_for_pd (instance_list, pd);
  SdaiShape_representation *sr = (SdaiShape_representation *)sdr->used_representation_ ();

  info.object = make_object3d ((char *)"Test");
  identity_4x4 (info.current_transform);

  info.styled_items = build_hash_of_styled_items (instance_list);

  step_model = process_sr_or_subtype (instance_list, sr, &info);

  if (step_model != NULL)
    {
      /* KLUDGE */
      SdaiAxis2_placement_3d *part_origin = find_axis2_placement_3d_in_sr (sr);
      if (part_origin == NULL)
        std::cout << "WARNING: Could not find AXIS2_PLACEMENT_3D entity in SHAPE_REPRESENTATION" << std::endl;

      if (part_origin != NULL)
        {
          step_model->ox = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ())->value;
          step_model->oy = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ()->NextNode ())->value;
          step_model->oz = ((RealNode *)part_origin->location_ ()->coordinates_ ()->GetHead ()->NextNode ()->NextNode ())->value;
          step_model->ax = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ())->value;
          step_model->ay = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
          step_model->az = ((RealNode *)part_origin->axis_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
          step_model->rx = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ())->value;
          step_model->ry = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ())->value;
          step_model->rz = ((RealNode *)part_origin->ref_direction_ ()->direction_ratios_ ()->GetHead ()->NextNode ()->NextNode ())->value;
        }

      /* KLUDGE */
      step_model->object = info.object;
    }
  else
    {
      printf ("WARNING: Got NULL step_model.. must have been some problem loading it\n");
    }

  delete instance_list;
  delete registry;

  return step_model;
}

void step_model_free(step_model *step_model)
{
//  g_list_free (step_model->instances);
//  g_free ((char *)step_model->filename);
  destroy_object3d (step_model->object);
  g_free (step_model);
}

/* Geometry surface and face types encountered so far..

Toroidal_surface     Circle (x5)
Toroidal_surface     Circle (x4)
Toroidal_surface     Circle (x3) + B_Spline_Curve_With_Knots

Cylindrical_surface  Circle + Line + B_Spline_Curve_With_Knots
Cylindrical_surface  Circle + Line

Plane                Circle (xn) + Line (xn) + B_Spline_Curve_With_Knots

*/
