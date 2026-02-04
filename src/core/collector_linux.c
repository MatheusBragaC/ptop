#define _POSIX_C_SOURCE 200809L
#include "collector.h"
#include "parser.h"
#include "cfg.h"
#include "process_list.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sysinfo.h>

struct CpuCollectorState
{
    int fd_stat;
    int fd_temp;
    int fd_freq[PHY_CORES_N];

    // Ticks anteriores para cálculo de uso
    uint64_t prev_total[CORES_N];
    uint64_t prev_idle[CORES_N];

    // Process State Cache
    ProcessState proc_cache[MAX_TRACKED_PIDS];
};

// --- Funções Auxiliares ---

static inline int read_sysfs_int_file(int fd)
{
    char buf[16];
    ssize_t bytes_read = pread(fd, buf, sizeof(buf) - 1, 0);

    if (bytes_read > 0)
    {
        buf[bytes_read] = '\0';
        return parse_sysfs_int(buf);
    }
    return 0;
}

static int get_coretemp_id()
{
    char path[64];
    char buf[64];
    char *p;

    for (size_t i = 0; i < HWMON_N; i++)
    {
        p = path;
        p = append_str(p, "/sys/class/hwmon/hwmon");
        p = append_int(p, i);
        p = append_str(p, "/name");
        *p = '\0';
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;
        
        ssize_t bytes_read = read(fd, buf, sizeof(buf));
        close(fd);
        if (bytes_read <= 0) continue;

        p = buf;
        int j = 0;
        // Verifica simples: corresponde a "coretemp" ou similar poderia ser melhor
        // Por enquanto usando a lógica existente (verificando contra CORE_LABEL_NAME)
        while (*p && j < CORE_LABEL_NAME_N)
        {
            if (*p++ != CORE_LABEL_NAME[j++])
                break;
        }
        if (j == CORE_LABEL_NAME_N) return i;
    }
    return -1;
}

// --- Implementação ---

CpuCollector* collector_init()
{
    CpuCollector* self = calloc(1, sizeof(CpuCollector));
    if (!self) return NULL;

    self->fd_stat = open(STAT_PATH, O_RDONLY);

    int hwmon_cpu_id = get_coretemp_id();
    if (hwmon_cpu_id >= 0)
    {
        char path[128];
        snprintf(path, sizeof(path), "/sys/class/hwmon/hwmon%d/temp1_input", hwmon_cpu_id);
        self->fd_temp = open(path, O_RDONLY);
    }
    else
    {
        self->fd_temp = -1;
    }

    for (size_t i = 0; i < PHY_CORES_N; i++)
    {
        char path[128];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%zu/cpufreq/scaling_cur_freq", i);
        self->fd_freq[i] = open(path, O_RDONLY);
    }

    return self;
}

void collector_cleanup(CpuCollector* self)
{
    if (!self) return;
    if (self->fd_stat >= 0) close(self->fd_stat);
    if (self->fd_temp >= 0) close(self->fd_temp);
    for (int i = 0; i < PHY_CORES_N; i++)
    {
        if (self->fd_freq[i] >= 0) close(self->fd_freq[i]);
    }
    free(self);
}

static void update_freq(CpuCollector* self, CpuModel* model)
{
    int total = 0;
    int valid_cores = 0;
    for (int i = 0; i < PHY_CORES_N; i++)
    {
        if (self->fd_freq[i] >= 0)
        {
            total += read_sysfs_int_file(self->fd_freq[i]);
            valid_cores++;
        }
    }
    if (valid_cores > 0)
        model->freq_mhz = total / (1000 * valid_cores);
    else 
        model->freq_mhz = 0;
}

static void update_temp(CpuCollector* self, CpuModel* model)
{
    if (self->fd_temp >= 0)
        model->temp_c = read_sysfs_int_file(self->fd_temp) / 1000;
    else
        model->temp_c = 0;
}

static void update_cpu_usage(CpuCollector* self, CpuModel* model)
{
    if (self->fd_stat < 0) return;

    static char buf[STAT_BUFF_LEN] __attribute__((aligned(64)));
    ssize_t bytes_read = pread(self->fd_stat, buf, sizeof(buf) - 1, 0);
    if (bytes_read < 0) return;
    buf[bytes_read] = '\0';

    parse_proc_stat(buf, model, self->prev_total, self->prev_idle);
}

static void update_sysinfo(CpuModel* model)
{
    struct sysinfo si;
    if (sysinfo(&si) == 0)
    {
        model->load_avg_1 = si.loads[0];
        model->load_avg_5 = si.loads[1];
        model->load_avg_15 = si.loads[2];
        model->uptime_sec = si.uptime;
    }
}

static void fetch_cpu_name(CpuModel* model)
{
    int fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) return;
    
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    close(fd);
    
    if (n > 0) {
        buf[n] = '\0';
        parse_cpu_model_name(buf, model->cpu_name, sizeof(model->cpu_name));
    }
}


void collector_update(CpuCollector* self, CpuModel* model)
{
    update_freq(self, model);
    update_temp(self, model);
    update_cpu_usage(self, model);
    update_sysinfo(model);
    
    if (model->cpu_name[0] == '\0') {
        fetch_cpu_name(model);
    }
    
    // Update Process List (Netlink)
    update_process_list(model, self->proc_cache, MAX_TRACKED_PIDS);
}
