/*
 * Copyright (c) 2016-2023 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <string.h>

#include "celluloid-controller-private.h"
#include "celluloid-controller.h"
#include "celluloid-controller-input.h"
#include "celluloid-mpv.h"
#include "celluloid-main-window.h"
#include "celluloid-video-area.h"
#include "celluloid-def.h"

static gchar *
keyval_to_keystr(guint keyval);

static gchar *
get_modstr(guint state);

static gchar *
get_full_keystr(guint keyval, GdkModifierType state);

static void
mouse_up(CelluloidController *controller, const guint button);

static gboolean
key_pressed_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data );

static void
key_released_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data );

static void
button_pressed_handler(	GtkGestureSingle *gesture,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data );

static void
button_released_handler(	GtkGestureSingle *gesture,
				gint n_press,
				double x,
				gdouble y,
				gpointer data );

static void
button_stopped_handler(GtkGestureSingle *gesture, gpointer data);

static gboolean
mouse_move_handler(	GtkEventControllerMotion *motion_controller,
			gdouble x,
			gdouble y,
			gpointer data );

static gboolean
scroll_handler(	GtkEventControllerScroll *scroll_controller,
		gdouble dx,
		gdouble dy,
		gpointer data );

typedef struct {
    CelluloidVideoArea *video_area;
    gchar *uri;
    gchar *title;
} TDownloader;
static gpointer ecotube_downloand_video(gpointer user_data);
static gpointer ecotube_downloand_audio(gpointer user_data);
static gboolean ecotube_downloand_done(gpointer user_data);


static gchar *
keyval_to_keystr(guint keyval)
{
	const gchar *keystrmap[] = KEYSTRING_MAP;
	gboolean found = FALSE;
	gchar key_utf8[7] = {0}; // 6 bytes for output and 1 for NULL terminator
	const gchar *result = key_utf8;

	g_unichar_to_utf8(gdk_keyval_to_unicode(keyval), key_utf8);
	result = result[0] ? result : gdk_keyval_name(keyval);
	found = !result;

	for(gint i = 0; !found && keystrmap[i]; i += 2)
	{
		const gint rc = g_ascii_strncasecmp
				(result, keystrmap[i+1], KEYSTRING_MAX_LEN);

		if(rc == 0)
		{
			result = keystrmap[i][0] ? keystrmap[i] : NULL;
			found = TRUE;
		}
	}

	return result ? g_strdup(result) : NULL;
}

static gchar *
get_modstr(guint state)
{
	const struct
	{
		guint mask;
		gchar *str;
	}
	mod_map[] = {	{GDK_SHIFT_MASK, "Shift+"},
			{GDK_CONTROL_MASK, "Ctrl+"},
			{GDK_ALT_MASK, "Alt+"},
			{GDK_SUPER_MASK, "Meta+"}, // Super is Meta in mpv
			{0, NULL} };

	const gsize max_len = G_N_ELEMENTS("Ctrl+Alt+Shift+Meta+") + 1;
	gchar *result = g_malloc0(max_len);

	for(gint i = 0; mod_map[i].str; i++)
	{
		if(state & mod_map[i].mask)
		{
			g_strlcat(result, mod_map[i].str, max_len);
		}
	}

	return result;
}

static gchar *
get_full_keystr(guint keyval, GdkModifierType state)
{
	gchar *modstr = get_modstr(state);
	gchar *keystr = keyval_to_keystr(keyval);
	char *result = keystr ? g_strconcat(modstr, keystr, NULL) : NULL;

	g_free(keystr);
	g_free(modstr);

	return result;
}

static void
mouse_up(CelluloidController *controller, const guint button)
{
	gchar *name = g_strdup_printf("MOUSE_BTN%u", button - 1);

	celluloid_model_key_up(controller->model, name);
	g_free(name);
}

static gboolean
key_pressed_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data )

{
	CelluloidController *controller = data;
	gchar *keystr = get_full_keystr(keyval, state);
	gboolean searching = FALSE;

	g_object_get(controller->view, "searching", &searching, NULL);

	if(keystr && !searching)
	{
		celluloid_model_key_down(controller->model, keystr);
		g_free(keystr);
	}
	gchar *ckeystr = keyval_to_keystr(keyval);
	//Added  by Sako
	GSettings *settings =		g_settings_new(CONFIG_ROOT);
	gboolean theater_mode = FALSE; //g_settings_get_boolean(settings, "youtube-theater-mode");
	if((state & GDK_CONTROL_MASK) && g_strcmp0(ckeystr, "t")==0){
		GSettings *settings = g_settings_new(CONFIG_ROOT);
		gboolean is_floating = FALSE;//g_settings_get_boolean(settings, "always-use-floating-header-bar");
		g_settings_set_boolean(settings, "always-use-floating-header-bar", !is_floating);
		g_settings_set_boolean(settings, "always-use-floating-controls", !is_floating);
		//g_settings_set_boolean(settings, "youtube-theater-mode", !is_floating);
		int video_resolution_index = g_settings_get_int(settings, "youtube-video-quality");
		if(video_resolution_index < 3){
			int width = 768;
			int height = 432;
			if(video_resolution_index == 1){
				width = 1278;
				height = 720;
			}
			celluloid_view_resize_video_area(controller->view, width, height);
			//g_signal_emit_by_name(data, "resize", 120, 60);
		}else if(is_floating && video_resolution_index==0){
			int width = 854;
			int height = 480;
			celluloid_view_resize_video_area(controller->view, width, height);			
		}
		g_object_unref(settings);
	}
	if(theater_mode && g_strcmp0(ckeystr, "ESC")==0){
		//g_settings_set_boolean(settings, "always-use-floating-header-bar", FALSE);
		//g_settings_set_boolean(settings, "always-use-floating-controls", FALSE);
		//gtk_window_set_resizable(controller->view, TRUE);		
		int width = 854;
		int height = 480;
		celluloid_view_resize_video_area(controller->view, width, height);	
	}
	if((state & GDK_CONTROL_MASK) && g_strcmp0(ckeystr, "a")==0){

		CelluloidView *view = celluloid_controller_get_view(controller);
		CelluloidMainWindow *window = CELLULOID_MAIN_WINDOW(view);
		CelluloidPlaylistWidget *playlist = celluloid_main_window_get_playlist(window);
		GPtrArray *contents = celluloid_playlist_widget_get_contents(playlist);
		g_assert(contents->len > 0);
		CelluloidPlaylistEntry *entry = g_ptr_array_index(contents, 0);
		CelluloidVideoArea *video_area = celluloid_main_window_get_video_area(window);
				
		TDownloader *nloader = g_new(TDownloader, 1);
		nloader->video_area = video_area;		
		nloader->title = "placeholder";
		nloader->uri = entry->filename;
		
		celluloid_video_area_show_toast_message(video_area, "Downloading the video ...");
		g_thread_new("worker", ecotube_downloand_video, nloader);

	}
	if((state & GDK_CONTROL_MASK) && g_strcmp0(ckeystr, "d")==0){

		CelluloidView *view = celluloid_controller_get_view(controller);
		CelluloidMainWindow *window = CELLULOID_MAIN_WINDOW(view);
		CelluloidPlaylistWidget *playlist = celluloid_main_window_get_playlist(window);
		GPtrArray *contents = celluloid_playlist_widget_get_contents(playlist);
		g_assert(contents->len > 0);
		CelluloidPlaylistEntry *entry = g_ptr_array_index(contents, 0);
		CelluloidVideoArea *video_area = celluloid_main_window_get_video_area(window);
				
		TDownloader *nloader = g_new(TDownloader, 1);
		nloader->video_area = video_area;		
		nloader->title = "placeholder";
		nloader->uri = entry->filename;
		
		celluloid_video_area_show_toast_message(video_area, "Downloading the audio ...");
		g_thread_new("worker", ecotube_downloand_audio, nloader);

	}
	return FALSE;
}

static void
key_released_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data )
{
	CelluloidController *controller = data;
	gchar *keystr = get_full_keystr(keyval, state);

	const gboolean playlist_visible =
		celluloid_view_get_playlist_visible(controller->view);
	if(keystr && !playlist_visible)
	{
		celluloid_model_key_up(controller->model, keystr);
		g_free(keystr);
	}
}

static void
button_pressed_handler(	GtkGestureSingle *gesture,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data )
{
	CelluloidController *controller =
		data;
	const guint button_number =
		gtk_gesture_single_get_current_button(gesture);

	if(button_number > 0)
	{
		gchar *button_name =
			g_strdup_printf("MOUSE_BTN%u", button_number - 1);

		celluloid_model_key_down(controller->model, button_name);
		g_free(button_name);

		CelluloidView *view = celluloid_controller_get_view(controller);
		gtk_window_set_focus(GTK_WINDOW(view), NULL);
	}
}

static void
button_stopped_handler(GtkGestureSingle *gesture, gpointer data)
{
	const guint button = gtk_gesture_single_get_current_button(gesture);

	if(button > 0)
	{
		mouse_up(CELLULOID_CONTROLLER(data), button);
	}
}

static void
button_released_handler(	GtkGestureSingle *gesture,
				gint n_press,
				double x,
				gdouble y,
				gpointer data )
{
	const guint button = gtk_gesture_single_get_current_button(gesture);

	if(button > 0)
	{
		mouse_up(CELLULOID_CONTROLLER(data), button);
	}
}

static gboolean
mouse_move_handler(	GtkEventControllerMotion *motion_controller,
			gdouble x,
			gdouble y,
			gpointer data )
{
	CelluloidController *controller = data;

	if(controller->model)
	{
		celluloid_model_mouse(controller->model, (gint)x, (gint)y);
	}

	return FALSE;
}

static gboolean
scroll_handler(	GtkEventControllerScroll *scroll_controller,
		gdouble dx,
		gdouble dy,
		gpointer data )
{
	guint button = 0;
	gint count = 0;

	/* Only one axis will be used at a time to prevent accidental activation
	 * of commands bound to buttons associated with the other axis.
	 */
	if(ABS(dx) > ABS(dy))
	{
		count = (gint)ABS(dx);

		if(dx <= -1)
		{
			button = 6;
		}
		else if(dx >= 1)
		{
			button = 7;
		}
	}
	else
	{
		count = (gint)ABS(dy);

		if(dy <= -1)
		{
			button = 4;
		}
		else if(dy >= 1)
		{
			button = 5;
		}
	}

	if(button > 0)
	{
		gchar *button_name = g_strdup_printf("MOUSE_BTN%u", button - 1);
		CelluloidModel *model = CELLULOID_CONTROLLER(data)->model;

		while(count-- > 0)
		{
			celluloid_model_key_press(model, button_name);
		}

		g_free(button_name);
	}

	return TRUE;
}

