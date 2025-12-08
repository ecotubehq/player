#include "ecotube-thumbnail.h"
#include "ecotube-config.h"


ThumbnailGenerator* 
ecotube_thumbnail_generator_new(mpv_handle *mpv){

    ThumbnailGenerator *tg = g_new0(ThumbnailGenerator, 1);
    tg->mpv = mpv;
    tg->thumbnail_width = 200;
    tg->thumbnail_height = 150;
    tg->thumbnails_enabled = TRUE;
    
    // Create thumbnail directory
    tg->thumbnail_dir = g_build_filename(DATADIR, "thumbnails", NULL); 
    g_mkdir_with_parents(tg->thumbnail_dir, 0755);
    
    return tg;
}


// Generate thumbnail at specific time
gchar* 
ecotube_thumbnail_generator_generate(ThumbnailGenerator *tg, double time_sec) {
    if (!tg->current_video_path) {
        g_warning("No current video path set");
        return NULL;
    }
    
    // Create unique filename based on time and video
    gchar *video_basename = g_path_get_basename(tg->current_video_path);
    gchar *filename = g_strdup_printf("thumb_%s_%.0f.png", video_basename, time_sec);
    gchar *full_path = g_build_filename(tg->thumbnail_dir, filename, NULL);
    
    g_free(video_basename);
    g_free(filename);
    
    GError *error = NULL;
    gchar *command = g_strdup_printf(
        "ffmpeg -ss %.2f -i \"%s\" -vframes 1 -s %dx%d \"%s\" -y",
        time_sec, tg->current_video_path, 
        tg->thumbnail_width, tg->thumbnail_height,
        full_path
    );
    
    gboolean success = g_spawn_command_line_sync(command, NULL, NULL, NULL, &error);
    g_free(command);
    
    if (!success) {
        g_warning("Failed to generate thumbnail: %s", error->message);
        g_error_free(error);
        g_free(full_path);
        return NULL;
    }
    
    return full_path;
}

void 
ecotube_thumbnail_show(mpv_handle *mpv, const gchar *thumbnail_path, int x, int y) {
    if (!thumbnail_path || !g_file_test(thumbnail_path, G_FILE_TEST_EXISTS)) {
        g_warning("Thumbnail file does not exist: %s", thumbnail_path);
        return;
    }
    
    // Send command to mpv to show the thumbnail
    const char *cmd[] = {
        "script-message", "thumbfast_display", "show",
        thumbnail_path,
        g_strdup_printf("%d", x),
        g_strdup_printf("%d", y),
        NULL
    };
    
    mpv_command_async(mpv, 0, cmd);
}


void 
ecotube_thumbnail_hide(mpv_handle *mpv) {
    const char *cmd[] = {
        "script-message", "thumbfast_display", "hide", NULL
    };
    mpv_command_async(mpv, 0, cmd);
}

void 
ecotube_thumbnail_generator_set_video(ThumbnailGenerator *tg, const gchar *video_path) {
    g_free(tg->current_video_path);
    tg->current_video_path = g_strdup(video_path);
}


void 
ecotube_thumbnail_generator_free(ThumbnailGenerator *tg) {
    g_free(tg->current_video_path);
    g_free(tg->thumbnail_dir);
    g_free(tg);
}

