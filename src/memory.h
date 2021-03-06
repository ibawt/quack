#ifndef MEMORY_H_
#define MEMORY_H_

#include "types.h"

typedef struct q_memory q_memory;

q_memory* q_memory_create(void);
void q_memory_destroy(q_memory*);

q_cons* q_memory_alloc_cons(q_memory *,q_atom car,q_cons *cdr );
q_string* q_memory_alloc_string(q_memory*, char *, size_t len);

void q_memory_mark_and_sweep(q_memory*);

q_bool q_memory_is_heap_object(q_atom a);
void q_memory_mark(q_atom a);



#endif