void
celluloid_controller_input_connect_signals(CelluloidController *controller)
{
	CelluloidMainWindow *wnd =	celluloid_view_get_main_window
					(controller->view);
	CelluloidVideoArea *video_area =	celluloid_main_window_get_video_area
						(wnd);

	g_signal_connect(	controller->key_controller,
				"key-pressed",
				G_CALLBACK(key_pressed_handler),
				controller );
	g_signal_connect(	controller->key_controller,
				"key-released",
				G_CALLBACK(key_released_handler),
				controller );

	GtkGesture *click_gesture =
		gtk_gesture_click_new();
	gtk_gesture_single_set_button
		(GTK_GESTURE_SINGLE(click_gesture), 0);
	gtk_widget_add_controller
		(GTK_WIDGET(video_area), GTK_EVENT_CONTROLLER(click_gesture));

	g_signal_connect(	click_gesture,
				"pressed",
				G_CALLBACK(button_pressed_handler),
				controller );
	g_signal_connect(	click_gesture,
				"released",
				G_CALLBACK(button_released_handler),
				controller );
	// As of GTK 4.17.4, the player crashes on exit when this signals is enabled
	/*
	g_signal_connect(	click_gesture,
				"stopped",
				G_CALLBACK(button_stopped_handler),
				controller );
	*/

	GtkEventController *motion_controller =
		gtk_event_controller_motion_new();
	gtk_widget_add_controller
		(GTK_WIDGET(video_area), GTK_EVENT_CONTROLLER(motion_controller));

	g_signal_connect(	motion_controller,
				"motion",
				G_CALLBACK(mouse_move_handler),
				controller );

	GtkEventController *scroll_controller =
		gtk_event_controller_scroll_new
		(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
	gtk_widget_add_controller
		(GTK_WIDGET(video_area), GTK_EVENT_CONTROLLER(scroll_controller));

	g_signal_connect(	scroll_controller,
				"scroll",
				G_CALLBACK(scroll_handler),
				controller );
}
static gpointer ecotube_downloand_video(gpointer user_data){
	TDownloader *data = user_data;
	gchar *command = g_strdup_printf("yt-dlp -f worst %s -o \"%%(title)s.%%(ext)s\" -P ~/  > /dev/null", data->uri);
	system(command);
	g_idle_add(ecotube_downloand_done, data);
	
	g_free(command);
	return NULL;
}
static gpointer ecotube_downloand_audio(gpointer user_data){
	TDownloader *data = user_data;
	gchar *command = g_strdup_printf("yt-dlp -f bestaudio %s -o \"%%(title)s.%%(ext)s\" -P ~/  > /dev/null", data->uri);
	system(command);
	g_idle_add(ecotube_downloand_done, data);
	
	g_free(command);
	return NULL;
}
static gboolean ecotube_downloand_done(gpointer user_data){
	TDownloader *data = user_data;
	celluloid_video_area_show_toast_message(data->video_area, "Download complete!");
	g_free(data);
	return G_SOURCE_REMOVE;
}
