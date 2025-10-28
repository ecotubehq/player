#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
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