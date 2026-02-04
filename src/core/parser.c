#include "parser.h"
#include <string.h>
#include <stdlib.h>

int parse_sysfs_int(const char* buffer)
{
    int val = 0;
    const char *p = buffer;
    while (*p && (*p < '0' || *p > '9')) p++; // Skip leading non-digits (though usually none)
    while (*p >= '0' && *p <= '9') val = (val * 10) + (*p++ - '0');
    return val;
}

void parse_proc_stat(const char* buffer, CpuModel* model, uint64_t* prev_total, uint64_t* prev_idle)
{
    const char *p = buffer;
    
    // Skip first line (cpu execution summary)
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;

    for (int cpu_id = 0; cpu_id < CORES_N; cpu_id++)
    {
        if (strncmp(p, "cpu", 3) != 0) break;
        p += 3;
        while (*p >= '0' && *p <= '9') p++; // Skip cpu id
        p++; // Skip space

        uint64_t active = 0;
        uint64_t total_idle = 0;
        uint64_t val;

        // user, nice, system
        for (int k=0; k<3; k++) {
            val = strtoull(p, (char**)&p, 10);
            active += val;
        }
        
        // idle, iowait
        for (int k=0; k<2; k++) {
            val = strtoull(p, (char**)&p, 10);
            total_idle += val;
        }

        // irq, softirq, steal
        for (int k=0; k<3; k++) {
            val = strtoull(p, (char**)&p, 10);
            active += val;
        }

        // Pula para próxima linha
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;

        uint64_t total = active + total_idle;
        uint64_t diff_total = total - prev_total[cpu_id];
        uint64_t diff_idle  = total_idle - prev_idle[cpu_id];

        uint8_t current_usage = 0;
        if (diff_total > 0)
            current_usage = (uint8_t)((diff_total - diff_idle) * 100 / diff_total);

        model->usage[cpu_id] = current_usage;
        prev_total[cpu_id] = total;
        prev_idle[cpu_id] = total_idle;
        
        // Atualiza histórico do gráfico no modelo
        model->graph_hist[cpu_id][model->graph_head] = current_usage;
    }
    model->graph_head = (model->graph_head + 1) % GRAPH_WIDTH;
}

void parse_cpu_model_name(const char* buffer, char* out_name, size_t max_len)
{
    const char *p = buffer;
    while(*p) {
        // Procura por "model name" (x86) ou "Hardware" (ARM as vezes)
        if (strncmp(p, "model name", 10) == 0) {
            while (*p && *p != ':') p++;
            if (*p == ':') p++;
            while (*p && *p == ' ') p++; // Pula espaços
            
            // Pula prefixos comuns
            if (strncmp(p, "Intel(R) Core(TM) ", 18) == 0) p += 18;
            else if (strncmp(p, "AMD Ryzen ", 10) == 0) p += 10;
            else if (strncmp(p, "12th Gen Intel(R) Core(TM) ", 27) == 0) p += 27; // Captura específico do usuário

            size_t i = 0;
            while (*p && *p != '\n' && i < max_len - 1) {
                out_name[i++] = *p++;
            }
            out_name[i] = '\0';
            return;
        }
        // Next line
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    snprintf(out_name, max_len, "Unknown CPU");
}

