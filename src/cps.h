#ifndef CPS_H
#define CPS_H

#include "types.h"
#include "memory.h"

q_err q_cps_transform(q_memory *memory, q_atom atom, q_atom cont, q_atom *ret);

#endif
