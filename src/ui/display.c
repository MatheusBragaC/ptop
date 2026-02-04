#include "display.h"
#include "logger.h"
#include "cfg.h"
#include "utils.h"
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

// --- COLORS ---
#define BG_BLACK "\033[48;5;234m"
#define GRAY "\033[38;5;245m"
#define WHITE "\033[38;5;253m"
#define PRESET "\033[0m" BG_BLACK WHITE
#define BOLD "\033[1m"
#define NOBOLD "\033[22m"

// --- TEMPERATURE ---
#define TEMP_0 "\033[38;5;21m"
#define TEMP_1 "\033[38;5;21m"
#define TEMP_2 "\033[38;5;27m"
#define TEMP_3 "\033[38;5;27m"
#define TEMP_4 "\033[38;5;33m"
#define TEMP_5 "\033[38;5;39m"
#define TEMP_6 "\033[38;5;45m"
#define TEMP_7 "\033[38;5;51m"
#define TEMP_8 "\033[38;5;87m"
#define TEMP_9 "\033[38;5;49m"
#define TEMP_10 "\033[38;5;46m"
#define TEMP_11 "\033[38;5;118m"
#define TEMP_12 "\033[38;5;226m"
#define TEMP_13 "\033[38;5;202m"
#define TEMP_14 "\033[38;5;196m"
#define TEMP_15 "\033[38;5;129m"

// --- PERCENTAGE ---
#define PERC_0 "\033[38;5;47m"
#define PERC_1 "\033[38;5;82m"
#define PERC_2 "\033[38;5;154m"
#define PERC_3 "\033[38;5;190m"
#define PERC_4 "\033[38;5;226m"
#define PERC_5 "\033[38;5;208m"
#define PERC_6 "\033[38;5;196m"
#define PERC_7 "\033[38;5;129m"

// --- BOX ---
#define BOX_TL "┌"
#define BOX_TR "┐"
#define BOX_BL "└"
#define BOX_BR "┘"
#define BOX_H "─"
#define BOX_V "│"

static struct termios original_term;

static const char *ctemp[16] = {TEMP_0,  TEMP_1,  TEMP_2,  TEMP_3, TEMP_4,  TEMP_5,
                                TEMP_6,  TEMP_7,  TEMP_8,  TEMP_9, TEMP_10, TEMP_11,
                                TEMP_12, TEMP_13, TEMP_14, TEMP_15};

static const char *cperc[8] = {PERC_0, PERC_1, PERC_2, PERC_3, PERC_4, PERC_5, PERC_6, PERC_7};

static const char *dots[8] = {
    "\xE2\xA3\x80", // ⣀
    "\xE2\xA3\xA0", // ⣠
    "\xE2\xA3\xA4", // ⣤
    "\xE2\xA3\xA6", // ⣦
    "\xE2\xA3\xB6", // ⣶
    "\xE2\xA3\xB7", // ⣷
    "\xE2\xA3\xBF", // ⣿
    "\xE2\xA3\xBF"  // ⣿
};

void update_layout(DisplayLayout *layout)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    {
        layout->width = 32;
        layout->height = CORES_N + 2;
    }
    else
    {
        layout->width = ws.ws_col;
        layout->height = ws.ws_row;
    }
    
    int needed_height = CORES_N + 2;
    int needed_width = 32 > layout->width ? layout->width : 32;

    layout->ui_top = (layout->height - needed_height) / 2;
    if (layout->ui_top < 1) layout->ui_top = 1;

    layout->ui_left = (layout->width - needed_width) / 2;
    if (layout->ui_left < 1) layout->ui_left = 1;

    // Largura adaptativa: Usa até 60 cols, ou largura total se menor, mas pelo menos 32
    int target_width = ws.ws_col > 60 ? 60 : ws.ws_col;
    if (target_width < 32) target_width = 32;

    layout->width = target_width;
    layout->height = needed_height;
    
    // Gráfico começa na esquerda+5, Porcentagem na direita-5
    // Espaço disponível para gráfico: width - 5 (left padding) - 6 (right padding/perc)
    layout->graph_width = layout->width - 11;
    if (layout->graph_width > GRAPH_WIDTH) layout->graph_width = GRAPH_WIDTH; // Ou deixa expandir? 
    // Vamos expandir! Melhor resolução
    layout->graph_width = layout->width - 11;

    // Process List Height
    // Use remaining height at bottom
    int used_height = layout->ui_top + layout->height + 2; // + uptime + padding
    int remaining = ws.ws_row - used_height;
    layout->process_list_height = (remaining > 0) ? remaining : 0;
    // Cap at MAX_PROCESSES + 1 (header)
    if (layout->process_list_height > MAX_PROCESSES + 1) layout->process_list_height = MAX_PROCESSES + 1;
}

