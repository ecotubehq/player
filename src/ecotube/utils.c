#include "utils.h"


void
load_default_scripts(){

    gchar *scripts_dir = get_scripts_dir_path();
    gchar *script_opts_dir = get_script_opts_dir_path();
    gchar *script_font_dir = get_script_fonts_dir_path();


    // remove modernx script
    gchar *modernx_path = g_build_filename(scripts_dir, "modernx.lua", NULL );
    if(access(modernx_path, F_OK) == 0){
        GError *error = NULL;
        GFile *modernx_file = g_file_new_for_path(modernx_path);
        if(!g_file_delete(modernx_file, NULL, &error)){
            if (error) {
                g_warning("Failed to delete file: %s\n", error->message);
                g_error_free(error);
            } else {
                g_warning("Unknown error occurred while deleting file.\n");
            }
        }else{
            g_warning("File deleted successfully.\n");
        }
        g_object_unref(modernx_file);
    }

    struct {
        char *script;
        char *script_path;
    } scripts [] = {
        {"modernz.lua", scripts_dir},
        {"thumbfast.lua", scripts_dir},
        {"modernz.conf", script_opts_dir},
        {"fluent-system-icons.ttf", script_font_dir},
        {"material-design-icons.ttf", script_font_dir},
        {"mpv-reload.lua", scripts_dir},
        {NULL, NULL}
    };

    gchar *src_path = NULL;
    gchar *dest_path = NULL;
    GError *error = NULL;

    
    for(size_t x=0; x<G_N_ELEMENTS(scripts); x++){
        gchar *script = scripts[x].script;
        if(!script){
            continue;
        }
        gchar *script_path = scripts[x].script_path;

        gchar *subdir = g_strconcat("ecotube", "/", script, NULL);

        src_path  = g_build_filename(DATADIR, subdir, NULL );
        dest_path = g_build_filename(script_path, script, NULL );
        g_free(subdir);

        if (access(src_path, F_OK) == 0) {
            GFile *src_file  = g_file_new_for_path(src_path);
            GFile *dest_file = g_file_new_for_path(dest_path);
            g_file_copy(g_file_new_for_path(src_path), g_file_new_for_path(dest_path), G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error );
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
    g_free(script_opts_dir);
    g_free(script_font_dir);

}


gboolean
is_modern_osd(){
    gchar *modernx_osd = "modernz.lua";
    gchar *scripts_dir = get_scripts_dir_path();

    gchar *script_path = g_build_filename(scripts_dir, modernx_osd, NULL );
    if (access(script_path, F_OK) == 0) {
        return TRUE;
    }

    return FALSE;
}

gboolean 
is_plugged(void){
    FILE *file = fopen("/sys/class/power_supply/AC/online", "r");
    if (!file) {
        g_debug("Unable to check power status");
        return FALSE;
    }
    int status;
    if (fscanf(file, "%d", &status) == 1) {
        if (status == 1) {
            g_debug("Laptop is plugged in.\n");
            return TRUE;
        } else if (status == 0) {
            g_debug("Laptop is on battery.\n");
            return FALSE;
        } else {
            g_debug("Unknown power status: %d\n", status);
            return FALSE;
        }
    } else {
        g_debug("Failed to read power status.\n");
        return FALSE;
    }

    fclose(file);
}


gboolean
is_laptop(void){
    FILE *file = fopen("/sys/class/dmi/id/chassis_type", "r");
    if (!file) {
        file = fopen("/sys/devices/virtual/dmi/id/chassis_type", "r");
        if (!file) return FALSE; 
    }
    
    char type[16];
    if (fgets(type, sizeof(type), file)) {
        int chassis_type = atoi(type);
        fclose(file);
        
        if (chassis_type >= 8 && chassis_type <= 15 && chassis_type != 13) {
            return TRUE;
        }
        return 0;
    }
    
    fclose(file);
    return FALSE;
}