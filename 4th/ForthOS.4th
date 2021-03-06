: .ForthOS.BootStrap. ;

: STATE 3 ;

: ] 1 STATE ! ;
: [ 0 STATE ! ; IMMEDIATE
: ?] STATE @ ;

: + ?] IF 10 , ELSE [ 10 , ] THEN ; IMMEDIATE

: // SOURCE + >IN ! ; IMMEDIATE

// *************************************************************************************************
// Addresses

// : unused     0   ;
// : (HERE)     1   ;
// : (LAST)     2   ;
// : STATE      3   ;
: BASE          4   ; 
// : (SOURCE)   5   ;
// : >IN        6   ;
: MEM_LAST      7 @ ;
: DEPTH         8 @ ;
// : unused     9   ;
// : unused    10   ;
: STDIN        11 @ ;
: STDOUT       12 @ ;
: STDERR       13 @ ;
// : unused    14   ;
: DBG.FLG      15   ;
: TIB         100   ;
: CODE_START 1000   ;

// *************************************************************************************************
// Primitives ... macro-assembler building blocks ...
//              1                                  // I_FETCH (@)
//              2                                  // I_STORE (!)
: PUSH          3 , ;                              // I_LITERAL
: DUP    ?] IF  4 , ELSE [  4 , ] THEN ; IMMEDIATE
: SWAP   ?] IF  5 , ELSE [  5 , ] THEN ; IMMEDIATE
: DROP   ?] IF  6 , ELSE [  6 , ] THEN ; IMMEDIATE
: PICK   ?] IF  7 , ELSE [  7 , ] THEN ; IMMEDIATE
: ROT    ?] IF  8 , ELSE [  8 , ] THEN ; IMMEDIATE
: 1+     ?] IF  9 , ELSE [  9 , ] THEN ; IMMEDIATE // not required as an opcode, for performance
//             10 - I_PLUS (+)                     // defined earlier
: -      ?] IF 11 , ELSE [ 11 , ] THEN ; IMMEDIATE
: *      ?] IF 12 , ELSE [ 12 , ] THEN ; IMMEDIATE
: /      ?] IF 13 , ELSE [ 13 , ] THEN ; IMMEDIATE
: =      ?] IF 14 , ELSE [ 14 , ] THEN ; IMMEDIATE
//             15 used to be <> , now it is obsolete
: <      ?] IF 16 , ELSE [ 16 , ] THEN ; IMMEDIATE
: >      ?] IF 17 , ELSE [ 17 , ] THEN ; IMMEDIATE
: >R     ?] IF 18 , ELSE [ 18 , ] THEN ; IMMEDIATE
: R>     ?] IF 19 , ELSE [ 19 , ] THEN ; IMMEDIATE
: R@     ?] IF 20 , ELSE [ 20 , ] THEN ; IMMEDIATE
//             21 - IF runtime logic               // if TOS == 0, then GOTO to ELSE or THEN
: DO     ?] IF 22 , ELSE [ 22 , ] THEN ; IMMEDIATE
: I      ?] IF 23 , ELSE [ 23 , ] THEN ; IMMEDIATE
: LEAVE  ?] IF 24 , ELSE [ 24 , ] THEN ; IMMEDIATE
: LOOP   ?] IF 25 , ELSE [ 25 , ] THEN ; IMMEDIATE
: +LOOP  ?] IF 26 , ELSE [ 26 , ] THEN ; IMMEDIATE
: GOTO         27 , ;
: AND    ?] IF 28 , ELSE [ 28 , ] THEN ; IMMEDIATE
: OR     ?] IF 29 , ELSE [ 29 , ] THEN ; IMMEDIATE
: XOR    ?] IF 30 , ELSE [ 30 , ] THEN ; IMMEDIATE
//             31 used to be OVER, now it is obsolete
: EMIT   ?] IF 32 , ELSE [ 32 , ] THEN ; IMMEDIATE
: DICTP        33 , ;                              // NOOP to skip over the dictionary entry back pointer
: >S     ?] IF 34 , ELSE [ 34 , ] THEN ; IMMEDIATE
: S>     ?] IF 35 , ELSE [ 35 , ] THEN ; IMMEDIATE
: fopen  ?] IF 36 , ELSE [ 36 , ] THEN ; IMMEDIATE 
: fclose ?] IF 37 , ELSE [ 37 , ] THEN ; IMMEDIATE 
: fread  ?] IF 38 , ELSE [ 38 , ] THEN ; IMMEDIATE 
: fwrite ?] IF 39 , ELSE [ 39 , ] THEN ; IMMEDIATE 
: fgetc  ?] IF 40 , ELSE [ 40 , ] THEN ; IMMEDIATE 
: .brk.        48 , ; IMMEDIATE
: RETURN,      49 , ; 
: EXIT   ?] IF RETURN, THEN ; IMMEDIATE

