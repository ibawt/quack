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

#define Q_FAILED(x) ( x == q_fail )
#define Q_OK(x) ( x == q_ok )

typedef bool q_bool;

#define q_true true
#define q_false false

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
  TAG_NIL = 0,
  TAG_INTEGER = 1,
  TAG_BOOL = 2,
  TAG_SYMBOL = 3
} q_atom_tag;

#define TAG_BITS 3
#define TAGMASK BITMASK(TAG_BITS)

typedef int q_symbol;

typedef union { uintptr_t ival; void* pval; } q_atom;

q_bool q_atom_is_literal(q_atom);

typedef struct _cons {
  q_atom car;
  struct _cons *cdr;
} q_cons;

typedef struct {
  size_t len;
  char   data[];
} q_string;

typedef struct {
  void *fn_address;

  const char *native_name;

  q_cons* args;
  q_cons* body;

} q_lambda;

typedef struct {
  q_atom_type type;
  size_t      size;
  q_heap_flag flags;
  int         _pad;
  char        buff[];
} q_heap_node;

q_atom q_atom_from_ptr(void* p);

/* these names are all a mess */
q_atom_type q_atom_type_of(q_atom a);
q_atom make_integer(const int64_t i);

int64_t q_atom_integer(q_atom a);

q_atom make_string(q_string *s);

#define make_cons(q) ((q_atom)q)

q_atom make_nil();

q_atom make_boolean(q_bool b);

q_symbol q_atom_symbol(q_atom a);
q_atom make_symbol(q_symbol sym);

int q_atom_print(FILE *, q_atom);

void q_dbg(const char *prefix, q_atom a);

size_t q_cons_length(q_cons *);

q_bool q_equals(q_atom a, q_atom b);

#define SYM(x) make_symbol( q_symbol_create( x) )

typedef enum {
  INVALID_TYPE    = 1,
  NOT_ENOUGH_ARGS = 2,
} q_exceptions;

#ifdef __cplusplus
}
#endif

#endif
