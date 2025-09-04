/*
 * Copyright (c) 2014-2025 gnome-mpv
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

enum
{
	PROP_0,
	PROP_PARENT,
	N_PROPERTIES
};

struct _CelluloidPreferencesDialog
{
	AdwPreferencesDialog parent_instance;
	GSettings *settings;
	gboolean needs_mpv_reset;
	GtkWindow *parent;
};

struct _CelluloidPreferencesDialogClass
{
	AdwPreferencesDialogClass parent_class;
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
	ITEM_INFO_VERSION_BOX,
	ITEM_4K_SWITCH,
	ITEM_PLAYER_DEFAULT_SIZE,
	ITEM_DEBAND_MODE,
	ITEM_HDR_MODE,
	ITEM_VULKAN_MODE,
	ITEM_INFO_CLOSE_BOX,
	ITEM_COMPUTER_TYPE
};

struct PreferencesDialogItem
{
	const gchar *label;
	const gchar *key;
	PreferencesDialogItemType type;
};

G_DEFINE_TYPE(CelluloidPreferencesDialog, celluloid_preferences_dialog, ADW_TYPE_PREFERENCES_DIALOG)

typedef struct {
    GtkWidget *pref_resolution;
    GtkWidget *pref_combo;
    GtkStringList *all_resolutions;
    GtkStringList *powersave_resolutions;
    GtkFilterListModel *filtered_model;
} ComboBoxPair;
ComboBoxPair *combo_pair = NULL;

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static void
constructed(GObject *object);

static void
file_set_handler(CelluloidFileChooserButton *button, gpointer data);

static void
handle_changed(GSettings *settings, const gchar *key, gpointer data);

static void
handle_plugins_manager_error(	CelluloidPluginsManager *pmgr,
				const gchar *message,
				gpointer data );

static gboolean
save_settings(AdwPreferencesDialog *dialog);

static void
free_signal_data(gpointer data, GClosure *closure);

static GtkWidget *
build_page(	const PreferencesDialogItem *items,
		CelluloidPreferencesDialog *dlg,
		GSettings *settings,
		const char *title,
		const char *icon_name );

static void
finalize(GObject *object);

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidPreferencesDialog *self = CELLULOID_PREFERENCES_DIALOG(object);

	switch(property_id)
	{
		case PROP_PARENT:
		self->parent = g_value_get_pointer(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidPreferencesDialog *self = CELLULOID_PREFERENCES_DIALOG(object);

	switch(property_id)
	{
		case PROP_PARENT:
		g_value_set_pointer(value, self->parent);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
constructed(GObject *object)
{
	G_OBJECT_CLASS(celluloid_preferences_dialog_parent_class)->constructed(object);

	CelluloidPreferencesDialog *dlg = CELLULOID_PREFERENCES_DIALOG(object);

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
			"draggable-video-area-enable",
			ITEM_TYPE_SWITCH},
			{NULL,
			"always-autohide-cursor",
			ITEM_TYPE_SWITCH},
			{NULL,
			"last-folder-enable",
			ITEM_TYPE_SWITCH},
			{NULL, NULL, ITEM_TYPE_INVALID} };

	const PreferencesDialogItem av_items[]
		= {	{NULL,
			"youtube-info-link",
			ITEM_INFO_LABEL_BOX},
			{NULL,
			"ecotube-computer-type",
			ITEM_COMPUTER_TYPE},
			{NULL,
			"mpv-use-vulkan",
			ITEM_VULKAN_MODE},
			{NULL,
			"youtube-display-deband",
			ITEM_DEBAND_MODE},
			{NULL,
			"youtube-allow-hdr",
			ITEM_HDR_MODE},
			{NULL,
			"youtube-separator-line",
			ITEM_SEPARATOR_LABEL_BOX},
			{NULL,
			"youtube-video-quality",
			ITEM_TYPE_COMBO_BOX},
			{NULL,
			"youtube-audio-quality",
			ITEM_TYPE_COMBO_AUDIO},
			{NULL,
			"close-info-pref",
			ITEM_INFO_CLOSE_BOX},
			{NULL, NULL, ITEM_TYPE_INVALID} };

			
	GtkWidget *page = NULL;

	page = build_page
		(	av_items,
			dlg,
			dlg->settings,
			_("AV Options"),
			"applications-graphics-symbolic" );
	adw_preferences_dialog_add(	ADW_PREFERENCES_DIALOG(dlg),
					ADW_PREFERENCES_PAGE(page));
					
	page = build_page
		(	interface_items,
			dlg,
			dlg->settings,
			_("Interface"),
			"applications-graphics-symbolic" );
	adw_preferences_dialog_add(	ADW_PREFERENCES_DIALOG(dlg),
					ADW_PREFERENCES_PAGE(page));



	g_signal_connect(	dlg,
				"closed",
				G_CALLBACK(save_settings),
				NULL );
	g_signal_connect(	dlg->settings,
				"changed",
				G_CALLBACK(handle_changed),
				dlg );
	/*g_signal_connect(	plugins_manager,
				"error-raised",
				G_CALLBACK(handle_plugins_manager_error),
				dlg );
				*/
}

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
	dlg->needs_mpv_reset |= g_strcmp0(key, "youtube-video-quality") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "youtube-audio-quality") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "youtube-video-codec") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "youtube-video-output") == 0;
	dlg->needs_mpv_reset |= g_strcmp0(key, "mpv-use-vulkan") == 0;
}

