#include "compiler.h"
#include "env.h"
#include "memory.h"
#include "symbol.h"
#include "types.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "libgccjit.h"

typedef struct _lambda_list_node {
  q_lambda *item;
  struct _lambda_list_node *next;
} lambda_list_node;

struct q_compiler {
  gcc_jit_context *ctx;
  gcc_jit_type *atom_type;
  gcc_jit_type *int_type;
  gcc_jit_type *void_ptr;
  gcc_jit_struct *string_type;
  gcc_jit_struct *lambda_type;

  gcc_jit_field *atom_fields[2];

  gcc_jit_struct *cons_type;
  gcc_jit_field *cons_params[2];

  int counter;
  int lambda_counter;

  q_memory *q_mem;

  gcc_jit_function *env_get;
  gcc_jit_function *cons;
  gcc_jit_function *car;
  gcc_jit_function *cdr;
  gcc_jit_function *define;

  gcc_jit_function *current_func;
  gcc_jit_block *current_block;

  lambda_list_node *lambda_list;
};

/* TODO: I think I don't have to do this  */
static void push_lambda(q_compiler *c, q_lambda *l) {
  lambda_list_node *n = malloc(sizeof(lambda_list_node));
  memset(n, 0, sizeof(lambda_list_node));

  n->item = l;
  if (c->lambda_list) {
    n->next = c->lambda_list;
  }
  c->lambda_list = n;
}

static void destroy_lambdas(q_compiler *c) {
  while (c->lambda_list) {
    lambda_list_node *t = c->lambda_list;
    c->lambda_list = t->next;
    free(t);
  }
}

static gcc_jit_rvalue *compile_atom(q_compiler *c, q_atom a);
static gcc_jit_rvalue *atom_from_ptr(q_compiler *c, void *ptr);

static gcc_jit_rvalue *compile_lambda(q_compiler *c, q_atom args, q_atom body) {
  assert(q_atom_type_of(args) == CONS || q_atom_type_of(args) == NIL);
  assert(q_atom_type_of(body) == CONS);

  int len = q_cons_length(args.pval);

  gcc_jit_param **params = malloc(sizeof(gcc_jit_param *) * len+1);
  q_cons *cons = args.pval;

  params[0] = gcc_jit_context_new_param(c->ctx, NULL, c->void_ptr, "env");
  for (int i = 1; i < len; ++i) {
    params[i] = gcc_jit_context_new_param(c->ctx, NULL, c->atom_type,
                                          q_symbol_string(cons->car.ival));
    cons = cons->cdr;
  }

  char namebuf[256];
  snprintf(namebuf, sizeof(namebuf), "_lambda_%d", c->lambda_counter++);

  gcc_jit_function *fn =
      gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_EXPORTED,
                                   c->atom_type, namebuf, len+1, params, 0);

  gcc_jit_block *b = gcc_jit_function_new_block(fn, "entry");

  /* TODO: implement a compiler context stack thing so this is less terrible  */
  gcc_jit_function *old_fun = c->current_func;
  gcc_jit_block *old_b = c->current_block;

  c->current_block = b;
  c->current_func = fn;

  gcc_jit_rvalue *e = NULL;

  for (q_cons *cons = body.pval; cons; cons = cons->cdr) {
    e = compile_atom(c, cons->car);
    assert(e);
    if(!e) {
      return NULL;
    }
  }

  gcc_jit_block_end_with_return(b, NULL, e);

  q_lambda *lam = q_memory_alloc_lambda(c->q_mem, args.pval, body.pval);

  lam->native_name = strdup(namebuf);

  c->current_block = old_b;
  c->current_func = old_fun;

  free(params);

  push_lambda(c, lam);

  return atom_from_ptr(c, lam);
}

static gcc_jit_rvalue *atom_from_long(q_compiler *c, long i) {
  assert(c->atom_type);
  return gcc_jit_context_new_union_constructor(
      c->ctx, NULL, c->atom_type, c->atom_fields[0],
      gcc_jit_context_new_rvalue_from_long(c->ctx, c->int_type, i));
}

