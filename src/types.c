#include "types.h"
#include <stdio.h>
#include "symbol.h"
#include <string.h>
#include <printf.h>

void q_init(void)
{
}

q_atom_type q_atom_type_of(q_atom a) {
  switch (a.ival & TAGMASK) {
  case TAG_INTEGER:
    return NUMBER;
  case TAG_BOOL:
    return BOOLEAN;
  case TAG_SYMBOL:
    return SYMBOL;
  }

  if( a.ival == 0) {
    return NIL;
  }

  return ((q_heap_node *)(a.pval - offsetof(q_heap_node, buff)))->type;
}

q_atom make_integer(const int64_t i) {
  q_atom v = {.ival = TAG_INTEGER | i << TAG_BITS};
  return v;
}

int64_t q_atom_integer(q_atom a) {
  assert(q_atom_type_of(a) == NUMBER);
  return a.ival >> TAG_BITS;
}

q_bool q_atom_boolean(q_atom a) {
  assert(q_atom_type_of(a) == BOOLEAN);
  return a.ival >> TAG_BITS;
}

q_symbol q_atom_symbol(q_atom a) {
  assert(q_atom_type_of(a) == SYMBOL);
  return a.ival >> TAG_BITS;
}

q_string* q_atom_string(q_atom a)
{
  return (q_string*)a.pval;
}

q_atom make_string(q_string *s) {
  q_atom v = { .pval = s };
  return v;
}


/* q_atom make_cons(q_cons *q) { return (q_atom)q; } */

q_atom make_boolean(q_bool b) {
  q_atom v = {.ival = TAG_BOOL | b << TAG_BITS };
  return v;
}

q_atom make_symbol(q_symbol sym) {
  q_atom v = { .ival = TAG_SYMBOL | sym << TAG_BITS };
  return v;
}

int q_atom_print(FILE* file, q_atom atom)
{
  q_atom_type t = q_atom_type_of(atom);
  switch(q_atom_type_of(atom)) {
  case NUMBER:
    return fprintf(file, "%ld", q_atom_integer(atom));
  case CONS: {
    fputc('(', file);
    int n = 1;
    if( atom.pval) {
      for (q_cons *c = (q_cons *)atom.pval; c; c = c->cdr) {
        n += q_atom_print(file, c->car);
        if (c->cdr) {
          fputc(' ', file);
          n++;
        }
      }
    }
    fputc(')', file);
    n++;
    return n;
  } break;
  case BOOLEAN:
    return fprintf(file, q_atom_boolean(atom) ? "#t" : "#f");
  case SYMBOL:
    return fprintf(file, "%s", q_symbol_string(q_atom_symbol(atom)));
  case STRING:
    return fprintf(file, "\"%s\"", q_atom_string(atom)->data);
  case NIL:
    return fprintf(file, "nil");
  case LAMBDA:
    return fprintf(file, "lambda");
  case ENV:
    return fprintf(file, "env");
  default:
    return fprintf(file, "idk: %d", t);
  }
}

q_bool q_string_equals(q_string* a, q_string *b)
{
  return strncmp(a->data, b->data, (a->len < b->len ? a->len : b->len)) == 0;
}

q_bool q_equals(q_atom a, q_atom b)
{
  q_atom_type type_a = q_atom_type_of(a);
  q_atom_type type_b = q_atom_type_of(b);

  if( type_a != type_b) {
    return false;
  }

  switch(type_a) {
  case CONS: {
    q_cons *pa = (q_cons*)a.pval;
    q_cons *pb = (q_cons*)b.pval;
    for(;;) {
      if(pa && pb) {
        if (!q_equals(pa->car, pb->car)) {
          return false;
        }
        pa = pa->cdr;
        pb = pb->cdr;
      } else if (!pa && !pb) {
        return true;
      } else {
        return false;
      }
    }
  } break;
  case STRING:
    return q_string_equals((q_string*)a.pval, (q_string*)b.pval);
  case NIL:
    return true;
  default:
    return a.ival == b.ival;
  }
}

q_cons *q_atom_as_cons(q_atom a) {
  return (q_cons*)a.pval;
}

q_bool q_atom_is_literal(q_atom a)
{
  switch(q_atom_type_of(a)) {
  case ENV:
  case CONS:
  case LAMBDA:
    return false;
  default:
    return true;
  }
}

void q_dbg(const char *prefix, q_atom a)
{
  printf("%s: ", prefix);
  q_atom_print(stdout, a);
  putchar('\n');
}

size_t q_cons_length(q_cons *c)
{
  size_t i = 0;
  for( ; c ; c = c->cdr, ++i);
  return i;
}

q_atom make_nil()
{
  q_atom a = {.ival = TAG_NIL};
  return a;
}

q_atom q_atom_from_ptr(void *p) {
  q_atom a = {.pval = p};
  return a;
}
