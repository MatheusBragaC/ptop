#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "model.h"

// Parses /proc/stat content
// Updates usage and graph_hist in the model
void parse_proc_stat(const char* buffer, CpuModel* model, uint64_t* prev_total, uint64_t* prev_idle);

// Parses an integer from a sysfs file content
// Parses an integer from a sysfs file content
int parse_sysfs_int(const char* buffer);

// Parse /proc/cpuinfo for model name
void parse_cpu_model_name(const char* buffer, char* out_name, size_t max_len);

#endif
