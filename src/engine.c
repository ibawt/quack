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

#include <setjmp.h>

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
  if(!f) {
    return q_fail;
  }
  jmp_buf jmp_env;
  switch(setjmp(jmp_env)) {
    case 0: {
      q_atom jmp_env_atom = {.pval = jmp_env};
      q_env_define(e->env, q_symbol_create("exception_env"), jmp_env_atom);
      *ret = f(e->env);
    } break;
    case INVALID_TYPE:
      printf("Invalid type!\n");
      return q_fail;
    case NOT_ENOUGH_ARGS:
      printf("not enough arguments!\n");
      return q_fail;
    default:
      printf("exception!\n");
      return q_fail;
  }
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
