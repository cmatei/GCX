/*******************************************************************************
  Copyright(c) 2009 Geoffrey Hausheer. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  Contact Information: gcx@phracturedblue.com <Geoffrey Hausheer>
*******************************************************************************/
//gcc -Wall -g -I. -o inditest indi.c indigui.c base64.c lilxml.c `pkg-config --cflags --libs gtk+-2.0 glib-2.0` -lz -DINDIMAIN
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../indi.h"
#include "../indigui.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtkled.h"
#include "indisave.h"

static GtkActionEntry menu_actions[] = {
	{ "indigui-file", NULL, "_File" },

	{ "indigui-file-save", GTK_STOCK_SAVE, "_Save", "<control>S", NULL,
	  G_CALLBACK(indisave_show_dialog_action) }
};

static char *menu_ui =
	"<ui>"
	"  <menubar name=\"indigui-menubar\">"
	"    <menu name=\"indigui-file\" action=\"indigui-file\">"
	"      <menuitem name=\"indigui-file-save\" action=\"indigui-file-save\"/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";


#if 0
static GtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,         NULL,           0, "<Branch>" },
  { "/File/_Save",    "<control>S", indisave_show_dialog,    0, "<StockItem>", GTK_STOCK_SAVE },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
#endif

enum {
	SWITCH_CHECKBOX,
	SWITCH_BUTTON,
	SWITCH_COMBOBOX,
};

void indigui_make_device_page(struct indi_device_t *idev)
{
	GtkWidget *parent_notebook;
	idev->window = gtk_notebook_new();
	parent_notebook = (GtkWidget *)g_object_get_data(G_OBJECT (idev->indi->window), "notebook");
	gtk_notebook_append_page(GTK_NOTEBOOK (parent_notebook), GTK_WIDGET (idev->window), gtk_label_new(idev->name));
	gtk_widget_show_all(parent_notebook);
}

void indigui_prop_add_signal(struct indi_prop_t *iprop, void *object, unsigned long signal)
{
	struct indi_signals_t *sig = (struct indi_signals_t *)calloc(1, sizeof(struct indi_signals_t));
	sig->object = object;
	sig->signal = signal;
	iprop->signals = il_prepend(iprop->signals, sig);
}

void indigui_prop_set_signals(struct indi_prop_t *iprop, int active)
{
	indi_list *isl;

	for (isl = il_iter(iprop->signals); ! il_is_last(isl); isl = il_next(isl)) {
                struct indi_signals_t *sig = (struct indi_signals_t *)il_item(isl);
		if(active) {
			g_signal_handler_unblock(G_OBJECT (sig->object), sig->signal);
		} else {
			g_signal_handler_block(G_OBJECT (sig->object), sig->signal);
		}
	}
}

static void indigui_set_state(GtkWidget *led, int state)
{
	switch(state) {
		case INDI_STATE_IDLE:  gtk_led_set_color(led, 0x808080); break;
		case INDI_STATE_OK:    gtk_led_set_color(led, 0x008000); break;
		case INDI_STATE_BUSY:  gtk_led_set_color(led, 0xFFFF00); break;
		case INDI_STATE_ALERT: gtk_led_set_color(led, 0xFF0000); break;
	}
	gtk_widget_set_tooltip_text(led, indi_get_string_from_state(state));
}

static int indigui_get_switch_type(struct indi_prop_t *iprop)
{
	int num_props = il_length(iprop->elems);

	if (iprop->rule == INDI_RULE_ANYOFMANY)
		return SWITCH_CHECKBOX;

	if (num_props <= 4)
		return SWITCH_BUTTON;

	return SWITCH_COMBOBOX;
}

void indigui_update_widget(struct indi_prop_t *iprop)
{
	void *value;
	GtkWidget *state_label;
	indi_list *isl;
	char val[80];
	int switch_type = -1;

	indigui_prop_set_signals(iprop, 0);
	if (iprop->type == INDI_PROP_SWITCH)
		switch_type = indigui_get_switch_type(iprop);
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);

		value = (GtkWidget *)g_object_get_data(G_OBJECT (iprop->widget), elem->name);
		switch (iprop->type) {
		case INDI_PROP_TEXT:
			gtk_label_set_text(GTK_LABEL (value), elem->value.str);
			break;
		case INDI_PROP_NUMBER:
			sprintf(val, "%f", elem->value.num.value);
			gtk_label_set_text(GTK_LABEL (value), val);
			break;
		case INDI_PROP_SWITCH:
			switch (switch_type) {
			case SWITCH_BUTTON:
			case SWITCH_CHECKBOX:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (value), elem->value.set);
				break;
			case SWITCH_COMBOBOX:
				if (elem->value.set) {
					GtkWidget *combo = (GtkWidget *)g_object_get_data(G_OBJECT (iprop->widget), "__combo");
					long iter = (long)value;
					gtk_combo_box_set_active(GTK_COMBO_BOX (combo), iter);
				}
				break;
			}
			break;
		}
	}
	state_label = (GtkWidget *)g_object_get_data(G_OBJECT (iprop->widget), "_state");
	indigui_set_state(state_label, iprop->state);

	//Display any message
	indigui_show_message(iprop->idev->indi, iprop->message);
	iprop->message[0] = 0;

	indigui_prop_set_signals(iprop, 1);
}