static gcc_jit_rvalue *atom_from_rvalue_ptr(q_compiler *c, gcc_jit_rvalue *r) {

  gcc_jit_rvalue *cast = gcc_jit_context_new_cast(c->ctx, NULL, r, c->void_ptr);

  return gcc_jit_context_new_union_constructor(c->ctx, NULL, c->atom_type,
                                               c->atom_fields[1], cast);
}

static gcc_jit_rvalue *atom_from_ptr(q_compiler *c, void *ptr) {
  return gcc_jit_context_new_union_constructor(
      c->ctx, NULL, c->atom_type, c->atom_fields[1],
      gcc_jit_context_new_rvalue_from_ptr(c->ctx, c->void_ptr, ptr));
}

static gcc_jit_rvalue *compile_string(q_compiler *c, q_atom a) {
  return atom_from_ptr(c, a.pval);
}

static gcc_jit_function *define(q_compiler *c) {
  gcc_jit_param *fn_param[] = {
      gcc_jit_context_new_param(c->ctx, NULL, c->void_ptr, "env"),
      gcc_jit_context_new_param(c->ctx, NULL, c->int_type, "symbol"),
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "value")};

  gcc_jit_function *fn = gcc_jit_context_new_function(
      c->ctx, NULL, GCC_JIT_FUNCTION_IMPORTED, c->atom_type, "q_env_define", 3,
      fn_param, 0);
  return fn;
}

static gcc_jit_function *env_get(q_compiler *c) {
  gcc_jit_param *fn_params[] = {
      gcc_jit_context_new_param(
          c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR),
          "env"),
      gcc_jit_context_new_param(c->ctx, NULL, c->int_type, "key")};

  gcc_jit_function *fn = gcc_jit_context_new_function(
      c->ctx, NULL, GCC_JIT_FUNCTION_IMPORTED, c->atom_type, "q_env_lookup", 2,
      fn_params, 0);

  return fn;
}

static gcc_jit_function *native_cons(q_compiler *c) {
  gcc_jit_param *params[] = {
      gcc_jit_context_new_param(c->ctx, NULL, c->void_ptr, "memory"),
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "head"),
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "tail")};

  gcc_jit_function *fn = gcc_jit_context_new_function(
      c->ctx, NULL, GCC_JIT_FUNCTION_IMPORTED, c->void_ptr,
      "q_memory_alloc_cons", 3, params, 0);
  return fn;
}

static gcc_jit_function *compile_cons(q_compiler *c) {
  gcc_jit_param *params[] = {
      gcc_jit_context_new_param(c->ctx, NULL, c->void_ptr, "env"),
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "head"),
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "tail")};

  gcc_jit_function *alloc_cons = native_cons(c);

  gcc_jit_function *fn =
      gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_ALWAYS_INLINE,
                                   c->atom_type, "cons", 3, params, 0);

  gcc_jit_block *b = gcc_jit_function_new_block(fn, "entry");

  gcc_jit_lvalue *res =
      gcc_jit_function_new_local(fn, NULL, c->void_ptr, "tmp");

  gcc_jit_rvalue *pparams[] = {gcc_jit_param_as_rvalue(params[0]),
                               gcc_jit_param_as_rvalue(params[1]),
                               gcc_jit_param_as_rvalue(params[2])};

  gcc_jit_rvalue *call_res =
      gcc_jit_context_new_call(c->ctx, NULL, alloc_cons, 3, pparams);

  gcc_jit_block_add_assignment(b, NULL, res, call_res);

  gcc_jit_block_end_with_return(
      b, NULL, atom_from_rvalue_ptr(c, gcc_jit_lvalue_as_rvalue(res)));

  return fn;
}

