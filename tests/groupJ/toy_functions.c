# include <stdio.h>
# include "toy_functions.h"

/* prints a friendly message on standard output */
void print_msg () {
	printf ( "Hello world\n " );
}

/* return zero when ’a ’ is ODD , return non - zero when is not */
int is_odd ( int a ) {
	return a %3;
}

/* return zero when ’a ’ is EVEN , return non - zero when is not */
int is_even ( int a ) {
	return a %2;
}

enum Operation current_op = ERROR ;

int result = 0;

/* returns the sum of variables ’a ’ and ’b ’ */
int sum ( int a , int b ) {
	return a + b ;
}
/* returns the substraction of variables ’a ’ minus ’b ’ */
int subs ( int a , int b ) {
	return a - b ;
}

/* returns the factorial of ’a ’ using a recursive method */
int fact_1 ( int a ) {
	if (0 > a ) a *= -1;
	if (0 == a ) a = 1;
	if (1 == a ) return a ;
	return a * fact_1 ( a - 1);
}

/* returns the factorial of ’a ’ using an iterative method */
int fact_2 ( int a ) {
	if (0 > a ) a *= -1;
	int i = 1;
	while ( a ) i *= a--;
	return i ;
}

/* performs an operation on a and b according to the
* variable ’ current_op ’. The return value is stored
* in the variable ’ result ’
*/
void perform_operation ( int a , int b ) {
	switch ( current_op ) {
	case SUM :
		result = sum (a , b );
	break ;
	case SUBS :
		result = subs (a , b );
	break ;
	case FACT_1 :
		result = fact_1 ( a );
	break ;
	case FACT_2 :
		result = fact_2 ( a );
	break ;
	case ERROR :
	default :
		result = -1;
		printf ( "Invalid operation\n" );
	break ;
	}
}

void print_result() {
	printf("result = %d\n", result);
}

