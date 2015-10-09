// Vortex math stuff ...

: VMath.module ;

: reduce dup 
	if 
		9 mod dup 0= if drop 9 then
	then ;

// other testing crap

: vectors.num 10 ;
vectors.num ARRAY vectors

: vector! vectors >array ;
: vector@ vectors array> ;
: vector.go vector@ EXECUTE ;
: vectors.init vectors.num 0 do 0 i vector! loop ; 
: .vector vector@ ?DUP IF BODY>HEAD HEAD>NAME COUNT TYPE ELSE ." null." THEN ;
: .vectors vectors.num 0 DO I . '-' EMIT .BL I .vector .CR LOOP ;
vectors.init
