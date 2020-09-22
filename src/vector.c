#include "vector.h"
#include <stdlib.h>
#include <string.h>

q_vec *q_vec_create(size_t cap) {
  q_vec *q = malloc(sizeof(q_vec) + cap * sizeof(q_atom));
  q->len = 0;
  q->size = cap;

  return q;
}

q_vec *q_vec_append(q_vec *v, q_atom a) {
  if (v->len >= v->size) {
    v = realloc(v, sizeof(q_vec) + (v->size * 2 * sizeof(q_atom)));
    v->size *= 2;
  }
  v->values[v->len++] = a;
  return v;
}

void q_vec_destroy(q_vec *v) { free(v); }
