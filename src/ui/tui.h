#ifndef TUI_H
#define TUI_H

#include "utils.h"
#include <stdint.h>

#define CLr_BG_BLACK "\033[48;5;234m"
#define CLr_WHITE "\033[38;5;253m"
#define CLr_GRAY "\033[38;5;245m"
#define CLr_RESET "\033[0m"
#define CLr_BOLD "\033[1m"
#define CLr_NOBOLD "\033[22m"

#define BOX_TL "┌"
#define BOX_TR "┐"
#define BOX_BL "└"
#define BOX_BR "┘"
#define BOX_H "─"
#define BOX_V "│"

extern const char *gradient_temp[16];
extern const char *gradient_perc[8];
extern const char *dots_braille[8];

void tui_setup();
void tui_restore();
char *tui_draw_box(char *p, int x, int y, int w, int h, char *color);
char *tui_draw_up_space(char *p, int x, int y, int len);
char *tui_draw_bottom_space(char *p, int x, int y, int len);

#endif
