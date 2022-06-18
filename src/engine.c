#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "memory.h"
#include "env.h"
#include "parser.h"
#include "compiler.h"
#include "cps.h"
#include "symbol.h"
#include "types.h"

struct q_engine {
  q_memory *memory;
  q_env    *env;
  q_compiler *compiler;
};

q_engine* q_engine_create(void)
{
  q_engine* e = malloc(sizeof(q_engine));
  e->memory = q_memory_create();
  e->env = q_env_create(NULL);
  e->compiler = q_compiler_create(e->memory);

  return e;
}

void q_engine_destroy(q_engine *e)
{
  q_memory_destroy(e->memory);
  q_env_destroy(e->env);
  q_compiler_destroy(e->compiler);

  free(e);
}

q_err q_engine_eval(q_engine* e, q_atom a, q_atom *ret)
{
  qmain_func f = q_compile(e->compiler, a);
  *ret = f(e->env);
  return q_ok;
}

q_err q_engine_eval_string(q_engine* e, const char *s, q_atom *ret)
{
  q_atom atom;
  printf("before parsing: %s\n", s);

  q_err r = q_parse_buffer(e->memory, s, strlen(s), &atom);
  if (r) {
    printf("failed parsing!\n");
    return r;
  }
  q_dbg("after parsing: ", atom);

  return q_engine_eval(e, atom, ret);
}
