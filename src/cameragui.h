#ifndef _CAMERAGUI_C_
#define _CAMERAGUI_C_

int goto_dialog_obs(GtkWidget *dialog);
int set_obs_object(GtkWidget *dialog, char *objname);
int center_matched_field(GtkWidget *dialog);
void save_frame_auto_name(struct ccd_frame *fr, GtkWidget *dialog);

void test_camera_open(void);
void status_message(GtkWidget *dialog, char *msg);
int capture_image( GtkWidget *window);

void camera_action(GtkAction *action, gpointer data);

#endif
