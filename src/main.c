#include <graphx.h>
#include <ti/getcsc.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define INPUT_CAPACITY 31
#define HISTORY_CAPACITY 12
#define VISIBLE_HISTORY 3

#define COLOR_BACKGROUND 0
#define COLOR_PANEL 1
#define COLOR_TEXT 2
#define COLOR_MUTED 3
#define COLOR_ACCENT 4
#define COLOR_SELECTION 5

typedef struct {
    char text[INPUT_CAPACITY];
} history_entry_t;

static history_entry_t history[HISTORY_CAPACITY];
static uint8_t history_count;
static int8_t selected_history = -1;
static char input[INPUT_CAPACITY];
static uint8_t input_length;
static uint8_t cursor_position;
static bool alpha_mode = true;
static bool uppercase_once;

static char alpha_character(uint8_t key) {
    switch (key) {
        case sk_Math:     return 'A';
        case sk_Apps:     return 'B';
        case sk_Prgm:     return 'C';
        case sk_Recip:    return 'D';
        case sk_Sin:      return 'E';
        case sk_Cos:      return 'F';
        case sk_Tan:      return 'G';
        case sk_Power:    return 'H';
        case sk_Square:   return 'I';
        case sk_Comma:    return 'J';
        case sk_LParen:   return 'K';
        case sk_RParen:   return 'L';
        case sk_Div:      return 'M';
        case sk_Log:      return 'N';
        case sk_7:        return 'O';
        case sk_8:        return 'P';
        case sk_9:        return 'Q';
        case sk_Mul:      return 'R';
        case sk_Ln:       return 'S';
        case sk_4:        return 'T';
        case sk_5:        return 'U';
        case sk_6:        return 'V';
        case sk_Sub:      return 'W';
        case sk_Store:    return 'X';
        case sk_1:        return 'Y';
        case sk_2:        return 'Z';
        case sk_0:        return ' ';
        default:          return '\0';
    }
}

static char number_character(uint8_t key) {
    switch (key) {
        case sk_0:      return '0';
        case sk_1:      return '1';
        case sk_2:      return '2';
        case sk_3:      return '3';
        case sk_4:      return '4';
        case sk_5:      return '5';
        case sk_6:      return '6';
        case sk_7:      return '7';
        case sk_8:      return '8';
        case sk_9:      return '9';
        case sk_DecPnt: return '.';
        case sk_Add:    return '+';
        case sk_Sub:    return '-';
        case sk_Mul:    return '*';
        case sk_Div:    return '/';
        case sk_Power:  return '^';
        case sk_LParen: return '(';
        case sk_RParen: return ')';
        case sk_Comma:  return ',';
        default:        return '\0';
    }
}

static void insert_character(char character) {
    if (character != '\0' && input_length < INPUT_CAPACITY - 1) {
        memmove(&input[cursor_position + 1], &input[cursor_position],
                input_length - cursor_position + 1);
        input[cursor_position++] = character;
        input_length++;
    }
}

static void paste_history(void) {
    const char *source;

    if (selected_history < 0) {
        return;
    }

    source = history[(uint8_t)selected_history].text;
    while (*source != '\0' && input_length < INPUT_CAPACITY - 1) {
        insert_character(*source++);
    }
    selected_history = -1;
}

static void submit_input(void) {
    if (input_length == 0) {
        return;
    }

    if (history_count == HISTORY_CAPACITY) {
        memmove(&history[0], &history[1],
                sizeof(history[0]) * (HISTORY_CAPACITY - 1));
        history_count--;
    }

    strcpy(history[history_count].text, input);
    history_count++;
    input[0] = '\0';
    input_length = 0;
    cursor_position = 0;
}

