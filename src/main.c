#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"

int main(void) {
  q_engine* engine = q_engine_create();

  for (;;) {
    char *line = readline(">");

    if (!line)
      break;

    q_atom atom;
    q_err err = q_engine_eval_string(engine, line, &atom);
    free(line);
    if (err) {
      printf("ERROR: %d\n", err);
      continue;
    }

    q_atom_print(stdout, atom);
    printf("\n");
    fflush(stdout);
  }
  q_engine_destroy(engine);

  return 0;
}
