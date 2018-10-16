#include <stdio.h>

#include <editline/readline.h>

#include "yalisp.c"
#include "parse.c"

/*
 * cell evaluation
 */

Cell *eval(Cell *c, Scope *scope) {
  if (is_builtin_name(c)) {
    return builtin_for(c);
  }
    
  if (c->type == TUPLE) {
    Cell *head = eval(first(c),scope);
    if (is_function(head)) {
      return apply_function(head, eval(rest(c), scope), scope);
    }
    Cell *r = malloc(sizeof(Cell));
    r->type = TUPLE;
    r->len = c->len;
    r->children = malloc(sizeof(Cell) * r->len);
    r->children[0] = head;
    for (int i = 1; i < r->len; i++) {
      r->children[i] = eval(c->children[i], scope);
    }
    return r;
  }
  
  if (c->type == SYMBOL) {
    if (lookup(scope, c->sym) != NULL) {
      return lookup(scope, c->sym);
    }
    return copy_cell(c);
  }
  return copy_cell(c);
}



/*
 * run
 */

int main(int argc, char** argv) {
  
  /* Start the REPL */
  
  puts("yalisp 0.001");

  while(1) {
    char *input = readline("yalisp> ");
    add_history(input);

    Cell *c = parse(input);

    if (c) {
      Scope *scope = new_scope();
      Cell *result = eval(c, scope);
      if(!c || !result) {
	printf("error\n");
	continue;
      }
      printf("input: "); println_cell(c);
      printf("result: "); println_cell(result);
      free_cell(c);
      free_cell(result);
    } else {
      printf("parse error\n");
    }

    free(input);
  }
  
  return 0;
}
