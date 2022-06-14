#include <stdlib.h>
#include <string.h>
#include "env.h"
#include "memory.h"
#include "symbol.h"
#include "types.h"
#include "compiler.h"

#include "libgccjit.h"

struct q_compiler {
  gcc_jit_context *ctx;
  gcc_jit_type    *atom_type;
  gcc_jit_type    *void_ptr;
  gcc_jit_struct  *string_type;

  gcc_jit_struct  *cons_type;
  gcc_jit_field   *cons_params[2];

  int counter;
  q_memory        *q_mem;


  gcc_jit_function *env_get;
  gcc_jit_function *cons;
  gcc_jit_function *car;

  gcc_jit_function *current_func;
  gcc_jit_block    *current_block;
};


static gcc_jit_rvalue* cast_pointer_to_atom(q_compiler *c, gcc_jit_rvalue* p) {
  return gcc_jit_context_new_binary_op(
      c->ctx, NULL, GCC_JIT_BINARY_OP_BITWISE_AND, c->atom_type, p, p);
}

static gcc_jit_rvalue *
compile_string(q_compiler *c, q_atom a) {
  q_string *s = (q_string*)a;

  return cast_pointer_to_atom(c, gcc_jit_context_new_rvalue_from_ptr(
      c->ctx, gcc_jit_type_get_pointer(
          gcc_jit_struct_as_type(c->string_type)), s));
}

q_atom env_get(q_env* env, q_atom key) {
  q_atom res;

  if( q_env_lookup(env, q_atom_symbol(key), &res)) {
    return make_nil();
  }
  return res;
}

gcc_jit_function* env_get_native(q_compiler *c) {
  gcc_jit_param *fn_params[] = {
    gcc_jit_context_new_param(c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR), "env"),
    gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "key")
  };

  gcc_jit_function* fn = gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_ALWAYS_INLINE, c->atom_type, "env_get_0", 2, fn_params , 0);

  gcc_jit_block* entry  = gcc_jit_function_new_block(fn, "entry");

  gcc_jit_lvalue* res = gcc_jit_function_new_local(fn, NULL, c->atom_type, "res");

  gcc_jit_block* then = gcc_jit_function_new_block(fn, "then");
  gcc_jit_block* else_b = gcc_jit_function_new_block(fn, "else");

  gcc_jit_param *params[] = {
      gcc_jit_context_new_param(
          c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR),
          "env"),
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "key"),
      gcc_jit_context_new_param(c->ctx, NULL, gcc_jit_type_get_pointer(c->atom_type), "res")
  };

  gcc_jit_function* func =  gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_IMPORTED,
                                                         c->atom_type, "env_get",
                                                         3, params, 0);

  gcc_jit_rvalue *func_params[] = {
      gcc_jit_param_as_rvalue(gcc_jit_function_get_param(fn, 0)),
      gcc_jit_param_as_rvalue(gcc_jit_function_get_param(fn, 1)),
      gcc_jit_lvalue_get_address(res, NULL)};

  gcc_jit_rvalue* func_result = gcc_jit_context_new_call(c->ctx, NULL, func, 3, func_params);

  gcc_jit_rvalue* pred = gcc_jit_context_new_comparison(c->ctx, NULL, GCC_JIT_COMPARISON_EQ, gcc_jit_context_zero(c->ctx, c->atom_type), func_result);

  gcc_jit_block_end_with_conditional(entry, NULL, pred, then, else_b);

  gcc_jit_block_end_with_return(then, NULL, gcc_jit_lvalue_as_rvalue(res));
  gcc_jit_block_end_with_return(else_b, NULL, gcc_jit_context_new_rvalue_from_long(c->ctx, c->atom_type, make_nil()));

  return fn;
}

static gcc_jit_function* native_cons(q_compiler *c) {
  gcc_jit_param *params[] = {
    gcc_jit_context_new_param(c->ctx, NULL, c->void_ptr, "memory"),
    gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "head"),
    gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "tail")
  };

  gcc_jit_function *fn = gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_IMPORTED, c->void_ptr, "q_memory_alloc_cons", 3, params, 0);
  return fn ;
}

static gcc_jit_function*
compile_cons(q_compiler *c) {
  gcc_jit_param *params[] = {
    gcc_jit_context_new_param(c->ctx, NULL, c->void_ptr, "env"),
    gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "head"),
    gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "tail")
  };

  gcc_jit_function* alloc_cons = native_cons(c); 

  gcc_jit_function *fn = gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_ALWAYS_INLINE, c->atom_type, "cons", 3, params, 0);

  gcc_jit_block *b = gcc_jit_function_new_block(fn, "entry");

  gcc_jit_lvalue *res = gcc_jit_function_new_local(fn, NULL, c->void_ptr, "tmp");

  gcc_jit_rvalue *pparams[] = {
    gcc_jit_param_as_rvalue( params[0]),
    gcc_jit_param_as_rvalue( params[1]),
    gcc_jit_param_as_rvalue( params[2])
  };

  gcc_jit_rvalue* call_res = gcc_jit_context_new_call(c->ctx, NULL, alloc_cons, 3, pparams);

  gcc_jit_block_add_assignment(b, NULL, res, call_res);


  gcc_jit_rvalue* p =  cast_pointer_to_atom( c, gcc_jit_lvalue_as_rvalue(res));

  gcc_jit_block_end_with_return(b, NULL, p);

  return fn;
}

