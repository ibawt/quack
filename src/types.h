#ifndef TYPES_H_
#define TYPES_H_

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  q_ok = 0,
  q_fail = 1,
} q_err;

typedef bool q_bool;

typedef enum {
  NUMBER = 0,
  CONS,
  SYMBOL,
  STRING,
  NIL,
  BOOLEAN,
  LAMBDA,
  ENV
} q_atom_type;

typedef enum { USED = 1, LOCKED = 2 } q_heap_flag;

#define BITMASK(x) ((1 << x) - 1)

typedef enum {
  TAG_INTEGER = 1,
  TAG_BOOL = 2,
  TAG_NIL = 3,
  TAG_SYMBOL = 4
} q_atom_tag;

#define TAG_BITS 3
#define TAGMASK BITMASK(TAG_BITS)

typedef int q_symbol;

typedef uintptr_t q_atom;

typedef struct _cons {
  q_atom car;
  struct _cons *cdr;
} q_cons;

typedef struct {
  size_t len;
  char data[];
} q_string;

typedef struct {
  q_atom_type type;
  size_t      size;
  q_heap_flag flags;
  int         _pad;
  char buff[];
} q_heap_node;

q_atom_type q_atom_type_of(q_atom a);
q_atom make_integer(const int64_t i);

int64_t q_atom_integer(q_atom a);

q_atom make_string(q_string *s);

q_atom make_cons(q_cons *q);

q_atom make_nil(void);

q_atom make_boolean(q_bool b);

q_atom make_symbol(q_symbol sym);

void q_atom_print(FILE*, q_atom);

q_bool q_equals(q_atom a, q_atom b);

#ifdef __cplusplus
}
#endif

#endif
