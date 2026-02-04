#ifndef NETLINK_DRIVER_H
#define NETLINK_DRIVER_H

#include <stdint.h>
#include <sys/types.h>

// Estrutura para armazenar estatísticas de processos buscadas do Kernel
typedef struct
{
    uint32_t pid;
    char comm[32];      // Nome do comando
    uint64_t cpu_usage; // Tempo de CPU (user + sys) em nanosegundos
    uint64_t vm_rss;    // Resident Set Size (KB)
} ProcessStats;

// Inicializa o Socket Netlink e resolve o ID da Família GENL
// Retorna 0 em sucesso, < 0 em falha
int netlink_init(void);

// Limpa recursos
void netlink_cleanup(void);

// Busca stats para um PID específico
// Retorna 0 em sucesso, < 0 em falha
int netlink_get_stats(pid_t pid, ProcessStats* stats);

#endif
