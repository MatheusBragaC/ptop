#ifndef CFG_H
#define CFG_H

// --- HARDWARE DEFINITIONS ---
#define MODEL "i7-10750H"
#define MODEL_LEN 9
#define CORES_N 12
#define PHY_CORES_N 6
#define HWMON_N 9

// --- PATHS & BUFFERS ---
#define STAT_PATH "/proc/stat"
#define STAT_BUFF_LEN 2048
#define OUT_BUFF_LEN 8192
#define CORE_LABEL_NAME "coretemp"
#define CORE_LABEL_NAME_N 8

// --- SETTINGS ---
#define DELAY_MS 500

// --- UI CONSTANTS NEEDED FOR STRUCTS ---
#define GRAPH_WIDTH 21

#endif
