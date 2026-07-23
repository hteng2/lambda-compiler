#ifndef diagnostic_h
#define diagnostic_h

#include "loc.h"

/**
 * .msg field should not be owned
 */
typedef struct {
  Loc loc;
  const char *src;
  const char *msg;
} Diagnostic;

#endif
