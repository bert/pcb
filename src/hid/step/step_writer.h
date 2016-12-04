typedef int step_id;

typedef GList* step_id_list;

typedef struct {
  FILE *f;
  step_id next_id;
  GHashTable *cartesian_point_hash;
  GHashTable *direction_hash;
  GHashTable *vector_hash;
  GHashTable *axis2_hash;
  GHashTable *colour_hash;
  GHashTable *cylindrical_hash;
  GHashTable *circle_hash;
} step_file;

step_id_list step_id_list_append (step_id_list list, step_id id);
step_id_list make_step_id_list (int count, ...);

step_file *step_output_file (FILE *f);
void destroy_step_output_file (step_file *file);

step_id make_3d_metric_step_geometric_representation_context (step_file *file); /* XXX: Just reading out boiler-plate at this point */

step_id step_application_context (step_file *file, char *application);
step_id step_application_protocol_definition (step_file *file, char *status, char *application_interpreted_model_schema_name, char *application_protocol_year, step_id application);
step_id step_product_context (step_file *file, char *name, step_id frame_of_reference, char *discipline_type);
step_id step_product (step_file *file, char *id, char *name, char *description, step_id_list frame_of_reference);
step_id step_product_related_product_category (step_file *file, char *name, char *description, step_id_list products);
step_id step_product_definition_context (step_file *file, char *name, step_id frame_of_reference, char *life_cycle_stage);
step_id step_product_definition_formation (step_file *file, char *id, char *description, step_id of_product);
step_id step_product_definition (step_file *file, char *id, char *description, step_id formation, step_id frame_of_reference);
step_id step_product_definition_shape (step_file *file, char *name, char *description, step_id definition);

step_id step_cartesian_point (step_file *file, char *name, double x, double y, double z);
step_id step_direction (step_file *file, char *name, double x, double y, double z);
step_id step_axis2_placement_3d (step_file *file, char *name, step_id location, step_id axis, step_id ref_direction);
step_id step_plane (step_file *file, char *name, step_id position);
step_id step_cylindrical_surface (step_file *file, char *name, step_id position, double radius);
step_id step_circle (step_file *file, char *name, step_id position, double radius);
step_id step_vector (step_file *file, char *name, step_id orientation, double magnitude);
step_id step_line (step_file *file, char *name, step_id pnt, step_id dir);
step_id step_vertex_point (step_file *file, char *name, step_id pnt);
step_id step_edge_curve (step_file *file, char *name, step_id edge_start, step_id edge_end, step_id edge_geometry, bool same_sense);
step_id step_oriented_edge (step_file *file, char *name, step_id edge_element, bool orientation);
step_id step_edge_loop (step_file *file, char *name, step_id_list edge_list);
step_id step_face_bound (step_file *file, char *name, step_id bound, bool orientation);
step_id step_face_outer_bound (step_file *file, char *name, step_id bound, bool orientation);
step_id step_advanced_face (step_file *file, char *name, step_id_list bounds, step_id face_geometry, bool same_sense);
step_id step_closed_shell (step_file *file, char *name, step_id_list cfs_faces);
step_id step_manifold_solid_brep (step_file *file, char *name, step_id outer);
step_id step_advanced_brep_shape_representation (step_file *file, char *name, step_id_list items, step_id context_of_items);
step_id step_shape_definition_representation (step_file *file, step_id definition, step_id used_representation);

step_id step_colour_rgb (step_file *file, char *name, double red, double green, double blue);
step_id step_fill_area_style_colour (step_file *file, char *name, step_id fill_colour);
step_id step_fill_area_style (step_file *file, char *name, step_id_list fill_styles);
step_id step_surface_style_fill_area (step_file *file, step_id fill_area);
step_id step_surface_side_style (step_file *file, char *name, step_id_list styles);
step_id step_surface_style_usage (step_file *file, char *surface_side, step_id style); /* XXX: surface_side should be an enum "POSITIVE, NEGATIVE or BOTH" */
step_id step_presentation_style_assignment (step_file *file, step_id_list styles);
step_id step_styled_item (step_file *file, char *name, step_id_list styles, step_id item);
step_id step_over_riding_styled_item (step_file *file, char *name, step_id_list styles, step_id item, step_id over_ridden_style);
step_id step_presentation_layer_assignment (step_file *file, char *name, char *description, step_id_list assigned_items);
step_id step_mechanical_design_geometric_presentation_representation (step_file *file, char *name, step_id_list items, step_id context_of_items);