static inline char *draw_box(char *p, DisplayLayout *layout, CpuModel* model)

{
    p = append_str(p, "\033[38;5;240m");
    p = append_str(p, "\033[");
    p = append_int(p, layout->ui_top);
    p = append_str(p, ";");
    p = append_int(p, layout->ui_left);
    p = append_str(p, "H");

    p = append_str(p, BOX_TL BOX_TR);
    p = append_str(p, WHITE);
    p = append_str(p, model->cpu_name[0] ? model->cpu_name : "Unknown CPU");
    p = append_str(p, NOBOLD);
    p = append_str(p, "\033[38;5;240m");
    p = append_str(p, BOX_TL BOX_H BOX_TR);
    p = append_str(p, "    ");
    p = append_str(p, BOX_TL BOX_H BOX_TR);
    p = append_str(p, "       ");
    p = append_str(p, BOX_TL);
    int name_len = strlen(model->cpu_name[0] ? model->cpu_name : "Unknown CPU");
    for (int i = 0; i < layout->width - name_len - 21; i++)
        p = append_str(p, BOX_H);
    p = append_str(p, BOX_TR);

    for (int i = 1; i < layout->height - 1; i++)
    {
        p = append_str(p, "\033[");
        p = append_int(p, layout->ui_top + i);
        p = append_str(p, ";");
        p = append_int(p, layout->ui_left);
        p = append_str(p, "H");
        p = append_str(p, BOX_V);
    }
    for (int i = 1; i < layout->height - 1; i++)
    {
        p = append_str(p, "\033[");
        p = append_int(p, layout->ui_top + i);
        p = append_str(p, ";");
        p = append_int(p, layout->ui_left + layout->width - 1);
        p = append_str(p, "H");
        p = append_str(p, BOX_V);
    }

    p = append_str(p, "\033[");
    p = append_int(p, layout->ui_top + layout->height - 1);
    p = append_str(p, ";");
    p = append_int(p, layout->ui_left);
    p = append_str(p, "H");
    p = append_str(p, BOX_BL);
    p = append_str(p, BOX_BR);
    for (int i = 0; i < 21; i++)
        p = append_str(p, " ");
    p = append_str(p, BOX_BL);
    for (int i = 0; i < layout->width - 32; i++)
        p = append_str(p, BOX_H);

    p = append_str(p, BOX_BR);
    p = append_str(p, WHITE);
    p = append_int(p, DELAY_MS);
    p = append_str(p, "ms");
    p = append_str(p, "\033[38;5;240m");
    p = append_str(p, BOX_BL);
    p = append_str(p, BOX_BR);

    return p;
}

// Desenha Temperatura alinhada à direita
static inline char *draw_temperature(char *p, int temp, int row, int left, int width)
{
    // Aprox 13 chars da direita: " 100°C" + espacos
    int offset = width - 14; 
    
    p = append_str(p, "\033[");
    p = append_int(p, row);
    p = append_str(p, ";");
    p = append_int(p, left + offset);
    p = append_str(p, "H");
    int temp_val = temp;
    p = append_str(p, BOLD);
    p = append_str(p, ctemp[(temp_val + 128) >> 4]);
    p = append_int(p, temp_val);
    p = APPEND_LIT(p, WHITE NOBOLD);
    p = APPEND_LIT(p, "°C");
    p = append_str(p, NOBOLD);

    return p;
}

