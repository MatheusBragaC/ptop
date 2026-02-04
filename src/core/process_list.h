#ifndef PROCESS_LIST_H
#define PROCESS_LIST_H

#include "model.h"
#include <stdint.h>

#define MAX_TRACKED_PIDS 128

// Estado interno para rastrear tempos de CPU anteriores para cálculo de %
typedef struct {
    uint32_t pid;
    uint64_t prev_cpu_ns;
    uint64_t last_seen_ms; // Para GC de pids antigos
} ProcessState;

// Atualiza a lista dos top processos no modelo usando Netlink
// Requer um socket Netlink inicializado
void update_process_list(CpuModel *model, ProcessState *state_cache, int max_processes);

#endif
