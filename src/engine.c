#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "memory.h"
#include "env.h"
#include "parser.h"
#include "compiler.h"
#include "cps.h"
#include "symbol.h"

struct q_engine {
  q_memory *memory;
  q_env    *env;
};

q_engine* q_engine_create(void)
{
  q_engine* e = malloc(sizeof(q_engine));
  e->memory = q_memory_create();
  e->env = q_env_create(NULL);
  return e;
}

void q_engine_destroy(q_engine *e)
{
  q_memory_destroy(e->memory);
  q_env_destroy(e->env);


  free(e);
}

q_err q_engine_eval(q_engine* e, q_atom a, q_atom *ret)
{
  main_func f = compile(a);
  *ret = f(e->env);
  return q_ok;
}

q_err q_engine_eval_string(q_engine* e, const char *s, q_atom *ret)
{
  q_atom atom;
  printf("evaling: %s\n", s);

  q_err r = q_parse_buffer(e->memory, s, strlen(s), &atom);
  if (r) {
    return r;
  }

  q_atom translated;
  if( q_cps_transform(e->memory, atom, make_symbol(q_symbol_create("display")), &translated)) {
    return q_fail;
  }
  printf("\nAFTER\n");
  q_atom_print(stdout, translated);
  printf("\n");
  *ret = make_nil();

  return q_ok;

  /* return q_engine_eval(e, atom, ret); */
}