static void handle_key(uint8_t key) {
    char character;

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
        if (cursor_position > 0) {
            cursor_position--;
        }
        return;
    }

    if (key == sk_Right) {
        selected_history = -1;
        if (cursor_position < input_length) {
            cursor_position++;
        }
        return;
    }

    if (key == sk_Up) {
        if (history_count == 0) {
            return;
        }
        if (selected_history < 0) {
            selected_history = (int8_t)history_count - 1;
        } else if (selected_history > 0) {
            selected_history--;
        }
        return;
    }

    if (key == sk_Down) {
        if (selected_history < 0) {
            return;
        }
        if (selected_history < (int8_t)history_count - 1) {
            selected_history++;
        } else {
            selected_history = -1;
        }
        return;
    }

    if (key == sk_Enter) {
        if (selected_history >= 0) {
            paste_history();
        } else {
            submit_input();
        }
        return;
    }

    if (key == sk_Clear) {
        selected_history = -1;
        input[0] = '\0';
        input_length = 0;
        cursor_position = 0;
        uppercase_once = false;
        return;
    }

    if (key == sk_Del) {
        selected_history = -1;
        if (cursor_position > 0) {
            memmove(&input[cursor_position - 1], &input[cursor_position],
                    input_length - cursor_position + 1);
            cursor_position--;
            input_length--;
        }
        return;
    }

    selected_history = -1;
    character = alpha_mode ? alpha_character(key) : number_character(key);
    if (alpha_mode && character >= 'A' && character <= 'Z') {
        if (!uppercase_once) {
            character += 'a' - 'A';
        }
        uppercase_once = false;
    }
    insert_character(character);
}

static void print_at(const char *text, int x, int y, uint8_t color) {
    gfx_SetTextFGColor(color);
    gfx_SetTextXY(x, y);
    gfx_PrintString(text);
}

static int input_cursor_x(void) {
    char saved = input[cursor_position];
    int x;

    input[cursor_position] = '\0';
    x = 20 + gfx_GetStringWidth(input);
    input[cursor_position] = saved;
    return x;
}

static uint8_t first_visible_history(void) {
    uint8_t first;

    if (history_count <= VISIBLE_HISTORY) {
        return 0;
    }

    first = history_count - VISIBLE_HISTORY;
    if (selected_history >= 0 && selected_history < (int8_t)first) {
        first = (uint8_t)selected_history;
    }
    return first;
}

static void draw_screen(void) {
    uint8_t first = first_visible_history();
    uint8_t shown = history_count - first;
    uint8_t i;
    int y = 32;

    if (shown > VISIBLE_HISTORY) {
        shown = VISIBLE_HISTORY;
    }

    gfx_FillScreen(COLOR_BACKGROUND);

    gfx_SetColor(COLOR_PANEL);
    gfx_FillRectangle(0, 0, 320, 24);
    print_at("units", 8, 8, COLOR_TEXT);
    print_at(uppercase_once ? "(ABC)" : (alpha_mode ? "(abc)" : "(123)"),
             270, 8, COLOR_ACCENT);

    for (i = 0; i < shown; i++) {
        uint8_t index = first + i;

        if ((int8_t)index == selected_history) {
            gfx_SetColor(COLOR_SELECTION);
            gfx_FillRectangle(4, y - 3, 312, 38);
        }

        print_at(">", 8, y, COLOR_ACCENT);
        print_at(history[index].text, 20, y, COLOR_TEXT);
        print_at("hello ", 20, y + 19, COLOR_MUTED);
        gfx_PrintString(history[index].text);
        y += 45;
    }

    if (history_count == 0) {
        print_at("Type a message, then press ENTER.", 8, 42, COLOR_MUTED);
    }

    gfx_SetColor(COLOR_PANEL);
    gfx_FillRectangle(0, 190, 320, 50);
    print_at(selected_history >= 0 ? "ENTER: paste selected line" :
             "2ND+ON: exit  ALPHA: abc/123", 8, 196, COLOR_MUTED);
    print_at(">", 8, 219, COLOR_ACCENT);
    print_at(input, 20, 219, COLOR_TEXT);

    gfx_SetColor(COLOR_ACCENT);
    gfx_VertLine(input_cursor_x(), 216, 18);
}

int main(void) {
    uint8_t key;

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
        if (boot_CheckOnPressed() && uppercase_once) {
            break;
        }
        key = os_GetCSC();
        if (key != 0) {
            handle_key(key);
            draw_screen();
            gfx_SwapDraw();
        }
    }

    gfx_End();
    return 0;
}