static void
handle_plugins_manager_error(	CelluloidPluginsManager *pmgr,
				const gchar *message,
				gpointer data )
{
	CelluloidPreferencesDialog *dialog =
		CELLULOID_PREFERENCES_DIALOG(data);

	g_signal_emit_by_name(dialog, "error-raised", message);
}

static gboolean
save_settings(AdwPreferencesDialog *dialog)
{
	CelluloidPreferencesDialog *dlg = CELLULOID_PREFERENCES_DIALOG(dialog);

	g_settings_apply(dlg->settings);

	if(dlg->needs_mpv_reset)
	{
		g_signal_emit_by_name(dlg, "mpv-reset-request");
		dlg->needs_mpv_reset = FALSE;
	}	
	return FALSE;
}
static gboolean
save_and_close_settings(GtkWidget *button, gpointer *data)
{
	CelluloidPreferencesDialog *dlg = CELLULOID_PREFERENCES_DIALOG(data);
	if(dlg != NULL){
		g_settings_apply(dlg->settings);

		if(dlg->needs_mpv_reset)
		{
			g_signal_emit_by_name(dlg, "mpv-reset-request");
			dlg->needs_mpv_reset = FALSE;
		}	
		adw_dialog_force_close(ADW_DIALOG(dlg));	

	}
	return FALSE;
}

static void
on_playbak_t_changed(GtkComboBox *playback_type, gpointer user_data){
	gint selected_item = gtk_drop_down_get_selected(GTK_DROP_DOWN(combo_pair->pref_combo));
	gint current_resolution = gtk_drop_down_get_selected(GTK_DROP_DOWN(combo_pair->pref_resolution));
	guint p_save_size = g_list_model_get_n_items (G_LIST_MODEL (combo_pair->powersave_resolutions));
	if(selected_item == 0){
		// Switch to Powersave resolutions
		gtk_drop_down_set_model(GTK_DROP_DOWN(combo_pair->pref_resolution), 
                               G_LIST_MODEL(combo_pair->powersave_resolutions));
        gtk_drop_down_set_selected(GTK_DROP_DOWN(combo_pair->pref_resolution), 
        		current_resolution > p_save_size - 1?p_save_size - 1: current_resolution);
	}else{
        // Switch to all resolutions
        gtk_drop_down_set_model(GTK_DROP_DOWN(combo_pair->pref_resolution), 
                               G_LIST_MODEL(combo_pair->all_resolutions));
        gtk_drop_down_set_selected(GTK_DROP_DOWN(combo_pair->pref_resolution), current_resolution);
	}
}
static void
free_signal_data(gpointer data, GClosure *closure)
{
	g_free(data);
}