// No opcodes needed for these
: BEGIN  ?] IF HERE   THEN ; IMMEDIATE 
: REPEAT ?] IF GOTO , THEN ; IMMEDIATE

// *************************************************************************************************
: ( SOURCE >IN @ DO DUP I + @ ')' = IF DROP I 1+ >IN ! LEAVE THEN LOOP ; IMMEDIATE

: 1- 1 - ;
: 0= 0 = ;
: <> = 0= ;
: <= > 0= ;
: >= < 0= ;
: 2* 2 * ;
: 2/ 2 / ;
: 2+ 1+ 1+ ;
: .dec. S> DROP ;
: true -1 ; 
: false 0 ;

: 2DROP DROP DROP ;
: 2DUP OVER OVER ;
: ?DUP DUP IF DUP THEN ;

: NIP SWAP DROP ;
: TUCK SWAP OVER ;
: -ROT ROT ROT ;
: NEGATE -1 * ;
: ABS DUP 0 < IF NEGATE THEN ;

: S@ dup @ + @ then ; 
: S! dup @ ?dup if + ! then ;

: mod 2dup / * - ;
: decimal 10 base ! ;
: hex 16 base ! ;
: binary 2 base ! ;
: ?hex base @ 10 > ; 

: +! dup ..@ rot + swap ! ;
: -! dup @ rot - swap ! ;
: ON -1 SWAP ! ;
: OFF 0 SWAP ! ;

// string stuff
: string, count dup , 0 do dup @ ,    1+ loop drop ;
: TYPE ( addr n -- )  0 DO DUP @ EMIT 1+ LOOP DROP ;

// ( from-addr to-addr -- ) 
: STRCAT SWAP COUNT 0 DO 
		DUP @ 2 PICK >S 1+
	LOOP 2DROP
	;

//	( from-addr to-addr -- )
: STRCPY DUP OFF STRCAT ;

: ALLOCATE HERE SWAP 0 DO 0 , LOOP ;
: ALLOT ALLOCATE DROP ;
: CONSTANT CREATE DICTP LAST , PUSH , RETURN, ;
: VARIABLE HERE 5 + CONSTANT 0 , ;

// These should both end up with the same machine code
32 CONSTANT BL
: CR 13 ;

: BETWEEN 2 PICK >= -ROT >= = ;     // ( c min max -- bool )
: min 2dup < if drop else nip then ; // ( a b -- a|b )
: max 2dup > if drop else nip then ; // ( a b -- a|b )

// ( n -- q r )
: /MOD DUP BASE @ / DUP BASE @ * ROT SWAP - ;

: .BL BL EMIT ;
: .CR CR EMIT 10 EMIT ;

// Case sensitive string compare
// ( addr1 addr2 -- bool ) 
: strcmp 2DUP @ SWAP @ =
	IF
		-1 >R
		1+ SWAP COUNT 0 
		DO
			2DUP @ SWAP @ =
			IF 
				1+ SWAP 1+
			ELSE
				R> DROP 0 >R LEAVE 
			THEN
		LOOP
		2DROP R>
	ELSE
		2DROP
		0
	THEN ;

// ( c -- C )
: TO-UPPER DUP 'a' 'z' BETWEEN IF 32 - THEN ;

// Case insensitve string compare
: STRCMPI ( ADDR1 ADDR2 -- BOOL ) 2DUP @ SWAP @ =
	IF
		-1 >R
		1+ SWAP COUNT 0 
		DO
			2DUP @ TO-UPPER SWAP @ TO-UPPER =
			IF 
				1+ SWAP 1+
			ELSE
				R> DROP 0 >R LEAVE 
			THEN
		LOOP
		2DROP R>
	ELSE
		2DROP
		0
	THEN ;

: HEAD>NAME 3 + ;
: HEAD>BODY 2+ @ ;
: NAME>HEAD 3 - ;
: BODY>HEAD 1+ @ ;

// <# #S #> ...
variable #S.buf 32 allot
here 1- constant #S.bufEnd
variable #S.isNeg
: <# #S.buf off dup 0 < #S.isNeg ! abs ;
: #S.buf+ #S.bufEnd #S.buf @ - ! #S.buf .inc. ;
: # /mod dup 9 > if 10 - 'A' + else '0' + then #S.buf+ ;
: #S # begin ?dup if # else exit then repeat ;
: #S.zFill dup #S.buf @ > if #S.buf @ do '0' #S.buf+ loop else drop then ;
: #> #s.isNeg @ if '-' #S.buf+ false #S.isNeg ! then #S.buf @ dup #S.bufEnd swap - >R R@ ! R> count ;
: #S.rJ dup #S.buf @ > if #S.buf @ do BL #S.buf+ loop else drop then ;

: . <# #S #> type .BL ;
: .S '<' EMIT DEPTH <# #S #> type '>' EMIT .BL DEPTH IF -1 DEPTH 2 - DO I PICK . -1 +LOOP THEN ;

: forget.last  LAST HEAD>BODY (HERE) ! LAST DUP @ + 1+ (LAST) ! ;

VARIABLE .cw.
: word.this .cw. @ ;
: word.first LAST .cw. ! word.this ;
: word.next word.this ?DUP IF COUNT + DUP MEM_LAST >= IF DROP 0 THEN .cw. ! THEN word.this ;

// ( c-addr -- c-addr 0 | xt 1 | xt -1 ) 
: FIND 
	word.first
	BEGIN
		IF
			DUP word.this HEAD>NAME STRCMPI
			IF
				DROP
				word.this HEAD>BODY
				word.this 1+ @ 
				0= IF -1 ELSE 1 THEN
				EXIT
			THEN
		ELSE 
			0 EXIT
		THEN

		word.next
	REPEAT
	;

: ' .word. PAD FIND 0= IF DROP false THEN ;
: EXECUTE ( addr -- ) ?DUP IF >R THEN ;

: FILL ( addr n b -- ) -ROT OVER + SWAP DO DUP I ! LOOP DROP ;
: ERASE ( addr n -- ) 0 FILL ;

// example: variable vector  :NONAME 1 2 3 + + . NONAME;  vector !
: :NONAME ( -- code-addr ) HERE ] ;
: NONAME; ?] IF RETURN, 0 STATE ! THEN ; IMMEDIATE

