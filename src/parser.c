#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

#include "parser.h"
#include "vector.h"
#include "symbol.h"
#include <string.h>

typedef struct {
  size_t pos;
  size_t len;
  char *data;
} buffer;

static int peek(buffer *b)  {
  if( b->pos >= b->len ) {
    return EOF;
  }
  return b->data[b->pos];
}

static int next(buffer *b) {
  if( b->pos >= b->len) {
    return EOF;
  }

  return b->data[b->pos++];
}

static void read_to_newline(buffer *b)
{
  for(;;) {
    int c  = peek(b);
    if( c == EOF )
      return;
    next(b);
    if (c == '\n') {
      return;
    }
  }
}

static q_err read_file(const char *filename, buffer *b) {
  struct stat st;
  if(stat(filename, &st)) {
    return 1;
  }

  char *buff = malloc(sizeof(char)* st.st_size);

  FILE *file = fopen(filename, "r");
  if (!file) {
    return 1;
  }

  if(fread(buff, sizeof(char), st.st_size, file) != (unsigned)st.st_size) {
    fclose(file);
    free(buff);
    return 1;
  }

  fclose(file);

  b->data = buff;
  b->len = st.st_size;
  b->pos = 0;

  return q_ok;
}

static q_err quote_atom(q_memory *mem, q_atom a, q_atom *ret)
{
  q_cons* cons = q_memory_alloc_cons(mem, a, NULL);
  cons = q_memory_alloc_cons(mem, q_symbol_create("quote"), cons);

  *ret = make_cons(cons);

  return q_ok;
}

static q_err parse_atom(q_memory *mem, buffer *b, q_atom *ret);

static q_err parse_list(q_memory *mem, buffer *b, q_atom *ret)
{
  q_vec *list = q_vec_create(16);

  for(;;) {
    int c = peek(b);
    if( c == ')') {
      next(b);
      break;
    } else if(c == EOF) {
      q_vec_destroy(list);
      return EOF;
    }
    else {
      if( isspace(c)) {
        next(b);
      } else {
        q_atom a;
        if( parse_atom(mem, b, &a)) {
          q_vec_destroy(list);
          return 1;
        }
        list = q_vec_append(list, a);
      }
    }
  }

  q_cons *head = NULL;
  for(int i = list->len - 1 ; i >= 0 ; --i) {
    head = q_memory_alloc_cons(mem, list->values[i], head);
  }
  q_vec_destroy(list);

  *ret = make_cons(head);

  return q_ok;
}

typedef struct {
  char *data;
  int size;
  int len;
} char_vector;

void char_add(char_vector *v, char c)
{
  if(!v->data) {
    v->data = malloc(64);
    v->len = 0;
    v->size = 64;
  }

  if( v->len >= v->size ) {
    v->data = realloc(v->data, v->size*2);
    v->size *= 2;
  }

  v->data[v->len++] = c;
}

static q_err read_string(q_memory *mem, buffer *b, q_atom *ret)
{
  char_vector chars = {0};

  for (;; ) {
    int c = peek(b);
    switch(c) {
    case '\\': {
      next(b);
      int c = peek(b);
      switch(c) {
      case EOF:
        free(chars.data);
        return 1;
      default:
        char_add(&chars, c);
      }
    }
      break;
    case '"':
      next(b);
      *ret = make_string(q_memory_alloc_string(mem, chars.data, chars.len));
      free(chars.data);
      return q_ok;
    default:
      char_add(&chars, c);
    }
  }
}

static q_err parse_integer(buffer *b, int64_t *ret)
{
  char *end;

  errno = 0;
  long long i = strtoll(b->data + b->pos, &end, 0);
  if(errno) {
    return q_fail;
  }
  if( b->data[b->pos] && end != b->data + b->pos) {
    *ret = i;

    return q_ok;
  }
  return q_fail;
}

static q_err next_token(const buffer *b, size_t *ret)
{
  size_t len = 0;
  for(; len < b->len ;) {
    int c = b->data[b->pos + len];

    if( c == EOF) {
      if(len > 0) {
        *ret = len;
        return q_ok;
      }
      return q_fail;
    } else if( !isspace(c) && c != ')') {
      len++;
    } else {
      *ret = len;
      return q_ok;
    }
  }
  *ret = len;
  return q_ok;
}

static q_bool string_equals(const char *s, char *buff, size_t len)
{
  for(unsigned i = 0 ; i < len ; ++i) {
    if(!s[i]) {
      return true;
    }
    if(s[i] != buff[i]) {
      return false;
    }
  }
  return false;
}

static q_err read_atom(q_memory *mem, buffer *b, q_atom *ret)
{
  if( peek(b) == '"') {
    next(b);
    return read_string(mem, b, ret);
  }

  size_t next_end;
  if( next_token(b, &next_end)) {
    return q_fail;
  }
  if( !next_end ) {
    return q_fail;
  }

  if(string_equals("nil", b->data + b->pos, next_end)) {
    return make_nil();
  } else if(string_equals("#t", b->data + b->pos, next_end)) {
    return make_boolean(true);
  } else if(string_equals("#f", b->data + b->pos, next_end)) {
    return make_boolean(false);
  }

  int64_t i;
  if(!parse_integer(b, &i)) {
    *ret = make_integer(i);
  } else {
    *ret = make_symbol(q_symbol_create_buffer(b->data + b->pos, next_end));
  }

  b->pos += next_end;

  return q_ok;
}

static q_err parse_atom(q_memory *mem, buffer *b, q_atom *ret)
{
  for(;;) {
    int c = peek(b);
    switch(c) {
    case EOF:
      return 1;
    case '\'': {
      next(b);
      q_atom a;

      if(parse_atom(mem, b, &a)) {
        return 1;
      }
      if(quote_atom(mem, a, ret) ) {
        return 1;
      }
    }
      break;
    case '(':
      next(b);
      return parse_list(mem, b, ret);
    case ';':
      next(b);
      read_to_newline(b);
      break;
    case ',': {
      next(b);
      if( peek(b) == '@') {
        next(b);
        q_atom a;
        if( parse_atom(mem, b, &a) ) {
          return 1;
        }
        q_cons * head = NULL;
        head = q_memory_alloc_cons(mem, a, head);
        head = q_memory_alloc_cons(mem, q_symbol_create("splice"), head);
        *ret = make_cons(head);
        return q_ok;
      } else {
        q_atom a;
        if(parse_atom(mem, b, &a)) {
          return 1;
        }
        q_cons *head = q_memory_alloc_cons(mem, a, NULL);
        head = q_memory_alloc_cons(mem, q_symbol_create("unquote"), head);
        *ret = make_cons(head);
        return q_ok;
      }
    }
      break;
    default:
      if(isspace(c)) {
        next(b);
      } else {
        return read_atom(mem, b, ret);
      }
    }
  }
}

q_err q_parse_buffer(q_memory* mem, const char* data, size_t len, q_atom *ret) {
  buffer b = { .data = (char *)data, .len = len, .pos = 0};

  return parse_atom(mem, &b, ret);
}


q_err q_parse_file(q_memory *mem, const char *filename, q_atom *ret)
{
  assert(ret);
  buffer b = {0};

  if(read_file(filename, &b) ) {
    return 1;
  }

  if( parse_atom(mem, &b, ret)) {
    free(b.data);
    return 1;
  }

  free(b.data);

  return q_ok;
}
