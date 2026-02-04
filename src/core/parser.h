#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "model.h"

// Parses /proc/stat content
// Updates usage and graph_hist in the model
void parse_proc_stat(const char* buffer, CpuModel* model, uint64_t* prev_total, uint64_t* prev_idle);

// Parses an integer from a sysfs file content
int parse_sysfs_int(const char* buffer);

#endif
