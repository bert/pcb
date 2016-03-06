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


typedef struct srr_rrwt {
  SdaiRepresentation *rep_2;
  SdaiItem_defined_transformation *idt;
} srr_rrwt;

typedef std::list<SdaiProduct_definition *> pd_list;
typedef std::list<SdaiShape_representation_relationship *> srr_list;
typedef std::list<srr_rrwt *> srr_rrwt_list;


void find_all_pd_with_sdr (InstMgr *instance_list, pd_list *pd_list, int start_after_id);

/* entityName should be the name of entity Assembly_component_usage or one of its subtypes
 * typically this will be "Assembly_component_usage" or "Next_assembly_usage_occurance"
 */
void find_and_remove_child_pd (InstMgr *instance_list, pd_list *pd_list, int start_after_id, const char *entityName);
void find_and_remove_child_pd_mi_rm_sr (InstMgr *instance_list, pd_list *pd_list, int start_after_id);

SdaiShape_definition_representation *find_sdr_for_pd (InstMgr *instance_list, SdaiProduct_definition *target_pd);

SdaiShape_representation *find_sr_for_pd (InstMgr *instance_list, SdaiProduct_definition *target_pd);

SdaiAxis2_placement_3d *find_axis2_placement_3d_in_sr (SdaiShape_representation *sr);

void find_all_srr_with_rep_1( InstMgr *instance_list, srr_list *srr_list, int start_after_id, SdaiRepresentation *rep_1);
void find_all_srr_rrwt_with_rep_1( InstMgr *instance_list, srr_rrwt_list *srr_rrwt_list, int start_after_id, SdaiRepresentation *rep_1);
