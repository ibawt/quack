#ifndef MEMORY_H_
#define MEMORY_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct q_memory q_memory;

q_memory* q_memory_create(void);
void q_memory_destroy(q_memory*);

q_cons* q_memory_alloc_cons(q_memory *,q_atom car,q_cons *cdr );
q_string* q_memory_alloc_string(q_memory*, char *, size_t len);
q_lambda* q_memory_alloc_lambda(q_memory*, q_cons* args, q_cons* body);

void q_memory_mark_and_sweep(q_memory*);

q_bool q_memory_is_heap_object(q_atom a);
void q_memory_mark(q_atom a);

#ifdef __cplusplus
}
#endif


#endif
