forget (reduce)
: (reduce) 0 swap
	begin 
		/mod rot + swap
		?dup 0= 
		if exit then 
	repeat ;
	
: reduce begin (reduce) dup 10 < if exit then repeat ;