q_atom throw(q_env *env, int a) {
  q_atom e = q_env_lookup(env, q_symbol_create("exception_env"));
  assert( e.ival != 0);

  longjmp(e.pval, a);
  return make_nil();
}

static gcc_jit_rvalue* compile_throw(q_compiler *c, gcc_jit_rvalue* e, gcc_jit_rvalue* r) {
  gcc_jit_param *throw_params[] = {
    gcc_jit_context_new_param(c->ctx, NULL, c->void_ptr, "env"),
    gcc_jit_context_new_param(c->ctx, NULL, c->int_type, "a")
  };

  gcc_jit_function* fn = gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_IMPORTED, c->atom_type,"throw" ,
                                                      2, throw_params, 0);

  gcc_jit_rvalue* params[] = {
    e, r
  };

  return gcc_jit_context_new_call(c->ctx, NULL, fn, 2, params);
}

static gcc_jit_rvalue* atom_to_int(q_compiler *c, gcc_jit_rvalue *in) {
  in = gcc_jit_rvalue_access_field(in, NULL, c->atom_fields[0]);
  return gcc_jit_context_new_binary_op(c->ctx, NULL, GCC_JIT_BINARY_OP_RSHIFT, c->int_type,in,
                                       gcc_jit_context_new_rvalue_from_int(c->ctx, c->int_type,TAG_BITS)
                                       );
}

static gcc_jit_rvalue* int_to_atom(q_compiler *c, gcc_jit_rvalue *in) {
  gcc_jit_rvalue* a = gcc_jit_context_new_binary_op(
      c->ctx, NULL, GCC_JIT_BINARY_OP_LSHIFT, c->int_type,in,
      gcc_jit_context_new_rvalue_from_int(c->ctx, c->int_type, TAG_BITS));

  gcc_jit_rvalue *b =  gcc_jit_context_new_binary_op(c->ctx, NULL, GCC_JIT_BINARY_OP_BITWISE_OR, c->int_type,
                                                     gcc_jit_context_new_rvalue_from_int(c->ctx, c->int_type, TAG_INTEGER), a);

  return gcc_jit_context_new_union_constructor(c->ctx, NULL, c->atom_type, c->atom_fields[0], b);
}

static gcc_jit_rvalue* compile_add(q_compiler *c, int num_values, gcc_jit_rvalue **values) {
  char buf[256];

  snprintf(buf, sizeof(buf), "_add_%d", c->lambda_counter++);
  gcc_jit_function *func = gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_ALWAYS_INLINE, c->atom_type, buf, 0, NULL, 0);


  gcc_jit_block* entry = gcc_jit_function_new_block(func, "entry");

  gcc_jit_lvalue* acc = gcc_jit_function_new_local(func, NULL, c->int_type, "acc");
  gcc_jit_block_add_assignment(entry, NULL, acc, gcc_jit_context_zero(c->ctx, c->int_type));

  for( int i = 0 ; i < num_values ; ++i) {
    gcc_jit_rvalue *v = atom_to_int(c, values[i]);
    gcc_jit_block_add_assignment_op(entry, NULL, acc, GCC_JIT_BINARY_OP_PLUS, v) ;
  }
  gcc_jit_rvalue* ret = int_to_atom(c, gcc_jit_lvalue_as_rvalue(acc));

  gcc_jit_block_end_with_return(entry, NULL, ret);

  return gcc_jit_context_new_call(c->ctx, NULL, func, 0, NULL); 
}

static gcc_jit_function *compile_car(q_compiler *c) {
  gcc_jit_param *param =
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "a");
  gcc_jit_function *fn =
      gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_ALWAYS_INLINE,
                                   c->atom_type, "car", 1, &param, 0);

  gcc_jit_block *b = gcc_jit_function_new_block(fn, "entry");

  gcc_jit_rvalue *as_cons = gcc_jit_rvalue_access_field(
      gcc_jit_param_as_rvalue(param), NULL, c->atom_fields[1]);

  as_cons = gcc_jit_context_new_cast(
      c->ctx, NULL, as_cons,
      gcc_jit_type_get_pointer(gcc_jit_struct_as_type(c->cons_type)));
  gcc_jit_rvalue *car = gcc_jit_lvalue_as_rvalue(
      gcc_jit_rvalue_dereference_field(as_cons, NULL, c->cons_params[0]));

  gcc_jit_block_end_with_return(b, NULL, car);

  return fn;
}

