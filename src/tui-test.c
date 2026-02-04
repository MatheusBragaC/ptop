#include "tui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_BUFF_LEN 4096

int main()
{
    char buffer[TEST_BUFF_LEN];
    char *p = buffer;

    p = tui_draw_box(p, 2, 5, 2, 2, "\033[32m");
    p = tui_draw_box(p, 2, 8, 3, 3, "\033[34m");
    p = tui_draw_box(p, 2, 12, 12, 7, "\033[36m");
    p = tui_draw_up_space(p, 3, 12, 3);
    p = tui_draw_bottom_space(p, 4, 12 + 7, 4);
    write(STDOUT_FILENO, buffer, p - buffer);

    sleep(2);

    return 0;
}
