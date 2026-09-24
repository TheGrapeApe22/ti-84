#include <fileioc.h>
#include <graphx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ti/getcsc.h>

#include "units.h"

#define INPUT_CAPACITY 65
#define HISTORY_CAPACITY 6
#define HISTORY_APPVAR "UNITHIST"
#define HISTORY_HEADER "UCH1"
#define COLOR_BACKGROUND 0
#define COLOR_PANEL 1
#define COLOR_TEXT 2
#define COLOR_MUTED 3
#define COLOR_ACCENT 4
#define COLOR_SELECTION 5

typedef enum { PROMPT_HAVE, PROMPT_WANT } prompt_t;
typedef struct {
	char have[INPUT_CAPACITY];
	char want[INPUT_CAPACITY];
	char result[UNITS_RESULT_CAPACITY];
} history_entry_t;

static history_entry_t history[HISTORY_CAPACITY];
static uint8_t history_count;
static int8_t selected_history = -1;
static char input[INPUT_CAPACITY];
static char pending_have[INPUT_CAPACITY];
static uint8_t input_length, cursor_position;
static bool alpha_mode = true, uppercase_once;
static bool database_ready;
static char database_error[UNITS_RESULT_CAPACITY];
static char prompt_error[UNITS_RESULT_CAPACITY];
static prompt_t prompt = PROMPT_HAVE;

static void copy_text(char *out, size_t cap, const char *text) {
	strncpy(out, text, cap - 1);
	out[cap - 1] = '\0';
}

static void load_history(void) {
	uint8_t handle, count, index;
	uint16_t expected_size;
	char header[4];

	handle = ti_Open(HISTORY_APPVAR, "r");
	if (!handle)
		return;
	if (ti_Read(header, 1, sizeof(header), handle) != sizeof(header) ||
		memcmp(header, HISTORY_HEADER, sizeof(header)) ||
		ti_Read(&count, 1, 1, handle) != 1 || count > HISTORY_CAPACITY) {
		ti_Close(handle);
		return;
	}
	expected_size = 5 + (uint16_t)count * sizeof(history[0]);
	if (ti_GetSize(handle) != expected_size ||
		(count &&
		 ti_Read(history, sizeof(history[0]), count, handle) != count)) {
		ti_Close(handle);
		return;
	}
	ti_Close(handle);
	for (index = 0; index < count; index++) {
		if (history[index].have[INPUT_CAPACITY - 1] ||
			history[index].want[INPUT_CAPACITY - 1] ||
			history[index].result[UNITS_RESULT_CAPACITY - 1]) {
			memset(history, 0, sizeof(history));
			return;
		}
	}
	history_count = count;
}

static bool save_history(void) {
	uint8_t handle;

	handle = ti_Open(HISTORY_APPVAR, "w");
	if (!handle)
		return false;
	if (ti_Write(HISTORY_HEADER, 1, 4, handle) != 4 ||
		ti_Write(&history_count, 1, 1, handle) != 1 ||
		(history_count && ti_Write(history, sizeof(history[0]), history_count,
								   handle) != history_count)) {
		ti_Close(handle);
		return false;
	}
	ti_Close(handle);

	handle = ti_Open(HISTORY_APPVAR, "r");
	if (!handle)
		return false;
	ti_SetGCBehavior(NULL, NULL);
	if (!ti_SetArchiveStatus(true, handle) || !ti_IsArchived(handle)) {
		ti_Close(handle);
		return false;
	}
	ti_Close(handle);
	return true;
}

static char alpha_character(uint8_t key) {
	switch (key) {
	case sk_Math:
		return 'A';
	case sk_Apps:
		return 'B';
	case sk_Prgm:
		return 'C';
	case sk_Recip:
		return 'D';
	case sk_Sin:
		return 'E';
	case sk_Cos:
		return 'F';
	case sk_Tan:
		return 'G';
	case sk_Power:
		return 'H';
	case sk_Square:
		return 'I';
	case sk_Comma:
		return 'J';
	case sk_LParen:
		return 'K';
	case sk_RParen:
		return 'L';
	case sk_Div:
		return 'M';
	case sk_Log:
		return 'N';
	case sk_7:
		return 'O';
	case sk_8:
		return 'P';
	case sk_9:
		return 'Q';
	case sk_Mul:
		return 'R';
	case sk_Ln:
		return 'S';
	case sk_4:
		return 'T';
	case sk_5:
		return 'U';
	case sk_6:
		return 'V';
	case sk_Sub:
		return 'W';
	case sk_Store:
		return 'X';
	case sk_1:
		return 'Y';
	case sk_2:
		return 'Z';
	case sk_0:
		return ' ';
	default:
		return '\0';
	}
}

