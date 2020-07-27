#ifndef ENV_H
#define ENV_H

#include "types.h"

typedef struct q_env q_env;

q_env* q_env_create(q_env*);
void q_env_destroy(q_env*);

q_err q_env_define(q_env*, q_symbol sym, q_atom value);
q_err q_env_lookup(q_env*, q_symbol, q_atom *);
q_err q_env_update(q_env*, q_symbol key, q_atom value);

#endif