// Return the number of words in the dictionary
: ?num.words ( -- n ) 0 word.first
	BEGIN
		IF 1+ ELSE EXIT THEN
		word.next
	REPEAT
	;

: .dict.size MEM_LAST LAST - . ;
: .code.size HERE CODE_START 1+ - . ;
: .word.count ?num.words . ;

: .dict. ( -- ) word.first 
	BEGIN
		?DUP
		IF
			DUP . DUP 1+ @ . DUP HEAD>BODY . .bl HEAD>NAME COUNT TYPE .CR 
		ELSE
			EXIT
		THEN
		word.next
	REPEAT
	;

: words ( -- ) word.first
	BEGIN
		?DUP 
		IF
			HEAD>NAME COUNT TYPE .BL
		ELSE
			EXIT
		THEN
		word.next
	REPEAT
	;

: .collect. ( addr delim -- )
	>R DUP off
	SOURCE >IN @ 1+
	DO DUP I + @ DUP R@ = 
		IF 
			2DROP I 1+ >IN ! LEAVE 
		ELSE
			2 PICK >S 
		THEN 
	LOOP
	R> 2DROP
	;

: " PAD '"' .collect. ?] 
	IF 
	  PUSH HERE 3 + , GOTO HERE 0 ,
		PAD string,
		HERE SWAP !
	ELSE 
		PAD
	THEN ; IMMEDIATE

