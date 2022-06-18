#include "memory.h"
#include "types.h"
#include <stdlib.h>
#include <string.h>

typedef struct _node {
  struct _node *next;
  q_heap_node   data;
} node;

struct q_memory {
  node *allocations;
};

static void clear_allocations(q_memory *m) {
  node *n = m->allocations;
  node *t;
  for (; n;) {
    t = n;
    n = n->next;
    free(t);
  }
  m->allocations = NULL;
}

q_memory* q_memory_create(void)
{
  q_memory *m = calloc(1, sizeof(q_memory));
  return m;
}

void q_memory_destroy(q_memory *m) {
  clear_allocations(m);
  free(m);
}

void q_memory_sweep(q_memory *m)
{
  node *prev = NULL;
  node *t;
  node *node = m->allocations;

  while(node) {
    if(!node->data.flags) {
      if (prev) {
        prev->next = node->next;
      } else {
        m->allocations = node->next;
      }
      t = node;
      node = node->next;

      free(t);
    } else {
      node->data.flags &= !USED;
      prev = node;
      node = node->next;
    }
  }
}

static q_heap_node* alloc(q_memory *m, q_atom_type type, size_t size)
{
  node *n = calloc(1, sizeof(node) + size);

  n->data.size = size;
  n->data.type = type;
  assert((((uintptr_t)&n->data.buff) & TAGMASK) == 0);

  n->next = m->allocations;
  m->allocations = n;

  return &n->data;
}

q_cons* q_memory_alloc_cons(q_memory *m, q_atom car, q_cons *cdr)
{
  q_heap_node* hn = alloc(m, CONS, sizeof(q_cons));
  q_cons *c = (q_cons*)hn->buff;
  c->car = car;
  c->cdr = cdr;

  return c;
}

q_string* q_memory_alloc_string(q_memory *m, char *b, size_t len)
{
  q_heap_node* hn = alloc(m, STRING, sizeof(q_string) + len +1);

  q_string *s = (q_string*)hn->buff;
  memcpy(s->data, b, len);
  s->data[len] = 0;
  s->len = len;

  return s;
}

q_lambda *q_memory_alloc_lambda(q_memory *m, q_cons *args, q_cons *body)
{
  q_heap_node* hn = alloc(m, LAMBDA, sizeof(q_lambda) );

  q_lambda* l = (q_lambda*)hn->buff;

  memset(l, 0, sizeof(q_lambda));

  l->args = args;
  l->body = body;

  return l;
}
q_bool q_memory_is_heap_object(q_atom a)
{
  switch(q_atom_type_of(a)) {
  case STRING:
  case CONS:
  case LAMBDA:
  case ENV:
    return true;
  default:
    return false;
  }
}
void q_memory_mark(q_atom a)
{
  if(q_memory_is_heap_object(a)) {
    q_heap_node *n = ((q_heap_node*)(a.pval - offsetof(q_heap_node, buff)));
    n->flags |= USED;
  }
}
