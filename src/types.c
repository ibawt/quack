#include "types.h"
#include <stdio.h>
#include "symbol.h"
#include <string.h>

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

q_atom make_cons(q_cons *q) { return (q_atom)q; }

q_atom make_nil(void) { return TAG_NIL; }

q_atom make_boolean(q_bool b) { return TAG_BOOL | b << TAG_BITS; }

q_atom make_symbol(q_symbol sym) { return TAG_SYMBOL | sym << TAG_BITS; }

void q_atom_print(FILE* file, q_atom atom)
{
  switch(q_atom_type_of(atom)) {
  case NUMBER:
    fprintf(file, "%ld", q_atom_integer(atom));
    break;

  case CONS: {
    fputc('(', file);
    for(q_cons* c = (q_cons*)atom ; c ; c = c->cdr) {
      q_atom_print(file, c->car);
      if(c->cdr) {
        fputc(' ', file);
      }
    }
    fputc(')', file);
  } break;
  case BOOLEAN:
    fprintf(file, q_atom_boolean(atom) ? "#t" : "#f");
    break;
  case SYMBOL:
    fprintf(file, "%s", q_symbol_string(q_atom_symbol(atom)));
    break;
  case STRING:
    fprintf(file, "\"%s\"", q_atom_string(atom)->data);
    break;
  case NIL:
    fprintf(file, "nil");
    break;
  case LAMBDA:
    fprintf(file, "lambda");
    break;
  case ENV:
    fprintf(file, "env");
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
