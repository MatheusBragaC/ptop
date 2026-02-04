#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <stdint.h>
#include "cfg.h"

#define MAX_PROCESSES 10

typedef struct {
    uint32_t pid;
    char comm[16];
    double cpu_percent;
} ProcessInfo;

typedef struct
{
    // Info Estática
    char cpu_name[64];

    // Métricas Dinâmicas
    uint8_t usage[CORES_N];
    uint8_t graph_hist[CORES_N][GRAPH_WIDTH];
    int graph_head;
    
    int temp_c;
    int freq_mhz;
    
    unsigned long load_avg_1;
    unsigned long load_avg_5;
    unsigned long load_avg_15;

    int uptime_sec;

    // Process List (Kernel Hacker Feature)
    int process_count;
    ProcessInfo processes[MAX_PROCESSES];

} CpuModel;

#endif
