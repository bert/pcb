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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if 0
#  define DEBUG_PRODUCT_DEFINITION_SEARCH
#  define DEBUG_CHILD_REMOVAL
#  define DEBUG_PRODUCT_DEFINITION
#  define DEBUG_SHAPE_REPRESENTATION_RELATIONSHIP_SEARCH
#else
#  undef DEBUG_PRODUCT_DEFINITION_SEARCH
#  undef DEBUG_CHILD_REMOVAL
#  undef DEBUG_PRODUCT_DEFINITION
#  undef DEBUG_SHAPE_REPRESENTATION_RELATIONSHIP_SEARCH
#endif

#include <glib.h>


void
find_all_pd_with_sdr (InstMgr *instance_list, pd_list *pd_list, int start_after_id)
{
  MgrNode * mnode = instance_list->FindFileId (start_after_id);
  int search_index;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  // Loop over the instances of SHAPE_DEFITION_REPRESENTATION in the file
  SdaiShape_definition_representation *sdr;
  while (ENTITY_NULL != (sdr = (SdaiShape_definition_representation *)
                               instance_list->GetApplication_instance ("Shape_definition_representation", search_index)))
    {
      SdaiRepresented_definition *sdr_definition = sdr->definition_ ();
      SdaiProduct_definition_shape *pds = (SdaiProduct_definition_shape *)(SdaiProperty_definition_ptr)(*sdr_definition);
      SdaiProduct_definition *pd;

      if (pds == NULL)
        {
          fprintf (stderr, "pds == NULL in find_all_pd_with_sdr\n");
          return;
        }

      pd = *(SdaiCharacterized_product_definition_ptr)(*pds->definition_ ());

      pd_list->push_back (pd);

#ifdef DEBUG_PRODUCT_DEFINITION_SEARCH
      STEPentity *sdr_used_representation = sdr->used_representation_ ();
      SdaiProduct_definition_formation *pdf = pd->formation_ ();
      SdaiProduct *p = pdf->of_product_ ();

      std::cout << "Got a SDR, #" << sdr->StepFileId ();
      std::cout << " used_representation (sr or descendant) = #" << sdr_used_representation->StepFileId ();
      std::cout << " definition (pds) = #" << pds->StepFileId ();
      std::cout << " pds->definition (pd) = #" << pd->StepFileId ();
      std::cout << " pd->formation (pdf) = #" << pdf->StepFileId ();
      std::cout << " pdf->product (p) = #" << p->StepFileId ();
      std::cout << std::endl;
      std::cout << "Product id = " << p->id_ ().c_str () << " name = " << p->name_ ().c_str ();
      std::cout << std::endl;
#endif

#ifdef DEBUG_PRODUCT_DEFINITION_SEARCH
      SdaiShape_representation *sr = (SdaiShape_representation *)sdr_used_representation;

      std::cout << "SR is actually of type " << ((STEPentity *)sr)->EntityName () << std::endl;
      std::cout << std::endl;
#endif

      int id = sdr->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }
}

/* entityName should be the name of entity Assembly_component_usage or one of its subtypes
 * typically this will be "Assembly_component_usage" or "Next_assembly_usage_occurance"
 */
void
find_and_remove_child_pd (InstMgr *instance_list, pd_list *pd_list, int start_after_id, const char *entityName)
{
  MgrNode * mnode = instance_list->FindFileId (start_after_id);
  int search_index;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  SdaiAssembly_component_usage *acu;
  while (ENTITY_NULL != (acu = (SdaiAssembly_component_usage *)
                               instance_list->GetApplication_instance (entityName, search_index)))
    {
      SdaiProduct_definition *related_pd = acu->related_product_definition_ ();

#ifdef DEBUG_CHILD_REMOVAL
      SdaiProduct_definition *relating_pd = acu->relating_product_definition_ ();

      std::cout << "Product " << related_pd->formation_ ()->of_product_ ()->id_ ().c_str ();
      std::cout << " is a child of " << relating_pd->formation_ ()->of_product_ ()->id_ ().c_str ();
      std::cout << ".. removing it from list of possible root products";
      std::cout << std::endl;
#endif

      /* Remove related_pd from the list of viable product definitions */
      pd_list->remove (related_pd);

      int id = acu->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }
#ifdef DEBUG_CHILD_REMOVAL
  std::cout << std::endl;
#endif
}

static SdaiProduct_definition *
find_pd_for_sr (InstMgr *instance_list, int start_after_id, SdaiShape_representation *target_sr)
{
  MgrNode * mnode = instance_list->FindFileId (start_after_id);
  int search_index;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  // Loop over the instances of SHAPE_DEFITION_REPRESENTATION in the file
  SdaiShape_definition_representation *sdr;
  while (ENTITY_NULL != (sdr = (SdaiShape_definition_representation *)
                               instance_list->GetApplication_instance ("Shape_definition_representation", search_index)))
    {
      SdaiShape_representation *sr = (SdaiShape_representation *)sdr->used_representation_ ();
      SdaiProduct_definition_shape *pds = (SdaiProduct_definition_shape *)(SdaiProperty_definition_ptr)(*sdr->definition_ ());
      SdaiProduct_definition *pd = *(SdaiCharacterized_product_definition_ptr)(*pds->definition_ ());

      if (sr == target_sr)
        return pd;

      int id = sdr->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }

  return NULL;
}