void indigui_show_message(struct indi_t *indi, const char *message)
{
	GtkTextBuffer *textbuffer;
	GtkTextIter text_start;

	if (strlen(message) > 0) {
		time_t curtime;
		char timestr[30];
		struct tm time_loc;

		textbuffer = (GtkTextBuffer *)g_object_get_data(G_OBJECT (indi->window), "textbuffer");
		gtk_text_buffer_get_start_iter(textbuffer, &text_start);
		gtk_text_buffer_place_cursor(textbuffer, &text_start);
		curtime = time(NULL);
		localtime_r(&curtime, &time_loc);
		strftime(timestr, sizeof(timestr), "%b %d %T: ", &time_loc);
		gtk_text_buffer_insert_at_cursor(textbuffer, timestr, -1);
		gtk_text_buffer_insert_at_cursor(textbuffer, message, -1);
		gtk_text_buffer_insert_at_cursor(textbuffer, "\n", -1);
	}
}

static void indigui_send_cb( GtkWidget *widget, struct indi_prop_t *iprop )
{
	const char *valstr;
	indi_list *isl;
	GtkWidget *value;
	GtkWidget *entry;

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		value = (GtkWidget *)g_object_get_data(G_OBJECT (iprop->widget), elem->name);
		switch (iprop->type) {
		case INDI_PROP_TEXT:
			entry = (GtkWidget *)g_object_get_data(G_OBJECT (value), "entry");
			valstr = gtk_entry_get_text(GTK_ENTRY (entry));
			strncpy(elem->value.str, valstr, sizeof(elem->value.str));
			gtk_entry_set_text(GTK_ENTRY (entry), "");
			break;
		case INDI_PROP_NUMBER:
			entry = (GtkWidget *)g_object_get_data(G_OBJECT (value), "entry");
			valstr = gtk_entry_get_text(GTK_ENTRY (entry));
			elem->value.num.value = strtod(valstr, NULL);
			gtk_entry_set_text(GTK_ENTRY (entry), "");
			break;
		}
	}
	indi_send(iprop, NULL);
}

static void indigui_send_switch_combobox_cb( GtkWidget *widget, struct indi_prop_t *iprop )
{
	indi_list *isl;
	GtkWidget *value;
	long iter, elem_iter;

	value = (GtkWidget *)g_object_get_data(iprop->widget, "__combo");
	if ((iter = gtk_combo_box_get_active(GTK_COMBO_BOX (value))) != -1) {
		for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
			struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
			elem_iter = (long)g_object_get_data(G_OBJECT (iprop->widget), elem->name);
			if (elem_iter == iter) {
				elem->value.set = 1;
				indi_send(iprop, elem);
				break;
			}
		}
	}
}

static void indigui_send_switch_button_cb( GtkWidget *widget, struct indi_prop_t *iprop )
{
	indi_list *isl;
	GtkWidget *value;
	int elem_state;

	//We let INDI handle the mutex condition
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		value = (GtkWidget *)g_object_get_data(G_OBJECT (iprop->widget), elem->name);
		elem_state  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (value));
		if (widget == value) {
			elem->value.set = elem_state;
			indi_send(iprop, elem);
			break;
		}
	}
}

static void indigui_create_text_widget(struct indi_prop_t *iprop, int num_props)
{
	int pos = 0;
	GtkWidget *value;
	GtkWidget *entry;
	GtkWidget *button;
	indi_list *isl;
	unsigned long signal;

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);

		gtk_table_attach(GTK_TABLE (iprop->widget),
			gtk_label_new(elem->label),
			0, 1, pos, pos + 1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

		value = gtk_label_new(elem->value.str);
		g_object_ref(G_OBJECT (value));
		g_object_set_data(G_OBJECT (iprop->widget), elem->name, value);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			value,
			1, 2, pos, pos + 1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

		if (iprop->permission != INDI_RO) {
			entry = gtk_entry_new();
			g_object_ref(G_OBJECT (entry));
			g_object_set_data(G_OBJECT (value), "entry", entry);
			gtk_table_attach(GTK_TABLE (iprop->widget),
				entry,
				2, 3, pos, pos + 1,
				(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
		}
	}
	if (iprop->permission != INDI_RO) {
		button = gtk_button_new_with_label("Set");
		signal = g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (indigui_send_cb), iprop);
		indigui_prop_add_signal(iprop, button, signal);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			button,
			3, 4, 0, num_props,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	}

}

