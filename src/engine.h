#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct q_engine q_engine;

q_engine* q_engine_create(void);
void q_engine_destroy(q_engine*);

q_err q_engine_eval_string(q_engine*, const char*, q_atom*);

#ifdef __cplusplus
}
#endif

#endif
