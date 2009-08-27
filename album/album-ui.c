/*
 * Album (Buddy Icon Archiver)
 *
 * Copyright (C) 2005-2008, Sadrul Habib Chowdhury <imadil@gmail.com>
 * Copyright (C) 2005-2008, Richard Laager <rlaager@pidgin.im>
 * Copyright (C) 2006, Jérôme Poulin (TiCPU) <jeromepoulin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

#include "album-ui.h"

/* We want to use the gstdio functions when possible so that non-ASCII
 * filenames are handled properly on Windows. */
#if GLIB_CHECK_VERSION(2,6,0)
#include <glib/gstdio.h>
#else
#include <sys/stat.h>
#define g_fopen fopen
#define g_stat stat
#endif

#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>

#include <sys/stat.h>
#include <string.h>

#include <account.h>
#include <debug.h>
#include <gtkblist.h>
#include <pidgin.h>
#include <plugin.h>
#include <request.h>
#include <util.h>

/* XXX: For DATADIR... There must be a better way. */
#ifdef _WIN32
#include <win32/win32dep.h>
#endif

/* The increment between sizes. */
#ifndef ICON_SIZE_MULTIPLIER
#define ICON_SIZE_MULTIPLIER 32
#endif

/* The size of the icon in the icon viewer's label. */
#ifndef LABEL_ICON_SIZE
#define LABEL_ICON_SIZE 24
#endif

/* How much to pad between two rows. */
#ifndef ROW_PADDING
#define ROW_PADDING 0
#endif

/* Padding around the vbox containing the image and text. */
#ifndef VBOX_BORDER
#define VBOX_BORDER 10
#endif

/* Spacing between children in the vbox containing the image and text. */
#ifndef VBOX_SPACING
#define VBOX_SPACING 5
#endif

/* Padding around the text label for an icon. */
#ifndef LABEL_PADDING
#define LABEL_PADDING 3
#endif

/* Padding around the image. */
#ifndef IMAGE_PADDING
#define IMAGE_PADDING 3
#endif

typedef struct _BuddyIcon BuddyIcon;
typedef struct _BuddyWindow BuddyWindow;
typedef struct _icon_viewer_key icon_viewer_key;

struct _BuddyIcon
{
	char *full_path;
	time_t timestamp;

	/* For suggesting a filename on image save. */
	char *buddy_name;
};

struct _BuddyWindow
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *iconview;
	GtkTextBuffer *text_buffer;
	int text_height;
	int text_width;
	GtkRequisition requisition;
};

/* contact or ((account and screenname) and possibly buddy) will be set.*/
struct _icon_viewer_key
{
	PurpleContact *contact;

	PurpleBuddy *buddy;

	PurpleAccount *account;
	char *screenname;
	GList *list;
};

static void update_icon_view(icon_viewer_key *key);
static void show_buddy_icon_window(icon_viewer_key *key, const char *name);

void icon_viewer_key_free(void *data)
{
	icon_viewer_key *key = (icon_viewer_key *)data;
	g_free(key->screenname);
	g_free(key);
}

guint icon_viewer_hash(gconstpointer data)
{
	const icon_viewer_key *key = data;

	if (key->contact != NULL)
		return g_direct_hash(key->contact);

	return g_str_hash(key->screenname) +
		g_str_hash(purple_account_get_username(key->account));
}

gboolean icon_viewer_equal(gconstpointer y, gconstpointer z)
{
	const icon_viewer_key *a, *b;

	a = y;
	b = z;

	if (a->contact != NULL)
	{
		if (b->contact != NULL)
			return (a->contact == b->contact);
		else
			return FALSE;
	}
	else if (b->contact != NULL)
		return FALSE;

	if (a->account == b->account)
	{
		char *normal = g_strdup(purple_normalize(a->account, a->screenname));
		if (!strcmp(normal, purple_normalize(b->account, b->screenname)))
		{
			g_free(normal);
			return TRUE;
		}
		g_free(normal);
	}

	return FALSE;
}

static void set_window_geometry(BuddyWindow *bw, int buddy_icon_size)
{
	GdkGeometry geom;

	g_return_if_fail(bw != NULL);

	/* Set the window geometry.  This controls window resizing. */
	geom.base_width  = bw->requisition.width  + 40; /* XXX: Where is the hardcoded value coming from? */
	geom.base_height = bw->requisition.height + 18; /* XXX: Where is the hardcoded value coming from? */
	geom.width_inc   = MAX(buddy_icon_size, bw->text_width) + 2 * VBOX_BORDER;
	geom.height_inc  = ROW_PADDING + 2 * VBOX_BORDER + buddy_icon_size + 2 * IMAGE_PADDING + VBOX_SPACING + bw->text_height + 2 * LABEL_PADDING;
	geom.min_width   = geom.base_width  + 3 * geom.width_inc;  /* Minimum size: 3 wide */
	geom.min_height  = geom.base_height +     geom.height_inc; /* Minimum size: 1 high */
	gtk_window_set_geometry_hints(GTK_WINDOW(bw->window), bw->vbox, &geom,
	                              GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC | GDK_HINT_BASE_SIZE);
}

