#include <limits.h>

typedef struct {
  int num_digits;
  unsigned long *digits;
} Integer;

void print_integer(Integer i) {
  for (int l = 1; l <= i.num_digits; l++) {
    printf("%lu,", i.digits[i.num_digits - l]);
  }
}

Integer copy_integer(Integer i) {
  Integer j;
  j.num_digits = i.num_digits;
  j.digits = malloc(sizeof(unsigned long) * i.num_digits);
  memcpy(j.digits, i.digits, sizeof(unsigned long) * i.num_digits);
  return j;
}

Integer new_integer(unsigned long value) {
  Integer r;
  r.num_digits = 1;
  r.digits = malloc(sizeof(unsigned long));
  r.digits[0] = value;
  return r;
}

void print_integer(Integer x) {
  for (int i = 0; i < x.num_digits; i++) {
    printf("%lu ", x.digits[i]);
  }
}

void destroy_integer(Integer x) {
  free(x.digits);
}

Integer add_integers(Integer x, Integer y) {

  /* TODO: add an optimized codepath for the case where x and y are both
   *       single digits and the operation doesn't overflow
   */
  
  int max_r_digits =
    x.num_digits > y.num_digits ? x.num_digits + 1 : y.num_digits + 1;
  unsigned long *result = malloc(max_r_digits * sizeof(unsigned long));

  int r_digits = 0;
  unsigned long overflow = 0;
  unsigned long next_x, next_y;

  while (r_digits < x.num_digits || r_digits < y.num_digits || overflow > 0) {
    
    next_x  = r_digits < x.num_digits ? x.digits[r_digits] : 0;
    next_y  = r_digits < y.num_digits ? y.digits[r_digits] : 0;
    if (ULONG_MAX - overflow >= next_x &&
	ULONG_MAX - next_y >= (next_x + overflow)) {
      result[r_digits] = next_x + next_y + overflow;
      overflow = 0;
    } else {
      if (next_x > next_y) {
	result[r_digits] = next_x - (ULONG_MAX - next_y) + overflow;
      } else {
	result[r_digits] = next_y - (ULONG_MAX - next_x) + overflow;
      }
      overflow = 1;
    }
    r_digits++;

  }

  Integer r;
  r.num_digits = r_digits;
  r.digits = malloc(r_digits * sizeof(unsigned long));
  memcpy(r.digits, result, r_digits * sizeof(unsigned long));

  memcpy(r.digits, result, (r_digits * sizeof(unsigned long)));
  free(result);
  
  return r;
}

Integer mul(Integer x, Integer y) {

  unsigned long next_x, next_y;
  Integer r = new_integer(0);

  for (int i = 0; i < x.num_digits; i++) {
    for (int j = 0; j < y.num_digits; j++) {

      next_x = x.digits[i];
      next_y = y.digits[j];
      
      if(next_x > ULONG_MAX / next_y){

        // 4294967295 = 2^32 - 1
        // todo this should use ulong_max or something
	unsigned long x_r = next_x & 4294967295;
	unsigned long x_l = (next_x & (4294967295 << 32)) >> 32;
	unsigned long y_r = next_y & 4294967295;
	unsigned long y_l = (next_y & (4294967295 << 32)) >> 32;
        
	unsigned long p = x_r * y_r;
        unsigned long p1 = x_l * y_r;
        unsigned long p1_l = (p1 & (4294967295 << 32)) >> 32;
        unsigned long p1_r = p1 & 4294967295;
	unsigned long p2 = x_r * y_l;
        unsigned long p2_l = (p2 & (4294967295 << 32)) >> 32;
        unsigned long p2_r = p2 & 4294967295;


	Integer product;
        product.num_digits = i + j + 1;
        product.digits = calloc(i + j + 1, sizeof(unsigned long));
        product.digits[i + j] = p + (p1_r << 32) + (p2_r << 32);

	unsigned long c = (x_l * y_l + p1_l + p2_l);
	Integer carry;
	carry.num_digits = i + j + 2;
	carry.digits = calloc(i + j + 2, sizeof(unsigned long));
	carry.digits[i + j + 1] = c;

	Integer r_1 = add_integers(product, r);
	Integer r_2 = add_integers(carry, r_1);

	destroy_integer(r);
	destroy_integer(product);
	destroy_integer(carry);

	r = r_2;

	destroy_integer(r_1);
      } else {
	Integer product;
        product.num_digits = i + j + 1;
        product.digits = calloc(i + j + 1, sizeof(unsigned long));
        product.digits[i + j] = next_x * next_y;

	Integer r_1 = add_integers(product, r);
	destroy_integer(r);
	r = r_1;
      
      }
      
    }
  }

  return r;	
}
