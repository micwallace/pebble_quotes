#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

// 7209a7d5-f36b-4253-a8db-e882e056606c
#define MY_UUID { 0x72, 0x09, 0xA7, 0xD5, 0xF3, 0x6B, 0x42, 0x53, 0xA8, 0xDB, 0xE8, 0x82, 0xE0, 0x56, 0x60, 0x6C }
PBL_APP_INFO(MY_UUID,
             "Quotes", "Wallace IT",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
ScrollLayer scroll_layer;
TextLayer text_layer;
InverterLayer inverter_layer;

enum {
  CMD_KEY = 0x0, // TUPLE_INTEGER
};
enum {
  CMD_UP = 0x01,
  CMD_DOWN = 0x02,
  QUOTE_TXT = 0x05,
  QUOTE_SRC = 0x06,
  QUOTE_CLEAR = 0x07,
  QUOTE_END = 0x07,
};

const int vert_scroll_text_padding = 4;

char quote_text[2048];

static void send_cmd(uint8_t cmd) {
  Tuplet value = TupletInteger(CMD_KEY, cmd);
  DictionaryIterator *iter;
  app_message_out_get(&iter);
  if (iter == NULL)
return;
  dict_write_tuplet(iter, &value);
  dict_write_end(iter);
  
  app_message_out_send();
  app_message_out_release();
}

// Modify these common button handlers
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	text_layer_set_text(&text_layer, "Connecting...");
  	GSize max_size = text_layer_get_max_used_size(app_get_current_graphics_context(), &text_layer);
  	scroll_layer_set_content_size(&scroll_layer, GSize(144, max_size.h + vert_scroll_text_padding));
  	scroll_layer_set_content_offset(&scroll_layer, GPoint(0,0), false);
  	send_cmd(CMD_UP);
}
void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
}

// This usually won't need to be modified
void override_click_config(ClickConfig **config, void *context) {
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;
}

void window_load(Window *me) {
	const GRect max_text_bounds = GRect(0, 0, 144, 2000);
	// Initialize the scroll layer
  	scroll_layer_init(&scroll_layer, me->layer.bounds);
  	// Attaches the callbacks before setting scroll clicks, the scroll config callback is x'd after that
	ScrollLayerCallbacks scrollOverride = {
		.click_config_provider = &override_click_config,
		.content_offset_changed_handler = NULL //Not interested.
	};
	scroll_layer_set_callbacks(&scroll_layer, scrollOverride);
	scroll_layer_set_click_config_onto_window(&scroll_layer, me);
	// Set the initial max size
  	scroll_layer_set_content_size(&scroll_layer, max_text_bounds.size);
  	// Initialize the text layer
  	text_layer_init(&text_layer, max_text_bounds);
	strcpy(quote_text, "");
  	text_layer_set_text(&text_layer, "Getting quote...");
	// set font
  	text_layer_set_font(&text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	// Trim text layer and scroll content to fit text box
  	GSize max_size = text_layer_get_max_used_size(app_get_current_graphics_context(), &text_layer);
  	text_layer_set_size(&text_layer, max_size);
  	scroll_layer_set_content_size(&scroll_layer, GSize(144, max_size.h + vert_scroll_text_padding));
	// Add the layers for display
  	scroll_layer_add_child(&scroll_layer, &text_layer.layer);
	// The inverter layer will highlight some text
  	//inverter_layer_init(&inverter_layer, GRect(0, 28, window.layer.frame.size.w, 28));
  	//scroll_layer_add_child(&scroll_layer, &inverter_layer.layer);
	layer_add_child(&me->layer, &scroll_layer.layer);
	// get the first quote
	send_cmd(CMD_UP);
}

static void out_sent_callback(DictionaryIterator *iter, void *context) {
	text_layer_set_text(&text_layer, "Loading...");
  	GSize max_size = text_layer_get_max_used_size(app_get_current_graphics_context(), &text_layer);
  	scroll_layer_set_content_size(&scroll_layer, GSize(144, max_size.h + vert_scroll_text_padding));
  	scroll_layer_set_content_offset(&scroll_layer, GPoint(0,0), false);
}

static void out_failed_callback(DictionaryIterator *iter, AppMessageResult reason, void* context) {
	text_layer_set_text(&text_layer, "Error while sending :(");
	GSize max_size = text_layer_get_max_used_size(app_get_current_graphics_context(), &text_layer);
  	scroll_layer_set_content_size(&scroll_layer, GSize(144, max_size.h + vert_scroll_text_padding));
  	scroll_layer_set_content_offset(&scroll_layer, GPoint(0,0), false);
}

static void in_received_callback(DictionaryIterator *iter, void *context) {
	// read the command value
	Tuple *start_tuple = dict_find(iter, QUOTE_CLEAR);
	if (start_tuple){
		strcpy(quote_text, "");
	}
	// read the values
	Tuple *txt_tuple = dict_find(iter, QUOTE_TXT);
	strcat(quote_text, txt_tuple->value->cstring);
        //snprintf(quote_text, 1024, txt_tuple->value->cstring);
	// if the transfer has ended, set the new text
	// read the command value
	//Tuple *end_tuple = dict_find(iter, QUOTE_END);
	//if (end_tuple){
		text_layer_set_text(&text_layer, quote_text);
		// set new scroll height and go to top
  		GSize max_size = text_layer_get_max_used_size(app_get_current_graphics_context(), &text_layer);
		text_layer_set_size(&text_layer, max_size);
  		scroll_layer_set_content_size(&scroll_layer, GSize(144, max_size.h + vert_scroll_text_padding));
  		scroll_layer_set_content_offset(&scroll_layer, GPoint(0,0), false);
	//}
}

static void in_dropped_callback(void* context, AppMessageResult reason) {
	text_layer_set_text(&text_layer, "Error while receiving :(");
	// set new scroll height and go to top
  	GSize max_size = text_layer_get_max_used_size(app_get_current_graphics_context(), &text_layer);
  	scroll_layer_set_content_size(&scroll_layer, GSize(144, max_size.h + vert_scroll_text_padding));
  	scroll_layer_set_content_offset(&scroll_layer, GPoint(0,0), false);
}

// Standard app initialisation
void handle_init(AppContextRef ctx) {
  window_init(&window, "Quotes");
  window_set_window_handlers(&window, (WindowHandlers) {
    .load = window_load,
  });
  window_stack_push(&window, true /* Animated */);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .messaging_info = {
      .buffer_sizes = {
        .inbound = 1024,
        .outbound = 32,
      },
      .default_callbacks.callbacks = {
        .out_sent = &out_sent_callback,
        .out_failed = &out_failed_callback,
        .in_received = &in_received_callback,
        .in_dropped = &in_dropped_callback,
      },
    }
  };
  app_event_loop(params, &handlers);
}
