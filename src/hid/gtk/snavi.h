GIOChannel *setup_snavi(void (*update_pan)(int dx, int dy, int dz, gpointer data),
                        void (*update_roll)(int dx, int dy, int dz, gpointer data),
                        void (*update_done)(gpointer data),
                        void (*button)(int button, int value, gpointer data),
                        gpointer data);
void snavi_set_led (GIOChannel *snavi, int value);
