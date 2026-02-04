#ifndef DISPLAY_H
#define DISPLAY_H

#include "model.h"

void setup_terminal();
void render_interface(CpuModel* model);
void restore_terminal();

#endif
