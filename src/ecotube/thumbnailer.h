#ifndef THUMBNAILER_H
#define THUMBNAILER_H

#include "celluloid-mpv.h"
#include <gdk-pixbuf/gdk-pixbuf.h>


void thumbnailer_generate_async(
        CelluloidMpv *mpv,
        const char *filepath,
        double timestamp,
        int size,
        void (*callback)(GdkPixbuf *pixbuf, void *user_data),
        void *user_data );

#endif
