typedef struct {
  float r, g, b;
} appearance;

appearance *make_appearance (void);
void destroy_appearance (appearance *appearance);
void appearance_set_color (appearance *appearance, float r, float g, float b);