static gboolean resize_icons(GtkWidget *combo, icon_viewer_key *key)
{
	int sel = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	BuddyWindow *bw;

	switch(sel)
	{
		case 0: /* Small */
		case 1: /* Medium */
		case 2: /* Large */
			purple_prefs_set_int(PREF_ICON_SIZE, sel);
			break;
		default:
			g_return_val_if_reached(FALSE);
	}
	update_icon_view(key);

	bw = g_hash_table_lookup(buddy_windows, key);
	g_return_val_if_fail(bw != NULL, FALSE);
	set_window_geometry(bw, ICON_SIZE_MULTIPLIER * (sel + 1));

	return FALSE;
}

/* Save the size of the window. */
static gboolean update_size(GtkWidget *win, GdkEventConfigure *event, gpointer data)
{
	int w;
	int h;

	gtk_window_get_size(GTK_WINDOW(win), &w, &h);

	purple_prefs_set_int(PREF_WINDOW_WIDTH, w);
	purple_prefs_set_int(PREF_WINDOW_HEIGHT, h);

	/* We want the normal handling to continue. */
	return FALSE;
}

static gboolean window_close(GtkWidget *win, gint resp, icon_viewer_key *key)
{
	g_hash_table_remove(buddy_windows, key);
	gtk_widget_destroy(win);
	return TRUE;
}

/* Returns a scroller window which contains widget. */
static GtkWidget *wrap_in_scroller(GtkWidget *widget)
{
	GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),
	                                    GTK_SHADOW_IN);

	gtk_container_add(GTK_CONTAINER(scroller), widget);

	return scroller;
}

