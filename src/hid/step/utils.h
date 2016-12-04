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


typedef std::list<SdaiProduct_definition *> pd_list;


void find_all_pd_with_sdr (InstMgr *instance_list, pd_list *pd_list);

/* entityName should be the name of entity Assembly_component_usage or one of its subtypes
 * typically this will be "Assembly_component_usage" or "Next_assembly_usage_occurance"
 */
void find_and_remove_child_pd (InstMgr *instance_list, pd_list *pd_list, const char *entityName);
void find_and_remove_child_pd_mi_rm_sr (InstMgr *instance_list, pd_list *pd_list);

SdaiShape_definition_representation *find_sdr_for_pd (InstMgr *instance_list, SdaiProduct_definition *target_pd);

SdaiShape_representation *find_sr_for_pd (InstMgr *instance_list, SdaiProduct_definition *target_pd);

SdaiAxis2_placement_3d *find_axis2_placement_3d_in_sr (SdaiShape_representation *sr);
