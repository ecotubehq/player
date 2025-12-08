#include <gtk/gtk.h>
#include <glib.h>
#include "celluloid-mpv.h"


typedef struct {
    mpv_handle *mpv;
    gchar *current_video_path;
    gchar *thumbnail_dir;
    int thumbnail_width;
    int thumbnail_height;
    gboolean thumbnails_enabled;
} ThumbnailGenerator;


ThumbnailGenerator* 
ecotube_thumbnail_generator_new(mpv_handle *mpv);

gchar* 
ecotube_thumbnail_generator_generate(ThumbnailGenerator *tg, double time_sec);

void 
ecotube_thumbnail_show(mpv_handle *mpv, const gchar *thumbnail_path, int x, int y);

void 
ecotube_thumbnail_hide(mpv_handle *mpv);

void 
ecotube_thumbnail_generator_set_video(ThumbnailGenerator *tg, const gchar *video_path);

void 
ecotube_thumbnail_generator_free(ThumbnailGenerator *tg);


void 
ecotube_init_thumbnail_system(mpv_handle *mpv);

void 
ecotube_on_video_loaded(mpv_handle *mpv, const gchar *video_path);

void 
ecotube_cleanup_thumbnail_system();

void 
setup_thumbnails(mpv_handle *mpv, const char *video_path);

void 
on_new_video_loaded(const char *video_path);

