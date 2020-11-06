#include "cps.h"
#include "symbol.h"
#include "memory.h"
#include <string.h>
#include <stdarg.h>
#include "vector.h"

#define LIST(m, ...) make_list(m, __VA_ARGS__, make_nil())

static q_bool is_primitive(q_atom a)
{
  q_atom_type type = q_atom_type_of(a);
  if( type != SYMBOL)
    return false;

  q_symbol s = (q_symbol)a;

  const char *name = q_symbol_string(s);

  if(strcmp(name, "+") == 0)
    return true;
  if(strcmp(name, "-") == 0 )
    return true;
  if(strcmp(name, "*") == 0 )
    return true;
  if(strcmp(name, "/") == 0 )
    return true;
  if(strcmp(name, "=") == 0 )
    return true;
  return false;
}

static q_atom
symbol(const char *s)
{
  return make_symbol(q_symbol_create(s));
}


static q_bool
is_atomic(q_atom a)
{
  switch(q_atom_type_of(a)) {
  case CONS: {
    q_cons * c = (q_cons *)a;
    if ( q_equals(c->car, symbol("lambda"))) {
      return q_true;
    }
  } break;
  case NUMBER:
  case SYMBOL:
  case NIL:
  case STRING:
  case BOOLEAN:
    return q_true;
  default:
    break;
  }
  return q_false;
}

static q_atom make_list(q_memory *m, ...)
{
  va_list args;
  va_start(args, m);
  q_vec *v = q_vec_create(6);

  for(;;) {
    q_atom a = va_arg(args, q_atom);
    if (a == make_nil()) {
      break;
    }
    v = q_vec_append(v, a);
  }
  va_end(args);

  q_cons* c = NULL;
  for( int i = v->len - 1 ; i >= 0 ; --i) {
    c = q_memory_alloc_cons(m, v->values[i], c);
  }
  q_vec_destroy(v);

  return make_cons(c);
}

static q_atom append_atom(q_memory *m, q_cons *c, q_atom a)
{
  for( ; c ; c = c->cdr ) {
    if(!c->cdr) {
      q_cons *c = q_memory_alloc_cons(m, a, NULL);
      c->cdr = c;
    }
  }
  return make_cons(c);
}

static q_err q_cps_atomic(q_memory *m, q_atom e, q_atom *r)
{
  switch(q_atom_type_of(e)) {
  case CONS: {
    q_cons *c = (q_cons*)e;

    if( q_equals(c->car, symbol("lambda"))) {
      q_symbol cont = q_gensym("k");
      q_atom l = make_list(m, symbol("lambda"), append_atom(m, (q_cons*)c->cdr->car, make_symbol(cont)), make_nil());

      q_atom body;
      q_err res = q_cps_transform(m, make_cons(c->cdr->cdr), make_symbol(cont),&body);
      if (res) {
        return q_fail;
      }
      *r = append_atom(m, (q_cons*)l, body);

      return q_ok;
    }
  }break;
  default:
    *r = e;
  }

  return q_ok;
}

q_err q_cps_transform_k(q_memory *m, q_atom a, q_atom cont, q_atom *r)
{
  q_dbg("a", a);
  q_dbg("cont", cont);

  if (is_atomic(a)) {
    q_atom atomic;
    if (q_cps_atomic(m, a, &atomic)) {
      return q_fail;
    }
    *r = atomic;
    return q_ok;
  }


  switch(q_atom_type_of(a)) {
  case CONS: {
    q_cons* c = (q_cons*)a;
  }break;
  }

  return q_fail;
}

q_err cps_star_k(q_memory *m, q_cons* c, q_atom *r)
{
}

q_err q_cps_transform(q_memory *m, q_atom a, q_atom cont, q_atom *r)
{
  q_dbg("CPS", a);

  if (is_atomic(a)) {
    // (? aexpr?) => `(,c ,(M expr))
    q_atom atomic_res;
    if (q_cps_atomic(m, a, &atomic_res)) {
      return q_fail;
    }

    q_atom l = make_list(m, cont, atomic_res, make_nil());
    *r = l;
    return q_ok;
  }

  switch(q_atom_type_of(a)) {
  case CONS: {
    q_cons *c = (q_cons*)a;
    if(q_equals(c->car, symbol("begin")) && c->cdr->cdr == NULL) {
      // lets ignore this for now
    }
    else if(q_equals(c->car, symbol("if"))) {
      q_symbol k = q_gensym("k");
      if(q_cons_length(c) < 3 ) {
        return q_false;
      }
      q_atom condition = c->cdr->car;
      q_atom texpr = c->cdr->cdr->car;
      q_atom fexpr = c->cdr->cdr->cdr->car;

      q_atom left,right;

      if( q_cps_transform(m, texpr, make_symbol(k), &left) ) {
        printf("left failed\n");
        return q_fail;
      }

      if( q_cps_transform(m, fexpr, make_symbol(k), &right)) {
        printf("right failed\n");
        return q_fail;
      }
      q_symbol cond = q_gensym("cond");
      q_atom t_k;

      if( is_atomic(condition)) {
        t_k = make_list(m, symbol("lambda"),
                             make_list(m, make_symbol(k), make_nil()),
                             make_list(m, symbol("if"), condition, left, right, make_nil()),
                             make_nil());
      } else if( q_cps_transform_k(m, condition, make_list(m, symbol("lambda"),
                                                    make_list(m, make_symbol(cond), make_nil()),
                                                    make_list(m, symbol("if"), make_symbol(cond), left, right, make_nil()),
                                                    make_nil()), &t_k)) {
        printf("transform_k failed\n");
        return q_fail;
      }

      q_atom l = LIST(m,
                      LIST(m, symbol("lambda"),
                           LIST(m, make_symbol(k))
                           , t_k)
                      , cont);
      q_dbg("l", l);

      *r = l;
      return q_ok;
    }
  }break;
  }
  printf("fall through\n");
  return q_fail;
}
