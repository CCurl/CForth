forget .Dis.4th.
: .Dis.4th. ;

// *************************************************************************************************
// Addresses

// : unused     0   ;
// : (HERE)     1   ;
// : (LAST)     2   ;
// : STATE      3   ;
// : BASE          4   ; 
// : (SOURCE)   5   ;
// : >IN        6   ;
// : MEM_LAST      7 @ ;
// : DEPTH         8 @ ;
// : unused     9   ;
// : unused    10   ;
// : STDIN        11 @ ;
// : STDOUT       12 @ ;
// : STDERR       13 @ ;
// : unused    14   ;
// : DBG.FLG      15   ;
// : TIB         100   ;
// : CODE_START 1000   ;

50 array mcode

// *************************************************************************************************
// Primitives ... macro-assembler building blocks ...

:NONAME " @" NONAME;  1 mcode >array
:NONAME " !" NONAME;  2 mcode >array
:NONAME " literal" NONAME;  3 mcode >array
:NONAME " dup" NONAME;  4 mcode >array
:NONAME " swap" NONAME;  5 mcode >array
:NONAME " drop" NONAME;  6 mcode >array
:NONAME " pick" NONAME;  7 mcode >array
:NONAME " rot" NONAME;  8 mcode >array
:NONAME " 1+" NONAME;  9 mcode >array
:NONAME " +" NONAME;  10 mcode >array
:NONAME " -" NONAME;  11 mcode >array
:NONAME " *" NONAME;  12 mcode >array
:NONAME " /" NONAME;  13 mcode >array
:NONAME " =" NONAME;  14 mcode >array
:NONAME " <>" NONAME;  15 mcode >array
:NONAME " <" NONAME;  16 mcode >array
:NONAME " >" NONAME;  17 mcode >array
:NONAME " >R" NONAME;  18 mcode >array
:NONAME " R>" NONAME;  19 mcode >array
:NONAME " R@" NONAME;  20 mcode >array
:NONAME " IF" NONAME;  21 mcode >array
:NONAME " do" NONAME;  22 mcode >array
:NONAME " I" NONAME;  23 mcode >array
:NONAME " leave" NONAME;  24 mcode >array
:NONAME " loop" NONAME;  25 mcode >array
:NONAME " +loop" NONAME;  26 mcode >array
:NONAME " goto" NONAME;  27 mcode >array
:NONAME " ." NONAME;  28 mcode >array
:NONAME " over" NONAME;  31 mcode >array
:NONAME " emit" NONAME;  32 mcode >array
:NONAME " dictp" NONAME;  33 mcode >array
:NONAME " 1-" NONAME;  34 mcode >array
:NONAME " 0=" NONAME;  35 mcode >array
:NONAME " fopen" NONAME;  36 mcode >array
:NONAME " fclose" NONAME;  37 mcode >array
:NONAME " fread" NONAME;  38 mcode >array
:NONAME " fwrite" NONAME;  39 mcode >array
:NONAME " fgetc" NONAME;  40 mcode >array
:NONAME " break" NONAME;  48 mcode >array
:NONAME " return" NONAME;  49 mcode >array

: ?.uses2 // ( addr1 instr -- addr2 )
	dup  3 = if drop dup @ . 1+ exit then   // ( literal )
	dup 21 = if drop dup @ . 1+ exit then   // IF 
	dup 27 = if drop dup @ . 1+ exit then   // GOTO 
	dup 33 = if drop dup @ . 1+ exit then   // 
	drop
	;

// ( instr -- is-prim )
: .mcode dup mcode ?array.in.bounds
	if
		+ @ ?dup 
		if 
			swap . .bl execute count type true 
		else 
			drop false 
		then
	else
		drop false
	then ;

: .mcodes 100 0 do i .mcode if .cr then loop ;
: dis.addr ( addr -- addr )
	dup . ." : " 
	dup @ .mcode 
	if 
		1+ dup 1- @ ?.uses2 
	else 
		. drop 1+
	then
	.cr  ;

: dis swap begin 2dup >= if dis.addr else 2drop exit then repeat ;
