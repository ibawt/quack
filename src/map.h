#ifndef MAP_H
#define MAP_H

#include "types.h"

typedef struct q_map q_map;

q_map* q_map_create(void);
void   q_map_destroy(q_map*);

q_err  q_map_lookup(q_map*, q_symbol key, q_atom *value);
q_map* q_map_define(q_map*, q_symbol key, q_atom value);
q_err  q_map_update(q_map*, q_symbol key, q_atom value);

#endif
