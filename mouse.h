#ifndef _MOUSE_H_
#define _MOUSE_H_

#include "common.h"

typedef enum {
  MOUSE_DEC_PHASE_WAITING_0XFA,
  MOUSE_DEC_PHASE_RECEIVED_1ST_BYTE,
  MOUSE_DEC_PHASE_RECEIVED_2ND_BYTE,
  MOUSE_DEC_PHASE_RECEIVED_3RD_BYTE,
} MouseDecPhase;

typedef struct {
  Byte buf[3];
  MouseDecPhase phase;
  int x, y, btn;
} MouseDecoder;

void enable_mouse(MouseDecoder *mdec);
int mouse_decode(MouseDecoder *mdec, Byte data);


#endif /* _MOUSE_H_ */