static char number_character(uint8_t key) {
	switch (key) {
	case sk_0:
		return '0';
	case sk_1:
		return '1';
	case sk_2:
		return '2';
	case sk_3:
		return '3';
	case sk_4:
		return '4';
	case sk_5:
		return '5';
	case sk_6:
		return '6';
	case sk_7:
		return '7';
	case sk_8:
		return '8';
	case sk_9:
		return '9';
	case sk_DecPnt:
		return '.';
	case sk_Add:
		return '+';
	case sk_Sub:
		return '-';
	case sk_Mul:
		return '*';
	case sk_Div:
		return '/';
	case sk_Power:
		return '^';
	case sk_LParen:
		return '(';
	case sk_RParen:
		return ')';
	default:
		return '\0';
	}
}

static void reset_input(void) {
	input[0] = '\0';
	input_length = 0;
	cursor_position = 0;
}

static void insert_character(char c) {
	if (c && input_length < INPUT_CAPACITY - 1) {
		memmove(&input[cursor_position + 1], &input[cursor_position],
				input_length - cursor_position + 1);
		input[cursor_position++] = c;
		input_length++;
	}
}

static void recall_history(void) {
	const char *text;
	if (selected_history < 0)
		return;
	text = prompt == PROMPT_HAVE ? history[(uint8_t)selected_history].have
								 : history[(uint8_t)selected_history].want;
	while (*text && input_length < INPUT_CAPACITY - 1)
		insert_character(*text++);
	selected_history = -1;
}

static void delete_selected_history(void) {
	uint8_t index = (uint8_t)selected_history;
	if (index + 1 < history_count)
		memmove(&history[index], &history[index + 1],
				sizeof(history[0]) * (history_count - index - 1));
	history_count--;
	if (!history_count)
		selected_history = -1;
	else if (index >= history_count)
		selected_history = (int8_t)history_count - 1;
}

static void submit_input(void) {
	history_entry_t *entry;
	char result[UNITS_RESULT_CAPACITY];

	if (prompt == PROMPT_HAVE) {
		if (!input_length)
			return;
		if (!units_validate_have(input, prompt_error, sizeof(prompt_error)))
			return;
		prompt_error[0] = '\0';
		copy_text(pending_have, sizeof(pending_have), input);
		reset_input();
		prompt = PROMPT_WANT;
		return;
	}

	if (input_length) {
		if (!units_convert(pending_have, input, result, sizeof(result))) {
			copy_text(prompt_error, sizeof(prompt_error), result);
			return;
		}
	} else if (!units_describe(pending_have, result, sizeof(result))) {
		copy_text(prompt_error, sizeof(prompt_error), result);
		return;
	}

	if (history_count == HISTORY_CAPACITY) {
		memmove(&history[0], &history[1],
				sizeof(history[0]) * (HISTORY_CAPACITY - 1));
		history_count--;
	}
	entry = &history[history_count++];
	copy_text(entry->have, sizeof(entry->have), pending_have);
	copy_text(entry->want, sizeof(entry->want), input);
	copy_text(entry->result, sizeof(entry->result), result);
	prompt_error[0] = '\0';
	reset_input();
	pending_have[0] = '\0';
	prompt = PROMPT_HAVE;
}