: ." PAD '"' .collect. ?] 
	IF 
	  PUSH HERE 0 , [ PUSH ' COUNT , ] , [ PUSH ' TYPE ,  ] ,  GOTO HERE SWAP 0 , HERE SWAP !
		PAD string,
		HERE SWAP !
	ELSE 
		PAD [ ' COUNT , ' TYPE , ]
	THEN ; IMMEDIATE

// ********************************************************************************

: forget .word. PAD FIND 
	if 
		dup 
		BODY>HEAD DUP @ + 1+ (LAST) ! 
		(HERE) ! 
	else 
		count type ."  not found." 
	then ;

: ?free last here - ;

// FILE stuff
: fopen.read.text " rt" fopen ;
: fopen.read.binary " rb" fopen ;
: fopen.write.text " wt" fopen ;
: fopen.write.binary " wb" fopen ;
: fread.line ." sorry." ;

: debug.on .cr 1 DBG.FLG ! ;
: debug.on.high .cr 2 DBG.FLG ! ;
: debug.off DBG.FLG off .cr ;

// Simple temporary variables.
// Sure, these could have more error checking.
// If I need it, I can add some later.
// This is all I need for now ... :)

// variable t.sp 32 allot
// : 0t.sp t.sp off ;
// : t@ t.sp @ if t.sp dup @ + @ then ;
// : t! t.sp @ if t.sp dup @ + ! then ;
// : >t t.sp @ 31 < if t.sp .inc. t! then ;
// : t> t.sp @      if t@ t.sp .dec. then ;
// : t.swap t> t> swap >t >t ;
// : t.drop t> drop ;

variable tmpVars 32 allot
: t@ tmpVars S@ ;
: t! tmpVars S! ;
: >t tmpVars >S ;
: t> tmpVars S> ;
: t.swap t> t> swap >t >t ;
: t.drop t> drop ;

: SPACES 0 DO .BL LOOP ;

: #.convStr " 0123456789ABCDEF" ;
: #N.isNum // ( c -- num bool )
	false >t 0 >t
	to-upper #.convstr 1+ base @ 1+ 0 
	do 
		2dup @ = 
		if 
			t.drop t.drop i true >t >t leave
		else 
			1+
		then
	loop 
	2drop
	t> t> ; 
: <N 0 -rot false #S.isNeg ! ; // ( addr num -- 0 addr num )
: N> rot #S.isNeg if negat1 2 3e then ; // ( n addr num -- addr num n )
: #N // ( n1 c -- n2 bool )
	#N.isNum
	if 
		swap base @ * + true
	else
		drop false
	then
	; 

: dump 0 do dup @ <# #S 4 #S.zFill #> type .BL 1+ loop drop ;
.cr .cr words
.cr .cr ." everything look ok?"
break;

// : src (source) @ ;
// : readline src off begin stdin fgetc dup CR = if drop exit else dup emit src >S then repeat ;

// variable cmds 100 allot

// : .num. 777777 . ;
// : .ew. dup head>body swap 1+ @ if execute else ?] if 30 , , else execute then then ;
// : .pw. pad .fw. ?dup if .ew. else .num. then ;
// : .pl. begin source nip >in @ <= if exit else .word. pad @ if .pw. then then repeat ;
// : forth begin ." 4th>" readline src " bye" strcmpi if exit else .BL 0 >IN ! .pl. ."  ok" .cr then repeat ;

break;
