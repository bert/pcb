typedef struct {
  float r, g, b;
  float a;
} appearance;

appearance *make_appearance (void);
void destroy_appearance (appearance *appear);
void appearance_set_color (appearance *appear, float r, float g, float b);
void appearance_set_alpha (appearance *appear, float a);
void appearance_set_appearance (appearance *appear, const appearance *from);
void appearance_apply_gl (appearance *appear);
