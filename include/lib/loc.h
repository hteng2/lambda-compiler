#ifndef loc_h
#define loc_h

#include "stdint.h"

typedef struct {
  uint64_t row, col;
} Pt;
typedef struct {
  Pt start, end;
} Loc;

#endif
