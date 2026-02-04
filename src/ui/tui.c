#include "tui.h"
#include "cfg.h"
#include "utils.h"
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static const char *dots[8] = {
    "\xE2\xA3\x80", // ⣀ (Nível 1)
    "\xE2\xA3\xA0", // ⣠
    "\xE2\xA3\xA4", // ⣤
    "\xE2\xA3\xA6", // ⣦
    "\xE2\xA3\xB6", // ⣶
    "\xE2\xA3\xB7", // ⣷
    "\xE2\xA3\xBF", // ⣿
    "\xE2\xA3\xBF"  // ⣿ (Nível 8 - Cheio)
};

static const char *graph_colors[8] = {"\033[38;5;47m",  "\033[38;5;82m",  "\033[38;5;154m",
                                      "\033[38;5;226m", "\033[38;5;214m", "\033[38;5;208m",
                                      "\033[38;5;196m", "\033[38;5;196m"};

static struct termios original_term;
void tui_setup()
{
    struct termios new_term;
    tcgetattr(STDIN_FILENO, &original_term);
    new_term = original_term;
    new_term.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
}

void tui_restore()
{
    printf("\033[0m\033[?1049l\033[?25h");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &original_term);
}

char *tui_draw_box(char *p, int x, int y, int w, int h, char *color)
{
    p = append_str(p, color);

    p = append_str(p, "\033[");
    p = append_int(p, y);
    p = append_str(p, ";");
    p = append_int(p, x);
    p = append_str(p, "H");
    p = append_str(p, BOX_TL);
    for (int i = 0; i < w - 2; i++)
        p = append_str(p, BOX_H);
    p = append_str(p, BOX_TR);

    for (int i = 0; i < h - 2; i++)
    {
        p = append_str(p, "\033[");
        p = append_int(p, y + 1 + i);
        p = append_str(p, ";");
        p = append_int(p, x + w - 1);
        p = append_str(p, "H");
        p = append_str(p, BOX_V);
    }

    for (int i = 0; i < h - 2; i++)
    {
        p = append_str(p, "\033[");
        p = append_int(p, y + 1 + i);
        p = append_str(p, ";");
        p = append_int(p, x);
        p = append_str(p, "H");
        p = append_str(p, BOX_V);
    }

    p = append_str(p, "\033[");
    p = append_int(p, y + h - 1);
    p = append_str(p, ";");
    p = append_int(p, x);
    p = append_str(p, "H");
    p = append_str(p, BOX_BL);
    for (int i = 0; i < w - 2; i++)
        p = append_str(p, BOX_H);
    p = append_str(p, BOX_BR);

    return p;
}

char *tui_draw_up_space(char *p, int x, int y, int len)
{
    p = append_str(p, "\033[");
    p = append_int(p, y);
    p = append_str(p, ";");
    p = append_int(p, x);
    p = append_str(p, "H");

    p = append_str(p, BOX_TR);
    for (int i = 0; i < len - 2; i++)
        p = append_str(p, " ");
    p = append_str(p, BOX_TL);

    return p;
}

char *tui_draw_bottom_space(char *p, int x, int y, int len)
{
    p = append_str(p, "\033[");
    p = append_int(p, y);
    p = append_str(p, ";");
    p = append_int(p, x);
    p = append_str(p, "H");

    p = append_str(p, BOX_BR);
    for (int i = 0; i < len - 2; i++)
        p = append_str(p, " ");
    p = append_str(p, BOX_BL);

    return p;
}

char *tui_draw_graph(char *p, int x, int y, uint8_t *data, int len, int head)
{
    p = append_str(p, "\033[");
    p = append_int(p, y);
    p = append_str(p, ";");
    p = append_int(p, x);
    p = append_str(p, "H");

    for (int i = 0; i < len; i++)
    {
        int idx = (head + i) % GRAPH_WIDTH;
        int val = data[idx];
        int dot_idx = (val * 9) >> 4;
        p = append_str(p, graph_colors[dot_idx]);

        if (val > 0)
            p = append_str(p, dots[dot_idx]);
        else
            p = append_str(p, "\033[38;5;236m·");
    }
    return p;
}
