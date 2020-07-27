#ifndef PARSER_H_
#define PARSER_H_

#include "types.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

q_err q_parse_file(q_memory*, const char *filename, q_atom* ret);
q_err q_parse_buffer(q_memory *mem, const char *data, size_t len, q_atom *ret);

#ifdef __cplusplus
}
#endif

#endif
