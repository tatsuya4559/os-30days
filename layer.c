#include "layer.h"

LayerController *
layerctl_init(MemoryManager *memman, Byte *vram, int xsize, int ysize)
{
  LayerController *ctl;
  ctl = (LayerController *) memman_alloc_4k(memman, sizeof(LayerController));
  if (ctl == 0) {
    goto err;
  }
  ctl->map = (LayerId *) memman_alloc_4k(memman, xsize * ysize);
  if (ctl->map == 0) {
    memman_free_4k(memman, (int) ctl, sizeof(LayerController));
    goto err;
  }
  ctl->vram = vram;
  ctl->xsize = xsize;
  ctl->ysize = ysize;
  ctl->top_zindex = -1; // no layer
  for (int i = 0; i < MAX_LAYERS; i++) {
    ctl->layers0[i].flags = LAYER_UNUSED;
    ctl->layers0[i].ctl = ctl;
  }

err:
  return ctl;
}

Layer *
layer_alloc(LayerController *ctl)
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
layer_refreshmap(LayerController *ctl, int vx0, int vy0, int vx1, int vy1, int z0)
{
  Layer *layer;
  Byte *buf, c;
  LayerId *map = ctl->map;
  int bx0, by0, bx1, by1, vx, vy;

  if (vx0 < 0) {
    vx0 = 0;
  }
  if (vy0 < 0) {
    vy0 = 0;
  }
  if (vx1 > ctl->xsize) {
    vx1 = ctl->xsize;
  }
  if (vy1 > ctl->ysize) {
    vy1 = ctl->ysize;
  }

  LayerId layer_id;
  for (int z = z0; z <= ctl->top_zindex; z++) {
    layer = ctl->layers[z];
    layer_id = (LayerId) (layer - ctl->layers0);
    buf = layer->buf;
    bx0 = vx0 - layer->vx0;
    by0 = vy0 - layer->vy0;
    bx1 = vx1 - layer->vx0;
    by1 = vy1 - layer->vy0;
    if (bx0 < 0) {
      bx0 = 0;
    }
    if (by0 < 0) {
      by0 = 0;
    }
    if (bx1 > layer->bxsize) {
      bx1 = layer->bxsize;
    }
    if (by1 > layer->bysize) {
      by1 = layer->bysize;
    }
    for (int by = by0; by < by1; by++) {
      vy = layer->vy0 + by;
      for (int bx = bx0; bx < bx1; bx++) {
        vx = layer->vx0 + bx;
        c = buf[by * layer->bxsize + bx];
        if (c != layer->col_inv) {
          map[vy * ctl->xsize + vx] = layer_id;
        }
      }
    }
  }
}

static
void
layer_refreshsub(LayerController *ctl, int vx0, int vy0, int vx1, int vy1, int z0, int z1)
{
  Layer *layer;
  Byte *buf, *vram = ctl->vram;
  LayerId *map = ctl->map;
  int bx0, by0, bx1, by1;
  LayerId layer_id;

  if (vx0 < 0) {
    vx0 = 0;
  }
  if (vy0 < 0) {
    vy0 = 0;
  }
  if (vx1 > ctl->xsize) {
    vx1 = ctl->xsize;
  }
  if (vy1 > ctl->ysize) {
    vy1 = ctl->ysize;
  }

  for (int z = z0; z <= z1; z++) {
    layer = ctl->layers[z];
    buf = layer->buf;
    bx0 = vx0 - layer->vx0;
    by0 = vy0 - layer->vy0;
    bx1 = vx1 - layer->vx0;
    by1 = vy1 - layer->vy0;
    if (bx0 < 0) {
      bx0 = 0;
    }
    if (by0 < 0) {
      by0 = 0;
    }
    if (bx1 > layer->bxsize) {
      bx1 = layer->bxsize;
    }
    if (by1 > layer->bysize) {
      by1 = layer->bysize;
    }
    layer_id = (LayerId) (layer - ctl->layers0);
    for (int by = by0; by < by1; by++) {
      int vy = layer->vy0 + by;
      for (int bx = bx0; bx < bx1; bx++) {
        int vx = layer->vx0 + bx;
        if (map[vy * ctl->xsize + vx] == layer_id) {
          vram[vy * ctl->xsize + vx] = buf[by * layer->bxsize + bx];
        }
      }
    }
  }
}

void
layer_refresh(Layer *layer, int bx0, int by0, int bx1, int by1)
{
  if (layer->zindex >= 0) {
    layer_refreshsub(layer->ctl, layer->vx0 + bx0, layer->vy0 + by0, layer->vx0 + bx1, layer->vy0 + by1, layer->zindex, layer->zindex);
  }
}

void
layer_updown(Layer *layer, int zindex)
{
  LayerController *ctl = layer->ctl;
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
      layer_refreshmap(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, zindex + 1);
      layer_refreshsub(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, zindex + 1, old_zindex);
    } else {
      if (ctl->top_zindex > old_zindex) {
        for (int i = old_zindex; i < ctl->top_zindex; i++) {
          ctl->layers[i] = ctl->layers[i + 1];
          ctl->layers[i]->zindex = i;
        }
      }
      ctl->top_zindex--;
      layer_refreshmap(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, 0);
      layer_refreshsub(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, 0, old_zindex - 1);
    }
  } else if (old_zindex < zindex) {
    if (old_zindex >= 0) {
      for (int z = old_zindex; z < zindex; z++) {
        ctl->layers[z] = ctl->layers[z + 1];
        ctl->layers[z]->zindex = z;
      }
      ctl->layers[zindex] = layer;
    } else {
      for (int z = ctl->top_zindex; z >= zindex; z--) {
        ctl->layers[z + 1] = ctl->layers[z];
        ctl->layers[z + 1]->zindex = z + 1;
      }
      ctl->layers[zindex] = layer;
      ctl->top_zindex++;
    }
    layer_refreshmap(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, zindex);
    layer_refreshsub(ctl, layer->vx0, layer->vy0, layer->vx0 + layer->bxsize, layer->vy0 + layer->bysize, zindex, zindex);
  }
}

void
layer_slide(Layer *layer, int vx0, int vy0)
{
  int old_vx0 = layer->vx0;
  int old_vy0 = layer->vy0;
  layer->vx0 = vx0;
  layer->vy0 = vy0;
  if (layer->zindex >= 0) {
    layer_refreshmap(layer->ctl, old_vx0, old_vy0, old_vx0 + layer->bxsize, old_vy0 + layer->bysize, 0);
    layer_refreshmap(layer->ctl, vx0, vy0, vx0 + layer->bxsize, vy0 + layer->bysize, layer->zindex);
    layer_refreshsub(layer->ctl, old_vx0, old_vy0, old_vx0 + layer->bxsize, old_vy0 + layer->bysize, 0, layer->zindex - 1);
    layer_refreshsub(layer->ctl, vx0, vy0, vx0 + layer->bxsize, vy0 + layer->bysize, layer->zindex, layer->zindex);
  }
}

void
layer_free(Layer *layer)
{
  if (layer->zindex >= 0) {
    layer_updown(layer, -1);
  }
  layer->flags = LAYER_UNUSED;
}