static gcc_jit_function *compile_cdr(q_compiler *c) {
  gcc_jit_param *param =
      gcc_jit_context_new_param(c->ctx, NULL, c->atom_type, "a");
  gcc_jit_function *fn =
      gcc_jit_context_new_function(c->ctx, NULL, GCC_JIT_FUNCTION_ALWAYS_INLINE,
                                   c->atom_type, "cdr", 1, &param, 0);

  gcc_jit_block *b = gcc_jit_function_new_block(fn, "entry");

  gcc_jit_rvalue *as_cons = gcc_jit_rvalue_access_field(
      gcc_jit_param_as_rvalue(param), NULL, c->atom_fields[1]);

  as_cons = gcc_jit_context_new_cast(
      c->ctx, NULL, as_cons,
      gcc_jit_type_get_pointer(gcc_jit_struct_as_type(c->cons_type)));
  gcc_jit_rvalue *cdr = gcc_jit_lvalue_as_rvalue(
      gcc_jit_rvalue_dereference_field(as_cons, NULL, c->cons_params[1]));

  gcc_jit_block_end_with_return(b, NULL, atom_from_rvalue_ptr(c, cdr));

  return fn;
}

static gcc_jit_rvalue *compile_atom(q_compiler *ctx, q_atom a) {
  q_dbg("compile_atom", a);
  switch (q_atom_type_of(a)) {
  case NUMBER:
    return atom_from_long(ctx, a.ival);
  case SYMBOL: {
    gcc_jit_rvalue *env = gcc_jit_param_as_rvalue(
        gcc_jit_function_get_param(ctx->current_func, 0));

    gcc_jit_rvalue *params[] = {
        env, gcc_jit_context_new_rvalue_from_int(ctx->ctx, ctx->int_type,
                                                 q_atom_symbol(a))};
    return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->env_get, 2, params);
  }
  case NIL:
    return atom_from_ptr(ctx, NULL);
  case STRING:
    return compile_string(ctx, a);
  case CONS: {
    q_cons *cons = (q_cons *)a.pval;

    if (q_equals(cons->car, SYM("quote"))) {
      return atom_from_long(ctx, cons->cdr->car.ival);
    } else if (q_equals(cons->car, SYM("cons"))) {
      gcc_jit_rvalue *params[] = {gcc_jit_context_new_rvalue_from_ptr(
                                      ctx->ctx, ctx->void_ptr, ctx->q_mem),
                                  compile_atom(ctx, cons->cdr->car),
                                  compile_atom(ctx, cons->cdr->cdr->car)};

      return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->cons, 3, params);
    } else if (q_equals(cons->car, SYM("car"))) {
      gcc_jit_rvalue *param = compile_atom(ctx, cons->cdr->car);
      return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->car, 1, &param);
    } else if (q_equals(cons->car, SYM("cdr"))) {
      gcc_jit_rvalue *param = compile_atom(ctx, cons->cdr->car);
      return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->cdr, 1, &param);
    } else if (q_equals(cons->car, SYM("lambda"))) {
      q_atom args = cons->cdr->car;
      q_atom body = {.pval = cons->cdr->cdr};

      return compile_lambda(ctx, args, body);
    } else if (q_equals(cons->car, SYM("define"))) {
      q_symbol name = q_atom_symbol(cons->cdr->car);
      gcc_jit_rvalue *value = compile_atom(ctx, cons->cdr->cdr->car);

      gcc_jit_rvalue *params[] = {
          gcc_jit_param_as_rvalue(
              gcc_jit_function_get_param(ctx->current_func, 0)),
          gcc_jit_context_new_rvalue_from_int(ctx->ctx, ctx->int_type, name),
          value};
      return gcc_jit_context_new_call(ctx->ctx, NULL, ctx->define, 3, params);
    } else if (q_equals(cons->car, SYM("throw"))) {
      gcc_jit_rvalue* arg = compile_atom(ctx, cons->cdr->car );

      return compile_throw(ctx, gcc_jit_param_as_rvalue(gcc_jit_function_get_param(ctx->current_func, 0)), arg);
    }
    else if(q_equals(cons->car, SYM("+"))) {
      int len = q_cons_length(cons->cdr);
      gcc_jit_rvalue** params = malloc(sizeof(gcc_jit_rvalue*)* len);
      cons = cons->cdr;
      for( int i = 0 ; i < len ; ++i, cons = cons->cdr) {
        assert(cons);
        params[i] =  compile_atom(ctx, cons->car);
      }
      return compile_add(ctx, len, params);
    }
    else {
      gcc_jit_rvalue* func = compile_atom(ctx, cons->car);

      int len = q_cons_length(cons->cdr) + 1;
      gcc_jit_rvalue **params = malloc(sizeof(gcc_jit_rvalue*) * len);
      params[0] = gcc_jit_param_as_rvalue(gcc_jit_function_get_param(ctx->current_func, 0));

      q_cons *arg = cons;
      for( int i = 1 ; i < len ; ++i, arg = arg->cdr) {
        assert(arg);
        params[i] = compile_atom(ctx, arg->car);
      }

      func = (gcc_jit_rvalue_access_field(func, NULL, ctx->atom_fields[1]));

      gcc_jit_rvalue* cast_to_lambda = gcc_jit_context_new_cast(ctx->ctx, NULL, func, gcc_jit_type_get_pointer(gcc_jit_struct_as_type(ctx->lambda_type)));

      gcc_jit_lvalue* fn_address = gcc_jit_rvalue_dereference_field( cast_to_lambda, NULL, gcc_jit_struct_get_field(ctx->lambda_type, 0));

      gcc_jit_type **t_params = malloc(sizeof(gcc_jit_param*) * len);
      t_params[0] = ctx->void_ptr;

      for(int i = 1 ; i < len ; ++i) {
        t_params[i] = ctx->atom_type;
      }

      gcc_jit_type* fn_type = gcc_jit_context_new_function_ptr_type(ctx->ctx, NULL, ctx->atom_type, len, t_params, 0);

      gcc_jit_rvalue* fn = gcc_jit_context_new_cast(ctx->ctx, NULL, gcc_jit_lvalue_as_rvalue(fn_address), fn_type);

      return gcc_jit_context_new_call_through_ptr(ctx->ctx, NULL, fn, len, params);
    }
  }
    return NULL;
  default:
    printf("idk how to deal with this: \n");
    q_atom_print(stdout, a);
    return NULL;
  }
}

