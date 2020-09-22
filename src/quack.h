#ifndef QUACK_H
#define QUACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "parser.h"
#include "compiler.h"
#include "symbol.h"
#include "memory.h"
#include "env.h"
#include "vector.h"
#include "map.h"

void q_init(void);

#ifdef __cplusplus
}
#endif

#endif
