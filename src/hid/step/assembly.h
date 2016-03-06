struct assembly_model {
  const char *filename;
  GList *instances;
  step_model *step_model;
};

struct assembly_model_instance {
  struct assembly_model *model;
  const char *name;
//  double ox;
//  double oy;
//  double rotation;
  double ox;
  double oy;
  double oz;
  double ax;
  double ay;
  double az;
  double rx;
  double ry;
  double rz;
};


void export_step_assembly (const char *filename, GList *assembly_models);