void
find_and_remove_child_pd_mi_rm_sr (InstMgr *instance_list, pd_list *pd_list, int start_after_id)
{
  MgrNode * mnode = instance_list->FindFileId (start_after_id);
  int search_index;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  SdaiMapped_item *mi;
  while (ENTITY_NULL != (mi = (SdaiMapped_item *)
                               instance_list->GetApplication_instance ("Mapped_item", search_index)))
    {
      SdaiRepresentation_map *mapping_source = mi->mapping_source_ ();
//      SdaiRepresentation_item *mapping_item = mi->mapping_item_ (); // E.g. an axis

//      SdaiRepresentation_item *mapping_origin = mapping_source->mapping_origin_ (); // <- Eg. an axis
      SdaiRepresentation *mapped_representation = mapping_source->mapped_representation_ (); // <- Shape representation of the product which is a child

      SdaiProduct_definition *child_pd = find_pd_for_sr (instance_list, start_after_id, (SdaiShape_representation *)mapped_representation);
      /* Need to find product_definition which has PD<-PDS.definition_<-SDR.definition_SDR.used_representation->SR */

//      SdaiProduct_definition *related_pd = mi->related_product_definition_ ();

#ifdef DEBUG_CHILD_REMOVAL
//      SdaiProduct_definition *relating_pd = acu->relating_product_definition_ ();

      std::cout << "Product " << child_pd->formation_ ()->of_product_ ()->id_ ().c_str ();
      std::cout << " is a child of " << "???"; // << relating_pd->formation_ ()->of_product_ ()->id_ ().c_str ();
      std::cout << ".. removing it from list of possible root products";
      std::cout << std::endl;
#endif

      /* Remove related_pd from the list of viable product definitions */
      pd_list->remove (child_pd);

      int id = mi->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }
#ifdef DEBUG_CHILD_REMOVAL
  std::cout << std::endl;
#endif
}

SdaiShape_definition_representation *
find_sdr_for_pd (InstMgr *instance_list, SdaiProduct_definition *target_pd)
{
  int search_index = 0;

  // Loop over the instances of SHAPE_DEFITION_REPRESENTATION in the file
  SdaiShape_definition_representation *sdr;
  while (ENTITY_NULL != (sdr = (SdaiShape_definition_representation *)
                               instance_list->GetApplication_instance ("Shape_definition_representation", search_index)))
    {
      SdaiProduct_definition_shape *pds = (SdaiProduct_definition_shape *)(SdaiProperty_definition_ptr)(*sdr->definition_ ());
      SdaiProduct_definition *pd = *(SdaiCharacterized_product_definition_ptr)(*pds->definition_ ());

      /* Return the SHAPE_REPRESETATION (or subclass) associated with the first SHAPE_DEFINITION_REPRESENTATION for the required PRODUCT_DEFINITION */
      if (pd == target_pd)
        return sdr;
//        return (SdaiShape_representation *)sdr->used_representation_ ();

      int id = sdr->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }

  return NULL;
}


SdaiShape_representation *
find_sr_for_pd (InstMgr *instance_list, SdaiProduct_definition *target_pd)
{
  SdaiShape_definition_representation *sdr = find_sdr_for_pd (instance_list, target_pd);
  return (SdaiShape_representation *)sdr->used_representation_ ();
}


SdaiAxis2_placement_3d *
find_axis2_placement_3d_in_sr (SdaiShape_representation *sr)
{
  SingleLinkNode *iter = sr->items_ ()->GetHead ();

  while (iter != NULL)
    {
      SDAI_Application_instance *node = ((EntityNode *)iter)->node;

      if (strcmp (node->EntityName (), "Axis2_Placement_3d") == 0)
        return (SdaiAxis2_placement_3d *)node;

      iter = iter->NextNode ();
    }

  return NULL;
}

void
find_all_srr_with_rep_1_or_2( InstMgr *instance_list, srr_list *srr_list, int start_after_id, SdaiRepresentation *desired)
{
  MgrNode * mnode = instance_list->FindFileId (start_after_id);
  int search_index;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  // Loop over the instances of SHAPE_REPRESENTATION_RELATIONSHIP in the file
  SdaiShape_representation_relationship *srr;
  while (ENTITY_NULL != (srr = (SdaiShape_representation_relationship *)
                               instance_list->GetApplication_instance ("Shape_representation_relationship", search_index)))
    {
      SdaiRepresentation *found_rep_1 = srr->rep_1_ ();
//#ifdef DEBUG_SHAPE_REPRESENTATION_RELATIONSHIP_SEARCH
      SdaiRepresentation *found_rep_2 = srr->rep_2_ ();
//#endif

      if (srr->IsComplex())
        {
          std::cout << "ERROR: Found a complex SRR when we were expecting a simplex" << std::endl;
          return;
        }

      if (found_rep_1 == desired ||
          found_rep_2 == desired)
        srr_list->push_back (srr);

#ifdef DEBUG_SHAPE_REPRESENTATION_RELATIONSHIP_SEARCH
      std::cout << "Got a SRR, #" << srr->StepFileId ();
      std::cout << " rep_1 = #" << found_rep_1->StepFileId ();
      std::cout << " rep_2 = #" << found_rep_2->StepFileId ();
      std::cout << std::endl;
#endif

      int id = srr->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }
}

