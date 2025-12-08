#include "thumbnailer.h"
#include "ecotube-config.h"
#include <unistd.h>
#include <sys/stat.h>

static int file_ready(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && st.st_size > 0);
}

static char *flatpak_safe_path(void) {
    const char *home = g_get_home_dir();
    return g_build_filename(DATADIR, "thumb.png", NULL);
}

/* Worker thread */
static void task_worker(GTask *task,
                        gpointer source_ptr,
                        gpointer task_data,
                        GCancellable *cancellable)
{
    CelluloidMpv *mpv = source_ptr;
    double ts = ((double*)task_data)[0];
    int size  = ((int*)task_data)[1];

    char *dest = flatpak_safe_path();
    unlink(dest);

    /* Seek */
    char timebuf[64];
    snprintf(timebuf, sizeof(timebuf), "%.6f", ts);

    const char *seek_cmd[] = { "seek", timebuf, "absolute", "exact", NULL };
    celluloid_mpv_command(mpv, seek_cmd);

    /* Let mpv render frame */
    double deadline = g_get_monotonic_time()/1e6 + 2.0;
    while (g_get_monotonic_time()/1e6 < deadline) {
        mpv_node node;
        if (celluloid_mpv_get_property(mpv, "time-pos", MPV_FORMAT_NODE, &node) == 0) {
            double cur = 0.0;
            if (node.format == MPV_FORMAT_DOUBLE) cur = node.u.double_;
            mpv_free_node_contents(&node);
            if (fabs(cur - ts) < 0.07) break;
        }
        usleep(40000);
    }

    /* Screenshot */
    const char *sc_cmd[] = { "screenshot-to-file", dest, "video", NULL };
    celluloid_mpv_command(mpv, sc_cmd);

    /* Wait for file */
    deadline = g_get_monotonic_time()/1e6 + 2.0;
    while (g_get_monotonic_time()/1e6 < deadline) {
        if (file_ready(dest)) break;
        usleep(30000);
    }

    if (!file_ready(dest)) {
        g_free(dest);
        g_task_return_pointer(task, NULL, NULL);
        return;
    }

    /* Load pixbuf */
    GError *err = NULL;
    GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_scale(dest, -1, size, TRUE, &err);
    g_free(dest);

    if (!pix) {
        g_warning("thumbnail: %s", err->message);
        g_clear_error(&err);
        g_task_return_pointer(task, NULL, NULL);
        return;
    }

    g_task_return_pointer(task, pix, g_object_unref);
}

static void thumbnailer_on_async_ready(GObject *source_obj,
                                       GAsyncResult *res,
                                       gpointer user_data)
{
    GTask *task = G_TASK(res);

    GdkPixbuf *pix = g_task_propagate_pointer(task, NULL);

    void (*callback)(GdkPixbuf*, void*) =
        g_object_get_data(G_OBJECT(task), "cb");

    void *ud = g_object_get_data(G_OBJECT(task), "ud");

    if (callback)
        callback(pix, ud);
}

/* API */
void thumbnailer_generate_async(
        CelluloidMpv *mpv,
        const char *filepath,
        double timestamp,
        int size,
        void (*callback)(GdkPixbuf*, void*),
        void *user_data)
{

    GTask *task = g_task_new(mpv, NULL, thumbnailer_on_async_ready, NULL);

    /* bundle arguments */
    double *args = g_new(double, 2);
    args[0] = timestamp;
    ((int*)args)[2] = size;   // same memory block

    g_task_set_task_data(task, args, g_free);

    /* store callback userdata */
    g_object_set_data(G_OBJECT(task), "cb", callback);
    g_object_set_data(G_OBJECT(task), "ud", user_data);

    /* run thumbnail worker thread */
    g_task_run_in_thread(task, task_worker);

    g_object_unref(task);
}

