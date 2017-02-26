
struct assembly_model_instance {
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


struct assembly_model {
  const char *filename;
  GList *instances;
};

void export_step_assembly (const char *filename, GList *assembly_models);
