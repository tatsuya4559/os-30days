#ifndef _LAYER_H_
#define _LAYER_H_

#include "common.h"
#include "memory.h"

typedef enum {
    LAYER_UNUSED,
    LAYER_USED,
} LayerStatus;

struct LayerController_tag;

typedef unsigned char LayerId;

typedef struct {
    Byte *buf;
    int bxsize, bysize, vx0, vy0, col_inv, zindex;
    LayerStatus flags;
    struct LayerController_tag *ctl;
} Layer;

#define MAX_LAYERS 256

typedef struct LayerController_tag {
    Byte *vram;
    LayerId *map;
    int xsize, ysize, top_zindex;
    Layer *layers[MAX_LAYERS]; // layer pointers ordered by zindex
    Layer layers0[MAX_LAYERS]; // real layer data
} LayerController;

LayerController *layerctl_init(MemoryManager *memman, Byte *vram, int xsize, int ysize);
Layer *layer_alloc(LayerController *ctl);
void layer_setbuf(Layer *layer, Byte *buf, int xsize, int ysize, int col_inv);
void layer_refresh(Layer *layer, int bx0, int by0, int bx1, int by1);
void layer_updown(Layer *layer, int zindex);
void layer_slide(Layer *layer, int vx0, int vy0);
void layer_free(Layer *layer);

#endif /* _LAYER_H_ */
