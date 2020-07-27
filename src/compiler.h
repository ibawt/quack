#ifndef COMPILER_H
#define COMPILER_H

#include "types.h"

typedef int (*main_func)(void);

main_func compile(q_atom a);

#endif
