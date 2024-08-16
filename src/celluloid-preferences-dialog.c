/*
 * Copyright (c) 2014-2024 gnome-mpv
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

#include <gio/gio.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <adwaita.h>

#include "celluloid-preferences-dialog.h"
#include "celluloid-file-chooser-button.h"
#include "celluloid-plugins-manager.h"
#include "celluloid-main-window.h"
#include "celluloid-def.h"

typedef struct PreferencesDialogItem PreferencesDialogItem;
typedef enum PreferencesDialogItemType PreferencesDialogItemType;

struct _CelluloidPreferencesDialog
{
	AdwPreferencesWindow parent_instance;
	GSettings *settings;
	gboolean needs_mpv_reset;
};

struct _CelluloidPreferencesDialogClass
{
	AdwPreferencesWindowClass parent_class;
};

enum PreferencesDialogItemType
{
	ITEM_TYPE_INVALID,
	ITEM_TYPE_SWITCH,
	ITEM_TYPE_FILE_CHOOSER,
	ITEM_TYPE_TEXT_BOX,
	ITEM_TYPE_COMBO_BOX,
	ITEM_TYPE_COMBO_AUDIO,
	ITEM_TYPE_COMBO_OUTPUTV,
	ITEM_TYPE_COMBO_CODECV,
	ITEM_SEPARATOR_LABEL_BOX,
	ITEM_INFO_LABEL_BOX,
	ITEM_4K_SWITCH,
	ITEM_PLAYER_DEFAULT_SIZE,
	ITEM_TYPE_THEATER_MODE
};

struct PreferencesDialogItem
{
	const gchar *label;
	const gchar *key;
	PreferencesDialogItemType type;
};

G_DEFINE_TYPE(CelluloidPreferencesDialog, celluloid_preferences_dialog, ADW_TYPE_PREFERENCES_WINDOW)

char c_prevSetting[256];
static gboolean check_change(void);

static void
file_set_handler(CelluloidFileChooserButton *button, gpointer data)
{
	GtkRoot *root = gtk_widget_get_root(GTK_WIDGET(button));
	CelluloidPreferencesDialog *self = CELLULOID_PREFERENCES_DIALOG(root);
	const gchar *key = data;
	GFile *file = celluloid_file_chooser_button_get_file(button);
	gchar *uri = g_file_get_uri(file) ?: g_strdup("");

	g_settings_set_string(self->settings, key, uri);

	g_free(uri);
	g_object_unref(file);
}

static void
handle_changed(GSettings *settings, const gchar *key, gpointer data)
{
	CelluloidPreferencesDialog *dlg = CELLULOID_PREFERENCES_DIALOG(data);

	dlg->needs_mpv_reset |= g_strcmp0(key, "mpv-config-enable") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "mpv-config-file") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "mpv-input-config-enable") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "mpv-input-config-file") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "mpv-options") == 0;
}

static gboolean
save_settings(AdwPreferencesWindow *dialog)
{
	CelluloidPreferencesDialog *dlg = CELLULOID_PREFERENCES_DIALOG(dialog);

	g_settings_apply(dlg->settings);

	if(dlg->needs_mpv_reset || check_change())
	{
		g_signal_emit_by_name(dlg, "mpv-reset-request");
		dlg->needs_mpv_reset = FALSE;
	}

	return FALSE;
}

static void
free_signal_data(gpointer data, GClosure *closure)
{
	g_free(data);
}

static GtkWidget *
build_page(	const PreferencesDialogItem *items,
		GSettings *settings,
		const char *title,
		const char *icon_name )
{
	GtkWidget *pref_page = adw_preferences_page_new();
	adw_preferences_page_set_title
		(ADW_PREFERENCES_PAGE(pref_page), title);
	adw_preferences_page_set_icon_name
		(ADW_PREFERENCES_PAGE(pref_page), icon_name);

	GtkWidget *pref_group = adw_preferences_group_new();
	adw_preferences_page_add
		(	ADW_PREFERENCES_PAGE(pref_page),
			ADW_PREFERENCES_GROUP(pref_group) );

	GSettingsSchema *schema = NULL;
	g_object_get(settings, "settings-schema", &schema, NULL);

	for(gint i = 0; items[i].type != ITEM_TYPE_INVALID; i++)
	{
		const gchar *key = items[i].key;
		GSettingsSchemaKey *schema_key =
			key ?
			g_settings_schema_get_key(schema, key) :
			NULL;
		const gchar *summary =
			schema_key ?
			g_settings_schema_key_get_summary(schema_key) :
			NULL;
		const gchar *label = items[i].label ?: summary;
		const PreferencesDialogItemType type = items[i].type;

		GtkWidget *widget = NULL;

		if(type == ITEM_TYPE_SWITCH)
		{
			GtkWidget *pref_switch;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			pref_switch = gtk_switch_new();
			gtk_widget_set_valign
				(pref_switch, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), pref_switch);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), pref_switch);

			g_settings_bind(	settings,
						key,
						pref_switch,
						"active",
						G_SETTINGS_BIND_DEFAULT );
		}
		else if(type == ITEM_TYPE_FILE_CHOOSER)
		{
			GtkWidget *button;
			GtkFileFilter *filter;
			gchar *uri;
			GFile *file;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			button = celluloid_file_chooser_button_new
				(	NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN );
			gtk_widget_set_valign
				(button, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), button);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), button);

			filter = gtk_file_filter_new();
			uri = g_settings_get_string(settings, key);
			file = g_file_new_for_uri(uri);

			gtk_file_filter_add_mime_type
				(filter, "text/plain");
			celluloid_file_chooser_button_set_filter
				(CELLULOID_FILE_CHOOSER_BUTTON(button), filter);

			if(g_file_query_exists(file, NULL))
			{
				celluloid_file_chooser_button_set_file
					(	CELLULOID_FILE_CHOOSER_BUTTON(button),
						 file,
						 NULL );
			}

			/* For simplicity, changes made to the GSettings
			 * database externally won't be reflected immediately
			 * for this type of widget.
			 */
			g_signal_connect_data(	button,
						"file-set",
						G_CALLBACK(file_set_handler),
						g_strdup(key),
						free_signal_data,
						0 );

			g_free(uri);
		}
		else if(type == ITEM_TYPE_TEXT_BOX)
		{

			widget = adw_entry_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);
			gtk_widget_set_hexpand
				(widget, TRUE);

			g_settings_bind(	settings,
						key,
						widget,
						"text",
						G_SETTINGS_BIND_DEFAULT );
		}
		/* Added by Sako */
	GtkListStore *liststore_resolution;
		liststore_resolution = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		if(type == ITEM_TYPE_COMBO_BOX)
		{
			GtkWidget *pref_combo;
			//GtkListStore *liststore;
			GtkCellRenderer *column;
			//GtkWidget *sep_label;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			//liststore_resolution = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			gtk_list_store_insert_with_values(liststore_resolution, NULL, -1,
											  0, NULL,
											  1, "144p",
											  -1);	
			gtk_list_store_insert_with_values(liststore_resolution, NULL, -1,
											  0, NULL,
											  1, "240p",
											  -1);		
			gtk_list_store_insert_with_values(liststore_resolution, NULL, -1,
											  0, NULL,
											  1, "360p",
											  -1);
			gtk_list_store_insert_with_values(liststore_resolution, NULL, -1,
											  0, NULL,
											  1, "480p",
											  -1);
			gtk_list_store_insert_with_values(liststore_resolution, NULL, -1,
											  0, NULL,
											  1, "720p",
											  -1);
			/*gtk_list_store_insert_with_values(liststore_resolution, NULL, -1,
											  0, NULL,
											  1, "None",
											  -1);*/									  
			pref_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(liststore_resolution));
			g_object_unref(liststore_resolution);
			column = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pref_combo), column, TRUE);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pref_combo), column,
										   "cell-background", 0,
										   "text", 1,
										   NULL);

			gtk_combo_box_set_active(GTK_COMBO_BOX(pref_combo), 3);

			gtk_widget_set_valign
				(pref_combo, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), pref_combo);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), pref_combo);

			g_settings_bind(	settings,
						key,
						pref_combo,
						"active",
						G_SETTINGS_BIND_DEFAULT );		
			/**/
			
		}
		if(type == ITEM_SEPARATOR_LABEL_BOX)
		{
			widget = adw_action_row_new(); //adw_entry_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);
		}
		if(type == ITEM_INFO_LABEL_BOX)
		{
			/*
			//widget = adw_banner_new(label);
			widget = gtk_label_new(NULL);
			char *markup;
			markup = g_markup_printf_escaped("<span color=\"blue\"><u>\%s</u></span>", label);
			gtk_label_set_markup(GTK_LABEL(widget), markup);
			g_free(markup);
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);
			*/
		}
		if(type == ITEM_TYPE_COMBO_AUDIO)
		{
			GtkWidget *pref_combo;
			GtkListStore *liststore;
			GtkCellRenderer *column;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "Lo",
											  -1);
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "Hi",
											  -1);
									  
			pref_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(liststore));
			g_object_unref(liststore);
			column = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pref_combo), column, TRUE);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pref_combo), column,
										   "cell-background", 0,
										   "text", 1,
										   NULL);

			gtk_combo_box_set_active(GTK_COMBO_BOX(pref_combo), 1);

			gtk_widget_set_valign
				(pref_combo, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), pref_combo);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), pref_combo);

			g_settings_bind(	settings,
						key,
						pref_combo,
						"active",
						G_SETTINGS_BIND_DEFAULT );		
			/**/
			
		}
		if(type == ITEM_TYPE_COMBO_OUTPUTV)
		{
			GtkWidget *pref_combo;
			GtkListStore *liststore;
			GtkCellRenderer *column;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "BQ",
											  -1);
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "HQ",
											  -1);
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "LE",
											  -1);
								  								  
			pref_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(liststore));
			g_object_unref(liststore);
			column = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pref_combo), column, TRUE);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pref_combo), column,
										   "cell-background", 0,
										   "text", 1,
										   NULL);

			gtk_combo_box_set_active(GTK_COMBO_BOX(pref_combo), 0);

			gtk_widget_set_valign
				(pref_combo, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), pref_combo);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), pref_combo);

			g_settings_bind(	settings,
						key,
						pref_combo,
						"active",
						G_SETTINGS_BIND_DEFAULT );		
			/**/
			
		}
		if(type == ITEM_TYPE_COMBO_CODECV)
		{
			GtkWidget *pref_combo;
			GtkListStore *liststore;
			GtkCellRenderer *column;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "av1",
											  -1);	
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "vp9",
											  -1);
			gtk_list_store_insert_with_values(liststore, NULL, -1,
											  0, NULL,
											  1, "h.264",
											  -1);											  								  
			pref_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(liststore));
			g_object_unref(liststore);
			column = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pref_combo), column, TRUE);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pref_combo), column,
										   "cell-background", 0,
										   "text", 1,
										   NULL);

			gtk_combo_box_set_active(GTK_COMBO_BOX(pref_combo), 1);

			gtk_widget_set_valign
				(pref_combo, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), pref_combo);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), pref_combo);

			g_settings_bind(	settings,
						key,
						pref_combo,
						"active",
						G_SETTINGS_BIND_DEFAULT );		
			/**/
			
		}		
		if(type == ITEM_TYPE_THEATER_MODE)
		{
			GtkWidget *switch_theather_mode;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			switch_theather_mode = gtk_switch_new();
			gtk_widget_set_valign
				(switch_theather_mode, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), switch_theather_mode);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), switch_theather_mode);

			g_settings_bind(	settings,
						key,
						switch_theather_mode,
						"active",
						G_SETTINGS_BIND_DEFAULT );
			//g_signal_connect (switch_4k, "notify::active", G_CALLBACK (on_notify_active), liststore_resolution);
		}
		/* End Added by Sako */

		adw_preferences_group_add
			(ADW_PREFERENCES_GROUP(pref_group), widget);
	}

	return pref_page;
}

