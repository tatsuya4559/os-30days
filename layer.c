#include "layer.h"

LayerCtl *
layerctl_init(MemoryManager *memman, Byte *vram, int xsize, int ysize)
{
    LayerCtl *ctl;
    ctl = (LayerCtl *) memman_alloc_4k(memman, sizeof(LayerCtl));
    if (ctl == 0) {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top_zindex = -1; // no layer
    for (int i = 0; i < MAX_LAYERS; i++) {
        ctl->layers0[i].flags = LAYER_UNUSED;
    }

err:
    return ctl;
}

Layer *
layer_alloc(LayerCtl *ctl)
{
    Layer *layer;
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (ctl->layers0[i].flags == LAYER_UNUSED) {
            layer = &ctl->layers0[i];
            layer->flags = LAYER_USED;
            layer->zindex = -1; // not shown
            return layer;
        }
    }
    return 0; // no available layer
}

void
layer_setbuf(Layer *layer, Byte *buf, int xsize, int ysize, int col_inv)
{
    layer->buf = buf;
    layer->bxsize = xsize;
    layer->bysize = ysize;
    layer->col_inv = col_inv;
}

static
void
layer_refreshsub(LayerCtl *ctl, int vx0, int vy0, int vx1, int vy1)
{
    Layer *layer;
    Byte *buf, c, *vram = ctl->vram;
    for (int i = 0; i <= ctl->top_zindex; i++) {
        layer = ctl->layers[i];
        buf = layer->buf;
        for (int by = 0; by < layer->bysize; by++) {
            int vy = layer->vy0 + by;
            if (vy0 <= vy && vy < vy1) {
                for (int bx = 0; bx < layer->bxsize; bx++) {
                    int vx = layer->vx0 + bx;
                    if (vx0 <= vx && vx < vx1) {
                        c = buf[by * layer->bxsize + bx];
                        if (c != layer->col_inv) {
                            vram[vy * ctl->xsize + vx] = c;
                        }
                    }
                }
            }
        }
    }
}

void
layer_refresh(LayerCtl *ctl, Layer *layer, int bx0, int by0, int bx1, int by1)
{
    if (layer->zindex >= 0) {
        layer_refreshsub(ctl, layer->vx0 + bx0, layer->vy0 + by0, layer->vx0 + bx1, layer->vy0 + by1);
    }
}

void
layer_updown(LayerCtl *ctl, Layer *layer, int zindex)
{
    int old_zindex = layer->zindex;

    // adjust zindex
    if (zindex > ctl->top_zindex + 1) {
        zindex = ctl->top_zindex + 1;
    }
    if (zindex < -1) {
        zindex = -1;
    }
    layer->zindex = zindex;

    // sort layers
    if (old_zindex > zindex) {
        if (zindex >= 0) {
            for (int i = old_zindex; i > zindex; i--) {
                ctl->layers[i] = ctl->layers[i - 1];
                ctl->layers[i]->zindex = i;
            }
            ctl->layers[zindex] = layer;
        } else {
            if (ctl->top_zindex > old_zindex) {
                for (int i = old_zindex; i < ctl->top_zindex; i++) {
                    ctl->layers[i] = ctl->layers[i + 1];
                    ctl->layers[i]->zindex = i;
                }
            }
            ctl->top_zindex--;
        }
        layer_refreshsub(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize);
    } else if (old_zindex < zindex) {
        if (old_zindex >= 0) {
            for (int i = old_zindex; i < zindex; i++) {
                ctl->layers[i] = ctl->layers[i + 1];
                ctl->layers[i]->zindex = i;
            }
            ctl->layers[zindex] = layer;
        } else {
            for (int i = ctl->top_zindex; i >= zindex; i--) {
                ctl->layers[i + 1] = ctl->layers[i];
                ctl->layers[i + 1]->zindex = i + 1;
            }
            ctl->layers[zindex] = layer;
            ctl->top_zindex++;
        }
        layer_refreshsub(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize);
    }
}

void
layer_slide(LayerCtl *ctl, Layer *layer, int vx0, int vy0)
{
    int old_vx0 = layer->vx0;
    int old_vy0 = layer->vy0;
    layer->vx0 = vx0;
    layer->vy0 = vy0;
    if (layer->zindex >= 0) {
        layer_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + layer->bxsize, old_vy0 + layer->bysize);
        layer_refreshsub(ctl, vx0, vy0, vx0 + layer->bxsize, vy0 + layer->bysize);
    }
}

void
layer_free(LayerCtl *ctl, Layer *layer)
{
    if (layer->zindex >= 0) {
        layer_updown(ctl, layer, -1);
    }
    layer->flags = LAYER_UNUSED;
}
