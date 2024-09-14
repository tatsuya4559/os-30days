#ifndef _LAYER_H_
#define _LAYER_H_

#include "common.h"
#include "memory.h"

typedef enum {
    LAYER_UNUSED,
    LAYER_USED,
} LayerStatus;

struct LayerCtl_tag;

typedef struct {
    Byte *buf;
    int bxsize, bysize, vx0, vy0, col_inv, zindex;
    LayerStatus flags;
    struct LayerCtl_tag *ctl;
} Layer;

#define MAX_LAYERS 256

typedef struct LayerCtl_tag {
    Byte *vram;
    int xsize, ysize, top_zindex;
    Layer *layers[MAX_LAYERS]; // layer pointers ordered by zindex
    Layer layers0[MAX_LAYERS]; // real layer data
} LayerCtl;

LayerCtl *layerctl_init(MemoryManager *memman, Byte *vram, int xsize, int ysize);
Layer *layer_alloc(LayerCtl *ctl);
void layer_setbuf(Layer *layer, Byte *buf, int xsize, int ysize, int col_inv);
void layer_refresh(Layer *layer, int bx0, int by0, int bx1, int by1);
void layer_updown(Layer *layer, int zindex);
void layer_slide(Layer *layer, int vx0, int vy0);
void layer_free(Layer *layer);

#endif /* _LAYER_H_ */
