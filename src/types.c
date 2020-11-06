#include "types.h"
#include <stdio.h>
#include "symbol.h"
#include <string.h>
#include <printf.h>

void q_init(void)
{
}

q_atom_type q_atom_type_of(q_atom a) {
  switch (a & TAGMASK) {
  case TAG_INTEGER:
    return NUMBER;
  case TAG_BOOL:
    return BOOLEAN;
  case TAG_NIL:
    return NIL;
  case TAG_SYMBOL:
    return SYMBOL;
  }

  return ((q_heap_node *)(a - offsetof(q_heap_node, buff)))->type;
}

q_atom make_integer(const int64_t i) {
  return TAG_INTEGER | i << TAG_BITS;
}

int64_t q_atom_integer(q_atom a) {
  assert(q_atom_type_of(a) == NUMBER);
  return a >> TAG_BITS;
}

q_bool q_atom_boolean(q_atom a) {
  assert(q_atom_type_of(a) == BOOLEAN);
  return a >> TAG_BITS;
}

q_symbol q_atom_symbol(q_atom a) {
  assert(q_atom_type_of(a) == SYMBOL);
  return a >> TAG_BITS;
}

q_string* q_atom_string(q_atom a)
{
  return (q_string*)a;
}

q_atom make_string(q_string *s) { return (q_atom)s; }

/* q_atom make_cons(q_cons *q) { return (q_atom)q; } */

q_atom make_boolean(q_bool b) { return TAG_BOOL | b << TAG_BITS; }

q_atom make_symbol(q_symbol sym) { return TAG_SYMBOL | sym << TAG_BITS; }

int q_atom_print(FILE* file, q_atom atom)
{
  /* fprintf(file, "[RAW:%p]:", (void*)atom); */
  switch(q_atom_type_of(atom)) {
  case NUMBER:
    return fprintf(file, "%ld", q_atom_integer(atom));
  case CONS: {
    fputc('(', file);
    int n = 1;
    for(q_cons* c = (q_cons*)atom ; c ; c = c->cdr) {
      n += q_atom_print(file, c->car);
      if(c->cdr) {
        fputc(' ', file);
        n++;
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

  if( type_a != type_b)
    return false;

  switch(type_a) {
  case CONS: {
    q_cons *pa = (q_cons*)a;
    q_cons *pb = (q_cons*)b;
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
    return q_string_equals((q_string*)a, (q_string*)b);
  case NIL:
    return true;
  default:
    return a == b;
  }
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
