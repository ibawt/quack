#ifndef COMPILER_H
#define COMPILER_H

#include "types.h"
#include "env.h"

typedef q_atom (*main_func)(q_env*);

main_func compile(q_atom a);

#endif
