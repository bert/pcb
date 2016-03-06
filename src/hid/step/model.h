#if 0
typedef struct step_model_instance {
  double x;
  double y;
  double rotation;
} step_model_instance;
#endif

typedef struct step_model {
  const char *filename;
//  GList *instances;
  double ox;
  double oy;
  double oz;
  double ax;
  double ay;
  double az;
  double rx;
  double ry;
  double rz;
  object3d *object;
} step_model;

step_model *step_model_to_shape_master (const char *filename);
void step_model_free (step_model *step_model);
