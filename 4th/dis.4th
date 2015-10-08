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

: .sc ."  ; " ;

: code.literal 3 ;
: code.if 21 ;
: code.goto 27 ;
: code.dictp 33 ;
: code.return 49 ;

// ?uses2 ( instr -- (instr true)|false )
: ?uses2 
	dup code.literal = if drop true exit then
	dup code.if = if drop true exit then
	dup code.goto = if drop true exit then
	dup code.dictp = if drop true exit then
	drop false
	;

// ?is.prim ( instr -- is-prim )
: is.prim dup mcode ?array.in.bounds 
	if
		mcode array> 
	else
		drop false
	then ;

// .mcode ( instr -- is-prim )
: .mcode dup is.prim
	if
		mcode array> dup
		if 
			execute count type true 
		then
	else
		drop false
	then ;

: .mcodes mcode @ 0 do i .mcode if .cr then loop ;

// ?is.word ( addr -- (dict-head true)|false )
: ?is.word dup @ code.dictp =
	if 
		body>head dup last <=
		if drop false else true then
	else
		drop false
	then
;

// ( head-addr -- )
: .word.name 
	if 
	then ;

: dis.addr ( addr1 -- addr2 )
	dup >R
	dup . ." : " 
	dup @ dup . dup ?uses2
	if 
		R> drop over 1+ dup >R @ . 
	else
		."     "
	then 
	.sc dup .mcode 
	if
		code.dictp = 
		if
			.BL R@ @ head>name count type
		then
	else
		?is.word
		if head>name count type then
	then
	.cr drop R> ;

: dis swap 1- 
	begin 
		1+ 
		2dup >= 
		if 
			dup @ code.dictp = 
			if .cr then
			dis.addr
		else 
			2drop exit 
		then 
	repeat ;
