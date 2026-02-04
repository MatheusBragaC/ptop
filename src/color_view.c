#include "display.c"
#include <stdio.h>

static const char *ctemp[16] = {TEMP_0,  TEMP_1,  TEMP_2,  TEMP_3, TEMP_4,  TEMP_5,
                                TEMP_6,  TEMP_7,  TEMP_8,  TEMP_9, TEMP_10, TEMP_11,
                                TEMP_12, TEMP_13, TEMP_14, TEMP_15};

static const char *cperc[8] = {PERC_0, PERC_1, PERC_2, PERC_3, PERC_4, PERC_5, PERC_6, PERC_7};

static const char *blocks[6] = {" ", ".", ".", ":", ":", ":"};

void print_header(const char *title) { printf(PRESET "\n" BOLD "%s" NOBOLD "\n", title); }

int main()
{
    // --- TEMPERATURE ---
    print_header("TEMP (Start: -128 | Step: 16)");

    printf("Fluxo: ");
    for (int i = 0; i < 16; i++)
    {
        printf("%s███", ctemp[i]);
    }
    printf(PRESET "\n\n");

    printf(PRESET "| ID | Cor | ID Macro | INIT °C |  FINAL °C |\n");
    printf(PRESET "|----|-----|----------|---------|-----------|\n");

    for (int i = 0; i < 16; i++)
    {
        int temp_val = -128 + (i * 16);

        printf(PRESET "|%3d | %s███" PRESET " | TEMP_%-2d  | " BOLD " %4d   " NOBOLD "|" BOLD
                      " %s%4d °C   " PRESET "|\n",
               i, ctemp[i], i, temp_val, ctemp[i], temp_val + 15);
    }

    // --- PERCENTAGE ---
    print_header("USAGE (Start: 0 | Step: 16)");

    printf("Fluxo: ");
    for (int i = 0; i < 8; i++)
    {
        printf("%s███", cperc[i]);
    }
    printf(PRESET "\n\n");

    printf(PRESET "| ID | Cor | ID Macro |  INIT %%  |  FINAL %% |\n");
    printf(PRESET "|----|-----|----------|----------|----------|\n");

    for (int i = 0; i < 8; i++)
    {
        int perc_val = 0 + (i * 16);

        printf(PRESET "|%3d | %s███" PRESET " | PERC_%-2d  | " BOLD "  %3d %%  " NOBOLD "|" BOLD
                      " %s %3d %%   " PRESET "|\n",
               i, cperc[i], i, perc_val, cperc[i], perc_val + 15);
    }

    printf("\033[0m\n");
    return 0;
}