// Desenha Frequencia alinhada à direita (após temperatura)
static inline char *draw_frequency(char *p, int freq, int row, int left, int width)
{
    // Aprox 20 chars da direita
    int offset = width - 23; 

    p = append_str(p, "\033[");
    p = append_int(p, row);
    p = append_str(p, ";");
    p = append_int(p, left + offset); 
    p = append_str(p, "H");
    int mhz = freq;
    p = append_str(p, BOLD);
    p = append_int(p, mhz / 1000);
    p = APPEND_LIT(p, ".");
    p = append_int(p, (mhz % 1000) / 100);
    p = append_str(p, NOBOLD);
    p = APPEND_LIT(p, " GHz");

    return p;
}

static inline char *draw_uptime(char *p, int uptime, int row, int left)
{
    p = append_str(p, "\033[");
    p = append_int(p, row);
    p = append_str(p, ";");
    p = append_int(p, left + 2);
    p = append_str(p, "H");
    long up = uptime;
    int hours = up / 3600;
    int mins = (up % 3600) / 60;
    int secs = up % 60;
    p = append_str(p, "Up: ");
    if (hours < 10)
        p = append_str(p, "0");
    p = append_int(p, hours);
    p = append_str(p, ":");
    if (mins < 10)
        p = append_str(p, "0");
    p = append_int(p, mins);
    p = append_str(p, ":");
    if (secs < 10)
        p = append_str(p, "0");
    p = append_int(p, secs);

    return p;
}



static inline char *draw_process_list(char *p, CpuModel *model, int row, int left, int width, int height)
{
    // Header
    p = append_str(p, "\033[");
    p = append_int(p, row);
    p = append_str(p, ";");
    p = append_int(p, left);
    p = append_str(p, "H");
    p = append_str(p, BOLD "PID   COMMAND          CPU%" NOBOLD);
    
    // List
    int max_rows = height - 1; 
    if (max_rows > model->process_count) max_rows = model->process_count;
    
    for (int i = 0; i < max_rows; i++)
    {
        p = append_str(p, "\033[");
        p = append_int(p, row + 1 + i);
        p = append_str(p, ";");
        p = append_int(p, left);
        p = append_str(p, "H");
        
        // PID
        int pid = model->processes[i].pid;
        if (pid < 10) p = append_str(p, "    ");
        else if (pid < 100) p = append_str(p, "   ");
        else if (pid < 1000) p = append_str(p, "  ");
        else if (pid < 10000) p = append_str(p, " ");
        p = append_int(p, pid);
        p = append_str(p, "  ");

        // COMMAND (Truncate/Pad to 16)
        char comm[17];
        strncpy(comm, model->processes[i].comm, 16);
        comm[16] = '\0';
        int len = strlen(comm);
        p = append_str(p, comm);
        for(int k=len; k<17; k++) p = append_str(p, " ");
        
        // CPU
        double cpu = model->processes[i].cpu_percent;
        if (cpu >= 1.0) p = append_str(p, PERC_5); // Orange for high usage
        else p = append_str(p, WHITE);
        
        int cpu_int = (int)cpu;
        int cpu_dec = (int)((cpu - cpu_int) * 10);
        
        if (cpu_int < 10) p = append_str(p, "  ");
        else if (cpu_int < 100) p = append_str(p, " ");
        p = append_int(p, cpu_int);
        p = append_str(p, ".");
        p = append_int(p, cpu_dec);
        p = append_str(p, "%");
        p = append_str(p, NOBOLD);
    }
    return p;
}

void setup_terminal()
{
    struct termios new_term;
    tcgetattr(STDIN_FILENO, &original_term);
    new_term = original_term;
    new_term.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    // Habilita Mouse Reporting (X11) + Tela Alternativa + Esconde Cursor + Limpa
    printf("\033[?1000h\033[?1049h\033[?25l%s\033[2J\033[H%s", BG_BLACK, WHITE);
    fflush(stdout);
}

