#include <stdlib.h>

#include "env.h"
#include "map.h"
#include "symbol.h"

struct q_env {
  struct q_env *parent;
  q_map *map;
};

q_env *q_env_create(q_env *parent) {
  q_env *env = malloc(sizeof(q_env));
  env->parent = parent;
  env->map = q_map_create();
  return env;
}

void q_env_destroy(q_env *env) {
  q_map_destroy(env->map);
  free(env);
}

q_err q_env_define(q_env *e, q_symbol sym, q_atom value) {
  e->map = q_map_define(e->map, sym, value);
  return q_ok;
}

q_err q_env_lookup(q_env *e, q_symbol k, q_atom *ret) {
  for (; e; e = e->parent) {
    if (!q_map_lookup(e->map, k, ret)) {
      return q_ok;
    }
  }
  return q_fail;
}

q_err q_env_update(q_env *e, q_symbol key, q_atom value) {
  for (; e; e = e->parent) {
    if (!q_map_update(e->map, key, value)) {
      return q_ok;
    }
  }
  return q_fail;
}