/* Based on Purple's gtkimhtml.c, image_save_yes_cb(). */
static void convert_image(GtkImage *image, const char *filename)
{
	gchar *type = NULL;
	GError *error = NULL;
	GSList *formats = gdk_pixbuf_get_formats();

	while (formats)
	{
		GdkPixbufFormat *format = formats->data;
		gchar **extensions = gdk_pixbuf_format_get_extensions(format);
		gpointer p = extensions;

		while (gdk_pixbuf_format_is_writable(format) && extensions && extensions[0])
		{
			gchar *fmt_ext = extensions[0];
			const gchar *file_ext = filename + strlen(filename) - strlen(fmt_ext);

			if (!strcmp(fmt_ext, file_ext))
			{
				type = gdk_pixbuf_format_get_name(format);
				break;
			}

			extensions++;
		}

		g_strfreev(p);

		if (type)
			break;

		formats = formats->next;
	}

	g_slist_free(formats);

	if (!type)
	{
		GtkWidget *dialog;

		/* Present an error dialog. */
		dialog = gtk_message_dialog_new_with_markup(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		                                            _("<span size='larger' weight='bold'>Unrecognized file type</span>\n\nDefaulting to PNG."));
		g_signal_connect_swapped(dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
		gtk_widget_show(dialog);

		/* Assume the user wants a PNG. */
		type = g_strdup("png");
	}

	gdk_pixbuf_save(gtk_image_get_pixbuf(image), filename, type, &error, NULL);

	if (error)
	{
		GtkWidget *dialog;

		/* Present an error dialog. */
		dialog = gtk_message_dialog_new_with_markup(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		                                            _("<span size='larger' weight='bold'>Error saving image</span>\n\n%s"),
		                                            error->message);
		g_signal_connect_swapped(dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
		gtk_widget_show(dialog);

		g_error_free(error);
	}

	g_free(type);
}

static void
image_save_cb(GtkWidget *widget, gint response, GtkImage *image)
{
	char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
	char *image_name = g_object_get_data(G_OBJECT(image), "filename");

	gtk_widget_destroy(widget);

	if (response != GTK_RESPONSE_ACCEPT)
		return;

	purple_debug_misc(PLUGIN_STATIC_NAME, "Saving image %s as: %s\n", image_name, filename);

	/* Keeping this crud out of this function is a Good Thing (TM). */
	convert_image(image, filename);

	g_free(filename);
}

static void save_dialog(GtkWidget *widget, GtkImage *image)
{
	GtkWidget *dialog;
	const char *ext;
	char *filename;

	dialog = gtk_file_chooser_dialog_new(_("Save Image"),
	                                     NULL,
	                                     GTK_FILE_CHOOSER_ACTION_SAVE,
	                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	                                     NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	/* Determine the extension. */
	ext = g_object_get_data(G_OBJECT(image), "filename");
	if (ext)
		ext = strrchr(ext, '.');
	if (ext == NULL)
		ext = "";

	filename = g_strdup_printf("%s%s", purple_escape_filename(g_object_get_data(G_OBJECT(image), "buddy_name")), ext);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
	g_free(filename);

	g_signal_connect(G_OBJECT(GTK_FILE_CHOOSER(dialog)), "response",
					 G_CALLBACK(image_save_cb), image);

	gtk_widget_show(dialog);
}

static gboolean save_menu(GtkWidget *event_box, GdkEventButton *event, GtkImage *image)
{
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *item = gtk_image_menu_item_new_with_mnemonic("_Save Icon");

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	                              gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(save_dialog), image);

	gtk_widget_show_all(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, event_box, 3, event->time);

	return FALSE;
}

static gint buddy_icon_compare(BuddyIcon *b1, BuddyIcon *b2)
{
	gint ret;

	/* NOTE: This sorts by timestamp DESCENDING. */
	if ((ret = b2->timestamp - b1->timestamp) != 0)
		return ret;

	/* This isn't strictly necessary, but it
	 * ensures the icon order is stable. */
	return strcmp(b1->full_path, b2->full_path);
}

/* Returns an unsorted list of available icons for buddy (as BuddyIcon *).
 * The items need to be freed.
 */
static GList *retrieve_icons(PurpleAccount *account, const char *name)
{
	char *path;
	GDir *dir;
	const char *filename;
	GList *list = NULL;

	path = album_buddy_icon_get_dir(account, name);
	if (path == NULL)
	{
		purple_debug_warning(PLUGIN_STATIC_NAME, "Path for buddy %s not found.\n", name);
		return NULL;
	}

	if (!(dir = g_dir_open(path, 0, NULL)))
	{
		purple_debug_warning(PLUGIN_STATIC_NAME, "Could not open path: %s\n", path);
		g_free(path);
		return NULL;
	}

	while ((filename = g_dir_read_name(dir)))
	{
		char *full_path = g_build_filename(path, filename, NULL);
		struct stat st;
		BuddyIcon *icon;

		/* We need the file's timestamp. */
		if (stat(full_path, &st) != 0)
		{
			g_free(full_path);
			continue;
		}

		icon = g_new0(BuddyIcon, 1);
		icon->full_path = full_path;
		icon->timestamp = st.st_mtime;
		icon->buddy_name = g_strdup(name);

		list = g_list_prepend(list, icon);
	}

	g_dir_close(dir);

	g_free(path);
	return list;
}

static gboolean add_icon_from_list_cb(gpointer data)
{
	GtkTextIter text_iter;
	int buddy_icon_pref = purple_prefs_get_int(PREF_ICON_SIZE);
	int buddy_icon_size;
	BuddyWindow *bw;
	GtkTextBuffer *text_buffer;
	GtkTextIter start, end;
	GtkWidget *iconview;
	icon_viewer_key *key = data;
	BuddyIcon *icon;
	GdkPixbuf *pixbuf;
	int width;
	int height;
	int xpad = 0;
	int ypad = 0;
	GdkPixbuf *scaled;
	GtkWidget *image;
	GtkWidget *event_box;
	GtkWidget *alignment;
	GtkWidget *widget;
	GtkWidget *label;
	const char *timestamp;
	GtkTextChildAnchor *anchor;
	BuddyIcon *prev_icon;
	char *prev_icon_filename;
	GList *l;


	/* If we're out of icons, kill this idle callback. */
	if (key->list == NULL)
		return FALSE;

	bw = g_hash_table_lookup(buddy_windows, key);
	g_return_val_if_fail(bw != NULL, FALSE);

	text_buffer = bw->text_buffer;
	iconview = bw->iconview;

	/* Clamp the pref value to the allowable range. */
	buddy_icon_pref = CLAMP(buddy_icon_pref, 0, 2);

	buddy_icon_size = ICON_SIZE_MULTIPLIER * (buddy_icon_pref + 1);

	gtk_text_buffer_get_end_iter(text_buffer, &text_iter);


	/* Duplicate Removal */

	prev_icon = key->list->data;
	/* Technically, this will yield "/filename.ext", but the
	 * code below will get the same thing, so it'll compare fine. */
	prev_icon_filename = strrchr(prev_icon->full_path, '/');
	if (prev_icon_filename == NULL)
	{
		/* This should never happen. */
		prev_icon_filename = prev_icon->full_path;
	}

	for (l = key->list->next ; l != NULL ; l = l->next)
	{
		BuddyIcon *this_icon = l->data;
		char *this_icon_filename = strrchr(this_icon->full_path, '/');

		if (this_icon_filename == NULL)
		{
			/* This should never happen. */
			this_icon_filename = this_icon->full_path;
		}

		/* The files are named by hash, so if they have the
		 * same basename, we can assume they have the same
		 * contents.  This happens when someone uses the
		 * same icon on multiple accounts.  We only want to
		 * show each icon once.
		 */
		if (!strcmp(this_icon_filename, prev_icon_filename))
		{
			key->list = g_list_delete_link(key->list, l);
		}
	}


	/* Pop off one icon to add. */
	icon = key->list->data;
	key->list = g_list_delete_link(key->list, key->list);

	pixbuf = gdk_pixbuf_new_from_file(icon->full_path, NULL);
	if (pixbuf == NULL)
	{
		purple_debug_warning(PLUGIN_STATIC_NAME, "Invalid image file: %s\n", icon->full_path);
		g_free(icon->full_path);
		g_free(icon->buddy_name);
		g_free(icon);
		return TRUE;
	}

	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	/* Never scale the image up. */
	if (height > buddy_icon_size || width > buddy_icon_size)
	{
		/* Scale the image proportionally to fit with a square of size buddy_icon_size. */
		if (width > height)
		{
				int new_height = (int)(buddy_icon_size / (double)width * height);
				ypad = buddy_icon_size - new_height;
				scaled = gdk_pixbuf_scale_simple(pixbuf, buddy_icon_size, new_height, GDK_INTERP_BILINEAR);
		}
		else
		{
				int new_width = (int)(buddy_icon_size / (double)height * width);
				xpad = buddy_icon_size - new_width;
				scaled = gdk_pixbuf_scale_simple(pixbuf, new_width, buddy_icon_size, GDK_INTERP_BILINEAR);
		}
		g_object_unref(G_OBJECT(pixbuf));
	}
	else
	{
		ypad = buddy_icon_size - height;
		xpad = buddy_icon_size - width;
		scaled = pixbuf;
	}


	/* Now we're ready to add the icon. */

	/* Create the image and an event box to which to attach the right-click menu. */
	image = gtk_image_new_from_pixbuf(scaled);
	g_object_unref(G_OBJECT(scaled));
	event_box = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
	gtk_container_add(GTK_CONTAINER(event_box), image);

	/* Save the filename and create a right-click handler. */
	g_object_set_data_full(G_OBJECT(image), "buddy_name", icon->buddy_name, g_free);
	g_object_set_data_full(G_OBJECT(image), "filename", icon->full_path, g_free);
	g_signal_connect(G_OBJECT(event_box), "button-press-event",
					 G_CALLBACK(save_menu), image);

	/* Add padding as required. */
	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	/* The + 1 is in case the padding is odd. It ensures that one side will get the extra one pixel so that
	 * the padding is always correct. Without it, we'd be one pixel short whenever xpad or ypad was odd. */
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), ypad / 2, (ypad + 1) / 2, xpad / 2, (xpad + 1) / 2);
	gtk_container_add(GTK_CONTAINER(alignment), event_box);

	widget = gtk_vbox_new(FALSE, VBOX_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(widget), VBOX_BORDER);
	gtk_box_pack_start(GTK_BOX(widget), alignment, FALSE, FALSE, IMAGE_PADDING);

	/* Label */
	timestamp = purple_utf8_strftime(_("%x\n%X"),  localtime(&icon->timestamp));
	label = gtk_label_new(NULL);
	gtk_label_set_text(GTK_LABEL(label), timestamp);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(widget), label, TRUE, TRUE, LABEL_PADDING);

	anchor = gtk_text_buffer_create_child_anchor(text_buffer, &text_iter);
	gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(iconview), widget, anchor);

	gtk_widget_show_all(widget);
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(text_buffer, "word_wrap", &start, &end);

	g_free(icon);
	return TRUE;
}

