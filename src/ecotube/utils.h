#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "celluloid-common.h"

#include "ecotube-config.h"

void
load_default_scripts(void);

gboolean
is_modern_osd(void);

gboolean 
is_plugged(void);

gboolean
is_laptop(void);

void
display_pdf (GtkApplication *app, gpointer user_data);

void 
replace_in_line(char *line, const char *old_str, const char *new_str, FILE *out);

int 
find_and_replace_file(const char *filename, const char *old_str, const char *new_str);
