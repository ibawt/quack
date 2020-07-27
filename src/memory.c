#include "memory.h"
#include <stdlib.h>
#include <string.h>

// TODO: fold this into heap_node directly
typedef struct _node {
  struct _node *next;
  q_heap_node *data;
} node;

struct q_memory {
  node *allocations;
};

q_memory* q_memory_create(void)
{
  q_memory *m = malloc(sizeof(q_memory));
  memset(m, 0, sizeof(q_memory));

  return m;
}

static void push_allocation(q_memory *m, q_heap_node *hn)
{
  node *n = malloc(sizeof(node));
  memset(n, 0, sizeof(node));

  n->data = hn;
  n->next = m->allocations;

  m->allocations = n;
}

static void clear_allocations(q_memory *m)
{
  node *n = m->allocations;
  node *t;
  for(; n ; ) {
    free(n->data);
    t = n;
    n = n->next;
    free(t);
  }
  m->allocations = NULL;
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
    if(!node->data->flags) {
      if (prev) {
        prev->next = node->next;
      } else {
        m->allocations = node->next;
      }
      t = node;
      node = node->next;

      free(t->data);
      free(t);
    } else {
      node->data->flags &= !USED;
      prev = node;
      node = node->next;
    }
  }
}

static q_heap_node* alloc(q_memory *m, q_atom_type type, size_t size)
{
  q_heap_node *hn = malloc(size);
  memset(hn, 0, size);

  hn->size = size;
  hn->type = type;
  assert(((uintptr_t)hn->buff & TAGMASK) == 0);

  push_allocation(m, hn);

  return hn;
}

q_cons* q_memory_alloc_cons(q_memory *m, q_atom car, q_cons *cdr)
{
  q_heap_node* hn = alloc(m, CONS, sizeof(q_heap_node) + sizeof(q_cons));
  q_cons *c = (q_cons*)hn->buff;
  c->car = car;
  c->cdr = cdr;

  return c;
}

q_string* q_memory_alloc_string(q_memory *m, char *b, size_t len)
{
  q_heap_node* hn = alloc(m, STRING, sizeof(q_string) + sizeof(q_heap_node) + len +1);

  q_string *s = (q_string*)hn->buff;
  memcpy(s->data, b, len);
  s->data[len] = 0;
  s->len = len;

  return s;
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
    q_heap_node *n = ((q_heap_node*)(a - offsetof(q_heap_node, buff)));
    n->flags |= USED;
  }
}
