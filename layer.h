#pragma once

#include "common.h"
#include "memory.h"

typedef enum {
    LAYER_UNUSED,
    LAYER_USED,
} LayerStatus;

struct LayerController_tag;

typedef uint8_t LayerId;

typedef struct {
    uint8_t *buf;
    int32_t bxsize, bysize, vx0, vy0, col_inv, zindex;
    LayerStatus flags;
    struct LayerController_tag *ctl;
} Layer;

#define MAX_LAYERS 256

typedef struct LayerController_tag {
    uint8_t *vram;
    LayerId *map;
    int32_t xsize, ysize, top_zindex;
    Layer *layers[MAX_LAYERS]; // layer pointers ordered by zindex
    Layer layers0[MAX_LAYERS]; // real layer data
} LayerController;

LayerController *layerctl_init(MemoryManager *memman, uint8_t *vram, int32_t xsize, int32_t ysize);
Layer *layer_alloc(LayerController *ctl);
void layer_setbuf(Layer *layer, uint8_t *buf, int32_t xsize, int32_t ysize, int32_t col_inv);
void layer_refresh(Layer *layer, int32_t bx0, int32_t by0, int32_t bx1, int32_t by1);
void layer_updown(Layer *layer, int32_t zindex);
void layer_slide(Layer *layer, int32_t vx0, int32_t vy0);
void layer_free(Layer *layer);