void render_interface(CpuModel* model, DisplayLayout* layout)
{
    static char buf[OUT_BUFF_LEN] __attribute__((aligned(64)));
    char *p = buf;

    // Limpa tela primeiro se dinâmico? Idealmente redesenhamos ou usamos buffer alternativo
    // Para suavidade, assumimos fundo constante. 
    // Se layout mudar (resize), limpamos tela na logica do main
    p = draw_box(p, layout, model);

    int row = layout->ui_top;
    
    // Desenha Freq primeiro (mais a esquerda) e Temp (mais a direita)
    // Ajustado para ficarem alinhados a direita
    p = draw_frequency(p, model->freq_mhz, row, layout->ui_left, layout->width);
    p = draw_temperature(p, model->temp_c, row, layout->ui_left, layout->width);

    for (int i = 0; i < CORES_N; i++)
    {
        row = layout->ui_top + i + 1;
        p = append_str(p, "\033[");
        p = append_int(p, row);
        p = append_str(p, ";");
        p = append_int(p, layout->ui_left + 1);
        p = append_str(p, "H");
        p = APPEND_LIT(p, BOLD);
        p = APPEND_LIT(p, "C");
        p = append_int(p, i);
        p = APPEND_LIT(p, WHITE NOBOLD);

        p = append_str(p, "\033[");
        p = append_int(p, row);
        p = append_str(p, ";");
        p = append_int(p, layout->ui_left + 5);
        p = append_str(p, "H");

        for (int k = 0; k < layout->graph_width; k++)
        {
            int idx = (model->graph_head + k) % GRAPH_WIDTH;
            int val = model->graph_hist[i][idx];
            if (val)
                p = append_str(p, cperc[(val >> 4) & 7]);
            else
                p = append_str(p, GRAY);
            p = append_str(p, dots[(val >> 4) & 7]);
        }

        p = append_str(p, "\033[");
        p = append_int(p, row);
        p = append_str(p, ";");
        p = append_int(p, layout->ui_left + layout->width - 5);
        p = append_str(p, "H");
        int usage = model->usage[i];
        if (usage)
            p = append_str(p, cperc[(usage >> 4) & 7]);
        else
            p = append_str(p, GRAY);
        if (usage < 10)
            *p++ = ' ';
        if (usage < 100)
            *p++ = ' ';
        p = append_int(p, usage);
        p = append_str(p, WHITE);
        p = APPEND_LIT(p, "%");
    }

    int load_row = layout->ui_top + layout->height - 1;
    p = append_str(p, "\033[");
    p = append_int(p, load_row);
    p = append_str(p, ";");
    p = append_int(p, layout->ui_left + 2);
    p = append_str(p, "H");
    p = append_str(p, "AVG: ");
    
    unsigned long loads[3] = {model->load_avg_1, model->load_avg_5, model->load_avg_15};

    for (int k = 0; k < 3; k++)
    {
        unsigned long raw = loads[k];
        int whole = raw >> 16;
        int frac = ((raw * 100) >> 16) % 100;
        unsigned int l_idx = raw >> 17;
        if (l_idx > 7)
            l_idx = 7;

        p = append_str(p, cperc[l_idx]);
        p = append_int(p, whole);
        p = append_str(p, ".");
        if (frac < 10)
            *p++ = '0';
        p = append_int(p, frac);
        if (k < 2)
            p = append_str(p, "  ");
    }
    p = append_str(p, WHITE);

    row = layout->ui_top + layout->height;
    p = draw_uptime(p, model->uptime_sec, row, layout->ui_left);
    
    // Draw Process List below status box
    // Need more height? "needed_height" in update_layout only accounts for box.
    // We should draw it at bottom if there is space.
    // For now, let's draw it at row + 2
    if (layout->process_list_height > 0)
    {
         p = draw_process_list(p, model, row + 2, layout->ui_left, layout->width, layout->process_list_height);
    }

    if (write(STDOUT_FILENO, buf, p - buf) == -1)
        log_error_errno("display: write failed");
}

void restore_terminal()
{
    // Desabilita Mouse Reporting + Restaura Tela + Mostra Cursor
    printf("\033[?1000l\033[0m\033[?1049l\033[?25h");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &original_term);
}
