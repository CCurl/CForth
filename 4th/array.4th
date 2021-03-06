// ********************************************************************************
// Arrays return the starting address of the array.
// This is a "counted" array, just like strings, so the first cell is the size.
// Use COUNT to get the number of elements and address of the first cell.
// Indexes go from 0 to n-1 (base 0)
// 10 array test    ... 
// 12 3 test >array ... stores 12 in position 3 of array 'test'
// 3 test array>    ... fetch value at position 3 from array 'test'
// ********************************************************************************

: ARRAY.Module ;

: ARRAY DUP 1+ ALLOCATE TUCK ! CREATE DICTP LAST , PUSH , RETURN, ;
// ( n array -- bool )
: ?ARRAY.in.bounds @ 0 swap between ;

( n array -- bool )
: ARRAY.Check.Bounds ?ARRAY.in.bounds
	dup 0= if 
		." index out of bounds." 
	then ;
: >ARRAY ( val pos array -- ) 2dup Array.Check.Bounds if count drop + ! else 2drop drop then ;
: ARRAY> ( pos array -- val ) 2dup Array.Check.Bounds if count drop + @ else 2drop then ;
: .ARRAY COUNT 0 DO DUP @ . 1+ LOOP DROP ;