static void update_icon_view(icon_viewer_key *key)
{
	GtkTextIter start;
	GtkTextIter end;
	GList *list = NULL;
	BuddyWindow *bw;
	GtkWidget *iconview;
	GtkTextBuffer *text_buffer;
	gboolean success = FALSE;

	bw = g_hash_table_lookup(buddy_windows, key);
	g_return_if_fail(bw != NULL);
	iconview = bw->iconview;
	text_buffer = bw->text_buffer;

	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	gtk_text_buffer_delete(text_buffer, &start, &end);

	if (key->contact != NULL)
	{
		PurpleBlistNode *bnode;

		/* Get the list of all the accounts of the contact and sort it. */
		for (bnode = ((PurpleBlistNode *)key->contact)->child; bnode ; bnode = bnode->next)
		{
			list = g_list_concat(retrieve_icons(purple_buddy_get_account((PurpleBuddy *)bnode),
			                                    purple_buddy_get_name((PurpleBuddy *)bnode)), list);
		}
	}
	else if (key->buddy != NULL)
	{
		list = retrieve_icons(purple_buddy_get_account(key->buddy),
		                      purple_buddy_get_name(key->buddy));
	}
	else
	{
		list = retrieve_icons(key->account, key->screenname);
	}

	/* Show icons for all the accounts of the contact. */
	if (list != NULL)
	{
		int id;

		list = g_list_sort(list, (GCompareFunc)buddy_icon_compare);
		success = (list != NULL);
		key->list = list;

		/* It's possible we already have one of these loops running, but
		 * having two won't harm anything, so we don't do anything about it. */
		id = g_idle_add(add_icon_from_list_cb, key);
		g_object_set_data_full(G_OBJECT(iconview), "update-idle-callback",
					GINT_TO_POINTER(id), (GDestroyNotify)g_source_remove);
	}

	if (!success)
	{
		/* No icons were found.  Display an appropriate message. */
		GtkWidget *hbox;
		char *filename;
		GdkPixbuf *pixbuf;
		GdkPixbuf *scaled;
		GtkWidget *image;
		char *str;
		GtkWidget *label;
		GtkTextIter text_iter;
		GtkTextChildAnchor *anchor;

		/* Hbox */
		/* Reuse spacing information that's used for the vbox which contains the image and text for each icon. */
		hbox = gtk_hbox_new(FALSE, VBOX_SPACING);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), VBOX_BORDER);

		/* Image */