static void
preferences_dialog_constructed(GObject *obj)
{
	G_OBJECT_CLASS(celluloid_preferences_dialog_parent_class)->constructed(obj);
}

static void
finalize(GObject *object)
{
	CelluloidPreferencesDialog *dialog = CELLULOID_PREFERENCES_DIALOG(object);

	g_clear_object(&dialog->settings);

	G_OBJECT_CLASS(celluloid_preferences_dialog_parent_class)
		->finalize(object);
}

static void
celluloid_preferences_dialog_class_init(CelluloidPreferencesDialogClass *klass)
{
	G_OBJECT_CLASS(klass)->constructed = preferences_dialog_constructed;
	G_OBJECT_CLASS(klass)->finalize = finalize;

	g_signal_new(	"mpv-reset-request",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
			check_change();
			
}

static void
celluloid_preferences_dialog_init(CelluloidPreferencesDialog *dlg)
{
	const PreferencesDialogItem interface_items[]
		= {	{NULL,
			"autofit-enable",
			ITEM_TYPE_SWITCH},
			{NULL,
			"csd-enable",
			ITEM_TYPE_SWITCH},
			{NULL,
			"dark-theme-enable",
			ITEM_TYPE_SWITCH},
			{NULL,
			"always-use-floating-controls",
			ITEM_TYPE_SWITCH},
			{NULL,
			"always-use-floating-header-bar",
			ITEM_TYPE_SWITCH},
			{NULL,
			"draggable-video-area-enable",
			ITEM_TYPE_SWITCH},
			{NULL,
			"always-show-title-buttons",
			ITEM_TYPE_SWITCH},
			{NULL,
			"present-window-on-file-open",
			ITEM_TYPE_SWITCH},
			{NULL,
			"always-autohide-cursor",
			ITEM_TYPE_SWITCH},
			{NULL,
			"use-skip-buttons-for-playlist",
			ITEM_TYPE_SWITCH},
			{NULL,
			"last-folder-enable",
			ITEM_TYPE_SWITCH},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	const PreferencesDialogItem config_items[]
		= {	{NULL,
			"mpv-config-enable",
			ITEM_TYPE_SWITCH},
			{_("mpv configuration file:"),
			"mpv-config-file",
			ITEM_TYPE_FILE_CHOOSER},
			{NULL,
			"mpv-input-config-enable",
			ITEM_TYPE_SWITCH},
			{_("mpv input configuration file:"),
			"mpv-input-config-file",
			ITEM_TYPE_FILE_CHOOSER},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	const PreferencesDialogItem misc_items[]
		= {	{NULL,
			"always-open-new-window",
			ITEM_TYPE_SWITCH},
			{NULL,
			"always-append-to-playlist",
			ITEM_TYPE_SWITCH},
			{NULL,
			"ignore-playback-errors",
			ITEM_TYPE_SWITCH},
			{NULL,
			"prefetch-metadata",
			ITEM_TYPE_SWITCH},
			{NULL,
			"mpris-enable",
			ITEM_TYPE_SWITCH},
			{_("Extra mpv options"),
			"mpv-options",
			ITEM_TYPE_TEXT_BOX},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	/* Added by Sako */
	const PreferencesDialogItem av_items[]
		= {	{NULL,
			"youtube-video-output",
			ITEM_TYPE_COMBO_OUTPUTV},
			{NULL,
			"youtube-separator-line",
			ITEM_SEPARATOR_LABEL_BOX},
			{NULL,
			"youtube-video-codec",
			ITEM_TYPE_COMBO_CODECV},
			{NULL,
			"youtube-video-quality",
			ITEM_TYPE_COMBO_BOX},
			{NULL,
			"youtube-audio-quality",
			ITEM_TYPE_COMBO_AUDIO},
			/*{NULL,
			"youtube-theater-mode",
			ITEM_TYPE_THEATER_MODE},*/
			{NULL,
			"youtube-info-link",
			ITEM_INFO_LABEL_BOX},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	/* End Added by Sako */
	dlg->settings = g_settings_new(CONFIG_ROOT);
	dlg->needs_mpv_reset = FALSE;

	g_settings_delay(dlg->settings);

	GtkWidget *page = NULL;

    /* Added by Sako */
	page = build_page
		(	av_items,
			dlg->settings,
			_("AV Options"),
			"document-open-symbolic" );
	adw_preferences_window_add(	ADW_PREFERENCES_WINDOW(dlg),
					ADW_PREFERENCES_PAGE(page));
					
	page = build_page
		(	interface_items,
			dlg->settings,
			_("Interface"),
			"preferences-desktop-appearance-symbolic" );
	adw_preferences_window_add(	ADW_PREFERENCES_WINDOW(dlg),
					ADW_PREFERENCES_PAGE(page));
	if(1>2){ // disable temporarly
		page = build_page
			(	config_items,
				dlg->settings,
				_("Config Files"),
				"document-properties-symbolic" );
		adw_preferences_window_add(	ADW_PREFERENCES_WINDOW(dlg),
						ADW_PREFERENCES_PAGE(page));
	}
	if(1>2){ // disable temporarly
		page = build_page
			(	misc_items,
				dlg->settings,
				_("Miscellaneous"),
				"preferences-other-symbolic" );
		adw_preferences_window_add(	ADW_PREFERENCES_WINDOW(dlg),
						ADW_PREFERENCES_PAGE(page));
	}
	if(1>2){ // disable temporarly
		page = GTK_WIDGET(celluloid_plugins_manager_new(GTK_WINDOW(dlg)));
		adw_preferences_window_add(	ADW_PREFERENCES_WINDOW(dlg),
						ADW_PREFERENCES_PAGE(page) );
	}


	/* End Added by Sako*/
	g_signal_connect(	dlg,
				"close-request",
				G_CALLBACK(save_settings),
				NULL );
	g_signal_connect(	dlg->settings,
				"changed",
				G_CALLBACK(handle_changed),
				dlg );
}

GtkWidget *
celluloid_preferences_dialog_new(GtkWindow *parent)
{
	GtkWidget *dlg;


	dlg = g_object_new(	celluloid_preferences_dialog_get_type(),
				"title", _("Preferences"),
				"modal", TRUE,
				"transient-for", parent,
				NULL );

	gtk_widget_set_visible(dlg, TRUE);

	return dlg;
}
static gboolean check_change(){
	char selectedOpions[256];
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gchar *v_quality[] = {"144" ,"240", "360", "480", "720", "None"};
	gchar *a_quality[] = {"opus", "m4a"};
	gchar *v_codec[4] = {"av01", "vp09", "avc"};
	gchar *v_output[] = {"ewa-lanczos", "bicubic_fast"};//, "ewa-hanning"
	//gchar *p_size[] = {"360", "480"};

	gchar *selected_v_quality= v_quality[g_settings_get_int(settings, "youtube-video-quality")];
	gchar *selected_a_quality= a_quality[g_settings_get_int(settings, "youtube-audio-quality")];
	gchar *selected_v_codec= v_codec[g_settings_get_int(settings, "youtube-video-codec")];
	gchar *selected_v_output= v_output[g_settings_get_int(settings, "youtube-video-output")];
	gchar *selected_p_size= ""; //v_output[g_settings_get_int(settings, "youtube-player-size")];
	//gboolean theater_mode = g_settings_get_boolean(settings, "youtube-theater-mode");

	snprintf(selectedOpions, sizeof(selectedOpions), "%s-%s-%s-%s-%s",
		selected_v_quality, selected_a_quality, selected_v_codec, selected_v_output, selected_p_size);
	if(strcmp(c_prevSetting, selectedOpions) == 0){
		//printf("Controler No change was made: %s\n", c_prevSetting);
		return 0;
	}
	//printf("Controler Change was made: %s\n", selectedOpions);
	memcpy(c_prevSetting, selectedOpions, sizeof c_prevSetting);
	return 1;
}