static GtkWidget *
build_page(	const PreferencesDialogItem *items, 
		CelluloidPreferencesDialog *dlg,
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
		if(type == ITEM_TYPE_COMBO_BOX)
		{
			GtkCellRenderer *column;
			GtkWidget *pref_resolution;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);


			gint current_p_type = g_settings_get_int(dlg->settings, "ecotube-computer-type");
			if(current_p_type == 1){
				pref_resolution = gtk_drop_down_new(G_LIST_MODEL(combo_pair->all_resolutions), NULL);
			}else{
				pref_resolution = gtk_drop_down_new(G_LIST_MODEL(combo_pair->powersave_resolutions), NULL);
			}
			combo_pair->pref_resolution = pref_resolution;
			column = gtk_cell_renderer_text_new();
			gtk_drop_down_set_selected(GTK_DROP_DOWN(pref_resolution), 3);

			gtk_widget_set_valign
				(pref_resolution, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), pref_resolution);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), pref_resolution);

			g_settings_bind(	settings,
						key,
						pref_resolution,
						"selected",
						G_SETTINGS_BIND_DEFAULT );		
			
		}
		if(type == ITEM_SEPARATOR_LABEL_BOX)
		{
			widget = adw_action_row_new(); //adw_entry_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);
		}
		if(type == ITEM_INFO_LABEL_BOX)
		{
				GtkWidget *version_label;

				widget = adw_action_row_new();
				adw_preferences_row_set_title
					(ADW_PREFERENCES_ROW(widget), "");
					
				version_label = gtk_label_new(NULL);
				char *markup;
				markup = g_markup_printf_escaped("<span><b>\%s</b>                                                             \n%s</span>", label, VERSION);
				gtk_label_set_markup(GTK_LABEL(version_label), markup);
				g_free(markup);
			
				gtk_widget_set_valign
				(version_label, GTK_ALIGN_START);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), version_label);
				
			
		}
		if(type == ITEM_INFO_CLOSE_BOX){
			widget = gtk_button_new_with_label(label);	
			g_signal_connect (widget, "clicked",
                        G_CALLBACK (save_and_close_settings), dlg);	
            
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
		if(type == ITEM_DEBAND_MODE)
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
			
		}
		if(type == ITEM_HDR_MODE)
		{
			GtkWidget *switch_hdr_mode;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			switch_hdr_mode = gtk_switch_new();
			gtk_widget_set_valign
				(switch_hdr_mode, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), switch_hdr_mode);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), switch_hdr_mode);

			g_settings_bind(	settings,
						key,
						switch_hdr_mode,
						"active",
						G_SETTINGS_BIND_DEFAULT );
			
		}
		if(type == ITEM_VULKAN_MODE)
		{
			GtkWidget *switch_vulkan_mode;

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);

			switch_vulkan_mode = gtk_switch_new();
			gtk_widget_set_valign
				(switch_vulkan_mode, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), switch_vulkan_mode);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), switch_vulkan_mode);

			g_settings_bind(	settings,
						key,
						switch_vulkan_mode,
						"active",
						G_SETTINGS_BIND_DEFAULT );
			
		}
		if(type == ITEM_COMPUTER_TYPE)
		{

			widget = adw_action_row_new();
			adw_preferences_row_set_title
				(ADW_PREFERENCES_ROW(widget), label);
										  								  
			combo_pair->pref_combo = gtk_drop_down_new(G_LIST_MODEL(gtk_string_list_new((const char *[]){
				"Powersave",
				"Quality",
				NULL
			})), NULL);

			
			gtk_drop_down_set_selected(GTK_DROP_DOWN(combo_pair->pref_combo), 0);

			gtk_widget_set_valign
				(combo_pair->pref_combo, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix
				(ADW_ACTION_ROW(widget), combo_pair->pref_combo);
			adw_action_row_set_activatable_widget
				(ADW_ACTION_ROW(widget), combo_pair->pref_combo);

			g_settings_bind(	settings,
						key,
						combo_pair->pref_combo,
						"selected",
						G_SETTINGS_BIND_DEFAULT );

			g_signal_connect(combo_pair->pref_combo, "notify::selected", G_CALLBACK(on_playbak_t_changed), NULL);
			/**/
			
			
		}	
		/* End Added by Sako */

		adw_preferences_group_add
			(ADW_PREFERENCES_GROUP(pref_group), widget);
	}

	return pref_page;
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
	GParamSpec *pspec = NULL;
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->constructed = constructed;
	object_class->finalize = finalize;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_pointer
		(	"parent",
			"Parent",
			"Parent window",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_PARENT, pspec);

	g_signal_new(	"mpv-reset-request",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );

	g_signal_new(	"error-raised",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
}

static void
celluloid_preferences_dialog_init(CelluloidPreferencesDialog *dlg)
{
	dlg->settings = g_settings_new(CONFIG_ROOT);
	dlg->needs_mpv_reset = FALSE;
	dlg->parent = NULL;
	combo_pair = g_new0(ComboBoxPair, 1);
	combo_pair->all_resolutions = gtk_string_list_new ((const char *[]) {
        "144p", "240p", "360p", "480p", "720p", NULL
    });
	combo_pair->powersave_resolutions = gtk_string_list_new ((const char *[]) {
        "144p", "240p", "360p", "480p", NULL
    });
	g_settings_delay(dlg->settings);
}

GtkWidget *
celluloid_preferences_dialog_new(GtkWindow *parent)
{
	GtkWidget *dlg;

	dlg = g_object_new(	celluloid_preferences_dialog_get_type(),
				"parent", parent,
				"title", _("Preferences"),
				NULL );

	return dlg;
}

void
celluloid_preferences_dialog_present(CelluloidPreferencesDialog *self)
{
	adw_dialog_present(ADW_DIALOG(self), GTK_WIDGET(self->parent));
}
