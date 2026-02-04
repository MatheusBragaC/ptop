#ifndef CPU_COLLECTOR_H
#define CPU_COLLECTOR_H

#include "model.h"

typedef struct CpuCollectorState CpuCollector;

// Lifecycle
CpuCollector* collector_init();
void collector_cleanup(CpuCollector* collector);

// Operations
void collector_update(CpuCollector* collector, CpuModel* model);

#endif
