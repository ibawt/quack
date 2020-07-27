#include "map.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SIZE 16

typedef struct {
  q_symbol key;
  q_atom value;
} node;

struct q_map {
  size_t len;
  size_t size;
  node nodes[];
};

q_map *q_map_create(void) {
  size_t len = sizeof(q_map) + sizeof(node) * DEFAULT_SIZE;
  q_map *p = malloc(len);
  memset(p, 0, len);

  p->len = 0;
  p->size = DEFAULT_SIZE;

  return p;
}

void q_map_destroy(q_map *p) { free(p); }

q_err q_map_lookup(q_map *m, q_symbol key, q_atom *value) {
  for (int i = m->len - 1; i >= 0; --i) {
    if (m->nodes[i].key == key) {
      *value = m->nodes[i].value;
      return q_ok;
    }
  }
  return q_fail;
}

q_map *q_map_define(q_map *m, q_symbol key, q_atom value) {
  if (m->len >= m->size) {
    m = realloc(m, sizeof(q_map) + sizeof(node) * m->size * 2);
    m->size *= 2;
  }
  m->nodes[m->len].key = key;
  m->nodes[m->len++].value = value;
  return m;
}

q_err q_map_update(q_map *m, q_symbol key, q_atom value) {
  for (int i = m->len - 1; i >= 0; --i) {
    if (m->nodes[i].key == key) {
      m->nodes[i].value = value;
      return q_ok;
    }
  }
  return q_fail;
}
