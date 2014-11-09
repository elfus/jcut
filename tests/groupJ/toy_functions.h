# ifndef TOY_FUNCTIONS
# define TOY_FUNCTIONS

enum Operation {
	ERROR = 0 ,
	SUM ,
	SUBS ,
	FACT_1 , /* recursive factorial */
	FACT_2 /* iterative factorial */
};

extern enum Operation current_op ;
extern int result ;
/* prints a friendly message on standard output */
void print_msg ();
/* return zero when ’a ’ is ODD , return non - zero when is not */
int is_odd ( int a );
/* return zero when ’a ’ is EVEN , return non - zero when is not */
int is_even ( int a );
/* returns the sum of variables ’a ’ and ’b ’ */
int sum ( int a , int b );
/* returns the substraction of variables ’a ’ minus ’b ’ */
int subs ( int a , int b );
/* returns the factorial of ’a ’ using a recursive method */
int fact_1 ( int a );
/* returns the factorial of ’a ’ using an iterative method */
int fact_2 ( int a );
/* performs an operation on a and b according to the variable
* ’ current_op ’. The return value is stored in the variable ’ result ’
*/
void perform_operation ( int a , int b );
# endif /* * TOY_FUNCTIONS * */
