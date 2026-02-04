#ifndef CFG_H
#define CFG_H

// --- DEFINIÇÕES DE HARDWARE ---
#define MODEL "i7-10750H"
#define MODEL_LEN 9
#define CORES_N 12
#define PHY_CORES_N 6
#define HWMON_N 9

// --- CAMINHOS & BUFFERS ---
#define STAT_PATH "/proc/stat"
#define STAT_BUFF_LEN 4096
#define OUT_BUFF_LEN 32768
#define CORE_LABEL_NAME "coretemp"
#define CORE_LABEL_NAME_N 8

// --- CONFIGURAÇÕES ---
#define DELAY_MS 500

// --- CONSTANTES DE UI NECESSÁRIAS PARA STRUCTS ---
#define GRAPH_WIDTH 21

#endif
