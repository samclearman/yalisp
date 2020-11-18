#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "numbers.c"

/*
 * cell definition and utility functions
 */

typedef char *symbol;

typedef struct NativeValue {
  int native_type;
  long long_val;
  Integer integer_val;
} NativeValue;

enum { LONG, FUNCTION, INTEGER };
  
typedef struct Cell {
  int type;
  // Symbol
  symbol sym;
  // Tuple
  int len;
  struct Cell **children;
  // Value
  NativeValue *val;
} Cell;

enum { SYMBOL, TUPLE, NIL, NATIVE};

struct Cell NIL_CELL = {.type = NIL};

void print_native_cell(NativeValue *);
void print_cell(Cell *c) {
  switch (c->type) {
  case SYMBOL:
    printf("%s", c->sym);
    break;
  case TUPLE:
    printf("(");
    print_cell(c->children[0]);
    for (int i = 1; i < c->len; i++) {
      printf(" ");
      print_cell(c->children[i]);
    }
    printf(")");
    break;
  case NIL:
    printf("$");
    break;
  case NATIVE:
    print_native_cell(c->val);
    break;
  } 
}

void print_native_cell(NativeValue *v) {
  switch (v->native_type) {
  case FUNCTION:
    printf("<native code at %p>", (void *) v->long_val);
    break;
  case LONG:
    printf("<native long: %ld>", v->long_val);
    break;
  case INTEGER:
    for (int i = 0; i < v->integer_val.num_digits; i++) {
      printf("%lu", v->integer_val.digits[i]);
    }
    break;
  }
} 
             
void println_cell(Cell* c) {
  print_cell(c);
  printf("\n");
}

void free_native(NativeValue *v) {
  switch(v->native_type) {
  case INTEGER:
    destroy_integer(v->integer_val);
    break;
  }
  free(v);
}

void free_cell(Cell *c) {
  switch (c->type) {
  case SYMBOL:
    free(c->sym);
    break;
  case TUPLE:
    for (int i = 0; i < c->len; i++) {
      free_cell(c->children[i]);
    }
    break;
  case NATIVE:
    free_native(c->val);
  }
  free(c);
}

Cell *new(int type, symbol sym, int len, Cell **children, NativeValue *val) {
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
    if (len < 0) {
      printf("error creating tuple\n");
      return NULL; /* error */
    }
    printf("creating tuple with length %d\n", len);
    c->len = len;
    c->children = malloc(sizeof(Cell *) * len);
    memcpy(c->children, children, sizeof(Cell *) * len);
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
  return new(NIL, NULL, 0, NULL, NULL);
}

Cell *new_symbol(symbol sym) {
  return new(SYMBOL, sym, 0, NULL, NULL);
}

Cell *new_tuple(int len, Cell **children) {
  return new(TUPLE, NULL, len, children, NULL);
}

Cell *new_native_fn(long x) {
  NativeValue *v = malloc(sizeof(NativeValue));
  v->native_type = FUNCTION;
  v->long_val = x;
  return new(NATIVE, NULL, 0, NULL, v);
}

Cell *new_native_int(Integer x) {
  NativeValue *v = malloc(sizeof(NativeValue));
  v->native_type = INTEGER;
  v->integer_val = x;
  return new(NATIVE, NULL, 0, NULL, v);
}

NativeValue *copy_native(NativeValue *v) {
  NativeValue *u = malloc(sizeof(NativeValue));
  u->native_type = v->native_type;
  u->long_val = v->long_val;
  switch (v->native_type) {
  case INTEGER:
    u->integer_val = copy_integer(v->integer_val);
    break;
  }
  return u;
}



Cell *copy_cell(Cell *c) {
  if (!c) {
    printf("tried to copy null cell\n");
    return NULL;
  }
  Cell *x = malloc(sizeof(Cell));
  x->type = c->type;
  switch (c->type) {
  case SYMBOL:
    x->sym = malloc(strlen(c->sym) + 1);
    strcpy(x->sym, c->sym);
    break;
  case TUPLE:
    x->len = c->len;
    x->children = malloc(sizeof(Cell) * c->len);
    for (int i = 0; i < c->len; i++) {
      x->children[i] = copy_cell(c->children[i]);
    }
    break;
  case NATIVE:
    x->val = copy_native(c->val);
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
    printf("not found\n");
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
  if (c->type != TUPLE ||
      c->len != 2) {
    /* error */
    printf("invalid arg format for builtin at eq\n");
    return NULL;
  }
  Cell *x = c->children[0];
  Cell *y = c->children[1];
  if (x->type == SYMBOL &&
      y->type == SYMBOL) {
    if(strcmp(x->sym, y->sym) == 0) {
      return new_symbol("true");
    } else {
      return new_symbol("false");
    }
  }
  printf("invalid args for eq\n");
  return NULL;
}

Cell *first(Cell *x) {
  if (x->type == TUPLE) {
    if (x->len == 0) {
      return new_nil();
    }
    return x->children[0];
  }
  printf("invalid args for first\n");
  return NULL;
}

Cell *rest(Cell *x) {
  if (x->type == TUPLE) {
    if (x->len == 0) {
      return new_tuple(0,NULL);
    }
    return new_tuple(x->len - 1, &(x->children[1]));
  }
  return NULL;
}

Cell *ifthenelse(Cell *x) {
  if (x->type == TUPLE &&
      x->len == 3 &&
      x->children[0]->type == SYMBOL) {
    if (strcmp(x->children[0]->sym, "true") == 0) {
      return(x->children[1]);
    } else if (strcmp(x->children[0]->sym, "false") == 0) {
      return(x->children[2]);
    }
  }
  printf("condition isn't a boolean\n");
  return NULL;
}

Cell *pair(Cell *left, Cell *right) {
  Cell **children = malloc(sizeof(Cell *) * 2);
  children[0] = left;
  children[1] = right;
  return new_tuple(2, children);
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
      c->len == 3 &&
      c->children[0]->type == SYMBOL &&
      strcmp(c->children[0]->sym, "#") == 0) {
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
  if (param_names->type == TUPLE && param_names->len == 0) {
    return scope;
  }
  
  if (param_names->type != TUPLE ||
      first(param_names)->type != SYMBOL ||
      args->type != TUPLE) {
    /* return error */
  }

  scope = add(scope, first(param_names)->sym, first(args));
  return add_to_scope(scope, rest(param_names), rest(args));
}

Cell *eval(Cell *c, Scope *scope);
Cell *apply_function(Cell *fn, Cell *args, Scope *scope) {
  printf("applying "); println_cell(fn);
  if (fn->type == NATIVE &&
      fn->val->native_type == FUNCTION) {
    Cell *((*f)(Cell *)) = (Cell *((*)(Cell *)))fn->val->long_val;
    return (*f)(args);
  } 
  if (!(fn->type == TUPLE) || !(fn->len == 3)) {
    printf("failed to apply function "); println_cell(fn);
    return NULL;
  }
  Cell *body = fn->children[2];
  Cell *param_names = fn->children[1];
  scope = add_to_scope(scope, param_names, args);
  Cell *result = eval(body, scope);
  printf("result: ");println_cell(result);
  return result;
}