q_compiler *q_compiler_create(q_memory *mem) {
  q_compiler *c = malloc(sizeof(q_compiler));
  memset(c, 0, sizeof(q_compiler));
  c->ctx = gcc_jit_context_acquire();
  gcc_jit_field *atom_types[] = {
      gcc_jit_context_new_field(
          c->ctx, NULL, gcc_jit_context_get_int_type(c->ctx, 8, 1), "ival"),
      gcc_jit_context_new_field(
          c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR),
          "pval")};
  c->atom_fields[0] = atom_types[0];
  c->atom_fields[1] = atom_types[1];

  c->int_type = gcc_jit_context_get_int_type(c->ctx, 8, 1);
  c->atom_type =
      gcc_jit_context_new_union_type(c->ctx, NULL, "q_atom", 2, atom_types);
  gcc_jit_context_set_bool_option(c->ctx,
                                  GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, 1);
  gcc_jit_context_set_str_option(c->ctx, GCC_JIT_STR_OPTION_PROGNAME, "quack");
  gcc_jit_context_set_int_option(c->ctx, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL,
                                 1);
  gcc_jit_type *char_array = gcc_jit_context_new_array_type(
      c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_CHAR), 0);

  gcc_jit_field *fields[2] = {
      gcc_jit_context_new_field(c->ctx, NULL, c->atom_type, "len"),
      gcc_jit_context_new_field(c->ctx, NULL, char_array, "buf")};

  c->string_type =
      gcc_jit_context_new_struct_type(c->ctx, NULL, "q_string", 2, fields);
  c->env_get = env_get(c);
  c->void_ptr = gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR);
  c->q_mem = mem;
  c->cons = compile_cons(c);

  c->cons_type = gcc_jit_context_new_opaque_struct(c->ctx, NULL, "q_cons");

  gcc_jit_field *cons_fields[] = {
      gcc_jit_context_new_field(c->ctx, NULL, c->atom_type, "car"),
      gcc_jit_context_new_field(
          c->ctx, NULL,
          gcc_jit_type_get_pointer(gcc_jit_struct_as_type(c->cons_type)),
          "cdr")};
  c->cons_params[0] = cons_fields[0];
  c->cons_params[1] = cons_fields[1];
  gcc_jit_struct_set_fields(c->cons_type, NULL, 2, cons_fields);

  c->car = compile_car(c);
  c->cdr = compile_cdr(c);
  c->define = define(c);

  gcc_jit_field *lambda_fields[] = {
    gcc_jit_context_new_field(c->ctx, NULL, c->void_ptr, "fn_address"),
    gcc_jit_context_new_field(c->ctx, NULL, c->void_ptr, "native_name"),
    gcc_jit_context_new_field(c->ctx, NULL, c->void_ptr, "args"),
    gcc_jit_context_new_field(c->ctx, NULL, c->void_ptr, "body")
  };

  c->lambda_type = gcc_jit_context_new_struct_type(c->ctx, NULL, "q_lambda", 4, lambda_fields);

  return c;
}