#ifndef _WIN32
		filename = g_build_filename(PIXMAPSDIR, "dialogs", "purple_info.png", NULL);
#else
		filename = g_build_filename(wpurple_install_dir(), "pixmaps", "pidgin", "dialogs", "purple_info.png", NULL);
#endif

		pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
		g_free(filename);

		scaled = gdk_pixbuf_scale_simple(pixbuf, 48, 48, GDK_INTERP_BILINEAR);
		g_object_unref(G_OBJECT(pixbuf));

		image = gtk_image_new_from_pixbuf(scaled);
		g_object_unref(G_OBJECT(scaled));
		gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);


		/* Label */
		str = g_strdup_printf("<span size='larger' weight='bold'>%s</span>", _("No icons were found."));
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), str);
		g_free(str);

		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);


		/* Add the hbox to the text view. */
		gtk_text_buffer_get_iter_at_offset (text_buffer, &text_iter, 0);
		anchor = gtk_text_buffer_create_child_anchor(text_buffer, &text_iter);
		gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(iconview), hbox, anchor);
	}

	gtk_widget_show_all(iconview);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(iconview), FALSE);
}

static void update_runtime(icon_viewer_key *key, gpointer value, PurpleBuddy *buddy)
{
	PurpleAccount *account = purple_buddy_get_account(buddy);

	if (key->contact != NULL)
	{
		char *name = g_strdup(purple_normalize(account, purple_buddy_get_name(buddy)));
		PurpleBlistNode *node;

		for (node = ((PurpleBlistNode *)key->contact)->child; node; node = node->next)
		{
			if (account == purple_buddy_get_account((PurpleBuddy *)node) &&
			    !strcmp(name, purple_normalize(account, purple_buddy_get_name((PurpleBuddy *)node))))
			{
				g_free(name);
				update_icon_view(key);
				return;
			}
		}
		g_free(name);
	}
	else if (account == key->account &&
	         !strcmp(key->screenname, purple_normalize(account, purple_buddy_get_name(buddy))))
	{
		update_icon_view(key);
	}
}

void album_update_runtime(PurpleBuddy *buddy, gpointer data)
{
	g_hash_table_foreach(buddy_windows, (GHFunc)update_runtime, buddy);
}

