#ifndef CPU_MODEL_H
#define CPU_MODEL_H

#include "cfg.h"
#include <stdint.h>

typedef struct
{
    // Snapshot Metrics
    uint16_t freq_mhz;
    int8_t temp_c;
    
    // Core Usage
    uint8_t usage[CORES_N];
    
    // System Load
    uint32_t uptime_sec;
    uint32_t load_avg_1;
    uint32_t load_avg_5;
    uint32_t load_avg_15;

    // Visualization History
    uint8_t graph_head;
    uint8_t graph_hist[CORES_N][GRAPH_WIDTH];

} CpuModel;

#endif
