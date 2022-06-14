#ifndef COMPILER_H
#define COMPILER_H

#include "types.h"
#include "env.h"
#include "memory.h"

typedef q_atom (*qmain_func)(q_env*);

typedef struct q_compiler q_compiler;

q_compiler* q_compiler_create(q_memory* mem);
void        q_compiler_destroy(q_compiler*);

qmain_func q_compile(q_compiler* c, q_atom a);

#endif