static GtkWidget *get_viewer_icon()
{
	char *filename = NULL;
	GdkPixbuf *pixbuf;
	int width;
	int height;
	GdkPixbuf *scaled;
	GtkWidget *image;

	if (filename == NULL)
#ifndef _WIN32
		filename = g_build_filename(PIXMAPSDIR, "icons", "online.png", NULL);
#else
		filename = g_build_filename(wpurple_install_dir(), "pixmaps", "pidgin", "icons", "online.png", NULL);
#endif

	pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);

	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	/* Never scale the image up. */
	if (height > LABEL_ICON_SIZE || width > LABEL_ICON_SIZE)
	{
		/* Scale the image proportionally to fit with a square of size buddy_icon_size. */
		if (width > height)
		{
				int new_height = (int)(LABEL_ICON_SIZE / (double)width * height);
				scaled = gdk_pixbuf_scale_simple(pixbuf, LABEL_ICON_SIZE, new_height, GDK_INTERP_BILINEAR);
		}
		else
		{
				int new_width = (int)(LABEL_ICON_SIZE / (double)height * width);
				scaled = gdk_pixbuf_scale_simple(pixbuf, new_width, LABEL_ICON_SIZE, GDK_INTERP_BILINEAR);
		}
		g_object_unref(G_OBJECT(pixbuf));
	}
	else
	{
		scaled = pixbuf;
	}

	image = gtk_image_new_from_pixbuf(scaled);
	g_object_unref(G_OBJECT(scaled));
	return image;
}

static void view_buddy_icons_cb(PurpleBlistNode *node, gpointer data)
{
	gboolean contact_expanded;
	icon_viewer_key *key = g_new0(icon_viewer_key, 1);
	const char *name;

	g_return_if_fail(node != NULL);

	contact_expanded = pidgin_blist_node_is_contact_expanded(node);

	if (PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		if (!contact_expanded)
		{
			/* Work on the contact, since it's collapsed. */

			name = purple_contact_get_alias((PurpleContact*)node->parent);
			if (name == NULL)
				name = purple_buddy_get_name(((PurpleContact *)node->parent)->priority);

			if (node->next != NULL)
			{
				/* The contact has at least two buddies.  */
				key->contact = (PurpleContact *)node->parent;
			}
			else
			{
				/* Treat one-buddy contacts similar to buddies. */
				key->account = purple_buddy_get_account((PurpleBuddy *)node);
				key->screenname = g_strdup(purple_normalize(key->account, purple_buddy_get_name((PurpleBuddy *)node)));
				key->buddy = (PurpleBuddy *)node;
			}
		}
		else
		{
			/* The contact is expanded and the user has chosen a buddy. */

			key->account = purple_buddy_get_account((PurpleBuddy *)node);
			key->screenname = g_strdup(purple_normalize(key->account, purple_buddy_get_name((PurpleBuddy *)node)));
			key->buddy = (PurpleBuddy *)node;

			name = purple_buddy_get_alias_only((PurpleBuddy *)node);
			if (name == NULL)
			{
				name = purple_buddy_get_name((PurpleBuddy *)node);
			}
		}
	}
	else if (PURPLE_BLIST_NODE_IS_CONTACT(node))
	{
		/* The contact is expanded and the user has chosen the contact. */

		if (node->child != NULL && node->child->next != NULL)
		{
			/* The contact has at least two buddies.  */
			key->contact = (PurpleContact *)node;
		}
		else
		{
			/* Treat one-buddy contacts similar to buddies. */
			key->account = purple_buddy_get_account((PurpleBuddy *)node->child);
			key->screenname = g_strdup(purple_normalize(key->account, purple_buddy_get_name((PurpleBuddy *)node->child)));
			key->buddy = (PurpleBuddy *)node->child;
		}

		name = purple_contact_get_alias((PurpleContact*)node);
		if (name == NULL)
			name = purple_buddy_get_name(((PurpleContact *)node)->priority);
	}
	else
		g_return_if_reached();

	show_buddy_icon_window(key, name);
}

static gboolean compare_buddy_keys(icon_viewer_key *key1, BuddyWindow *bw, icon_viewer_key *key2)
{
	g_return_val_if_fail(key2->contact == NULL, FALSE);

	if (key1->contact == NULL)
	{
		if (key1->account == key2->account)
		{
			char *normal = g_strdup(purple_normalize(key1->account, key1->screenname));
			if (!strcmp(normal, purple_normalize(key2->account, key2->screenname)))
			{
				g_free(normal);
				return TRUE;
			}
			g_free(normal);
		}
	}

	return FALSE;
}

