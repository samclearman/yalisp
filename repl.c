#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <editline/readline.h>
#include "mpc.h"


/*
 * cell definition and utility functions
 */

typedef char *symbol;

typedef struct NativeValue {
  int native_type;
  long long_val;
} NativeValue;

enum { LONG, FUNCTION };

typedef struct Cell {
  int type;
  symbol sym;
  struct Cell *left;
  struct Cell *right;
  NativeValue *val;
} Cell;

enum { SYMBOL, TUPLE, NIL, NATIVE};

struct Cell NIL_CELL = {.type = NIL};

void print_cell(Cell *c) {
  switch (c->type) {
  case SYMBOL:
    printf("%s", c->sym);
    break;
  case TUPLE:
    printf("(");
    print_cell(c->left);
    printf(" ");
    print_cell(c->right);
    printf(")");
    break;
  case NIL:
    printf("$");
    break;
  case NATIVE:
    printf("<native code at %p>", (void *) c->val->long_val);
    break;
  } 
}

void println_cell(Cell* c) {
  print_cell(c);
  printf("\n");
}

void free_cell(Cell *c) {
  switch (c->type) {
  case SYMBOL:
    free(c->sym);
    break;
  case TUPLE:
    free_cell(c->left);
    free_cell(c->right);
    break;
  }
  free(c);
}

Cell *new(int type, symbol sym, Cell *left, Cell *right, NativeValue *val) {
  Cell *c = malloc(sizeof(Cell));
  c->type = type;
  switch (type) {
  case SYMBOL:
    if (!sym) {
      printf("error creating sym\n");
      return NULL; /* error */
    }
    c->sym = malloc(strlen(sym) + 1);
    strcpy(c->sym, sym);
    return c;
  case TUPLE:
    if (!left || !right) {
      printf("error creating tuple\n");
      return NULL; /* error */
    }
    c->left = left;
    c->right = right;
    return c;
  case NATIVE:
    if (!val) {
      printf("error creating native val");
      return NULL;
    }
    c->val = val;
    return c;
  default:
    printf("error creating cell: unknown type\n");
    return NULL; /* error */
  }
}

Cell *new_symbol(symbol sym) {
  return new(SYMBOL, sym, NULL, NULL, NULL);
}

Cell *new_tuple(Cell *left, Cell *right) {
  return new(TUPLE, NULL, left, right, NULL);
}

Cell *new_native_fn(long x) {
  NativeValue *v = malloc(sizeof(NativeValue));
  v->native_type = FUNCTION;
  v->long_val = x;
  return new(NATIVE, NULL, NULL, NULL, v);
}

Cell *copy_cell(Cell *c) {
  Cell *x = malloc(sizeof(Cell));
  x->type = c->type;
  switch (c->type) {
  case SYMBOL:
    x->sym = malloc(strlen(c->sym) + 1);
    strcpy(x->sym, c->sym);
    break;
  case TUPLE:
    x->left = copy_cell(c->left);
    x->right = copy_cell(c->right);
    break;
  }
  return x;
}

/*
 * env def and fns
 */

typedef struct Scope {
  char *name;
  Cell *value;
  struct Scope *parent;
  
} Scope;

Scope *new_scope() {
  Scope *s = malloc(sizeof(Scope));
  s->name = malloc(sizeof(char) * 2);
  strcpy(s->name, "$");
  s->value = &NIL_CELL;
  s->parent = NULL;
  return s;
}

Cell *lookup(Scope *scope, char *name) {
  if (scope == NULL) {
    return NULL;
 } 

  if (strcmp(scope->name, name) == 0) {
    return copy_cell(scope->value);
  }
  
  return lookup(scope->parent, name);
}

Scope *add(Scope *scope, symbol name, Cell *value) {

  Scope *s = malloc(sizeof(Scope));
  
  s->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(s->name, name);
  s->value = value;
  s->parent = scope;

  return s;
}

/*
 * functions!
 */

const int NUM_BUILTINS = 6;
const char* BUILTINS[NUM_BUILTINS] = {"atom", "eq", "first", "rest", "if", "pair"};

Cell *atom(Cell *c) {
  if (c->type == SYMBOL) {
    return new_symbol("True");
  }
  return new_symbol("False");
}

Cell *eq(Cell *c) {
  if (c->type != TUPLE) {
    /* error */
    return NULL;
  }
  Cell *x = c->left;
  Cell *y = c->right;
  if (x->type == SYMBOL &&
      y->type == SYMBOL) {
    if(strcmp(x->sym, y->sym) == 0) {
      return new_symbol("true");
    } else {
      return new_symbol("false");
    }
  }
  return NULL;
}

Cell *first(Cell *x) {
  if (x->type == TUPLE) {
    return x->left;
  }
  return NULL;
}

Cell *rest(Cell *x) {
  if (x->type == TUPLE) {
    return x->right;
  }
  return NULL;
}

Cell *ifthenelse(Cell *x) {
  if (x->type == TUPLE &&
      x->left &&
      x->left->type == SYMBOL) {
    if (strcmp(x->left->sym, "true") == 0) {
      return(x->right->left);
    } else if (strcmp(x->left->sym, "false") == 0) {
      return(x->right->right);
    }
  }
  printf("condition isn't a boolean\n");
  return NULL;
}

Cell *pair(Cell *left, Cell *right) {
  return new_tuple(left, right);
}

