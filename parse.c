/*
 * ast creation
 */

Cell *parse_expr(char **input_ptr);
Cell *parse_tuple(char **input_ptr);
Cell *parse_atom(char **input_ptr);
Cell *parse_number(char **input_ptr);

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
  if ('0' <= **input_ptr &&
      **input_ptr <= '9') {
    return parse_number(input_ptr);
  }
  return parse_atom(input_ptr);
}

Cell *parse_tuple(char **input_ptr) {
  if (**input_ptr != '(') {
    printf("tuples must begin with (\n");
    return NULL;
  } else {
    (*input_ptr)++;
  }
  int i = 0;
  Cell **children = malloc(sizeof(Cell *));
  while (**input_ptr != '\0') {
    for(; **input_ptr == ' '; (*input_ptr)++);
    if (**input_ptr == ')') {
      break;
    }
    Cell *child = parse_expr(input_ptr);
    if (!child) {
      break;
    }
    if (i && !(i & (i - 1))) { // power of two
      Cell **new_children = malloc(sizeof(Cell *) * (2 * i));
      memcpy(new_children, children, sizeof(Cell *) * i);
      free(children);
      children = new_children;
    }
    children[i] = child;
    i++;
  }
  if (**input_ptr != ')') {
    printf("unmatched '('\n");
    return NULL;
  } else {
    (*input_ptr)++;
  }
  return new_tuple(i, children);
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

Cell *parse_number(char **input_ptr) {
  Integer ten = new_integer((unsigned long) 10);
  Integer x = new_integer((unsigned long) 0);
  for (; '0' <= **input_ptr && **input_ptr <= '9'; (*input_ptr)++) {
    unsigned long next = (unsigned long) (**input_ptr - '0');
    Integer digit = new_integer(next);
    Integer new_x = multiply_integer(x, ten);
    destroy_integer(x);
    x = add_integers(new_x, digit);
    destroy_integer(digit);
    destroy_integer(new_x);
  }
  destroy_integer(ten);
  Cell *c = new_native_int(x);
  return c;
}
