#pragma once

#include "common.h"

typedef enum {
  MOUSE_DEC_PHASE_WAITING_0XFA,
  MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE,
  MOUSE_DEC_PHASE_RECEIVED_2ND_BYTE,
  MOUSE_DEC_PHASE_RECEIVED_3RD_BYTE,
} MouseDecPhase;

typedef struct {
  uint8_t buf[3];
  MouseDecPhase phase;
  int32_t x, y, btn;
} MouseDecoder;

void enable_mouse(MouseDecoder *mdec);
int32_t mouse_decode(MouseDecoder *mdec, uint8_t data);