static void show_buddy_icon_window(icon_viewer_key *key, const char *name)
{
	char *title;
	GtkWidget *win;
	GtkWidget *vbox;
	char *str;
	GtkTextIter start;
	GtkTextIter end;
	GtkWidget *title_box;
	GtkWidget *label;
	GtkWidget *combo;
	GtkWidget *iconview;
	GtkTextBuffer *text_buffer;
	time_t now;
	const char *timestamp;
	PangoLayout *layout;
	int text_width;
	int text_height;
	int buddy_icon_pref = purple_prefs_get_int(PREF_ICON_SIZE);
	int buddy_icon_size;
	BuddyWindow *bw;

	/* Return if a window is already opened for the buddy. */
	if ((bw = g_hash_table_lookup(buddy_windows, key)) != NULL)
	{
		icon_viewer_key_free(key);
		gtk_window_present(GTK_WINDOW(bw->window));
		return;
	}

	/* If it was a contact, it would've matched above.
	 * Compare screennames...
	 */
	if (key->contact == NULL &&
	    (bw = g_hash_table_find(buddy_windows, (GHRFunc)compare_buddy_keys, key)) != NULL)
	{
		icon_viewer_key_free(key);
		gtk_window_present(GTK_WINDOW(bw->window));
		return;
	}

	/* Clamp the pref value to the allowable range. */
	buddy_icon_pref = MAX(0, buddy_icon_pref);
	buddy_icon_pref = MIN(2, buddy_icon_pref);

	buddy_icon_size = ICON_SIZE_MULTIPLIER * (buddy_icon_pref + 1);

	title = g_strdup_printf(_("Buddy Icons used by %s"), name);

	win = gtk_dialog_new_with_buttons(title, NULL, 0,
	                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_window_set_role(GTK_WINDOW(win), "buddy_icon_viewer");
	gtk_container_set_border_width(GTK_CONTAINER(win), 12);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(win)->vbox), vbox, TRUE, TRUE, 0);

	iconview = gtk_text_view_new();
	text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(iconview));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(iconview), FALSE);
#if ROW_PADDING != 0
	gtk_text_view_set_pixels_inside_wrap(GTK_TEXT_VIEW(iconview), ROW_PADDING);
#endif
	gtk_text_buffer_create_tag (text_buffer, "word_wrap",
	                            "wrap_mode", GTK_WRAP_WORD, NULL);
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(text_buffer, "word_wrap", &start, &end);

	/* Get the size of the text of a sample timestamp. */
	now = time(NULL);
	timestamp = purple_utf8_strftime("%x\n%X",  localtime(&now));
	layout = gtk_widget_create_pango_layout(iconview, timestamp);
	pango_layout_get_pixel_size(layout, &text_width, &text_height);


	/* Title Box */
	title_box = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(title_box), 6);
	gtk_box_pack_start(GTK_BOX(vbox), title_box, FALSE, FALSE, 0);


	/* Icon */
	gtk_box_pack_start(GTK_BOX(title_box), get_viewer_icon(), FALSE, FALSE, 0);


	/* Label */
	str = g_strdup_printf("<span size='larger' weight='bold'>%s</span>", title);
	g_free(title);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free(str);

	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(title_box), label, FALSE, FALSE, 0);


	/* Icon View */
	gtk_box_pack_start(GTK_BOX(vbox), wrap_in_scroller(iconview), TRUE, TRUE, 0);


	/* Size Selector */
	combo = gtk_combo_box_new_text();

	str = g_strdup_printf(_("Small (%1$ux%1$u)"), ICON_SIZE_MULTIPLIER);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), str);
	g_free(str);

	str = g_strdup_printf(_("Medium (%1$ux%1$u)"), ICON_SIZE_MULTIPLIER * 2);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), str);
	g_free(str);

	str = g_strdup_printf(_("Large (%1$ux%1$u)"), ICON_SIZE_MULTIPLIER * 3);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), str);
	g_free(str);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), buddy_icon_pref);

	gtk_widget_show_all(combo);
	gtk_signal_connect(GTK_OBJECT(combo), "changed", GTK_SIGNAL_FUNC(resize_icons), key);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(win)->action_area), combo, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(GTK_DIALOG(win)->action_area), combo, 0);

	/* Add the stuff in the hashtable. */
	bw = g_new0(BuddyWindow, 1);
	bw->window = win;
	bw->vbox = vbox;
	bw->iconview = iconview;
	bw->text_buffer = text_buffer;
	bw->text_height = text_height;
	bw->text_width = text_width;
	g_hash_table_insert(buddy_windows, key, bw);

	update_icon_view(key);

	gtk_widget_size_request(bw->iconview, &bw->requisition);
	set_window_geometry(bw, buddy_icon_size);

	gtk_window_set_default_size(GTK_WINDOW(win),
	                            purple_prefs_get_int(PREF_WINDOW_WIDTH),
	                            purple_prefs_get_int(PREF_WINDOW_HEIGHT));

	gtk_window_set_policy(GTK_WINDOW(win), FALSE, TRUE, FALSE);
	gtk_widget_show_all(win);

