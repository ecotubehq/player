#include "ecotube-yt-dlp-updater.h"


#define YT_DLP_COMMAND "yt-dlp"



char* get_user_yt_dlp_path() {
    const gchar *data_dir = g_get_user_data_dir();
    if (!data_dir) {
        return NULL;
    }
    

    return g_build_filename(data_dir, "ecotube", YT_DLP_COMMAND, NULL);
}

char* get_latest_yt_dlp_version(void) {
    static char latest_version[100] = "unknown";
    char command[512];
    
    // Use GitHub API to get latest release info
    snprintf(command, sizeof(command),
             "curl -s https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest | "
             "grep '\"tag_name\"' | "
             "sed -E 's/.*\"([^\"]+)\".*/\\1/'");
    
    FILE *fp = popen(command, "r");
    if (fp) {
        if (fgets(latest_version, sizeof(latest_version), fp)) {
            // Remove newline character
            latest_version[strcspn(latest_version, "\n")] = 0;
            // Remove 'v' prefix if present
            if (latest_version[0] == 'v') {
                memmove(latest_version, latest_version + 1, strlen(latest_version));
            }
        }
        pclose(fp);
    }
    
    return latest_version;
}

char* get_current_yt_dlp_version() {
    const char* yt_dlp_path = get_yt_dlp_path();
    char command[256];
    char version[100] = "unknown";
    
    snprintf(command, sizeof(command), "%s --version 2>/dev/null", yt_dlp_path);
    
    FILE *fp = popen(command, "r");
    if (fp) {
        if (fgets(version, sizeof(version), fp)) {
            // Remove newline character
            version[strcspn(version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    return strdup(version);
}

const char* get_yt_dlp_path() {
    static char* system_path = NULL;
    char* user_path = get_user_yt_dlp_path();
    
    if (!system_path) {
        system_path = find_system_yt_dlp_path();
        if (!system_path) {
            // Fallback to a reasonable default if not found
            system_path = g_strdup("/usr/bin/yt-dlp");
            g_debug("Using fallback system path: %s\n", system_path);
        }
    }
    
    if (!user_path) {
        return system_path;
    }
    
    // Check if user has an updated version that's executable
    if (access(user_path, X_OK) == 0) {
        return user_path;
    }
    
    // Fall back to system version
    return system_path;
}

const char* get_yt_dlp_path_for_mpv() {
    return get_yt_dlp_path();
}

int initialize_user_yt_dlp() {
    char* user_path = get_user_yt_dlp_path();
    if (!user_path) {
        return -1;
    }
    
    // If user yt-dlp already exists, we're done
    if (access(user_path, F_OK) == 0) {
        return 0;
    }
    
    // Create data directory using GLib
    char* data_dir = g_path_get_dirname(user_path);
    if (g_mkdir_with_parents(data_dir, 0755) != 0) {
        g_free(data_dir);
        return -1;
    }
    g_free(data_dir);
    
    // Copy system yt-dlp to user directory for future updates
    GFile *src = g_file_new_for_path(get_yt_dlp_path());
    GFile *dest = g_file_new_for_path(user_path);
    GError* error = NULL;
    if (!g_file_copy(src, dest, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
        g_error_free(error);
        return -1;
    }
    
    // Make it executable
    if (chmod(user_path, 0755) != 0) {
        return -1;
    }
    
    g_debug("Initialized user yt-dlp for future updates at: %s\n", user_path);
    return 0;
}

int is_yt_dlp_update_available() {
    const char* yt_dlp_path = get_yt_dlp_path();
    char* command = g_strdup_printf("\"%s\" --update-to stable --dry-run 2>/dev/null", yt_dlp_path);
    
    int result = system(command);
    g_free(command);
    
    // If command returns 0, no update needed; if 1, update available
    return (result == 1);
}

int update_yt_dlp_with_builtin(CelluloidMainWindow *window) {
    CelluloidVideoArea *video_area =celluloid_main_window_get_video_area(window);
    // Ensure user yt-dlp is initialized
    if (initialize_user_yt_dlp() != 0) {
        return -1;
    }
    
    char* user_path = get_user_yt_dlp_path();
    if (!user_path) {
        return -1;
    }
    
    g_debug("Checking for updates using yt-dlp's built-in updater...\n");
    
    // Get current version before update
    char* old_version = get_current_yt_dlp_version();
    g_debug("Current version: %s\n", old_version);
    
    // Use yt-dlp's own update mechanism
    char* command = g_strdup_printf("\"%s\" -U", user_path);
    
    g_debug("Running: %s\n", command);
    int result = system(command);
    g_free(command);
    if (result == 0) {
        // Get new version after update
        char* new_version = get_current_yt_dlp_version();
        g_debug("Update completed successfully!\n");
        if (strcmp(old_version, new_version) != 0) {
            g_debug("Updated from %s to %s\n", old_version, new_version);
            char* toast_message = g_strdup_printf("yt-dlp updated from %s to %s\n", old_version, new_version);
            celluloid_video_area_show_toast_message(video_area, toast_message);
            g_free(toast_message);
        } else {
            g_debug("Your yt-dlp is already up to date (%s)-(%s)\n", new_version, old_version);
        }
        free(old_version);
        free(new_version);
        return 0; // Success
    } else {
        g_debug("yt-dlp update failed with code %d\n", result);
        g_debug("This might be due to network issues or sandbox restrictions\n");
        celluloid_video_area_show_toast_message(video_area, "yt-dlp update failed - This might be due to network issues or sandbox restrictions");

        free(old_version);
        return -1; // Failure
    }
}

char* find_system_yt_dlp_path() {

    gchar *path = g_find_program_in_path("yt-dlp");
    if (path) {
        g_debug("Found system yt-dlp via g_find_program_in_path: %s\n", path);
        return path;
    }
   
    g_debug("Warning: Could not find system yt-dlp in common locations\n");
    return NULL;
}

int is_new_version_available() {
    char* current_version = get_current_yt_dlp_version();
    char* latest_version = get_latest_yt_dlp_version();
    
    g_debug("Current version: %s\n", current_version);
    g_debug("Latest version: %s\n", latest_version);
    
    int result = 0;
    if (strcmp(current_version, "unknown") != 0 && 
        strcmp(latest_version, "unknown") != 0 &&
        strcmp(current_version, latest_version) != 0) {
        result = 1; 
    }
    
    g_free(current_version);
    g_free(latest_version);
    return result;
}