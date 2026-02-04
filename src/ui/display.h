#ifndef DISPLAY_H
#define DISPLAY_H

#include "model.h"

typedef struct
{
    int width;
    int height;
    int ui_top;
    int ui_left;
    int graph_width;
    int process_list_height;
} DisplayLayout;

void setup_terminal();
void restore_terminal();
void render_interface(CpuModel* model, DisplayLayout* layout);
void update_layout(DisplayLayout* layout);

#endif