void
find_all_srr_rrwt_with_rep_1_or_2( InstMgr *instance_list, srr_rrwt_list *srr_rrwt_list, int start_after_id, SdaiRepresentation *desired)
{
  MgrNode * mnode = instance_list->FindFileId (start_after_id);
  int search_index;
  SDAI_Application_instance *entity;

  if (mnode == NULL)
    search_index = 0;
  else
    search_index = instance_list->GetIndex (mnode) + 1;

  while (ENTITY_NULL != (entity = instance_list->GetApplication_instance ("Representation_relationship", search_index)))
    {
      STEPcomplex *stepcomplex;
      STEPattribute *attr;
      SdaiRepresentation *found_rep_1 = NULL;
      SdaiRepresentation *found_rep_2 = NULL;
      SdaiTransformation *transform = NULL;
      SdaiItem_defined_transformation *idt = NULL;

      bool found_srr = false;
      bool found_rrwt = false;

      if (!entity->IsComplex())
        {
          std::cout << "WARNING: Found a non-complex Representation_relationship when looking for SRR + RRWT" << std::endl;
          continue;
        }

      stepcomplex = dynamic_cast<STEPcomplex *>(entity)->head;

      /* Assume the first is always the RR */
      stepcomplex->ResetAttributes ();
      while ((attr = stepcomplex->NextAttribute()) != NULL) {
          if (!strcmp (attr->Name (), "rep_1"))
            found_rep_1 = dynamic_cast<SdaiRepresentation *>(attr->Entity ());
          if (!strcmp (attr->Name (), "rep_2"))
            found_rep_2 = dynamic_cast<SdaiRepresentation *>(attr->Entity ());
      }

      while (stepcomplex) {
          if (stepcomplex->EntityName() == NULL)
            {
              std::cout << "ERROR: NULL whilst traversing complex" << std::endl;
              return;
            }
          else if (!strcmp (stepcomplex->EntityName (), "Shape_Representation_Relationship"))
            {
              found_srr = true;
            }
          else if (!strcmp (stepcomplex->EntityName (), "Representation_Relationship_With_Transformation"))
            {
              found_rrwt = true;

              stepcomplex->ResetAttributes ();
              while ((attr = stepcomplex->NextAttribute()) != NULL)
                {
                  if (!strcmp (attr->Name (), "transformation_operator"))
                    {
                      transform = dynamic_cast<SdaiTransformation *>(attr->Select ());
                      idt = *transform;
                    }
                }
            }
          stepcomplex = stepcomplex->sc;
      }

#ifdef DEBUG_SHAPE_REPRESENTATION_RELATIONSHIP_SEARCH
      std::cout << "GOT A complex, #" << entity->StepFileId () << ":" << std::endl;
      std::cout << "    RR";

      if (found_rep_1 != NULL)
        std::cout << " rep_1 = #" << found_rep_1->StepFileId ();
      else
        std::cout << " rep_1 = NIL";

      if (found_rep_2 != NULL)
        std::cout << " rep_2 = #" << found_rep_2->StepFileId ();
      else
        std::cout << " rep_2 = NIL";

      std::cout << std::endl;

      if (found_srr)
        std::cout << "    SRR" << std::endl;

      if (found_rrwt)
        {
          std::cout << "    RRWT, transform = ";
          if (transform != NULL)
            std::cout << "#" << idt->StepFileId () << std::endl;
          else
            std::cout << "NIL" << std::endl;
        }

      std::cout << std::endl;
#endif

#if 1
      if (found_rep_1 == desired &&
          found_srr &&
          found_rrwt)
        {
          srr_rrwt *item = new srr_rrwt;

          item->rep_2 = found_rep_2;
          item->idt = idt;
          item->forwards = true;

          srr_rrwt_list->push_back (item);
        }
#endif

#if 1
      /* Model is confused about rep1 / rep2 case */
      if (found_rep_2 == desired &&
          found_srr &&
          found_rrwt)
        {
          srr_rrwt *item = new srr_rrwt;

          item->rep_2 = found_rep_1;
          item->idt = idt;
          item->forwards = false;

          srr_rrwt_list->push_back (item);
        }
#endif

      int id = entity->StepFileId ();
      MgrNode * mnode = instance_list->FindFileId (id);
      search_index = instance_list->GetIndex (mnode) + 1;
    }

}