static void indigui_create_switch_combobox(struct indi_prop_t *iprop, int num_props)
{
	GtkTreeIter iter;
	GtkTreeStore *model = gtk_tree_store_new(1, G_TYPE_STRING);
	GtkWidget *box;
	indi_list *isl;
	unsigned long signal;
	long pos = 0;

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		gtk_tree_store_append(model, &iter, NULL);
		gtk_tree_store_set(model, &iter, 0, elem->label, -1);
		g_object_set_data(G_OBJECT (iprop->widget), elem->name, (void *)pos);
	}
	box = gtk_combo_box_new_with_model(GTK_TREE_MODEL (model));
	g_object_ref(box);
	g_object_set_data(G_OBJECT (iprop->widget), "__combo", box);
	gtk_table_attach(GTK_TABLE (iprop->widget),
		box,
		0, 1, 0, 1,
		(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	signal = g_signal_connect(G_OBJECT (box), "changed", G_CALLBACK (indigui_send_switch_combobox_cb), iprop);
	indigui_prop_add_signal(iprop, box, signal);
}

static void indigui_create_switch_button(struct indi_prop_t *iprop, int num_props, int type)
{
	int pos = 0;
	GtkWidget *button;
	indi_list *isl;
	unsigned long signal;

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);

		if (type == SWITCH_BUTTON)
			button = gtk_toggle_button_new_with_label(elem->label);
		else
			button = gtk_check_button_new_with_label(elem->label);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			button,
			pos % 4, (pos % 4) + 1, pos / 4, (pos / 4) + 1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
		g_object_ref(G_OBJECT (button));
		g_object_set_data(G_OBJECT (iprop->widget), elem->name, button);
		if (elem->value.set) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button), TRUE);
		}
		if (iprop->permission == INDI_RO) {
			gtk_widget_set_sensitive(button, FALSE);
		}
		signal = g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (indigui_send_switch_button_cb), iprop);
		indigui_prop_add_signal(iprop, button, signal);
	}
}

static void indigui_create_switch_widget(struct indi_prop_t *iprop, int num_props)
{
	int guitype = indigui_get_switch_type(iprop);

	switch (guitype) {
		case SWITCH_COMBOBOX: indigui_create_switch_combobox(iprop, num_props); break;
		case SWITCH_CHECKBOX:
		case SWITCH_BUTTON:   indigui_create_switch_button(iprop, num_props, guitype);   break;
	}
}