bool is_builtin_name(Cell *c) {
  if (c->type == SYMBOL) {
    for (int i = 0; i < NUM_BUILTINS; i++) {
      if (strcmp(c->sym, BUILTINS[i]) == 0) {
	return true;
      }
    }
  }
  return false;
}

bool is_function(Cell *c) {
  // if (is_builtin_name(c)) return true;
  if (c->type == NATIVE &&
      c->val->native_type == FUNCTION) {
    return true;
  }
  if (c->type == TUPLE &&
      c->left->type == SYMBOL &&
      strcmp(c->left->sym, "#") == 0) {
    return true;
  }
  return false;
}

Cell *builtin_for(Cell *c) {
  if (strcmp(c->sym, "atom") == 0) {
    return new_native_fn((long) &atom);
  } else if (strcmp(c->sym, "eq") == 0) {
    return new_native_fn((long) &eq);
  } else if (strcmp(c->sym, "first") == 0) {
    return new_native_fn((long) &first);
  } else if (strcmp(c->sym, "rest") == 0) {
    return new_native_fn((long) &rest);
  } else if (strcmp(c->sym, "if") == 0) {
    return new_native_fn((long) &ifthenelse);
  }
  /* Error */
  printf("failed to get builtin for: "); println_cell(c);
  return NULL;
}
      
bool is_null(Cell *c) {
  if (c->type == NIL) {
    return true;
  }
  return false;
}

Scope *add_to_scope(Scope *scope, Cell *param_names, Cell *args) {
  if (is_null(param_names) != is_null(args)) {
    /* return error */
  }
  if (is_null(param_names) && is_null(args)) {
    return scope;
  }

  if (param_names->type != TUPLE ||
      param_names->left->type != SYMBOL ||
      args->type != TUPLE) {
    /* return error */
  }

  scope = add(scope, param_names->left->sym, args->left);
  return add_to_scope(scope, param_names->right, args->right);
}

Cell *eval(Cell *c, Scope *scope);
Cell *apply_function(Cell *fn, Cell *args, Scope *scope) {
  printf("applying "); println_cell(fn);
  if (fn->type == NATIVE &&
      fn->val->native_type == FUNCTION) {
    Cell *((*f)(Cell *)) = (Cell *((*)(Cell *)))fn->val->long_val;
    return (*f)(args);
  } 
  if (!fn->right || fn->right->type != TUPLE) {
    printf("failed to apply function "); println_cell(fn);
    return NULL;
  }
  Cell *body = fn->right->right;
  Cell *param_names = fn->right->left;
  scope = add_to_scope(scope, param_names, args);
  Cell *result = eval(body, scope);
  printf("result: ");println_cell(result);
  return result;
}

  
/*
 * cell evaluation
 */

Cell *eval(Cell *c, Scope *scope) {
  printf("evaluating: ");println_cell(c);
  if (is_builtin_name(c)) {
    printf("builtin\n");
    return builtin_for(c);
  }	  
    
  if (c->type == TUPLE) {
    Cell *left = eval(c->left,scope);
    printf("left: ");println_cell(left);
    if (is_function(left)) {
      return apply_function(left, eval(c->right, scope), scope);
      
    }
    
    Cell *r = malloc(sizeof(Cell));
    r->type = TUPLE;
    r->left = left;
    r->right = eval(c->right, scope);
    printf("result: ");println_cell(r);
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
 * ast creation
 */

Cell *read(mpc_ast_t* t) {

  Cell *x = malloc(sizeof(Cell));
  
  if (strstr(t->tag, "symbol")) {
    x->type = SYMBOL;
    x->sym = malloc(strlen(t->contents) + 1);
    strcpy(x->sym, t->contents);
    return x;
  }
  
  if (strstr(t->tag, "nil")) {
    x->type = NIL;
    return x;
  }

  x->type = TUPLE;
  x->left = read(t->children[1]);
  x->right = read(t->children[2]);
  return x;

}

/*
 * run
 */

int main(int argc, char** argv) {
  
  /* Initialize */

  mpc_parser_t *Nil      = mpc_new("nil");
  mpc_parser_t *Symbol   = mpc_new("symbol");
  mpc_parser_t *Tuple    = mpc_new("tuple");
  mpc_parser_t *Expr     = mpc_new("expr");
  mpc_parser_t *Yalisp   = mpc_new("yalisp");


  mpca_lang(MPCA_LANG_DEFAULT,
    "  \
      nil       : '$' ; \
      symbol    : /[#a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
      tuple     : '(' <expr> <expr> ')' ; \
      expr      : <tuple> | <symbol> | <nil> ; \
      yalisp    : /^/ <expr> /$/ ; \
    ",
    Nil, Symbol, Tuple, Expr, Yalisp);


  /* Start the REPL */
  
  puts("yalisp 0.001");

  while(1) {
    char *input = readline("yalisp> ");
    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Yalisp, &r)) {
      Scope *scope = new_scope();
      mpc_ast_t *t = r.output;
      Cell *c = read(t->children[1]);
      Cell *result = eval(c, scope);
      if(!c || !result) {
	printf("error\n");
	continue;
      }
      printf("input: "); println_cell(c);
      printf("result: "); println_cell(result);
      free_cell(c);
      free_cell(result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Nil, Symbol, Tuple, Expr, Yalisp);
  return 0;
}
