#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

q_symbol q_symbol_create(const char *);
q_symbol q_symbol_create_buffer(char *, size_t);
const char* q_symbol_string(q_symbol sym);

#ifdef __cplusplus
}
#endif

#endif
