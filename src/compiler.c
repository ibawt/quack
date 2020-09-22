/*
** This file has been pre-processed with DynASM.
** http://luajit.org/dynasm.html
** DynASM version 1.3.0, DynASM x64 version 1.3.0
** DO NOT EDIT! The original file is in "../src/compiler.dynasm".
*/

#line 1 "../src/compiler.dynasm"
#define DASM_CHECKS 1
#include "dasm_proto.h"
#include "dasm_x86.h"
#include "types.h"
#include "compiler.h"
#include <sys/mman.h> 
#include "symbol.h" 
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

//|.arch x64
#if DASM_VERSION != 10300
#error "Version mismatch between DynASM and included encoding engine"
#endif
#line 13 "../src/compiler.dynasm"

static void* link_and_encode(dasm_State** d)
{
  size_t sz;
  void* buf;
  dasm_link(d, &sz);
#ifdef _WIN32
  buf = VirtualAlloc(0, sz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  buf = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
  dasm_encode(d, buf);
#ifdef _WIN32
  {DWORD dwOld; VirtualProtect(buf, sz, PAGE_EXECUTE_READ, &dwOld); }
#else
  mprotect(buf, sz, PROT_READ | PROT_EXEC);
#endif
  return buf;
}

static void test(dasm_State **Dst)
{
//|mov64 rax, make_integer(42)
//|ret
dasm_put(Dst, 0, (unsigned int)(make_integer(42)), (unsigned int)((make_integer(42))>>32));
#line 37 "../src/compiler.dynasm"
}

static q_err compile_atom(dasm_State **Dst, q_atom a)
{
  //|.define envPtr, rdi
  //|.section code, imports
#define DASM_SECTION_CODE	0
#define DASM_SECTION_IMPORTS	1
#define DASM_MAXSECTION		2
#line 43 "../src/compiler.dynasm"
  //|.macro call_extern, target
  //|  .imports
  //|  ->__imp__..target:
  //|  .dword (uint32_t)target
  //|  .dword ((uint64_t)target)>>32
  //|  .code
  //|  call qword [->__imp__..target]
  //|.endmacro
  //|.macro return, value
  //|.code
  //|move64 rax, value
  //|ret
  //|.endmacro
  switch(q_atom_type_of(a)) {
  case CONS: {
    q_cons* c = (q_cons*)a;
    switch(q_atom_type_of(c->car)) {
    case SYMBOL: {
       const char* s = q_symbol_string(c->car);
       if(strcmp("if", s) == 0 ) {
       }
       } break;
    }
  }break;
  case SYMBOL:
     //| lea rdx, [rsp]
     //| mov64 rsi, a>>TAG_BITS
     //| call_extern q_env_lookup
     dasm_put(Dst, 6, (unsigned int)(a>>TAG_BITS), (unsigned int)((a>>TAG_BITS)>>32));
     dasm_put(Dst, 16, (uint32_t)q_env_lookup, ((uint64_t)q_env_lookup)>>32);
#line 71 "../src/compiler.dynasm"
     //| lea rax, [rsp]
     dasm_put(Dst, 22);
#line 72 "../src/compiler.dynasm"
     break;
  default:
  //| mov64 rax, a
  dasm_put(Dst, 32, (unsigned int)(a), (unsigned int)((a)>>32));
#line 75 "../src/compiler.dynasm"
  }

  return q_ok;
}

main_func compile(q_atom a)
{
  dasm_State *d;
  unsigned npc = 8;
  unsigned nextpc = 0;
  dasm_init(&d, DASM_MAXSECTION);
  //|.globals lbl_
enum {
  lbl___imp__q_env_lookup,
  lbl_q_main,
  lbl__MAX
};
#line 87 "../src/compiler.dynasm"
  void* labels[lbl__MAX];
  dasm_setupglobal(&d, labels, lbl__MAX);
  //|.actionlist q_actions
static const unsigned char q_actions[40] = {
  72,184,237,237,195,255,72,141,20,36,72,190,237,237,254,1,248,10,237,237,254,
  0,252,255,21,244,10,72,141,4,36,255,72,184,237,237,255,248,11,255
};

#line 90 "../src/compiler.dynasm"
  dasm_setup(&d, q_actions);
  dasm_growpc(&d, npc);
  dasm_State** Dst = &d;
  //|.code
  dasm_put(Dst, 20);
#line 94 "../src/compiler.dynasm"
  //|->q_main:
  dasm_put(Dst, 37);
#line 95 "../src/compiler.dynasm"
  compile_atom(Dst, a);
  //|ret
  dasm_put(Dst, 4);
#line 97 "../src/compiler.dynasm"
  link_and_encode(&d);
  dasm_free(&d);
  return (main_func)labels[lbl_q_main];
}
