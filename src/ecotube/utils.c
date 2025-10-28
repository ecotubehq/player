#include "utils.h"


void
load_default_scripts(){

    gchar *scripts[2] = {"modernx.lua", "thumbfast.lua"};

    gchar *src_path = NULL;
    gchar *dest_path = NULL;
    GError *error = NULL;

    gchar *scripts_dir = get_scripts_dir_path();
    for(size_t x=0; x<G_N_ELEMENTS(scripts); x++){
        gchar *modernx_osd = scripts[x];
        gchar *subdir = g_strconcat("ecotube", "/", modernx_osd, NULL);

        src_path  = g_build_filename(DATADIR, subdir, NULL );
        dest_path = g_build_filename(scripts_dir, modernx_osd, NULL );
        g_free(subdir);

        if (access(src_path, F_OK) == 0 && access(dest_path, F_OK) != 0) {
            GFile *src_file  = g_file_new_for_path(src_path);
            GFile *dest_file = g_file_new_for_path(dest_path);
            g_file_copy(g_file_new_for_path(src_path), g_file_new_for_path(dest_path), G_FILE_COPY_NONE, NULL, NULL, NULL, &error );
            if(error){
                g_warning(  "Failed to copy file from '%s' to '%s'. "
                        "Reason: %s",
                        src_path,
                        dest_path,
                        error->message );   
                g_error_free(error);
                error = NULL;   
            }
            g_object_unref(src_file);
            g_object_unref(dest_file);

        }
    }
    g_free(src_path);
    g_free(dest_path);

}


gboolean
is_modern_osd(){
    gchar *modernx_osd = "modernx.lua";
    gchar *scripts_dir = get_scripts_dir_path();

    gchar *script_path = g_build_filename(scripts_dir, modernx_osd, NULL );
    if (access(script_path, F_OK) == 0) {
        return TRUE;
    }

    return FALSE;
}