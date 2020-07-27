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
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

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

main_func compile(q_atom a)
{
  dasm_State *d;
  unsigned npc = 8;
  unsigned nextpc = 0;
  //|.arch x64
#if DASM_VERSION != 10300
#error "Version mismatch between DynASM and included encoding engine"
#endif
#line 36 "../src/compiler.dynasm"
  //|.section code
#define DASM_SECTION_CODE	0
#define DASM_MAXSECTION		1
#line 37 "../src/compiler.dynasm"
  dasm_init(&d, DASM_MAXSECTION);
  //|.globals lbl_
enum {
  lbl_q_main,
  lbl__MAX
};
#line 39 "../src/compiler.dynasm"
  void* labels[lbl__MAX];
  dasm_setupglobal(&d, labels, lbl__MAX);
  //|.actionlist q_actions
static const unsigned char q_actions[10] = {
  254,0,248,10,72,184,237,237,195,255
};

#line 42 "../src/compiler.dynasm"
  dasm_setup(&d, q_actions);
  dasm_growpc(&d, npc);
  dasm_State** Dst = &d;
  //|.code
  dasm_put(Dst, 0);
#line 46 "../src/compiler.dynasm"
  //|->q_main:
  //| mov64 rax, 42L
  //| ret
  dasm_put(Dst, 2, (unsigned int)(42L), (unsigned int)((42L)>>32));
#line 49 "../src/compiler.dynasm"
  link_and_encode(&d);
  dasm_free(&d);
  return (main_func)labels[lbl_q_main];
}