static void indigui_create_number_widget(struct indi_prop_t *iprop, int num_props)
{
	int pos = 0;
	GtkWidget *value;
	GtkWidget *entry;
	GtkWidget *button;
	indi_list *isl;
	char val[80];
	unsigned long signal;

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);

		gtk_table_attach(GTK_TABLE (iprop->widget),
			gtk_label_new(elem->label),
			0, 1, pos, pos + 1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

		sprintf(val, "%f", elem->value.num.value);
		value = gtk_label_new(val);
		g_object_ref(G_OBJECT (value));
		g_object_set_data(G_OBJECT (iprop->widget), elem->name, value);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			value,
			1, 2, pos, pos + 1,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

		if (iprop->permission != INDI_RO) {
			entry = gtk_entry_new();
			g_object_ref(G_OBJECT (entry));
			g_object_set_data(G_OBJECT (value), "entry", entry);
			gtk_table_attach(GTK_TABLE (iprop->widget),
				entry,
				2, 3, pos, pos + 1,
				(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
		}
	}
	if (iprop->permission != INDI_RO) {
		button = gtk_button_new_with_label("Set");
		signal = g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (indigui_send_cb), iprop);
		indigui_prop_add_signal(iprop, button, signal);
		gtk_table_attach(GTK_TABLE (iprop->widget),
			button,
			3, 4, 0, num_props,
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	}
}

static void indigui_create_light_widget(struct indi_prop_t *iprop, int num_props)
{
}

static void indigui_create_blob_widget(struct indi_prop_t *iprop, int num_props)
{
}

static void indigui_build_prop_widget(struct indi_prop_t *iprop)
{
	GtkWidget *state_label;
	GtkWidget *name_label;
	int num_props;

 	num_props = il_length(iprop->elems);
	iprop->widget = gtk_table_new(num_props, 4, FALSE);

	state_label = gtk_led_new(0x000000);
	indigui_set_state(state_label, iprop->state);

	g_object_ref(G_OBJECT (state_label));
	g_object_set_data(G_OBJECT (iprop->widget), "_state", state_label);

	name_label = gtk_label_new(iprop->name);
	g_object_ref(G_OBJECT (name_label));
	g_object_set_data(G_OBJECT (iprop->widget), "_name", name_label);

	switch (iprop->type) {
	case INDI_PROP_TEXT:
		indigui_create_text_widget(iprop, num_props);
		break;
	case INDI_PROP_SWITCH:
		indigui_create_switch_widget(iprop, num_props);
		break;
	case INDI_PROP_NUMBER:
		indigui_create_number_widget(iprop, num_props);
		break;
	case INDI_PROP_LIGHT:
		indigui_create_light_widget(iprop, num_props);
		break;
	case INDI_PROP_BLOB:
		indigui_create_blob_widget(iprop, num_props);
		break;
	}
}

void indigui_add_prop(struct indi_device_t *idev, const char *groupname, struct indi_prop_t *iprop)
{
	GtkWidget *page;
	long next_free_row;

	page = (GtkWidget *)g_object_get_data(G_OBJECT (idev->window), groupname);
	if (! page) {
		page = gtk_table_new(1, 4, FALSE);
		gtk_notebook_append_page(GTK_NOTEBOOK (idev->window), page, gtk_label_new(groupname));
		g_object_set_data(G_OBJECT (page), "next-free-row", 0);
		g_object_set_data(G_OBJECT (idev->window), groupname, page);
	}
	next_free_row = (long) g_object_get_data(G_OBJECT (page), "next-free-row");

	indigui_build_prop_widget(iprop);
	gtk_table_attach(GTK_TABLE (page),
		GTK_WIDGET (g_object_get_data( G_OBJECT (iprop->widget), "_state")),
		0, 1, next_free_row, next_free_row + 1,
		GTK_FILL, GTK_FILL, 20, 10);
	gtk_table_attach(GTK_TABLE (page),
		GTK_WIDGET (g_object_get_data( G_OBJECT (iprop->widget), "_name")),
		1, 2, next_free_row, next_free_row + 1,
		GTK_FILL, GTK_FILL, 20, 10);
	gtk_table_attach(GTK_TABLE (page),
		GTK_WIDGET(iprop->widget),
		2, 3, next_free_row, next_free_row + 1,
		(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	g_object_set_data(G_OBJECT (page), "next-free-row", (gpointer) (next_free_row + 1));
	gtk_widget_show_all(page);
}

void *indigui_create_window(struct indi_t *indi)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *textview;
	GtkWidget *textscroll;
	GtkTextBuffer *textbuffer;

	GtkWidget *menubar;
	GtkUIManager *ui;
	GError *error = NULL;
	GtkActionGroup *action_group;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER (window), vbox);

	action_group = gtk_action_group_new("INDIAction");
	gtk_action_group_add_actions (action_group, menu_actions,
				      G_N_ELEMENTS (menu_actions), indi);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, action_group, 0);

	gtk_ui_manager_add_ui_from_string (ui, menu_ui, strlen(menu_ui), &error);

	menubar = gtk_ui_manager_get_widget (ui, "/indigui-menubar");

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (ui));

	/* Finally, return the actual menu bar created by the item factory. */
	gtk_box_pack_start(GTK_BOX (vbox), menubar, FALSE, TRUE, 0);

	g_object_unref (ui);

	notebook = gtk_notebook_new();
	g_object_ref(G_OBJECT (notebook));
	g_object_set_data(G_OBJECT (window), "notebook", notebook);
	gtk_widget_show(notebook);
	gtk_container_add(GTK_CONTAINER (vbox), notebook);

	textscroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(textscroll);
	gtk_container_add(GTK_CONTAINER (vbox), textscroll);

	textbuffer = gtk_text_buffer_new(NULL);
	g_object_ref(G_OBJECT (textbuffer));
	g_object_set_data(G_OBJECT (window), "textbuffer", textbuffer);

	textview = gtk_text_view_new_with_buffer(textbuffer);
	gtk_text_view_set_editable(GTK_TEXT_VIEW (textview), FALSE);
	g_object_ref(G_OBJECT (textview));
	g_object_set_data(G_OBJECT (window), "textview", textview);
	gtk_widget_show(textview);
	gtk_container_add(GTK_CONTAINER (textscroll), textview);

	gtk_window_set_title (GTK_WINDOW (window), "INDI Options");
	gtk_window_set_default_size (GTK_WINDOW (window), 640, 400);

	return window;
}

void indigui_show_dialog(void *data)
{
    struct indi_t *indi = (struct indi_t *)data;
    gtk_widget_show_all(GTK_WIDGET(indi->window));
}