#if 0
	/* TODO: We need a way to disconnect this signal handler when the window is closed.
	 * TODO: Otherwise, it segfaults in update_runtime. */

	/* Register a signal handler to update the viewer if a new buddy icon is cached. */
	purple_signal_connect_priority(purple_buddy_icons_get_handle(), "buddy-icon-cached",
	                             purple_plugins_find_with_id(PLUGIN_ID), PURPLE_CALLBACK(update_runtime),
	                             key, PURPLE_SIGNAL_PRIORITY_DEFAULT + 1);
#endif

	gtk_signal_connect(GTK_OBJECT(win), "configure_event",
	                   GTK_SIGNAL_FUNC(update_size), NULL);
	g_signal_connect(G_OBJECT(win), "response",
	                 G_CALLBACK(window_close), key);
}

static gboolean has_stored_icons(PurpleBuddy *buddy)
{
	char *path = album_buddy_icon_get_dir(purple_buddy_get_account(buddy),
	                                      purple_buddy_get_name(buddy));
	GDir *dir = g_dir_open(path, 0, NULL);

	g_free(path);

	if (dir)
	{
		if (g_dir_read_name(dir))
		{
			g_dir_close(dir);
			return TRUE;
		}
		g_dir_close(dir);
	}

	return FALSE;
}

static void
album_select_dialog_cb(gpointer data, PurpleRequestFields *fields)
{
	char *username;
	PurpleAccount *account;

	account  = purple_request_fields_get_account(fields, "account");

	username = g_strdup(purple_normalize(account,
		purple_request_fields_get_string(fields,  "screenname")));

	if (username != NULL && *username != '\0' && account != NULL )
	{
		icon_viewer_key *key = g_new0(icon_viewer_key, 1);
		key->account = account;
		key->screenname = username;

		show_buddy_icon_window(key, username);
	}
}

/* Based on Pidgin's gtkdialogs.c, pidgindialogs_log(). */
static void album_select_dialog(PurplePluginAction *action)
{
	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	fields = purple_request_fields_new();

	group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(fields, group);

	field = purple_request_field_string_new("screenname", _("_Name"), NULL, FALSE);
	purple_request_field_set_type_hint(field, "screenname-all");
	purple_request_field_set_required(field, TRUE);
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_account_new("account", _("_Account"), NULL);
	purple_request_field_set_type_hint(field, "account");
	purple_request_field_account_set_show_all(field, TRUE);
	purple_request_field_set_visible(field, (purple_accounts_get_all() != NULL &&
	                                       purple_accounts_get_all()->next != NULL));
	purple_request_field_set_required(field, TRUE);
	purple_request_field_group_add_field(group, field);

	purple_request_fields(purple_get_blist(), _("View Buddy Icons..."),
	                    NULL,
	                    _("Please enter the screen name or alias of the person whose icon album you want to view."),
	                    fields,
	                    _("OK"), G_CALLBACK(album_select_dialog_cb),
	                    _("Cancel"), NULL,
	                    NULL, NULL, NULL,
	                    NULL);
}

GList *album_get_plugin_actions(PurplePlugin *plugin, gpointer data)
{
	GList *actions = NULL;

	actions = g_list_append(actions, purple_plugin_action_new(_("View Buddy Icons"), album_select_dialog));

	return actions;
}

void album_blist_node_menu_cb(PurpleBlistNode *node, GList **menu)
{
	gboolean contact_expanded;
	PurpleMenuAction *action;
	void (*callback)() = view_buddy_icons_cb;

	if (!(PURPLE_BLIST_NODE_IS_CONTACT(node) || PURPLE_BLIST_NODE_IS_BUDDY(node)))
		return;

	contact_expanded = pidgin_blist_node_is_contact_expanded(node);

	if (PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		if (contact_expanded)
		{
			if (!has_stored_icons((PurpleBuddy *)node))
			{
				callback = NULL;
			}
		}
		else if (PURPLE_BLIST_NODE_IS_CONTACT(node))
		{
			/* We don't want to show this option in buddy submenus. */
			if ((PurpleBlistNode *)((PurpleContact *)node->parent)->priority != node)
				return;

			/* Find the contact and fall through to the contact handling code. */
			node = node->parent;
		}
	}

	if (PURPLE_BLIST_NODE_IS_CONTACT(node))
	{
		PurpleBlistNode *bnode;

		for (bnode = node->child; bnode; bnode = bnode->next)
		{
			if (has_stored_icons((PurpleBuddy *)bnode))
				break;
		}

		/* bnode == NULL when the for loop made it all the way through. */
		if (bnode == NULL)
		{
			/* No icons found. */
			callback = NULL;
		}
	}

	/* Separator */
	(*menu) = g_list_append(*menu, NULL);

	action = purple_menu_action_new(_("_View Buddy Icons"), PURPLE_CALLBACK(callback), NULL, NULL);
	(*menu) = g_list_append(*menu, action);
}
