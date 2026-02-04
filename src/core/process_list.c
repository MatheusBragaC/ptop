#include "process_list.h"
#include "netlink_driver.h"
#include "logger.h"
#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

// Comparador para qsort (uso de CPU decrescente)
static int compare_processes(const void *a, const void *b)
{
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    // Ordem decrescente
    if (pb->cpu_percent > pa->cpu_percent) return 1;
    if (pb->cpu_percent < pa->cpu_percent) return -1;
    return 0;
}

// Busca linear simples por estado (pode ser otimizado para hashmap depois)
static ProcessState* find_state(ProcessState *cache, int max, uint32_t pid)
{
    for (int i = 0; i < max; i++) {
        if (cache[i].pid == pid) return &cache[i];
    }
    return NULL;
}

static ProcessState* get_free_slot(ProcessState *cache, int max)
{
    for (int i = 0; i < max; i++) {
        if (cache[i].pid == 0) return &cache[i];
    }
    // Evicção simples: sobrescreve slot 0 se cheio (política ruim mas simples)
    // Implementação real deve evictar o 'last_seen' mais antigo
    return &cache[0]; 
}

void update_process_list(CpuModel *model, ProcessState *state_cache, int max_processes)
{
    DIR *d = opendir("/proc");
    if (!d) return;

    struct dirent *dir;
    ProcessInfo temp_list[MAX_TRACKED_PIDS];
    int count = 0;

    // Usa um delta wall clock simplificado (assumindo que taxa de atualização é constante DELAY_MS)
    // Para melhor precisão devemos capturar timestamps
    // 1% de CPU ~ 10ms (10,000,000 ns) por segundo
    // Se update é 500ms, 100% core = 500,000,000 ns
    double interval_ns = DELAY_MS * 1000000.0; 

    while ((dir = readdir(d)) != NULL)
    {
        if (!isdigit(dir->d_name[0])) continue;
        
        uint32_t pid = (uint32_t)atoi(dir->d_name);
        
        // Pula a si mesmo (opcional)
        
        ProcessStats stats;
        if (netlink_get_stats(pid, &stats) == 0)
        {
            // Encontrado via Netlink
            // Calcula uso
            ProcessState *st = find_state(state_cache, MAX_TRACKED_PIDS, pid);
            double usage_p = 0.0;
            
            if (st) {
                uint64_t diff = stats.cpu_usage - st->prev_cpu_ns;
                // Evita picos ou verifica overflow
                if (diff > 0) {
                     usage_p = (double)diff / interval_ns * 100.0 / CORES_N; // Normalizado pelo tempo total do sistema ou por core? 
                     // Geralmente Top mostra > 100% para multi-thread. Vamos mostrar % de um único core.
                     usage_p = (double)diff / interval_ns * 100.0;
                }
                st->prev_cpu_ns = stats.cpu_usage;
                st->last_seen_ms = 0; // Reset (não usado por enquanto)
            } else {
                // Novo processo
                st = get_free_slot(state_cache, MAX_TRACKED_PIDS);
                st->pid = pid;
                st->prev_cpu_ns = stats.cpu_usage;
            }

            if (count < MAX_TRACKED_PIDS) {
                temp_list[count].pid = pid;
                strncpy(temp_list[count].comm, stats.comm, 16);
                temp_list[count].cpu_percent = usage_p;
                // stats.vm_rss é KB. model espera o que? Digamos KB.
                // temp_list[count].mem_kb = stats.vm_rss; 
                count++;
            }
        }
    }
    closedir(d);

    // Ordena por Uso de CPU
    qsort(temp_list, count, sizeof(ProcessInfo), compare_processes);

    // Copia top N para modelo
    int limit = MAX_PROCESSES;
    if (count < limit) limit = count;
    
    model->process_count = limit;
    for (int i = 0; i < limit; i++) {
        model->processes[i] = temp_list[i];
    }
}