void q_compiler_destroy(q_compiler *c) {
  gcc_jit_context_release(c->ctx);
  destroy_lambdas(c);
  free(c);
}

static q_err populate_lambda_addresses(q_compiler *c, gcc_jit_result *result) {
  lambda_list_node *node = c->lambda_list;

  for(;node; node = node->next) {
    node->item->fn_address = gcc_jit_result_get_code(result, node->item->native_name);
    if(!node->item->fn_address) {
      return q_fail;
    }
  }
  return q_ok;
}

qmain_func q_compile(q_compiler *c, q_atom a) {
  char buf[256];
  snprintf(buf, sizeof(buf), "quackmain_%d", c->counter++);

  gcc_jit_param *param = gcc_jit_context_new_param(
      c->ctx, NULL, gcc_jit_context_get_type(c->ctx, GCC_JIT_TYPE_VOID_PTR),
      "env");
  gcc_jit_function *fn = gcc_jit_context_new_function(
      c->ctx, NULL, GCC_JIT_FUNCTION_EXPORTED, c->atom_type, buf, 1, &param, 0);
  assert(fn);
  c->current_func = fn;
  gcc_jit_block *block = gcc_jit_function_new_block(fn, "entry");
  c->current_block = block;

  gcc_jit_rvalue *ret = compile_atom(c, a);
  if (!ret) {
    fprintf(stderr, "error: %s\n", gcc_jit_context_get_last_error(c->ctx));
    return NULL;
  }

  gcc_jit_block_end_with_return(block, NULL, ret);

  gcc_jit_result *result = gcc_jit_context_compile(c->ctx);
  assert(result);
  if( Q_FAILED(populate_lambda_addresses(c, result))) {
    printf("failed to populate lambdas\n");
    return NULL;
  }

  return (qmain_func)gcc_jit_result_get_code(result, buf);
}
