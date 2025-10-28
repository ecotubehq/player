#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#include "ecotube-config.h"
#include "celluloid-main-window.h"
#include "celluloid-video-area.h"

char* get_latest_yt_dlp_version(void);
char* get_current_yt_dlp_version(void);
const char* get_yt_dlp_path(void);
int initialize_user_yt_dlp(void);
int update_yt_dlp_with_builtin(CelluloidMainWindow *window);
char* find_system_yt_dlp_path(void);
int is_new_version_available(void);