static void handle_key(uint8_t key) {
	char c;
	if (key == sk_2nd) {
		alpha_mode = true;
		uppercase_once = !uppercase_once;
		return;
	}
	if (key == sk_Alpha) {
		alpha_mode = !alpha_mode;
		uppercase_once = false;
		return;
	}
	if (key == sk_Left) {
		selected_history = -1;
		if (cursor_position)
			cursor_position--;
		return;
	}
	if (key == sk_Right) {
		selected_history = -1;
		if (cursor_position < input_length)
			cursor_position++;
		return;
	}
	if (key == sk_Up) {
		if (!history_count)
			return;
		if (selected_history < 0)
			selected_history = (int8_t)history_count - 1;
		else if (selected_history > 0)
			selected_history--;
		return;
	}
	if (key == sk_Down) {
		if (selected_history < 0)
			return;
		if (selected_history < (int8_t)history_count - 1)
			selected_history++;
		else
			selected_history = -1;
		return;
	}
	if (key == sk_Enter) {
		if (selected_history >= 0)
			recall_history();
		else
			submit_input();
		return;
	}
	if (key == sk_Clear) {
		if (selected_history >= 0) {
			delete_selected_history();
		} else {
			reset_input();
			prompt_error[0] = '\0';
			uppercase_once = false;
		}
		return;
	}
	if (key == sk_Del) {
		if (selected_history >= 0) {
			delete_selected_history();
			return;
		}
		if (cursor_position) {
			memmove(&input[cursor_position - 1], &input[cursor_position],
					input_length - cursor_position + 1);
			cursor_position--;
			input_length--;
			prompt_error[0] = '\0';
		}
		return;
	}
	selected_history = -1;
	c = alpha_mode ? alpha_character(key) : number_character(key);
	if (alpha_mode && c >= 'A' && c <= 'Z') {
		if (!uppercase_once)
			c += 'a' - 'A';
		uppercase_once = false;
	}
	if (c)
		prompt_error[0] = '\0';
	insert_character(c);
}

static void print_at(const char *text, int x, int y, uint8_t color) {
	gfx_SetTextFGColor(color);
	gfx_SetTextXY(x, y);
	gfx_PrintString(text);
}

static uint8_t wrapped_length(const char *text, unsigned int width) {
	char line[INPUT_CAPACITY + 1];
	uint8_t length = 0;
	while (text[length] && length < sizeof(line) - 1) {
		line[length] = text[length];
		line[length + 1] = '\0';
		if (gfx_GetStringWidth(line) > width)
			break;
		length++;
	}
	if (!length && *text)
		length = 1;
	return length;
}

static int draw_wrapped(const char *text, int x, int y, unsigned int width,
						uint8_t max_lines, uint8_t color) {
	char line[INPUT_CAPACITY + 1];
	uint8_t row = 0, length;
	while (*text && row < max_lines) {
		length = wrapped_length(text, width);
		memcpy(line, text, length);
		line[length] = '\0';
		print_at(line, x, y, color);
		text += length;
		y += 18;
		row++;
	}
	return y;
}

static void cursor_location(int *x, int *y) {
	const char *at = input;
	uint8_t consumed = 0, row = 0, length, prefix_length;
	char prefix[INPUT_CAPACITY + 1];
	while (row < 2) {
		length = wrapped_length(at, 292);
		if (cursor_position <= consumed + length || !at[length]) {
			prefix_length = cursor_position - consumed;
			memcpy(prefix, at, prefix_length);
			prefix[prefix_length] = '\0';
			*x = 20 + gfx_GetStringWidth(prefix);
			*y = 194 + row * 18;
			return;
		}
		consumed += length;
		at += length;
		row++;
	}
	*x = 20;
	*y = 212;
}

static uint8_t wrapped_line_count(const char *text, unsigned int width,
								  uint8_t maximum) {
	uint8_t lines = 0, length;
	do {
		length = wrapped_length(text, width);
		text += length;
		lines++;
	} while (*text && lines < maximum);
	return lines;
}

static void history_layout(uint8_t index, char *line, uint8_t *command_lines,
						   uint8_t *result_lines) {
	if (history[index].want[0])
		snprintf(line, INPUT_CAPACITY * 2 + 5, "%s -> %s", history[index].have,
				 history[index].want);
	else
		copy_text(line, INPUT_CAPACITY * 2 + 5, history[index].have);
	*command_lines = wrapped_line_count(line, 296, 3);
	*result_lines =
		wrapped_line_count(history[index].result, 296, 5 - *command_lines);
}

static uint8_t history_height(uint8_t index) {
	char line[INPUT_CAPACITY * 2 + 5];
	uint8_t command_lines, result_lines;
	history_layout(index, line, &command_lines, &result_lines);
	return (command_lines + result_lines) * 18 + 4;
}

