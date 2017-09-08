#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <editline/readline.h>


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
  case NIL:
    return c;
  default:
    printf("error creating cell: unknown type\n");
    return NULL; /* error */
  }
}

Cell *new_nil() {
  return new(NIL, NULL, NULL, NULL, NULL);
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
Cell *parse_expr(char **input_ptr);
Cell *parse_tuple(char **input_ptr);
Cell *parse_atom(char **input_ptr);
Cell *parse(char *input) {
  return parse_expr(&input);
}

Cell *parse_expr(char **input_ptr) {
  for(; **input_ptr == ' '; (*input_ptr)++);
  if (**input_ptr == '\0') {
    printf("end of string?\n");
    return NULL;
  }
  if (**input_ptr == '(') {
    return parse_tuple(input_ptr);
  }
  return parse_atom(input_ptr);
}

Cell *parse_tuple(char **input_ptr) {
  if (**input_ptr != '(') {
    printf("tuples must being with (\n");
    return NULL;
  } else {
    (*input_ptr)++;
  }
  Cell *left = parse_expr(input_ptr);
  if (!left) { return NULL; }
  Cell *right = parse_expr(input_ptr);
  if (!right) {return NULL; }
  for(; **input_ptr == ' '; (*input_ptr)++);
  if (**input_ptr != ')') {
    printf("unmatched '('\n");
    return NULL;
  } else {
    (*input_ptr)++;
  }
  return new_tuple(left, right);
}

Cell *parse_atom(char **input_ptr) {
  int i;
  for(i = 0;
      ('a' <= (*input_ptr)[i] && (*input_ptr)[i] <= 'z') ||
	(*input_ptr)[i] == '#' ||
	(*input_ptr)[i] == '$';
      i++);
  if (i == 0) {
    printf("unexpected %c", (*input_ptr)[i]);
    return NULL;
  }
  char *name = strndup(*input_ptr, i);
  *input_ptr += i;
  Cell *c;
  if (strcmp(name, "$") == 0) {
    c = new_nil();
  } else {
    c = new_symbol(name);
  }
  free(name);
  return c;
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
