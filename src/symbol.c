#include "symbol.h"
#include "uthash.h"

#define DEFAULT_TABLE_SIZE 2048

static const char** table = NULL;
static size_t table_size = 0;
static size_t table_len  = 0;

typedef struct {
    const char *key;
    int index;
    UT_hash_handle hh;
} node;

static node *intern_map = NULL;

static q_symbol intern(const char *s)
{
    node *n;

    HASH_FIND_STR(intern_map, s, n);

    if(!n) {
        n = malloc(sizeof(node));
        n->key = strdup(s);
        n->index = table_len;
        HASH_ADD_KEYPTR(hh, intern_map, n->key, strlen(n->key), n);

        if(!table) {
          table = malloc(sizeof(char*) * DEFAULT_TABLE_SIZE);
          table_size = DEFAULT_TABLE_SIZE;
        }

        if(table_len >= table_size) {
          table = realloc(table, sizeof(char*)*table_size*2);
          table_size *= 2;
        }
        table[table_len] = n->key;
        return table_len++;
    }
    return n->index;
}

q_symbol q_symbol_create(const char *s) {
    return intern(s);
}

q_symbol q_symbol_create_buffer(char *b, size_t len)
{
    char *s = (char *)malloc(len+1);
    memcpy(s, b, len);
    s[len] = 0;

    q_symbol sym =  intern(s);
    free(s);
    return sym;
}

const char* q_symbol_string(q_symbol s) {
  return table[s];
}

