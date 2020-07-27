#ifndef VECTOR_H
#define VECTOR_H

#include "types.h"

typedef struct {
  size_t len;
  size_t size;
  q_atom values[];
} q_vec;

q_vec* q_vec_create(size_t cap);
q_vec* q_vec_append(q_vec* v, q_atom a);
void   q_vec_destroy(q_vec*);

#endif
