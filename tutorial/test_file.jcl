
my_function(); 
group A {
	sum(2,2);
	group B {
		sum(2,2) == 4;	 
		group C {
			sum(2,2) != 5;
		}
	}
}

data { "data.csv"; }
sum(@, @);

data { "data-strings.csv"; }
print(@, @);


group D {
	fact_1(3) == 6;
	fact_2(3) == 6;
}

get_state() == 0;

group E { 	 	
	before_all { state = 10; } # Executed before all the tests in this group

	get_state() == 10; # test 1
	
	# jit-testing does not track what's changed inside this function call
	before { modify_state(); }
	get_state() == 15; #test 2

	modify_state(); # test 3

	after_all  { state == 20; } # Executed after all the tests in this group
}
 
group F {
	before_all { state = 5; }

	get_state() == 5; 
	
	before { modify_state(); } 	
	get_state() == 10; 
}

group G { 	


	get_state() == 0; 

	after_all { modify_state(); }
}

get_state() == 5;

#### For structs

# test 1
print_global_pair();

# test 2
before {global_pair = {1}; }
print_global_pair();

# test 3
before {global_pair = {1,2}; }
print_global_pair();

# test 4
before {global_pair = {1,2, {5} }; }
print_global_pair();

# test 5
before {global_pair = {1,2, {5,6} }; }
print_global_pair();


####### Pointers
print_g_ptr();

before { g_ptr = [1]; }
print_g_ptr();

before { g_ptr = [1024]; }
print_g_ptr();

before { g_ptr = [1:5]; }
print_g_ptr();

print_buffer([20:6], 20);

print_pair_ptr();

before {pair_ptr = [1:0]; }
print_pair_ptr();

#provide support for this one
#before {pair_ptr = [1:{1,2,{3,4}}]; }
#print_pair_ptr();

#Also provide support for when a struct is passed by value
#print_pair({1,2,{3,4}});

print_ptr_pair([1:{1,2,{3,4}}]);

before { state = 10; }
get_state();
after { state > 9; }