static gcc_jit_function *
compile_car(q_compiler *c)    {
  gcc_jit_param *param = gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "a");
  gcc_jit_function *fn = gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_ALWAYS_INLINE, c->atom_type, "car", 1, &param, 0);

  gcc_jit_block *b = gcc_jit_function_new_block(fn, "entry");

  gcc_jit_rvalue* = gcc_jit

  gcc_jit_rvalue* car = gcc_jit_rvalue_access_field( r, NULL, c->cons_params[0]);

  gcc_jit_block_end_with_return(b, NULL, car);

  return fn;
}

static gcc_jit_rvalue*
compile_atom(q_compiler* ctx, q_atom a) {
  q_dbg("compile_atom", a);
  switch (q_atom_type_of(a)) {
  case NUMBER:
    return gcc_jit_context_new_rvalue_from_int(ctx->ctx, ctx->atom_type, a);
  case SYMBOL: {
    gcc_jit_rvalue *env = gcc_jit_param_as_rvalue(gcc_jit_function_get_param(ctx->current_func, 0));

    gcc_jit_rvalue *params[] = { env, gcc_jit_context_new_rvalue_from_int(ctx->ctx, ctx->atom_type, a)};
    return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->env_get, 2, params);
  }
  case STRING:
    return compile_string(ctx, a);
  case CONS: {
    q_cons *cons = (q_cons *)a;
    if (cons->cdr == NULL) {
      return NULL;
    }

    if (q_equals(cons->car, SYM("quote"))) {
      return gcc_jit_context_new_rvalue_from_long(ctx->ctx, ctx->atom_type,
                                                  cons->cdr->car);
    }
    else if(q_equals(cons->car, SYM("cons"))) {
      gcc_jit_rvalue *params[] = {
        gcc_jit_context_new_rvalue_from_ptr( ctx->ctx, ctx->void_ptr, ctx->q_mem),
        compile_atom(ctx, cons->cdr->car),
        compile_atom(ctx, cons->cdr->cdr->car)
      };

      return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->cons, 3, params);
    }
    else if(q_equals(cons->car, SYM("car"))) {
      gcc_jit_rvalue* param = compile_atom(ctx, cons->cdr->car);
      return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->car, 1, &param);
    }
  }
    return NULL;
  default:
    printf("idk how to deal with this: \n");
    q_atom_print(stdout, a);
    return NULL;
  }
}

q_compiler* q_compiler_create(q_memory* mem) {
  q_compiler* c = malloc(sizeof(q_compiler));
  memset(c, 0, sizeof(q_compiler));
  c->ctx = gcc_jit_context_acquire();
  c->atom_type = gcc_jit_context_get_int_type(c->ctx, 8, 0);
  gcc_jit_context_set_bool_option(c->ctx, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, 1);
  gcc_jit_context_set_str_option(c->ctx, GCC_JIT_STR_OPTION_PROGNAME, "quack");
  gcc_jit_context_set_int_option(c->ctx, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 1);
  gcc_jit_type* char_array = gcc_jit_context_new_array_type(c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_CHAR), 0);

  gcc_jit_field *fields[2] = { gcc_jit_context_new_field(c->ctx, NULL, c->atom_type, "len"),
    gcc_jit_context_new_field(c->ctx, NULL, char_array, "buf")};

  c->string_type = gcc_jit_context_new_struct_type(c->ctx, NULL,
                                                   "q_string", 2,
                                                   fields);
  c->env_get = env_get_native(c);
  c->void_ptr = gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR);
  c->q_mem = mem;
  c->cons = compile_cons(c);

  c->cons_type = gcc_jit_context_new_opaque_struct(c->ctx, NULL, "q_cons");

  gcc_jit_field *cons_fields[] = {
    gcc_jit_context_new_field(c->ctx, NULL, c->atom_type, "car"),
    gcc_jit_context_new_field(c->ctx, NULL, gcc_jit_type_get_pointer(gcc_jit_struct_as_type(c->cons_type)), "cdr")
  };
  c->cons_params[0] = cons_fields[0];
  c->cons_params[1] = cons_fields[1];
  gcc_jit_struct_set_fields(c->cons_type, NULL, 2, cons_fields);

  c->car = compile_car(c);

  return c;
}

void q_compiler_destroy(q_compiler *c) {
  gcc_jit_context_release(c->ctx);
  free(c);
}

qmain_func q_compile(q_compiler* c, q_atom a)
{
  char buf[256];
  snprintf(buf, sizeof(buf), "quackmain_%d", c->counter++);

  gcc_jit_param *param =
      gcc_jit_context_new_param(c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR), "env");
  gcc_jit_function* fn = gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_EXPORTED, c->atom_type, buf, 1, &param, 0);
  assert(fn);
  c->current_func = fn;
  gcc_jit_rvalue* ret = compile_atom(c, a);
  if(!ret) {
    fprintf(stderr, "error: %s\n", gcc_jit_context_get_last_error(c->ctx));
    return NULL;
  }


  gcc_jit_block* block = gcc_jit_function_new_block(fn, "entry");

  gcc_jit_block_end_with_return(block, NULL, ret);

  gcc_jit_result* result = gcc_jit_context_compile(c->ctx);
  assert(result);

  gcc_jit_function_dump_to_dot(fn, "flow.dot");

  return (qmain_func)gcc_jit_result_get_code(result, buf);
}