static void draw_screen(void) {
	uint8_t first = 0, last = 0, index, used = 0;
	int cursor_x, cursor_y, y = 28;
	if (history_count) {
		last = selected_history >= 0 ? (uint8_t)selected_history
									 : history_count - 1;
		first = last;
		used = history_height(first);
		while (last + 1 < history_count &&
			   used + history_height(last + 1) <= 108) {
			last++;
			used += history_height(last);
		}
		while (first && used + history_height(first - 1) <= 108) {
			first--;
			used += history_height(first);
		}
	}
	gfx_FillScreen(COLOR_BACKGROUND);
	gfx_SetColor(COLOR_PANEL);
	gfx_FillRectangle(0, 0, 320, 24);
	print_at("units", 8, 8, COLOR_TEXT);
	print_at(uppercase_once ? "(ABC)" : (alpha_mode ? "(abc)" : "(123)"), 270,
			 8, COLOR_ACCENT);
	if (history_count)
		for (index = first; index <= last; index++) {
			char line[INPUT_CAPACITY * 2 + 5];
			uint8_t command_lines, result_lines, height;
			history_layout(index, line, &command_lines, &result_lines);
			height = (command_lines + result_lines) * 18 + 4;
			if ((int8_t)index == selected_history) {
				gfx_SetColor(COLOR_SELECTION);
				gfx_FillRectangle(4, y - 3, 312, height);
			}
			print_at(">", 8, y, COLOR_ACCENT);
			y = draw_wrapped(line, 20, y, 296, command_lines, COLOR_TEXT);
			y = draw_wrapped(history[index].result, 20, y, 296, result_lines,
							 COLOR_MUTED) +
				4;
		}
	if (!history_count) {
		if (database_ready)
			print_at("Enter a quantity and unit :P", 8, 48, COLOR_MUTED);
		else {
			print_at("Database error:", 8, 42, COLOR_ACCENT);
			draw_wrapped(database_error, 8, 63, 304, 3, COLOR_MUTED);
		}
	}
	gfx_SetColor(COLOR_PANEL);
	gfx_FillRectangle(0, 136, 320, 104);
	if (prompt_error[0])
		draw_wrapped(prompt_error, 8, 140, 304, 2, COLOR_ACCENT);
	else if (prompt == PROMPT_WANT) {
		char have_line[INPUT_CAPACITY + 7];
		snprintf(have_line, sizeof(have_line), "You have: %s", pending_have);
		draw_wrapped(have_line, 8, 140, 304, 2, COLOR_MUTED);
	}
	print_at(prompt == PROMPT_WANT ? "You want:" : "You have:", 8, 177,
			 COLOR_MUTED);
	print_at(">", 8, 197, COLOR_ACCENT);
	draw_wrapped(input, 20, 197, 292, 2, COLOR_TEXT);
	cursor_location(&cursor_x, &cursor_y);
	gfx_SetColor(COLOR_ACCENT);
	gfx_VertLine(cursor_x, cursor_y, 18);
}

int main(void) {
	uint8_t key;
	database_ready = units_load(database_error, sizeof(database_error));
	load_history();
	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_SetTextScale(1, 2);
	gfx_SetTextBGColor(COLOR_BACKGROUND);
	gfx_SetTextTransparentColor(COLOR_BACKGROUND);
	gfx_palette[COLOR_BACKGROUND] = gfx_RGBTo1555(18, 22, 30);
	gfx_palette[COLOR_PANEL] = gfx_RGBTo1555(31, 38, 51);
	gfx_palette[COLOR_TEXT] = gfx_RGBTo1555(235, 239, 245);
	gfx_palette[COLOR_MUTED] = gfx_RGBTo1555(145, 156, 173);
	gfx_palette[COLOR_ACCENT] = gfx_RGBTo1555(82, 189, 214);
	gfx_palette[COLOR_SELECTION] = gfx_RGBTo1555(48, 65, 82);
	draw_screen();
	gfx_SwapDraw();
	for (;;) {
		if (boot_CheckOnPressed() && uppercase_once)
			break;
		key = os_GetCSC();
		if (key) {
			handle_key(key);
			draw_screen();
			gfx_SwapDraw();
		}
	}
	gfx_End();
	save_history();
	return 0;
}
