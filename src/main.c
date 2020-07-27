#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "parser.h"
#include "compiler.h"
#include "env.h"

int main(void) {
  q_memory *memory = q_memory_create();
  q_env *env = q_env_create(NULL);

  for(;;) {
    char *line = readline(">");

    if(!line)
      break;

    q_atom atom;
    q_err err = q_parse_buffer(memory, line, strlen(line), &atom);
    if( err ) {
      printf("ERROR: %d\n", err);
      continue;
    }

    q_atom_print(stdout, atom);
    printf("\n");

    main_func f = compile(atom);

    int i = f();

    printf("i = %d\n", i);
  }
  q_memory_destroy(memory);
  q_env_destroy(env);

  return 0;
}
