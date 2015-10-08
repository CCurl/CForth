: BootStrap.4th ;

HERE 999 ! LAST 998 !
: .bsr 999 @ (HERE) ! 998 @ (LAST) ! ;

: ] 1 STATE ! ;
: [ 0 STATE ! ; IMMEDIATE
: ?] STATE @ ;

: + ?] IF 10 , ELSE [ 10 , ] THEN ; IMMEDIATE

: // SOURCE + >IN ! ; 

// Primitives ... macro-assembler building blocks ...
: PUSH 3 , ;
: SWAP   ?] IF  5 , ELSE [  5 , ] THEN ; IMMEDIATE
: DROP   ?] IF  6 , ELSE [  6 , ] THEN ; IMMEDIATE
: PICK   ?] IF  7 , ELSE [  7 , ] THEN ; IMMEDIATE
: ROT    ?] IF  8 , ELSE [  8 , ] THEN ; IMMEDIATE
: 1+     ?] IF  9 , ELSE [  9 , ] THEN ; IMMEDIATE // not required as an opcode, for performance
//             10 (+) was needed earlier
: -      ?] IF 11 , ELSE [ 11 , ] THEN ; IMMEDIATE
: *      ?] IF 12 , ELSE [ 12 , ] THEN ; IMMEDIATE
: /      ?] IF 13 , ELSE [ 13 , ] THEN ; IMMEDIATE
: =      ?] IF 14 , ELSE [ 14 , ] THEN ; IMMEDIATE
: <>     ?] IF 15 , ELSE [ 15 , ] THEN ; IMMEDIATE
: <      ?] IF 16 , ELSE [ 16 , ] THEN ; IMMEDIATE
: >      ?] IF 17 , ELSE [ 17 , ] THEN ; IMMEDIATE
: >R     ?] IF 18 , ELSE [ 18 , ] THEN ; IMMEDIATE
: R>     ?] IF 19 , ELSE [ 19 , ] THEN ; IMMEDIATE
: R@     ?] IF 20 , ELSE [ 20 , ] THEN ; IMMEDIATE
//             21 is the IF runtime logic
: DO     ?] IF 22 , ELSE [ 22 , ] THEN ; IMMEDIATE
: I      ?] IF 23 , ELSE [ 23 , ] THEN ; IMMEDIATE
: LEAVE  ?] IF 24 , ELSE [ 24 , ] THEN ; IMMEDIATE
: LOOP   ?] IF 25 , ELSE [ 25 , ] THEN ; IMMEDIATE
: +LOOP  ?] IF 26 , ELSE [ 26 , ] THEN ; IMMEDIATE
: BEGIN  ?] IF HERE THEN ; IMMEDIATE // No opcode needed for this
: AGAIN 27 , ;
: REPEAT ?] IF AGAIN , THEN ; IMMEDIATE
: .      ?] IF 28 , ELSE [ 28 , ] THEN ; IMMEDIATE
//             29 used to be TYPE, now it is obsolete
: CALL   30 , ; //             30 is CALL
: OVER   ?] IF 31 , ELSE [ 31 , ] THEN ; IMMEDIATE // not required as an opcode, for performance
: EMIT   ?] IF 32 , ELSE [ 32 , ] THEN ; IMMEDIATE
: DICTP 33 , ;                                     // NOOP to skip over the dictionary entry back pointer
: 1-     ?] IF 34 , ELSE [ 34 , ] THEN ; IMMEDIATE // not required as an opcode, for performance
: 0=     ?] IF 35 , ELSE [ 35 , ] THEN ; IMMEDIATE // not required as an opcode, for performance
: fopen  ?] IF 36 , ELSE [ 36 , ] THEN ; IMMEDIATE 
: fclose ?] IF 37 , ELSE [ 37 , ] THEN ; IMMEDIATE 
: fread  ?] IF 38 , ELSE [ 38 , ] THEN ; IMMEDIATE 
: fwrite ?] IF 39 , ELSE [ 39 , ] THEN ; IMMEDIATE 
: fgetc  ?] IF 40 , ELSE [ 40 , ] THEN ; IMMEDIATE 
: RETURN 99 , ; 
: EXIT   ?] IF RETURN THEN ; IMMEDIATE

: <= 1+ < ;
: >= 1- > ;
: 2* 2 * ;
: 2/ 2 / ;
: 2+ 1+ 1+ ;

: NIP SWAP DROP ;
: TUCK SWAP OVER ;
: -ROT ROT ROT ;
: NEGATE 0 SWAP - ;
: ABS DUP 0 < IF NEGATE THEN ;

: 2DROP DROP DROP ;
: 2DUP OVER OVER ;
: ?DUP DUP IF DUP THEN ;

: mod 2dup / * - ;
: decimal 10 base ! ;
: hex 16 base ! ;

// string stuff
: string, count dup , 0 do dup @ ,    1+ loop drop ;
: TYPE                0 DO DUP @ EMIT 1+ LOOP DROP ;

: STR+ DUP .INC. DUP @ + ! ;      // ( C ADDR -- ) 
: STRCLR 0 SWAP ! ;               // ( ADDR -- ) 

// ( from-addr to-addr -- ) 
: STRCAT SWAP COUNT 0 DO 
		DUP @ 2 PICK STR+ 1+
	LOOP 2DROP
	;

//	( from-addr to-addr -- )
: STRCPY DUP STRCLR STRCAT ;

: BL 32 ;
: CR 13 ;

: .ISWS.
	DUP 13 = IF DROP BL THEN
	DUP 10 = IF DROP BL THEN
	DUP BL = ;

: ( SOURCE >IN @ DO DUP I + @ ')' = IF DROP I 1+ >IN ! LEAVE THEN LOOP ; IMMEDIATE

// ( c min max -- bool )
: BETWEEN 2 PICK >= >R >= R> = ;

: ALLOCATE HERE SWAP 0 DO 0 , LOOP ;
: ALLOT ALLOCATE DROP ;
: VARIABLE CREATE DICTP LAST , PUSH HERE 2+ , RETURN 0 , ;

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
			2DUP @ SWAP @ <>
			IF 
				R> DROP 0 >R LEAVE 
			ELSE
				1+ SWAP 1+
			THEN
		LOOP
		2DROP R>
	ELSE
		2DROP
		0
	THEN ;

// ( C -- c )
: TO-UPPER DUP 'a' 'z' BETWEEN IF 32 - THEN ;

// Case insensitve string compare
: STRCMPI ( ADDR1 ADDR2 -- BOOL ) 2DUP @ SWAP @ =
	IF
		-1 >R
		1+ SWAP COUNT 0 
		DO
			2DUP @ TO-UPPER SWAP @ TO-UPPER <>
			IF 
				R> DROP 0 >R LEAVE 
			ELSE
				1+ SWAP 1+
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

: .FW. ( ADDR1 -- WORD-ADDR|0 ) LAST 
	BEGIN
		DUP MEM_LAST = IF 
			2DROP 0 EXIT
		THEN

		2DUP HEAD>NAME STRCMPI
		IF NIP EXIT THEN

		DUP @ +
	REPEAT
	;

: FIND-WORD .WORD. PAD .FW. ;
: ' FIND-WORD DUP IF HEAD>BODY THEN ;
: EXECUTE ( addr -- ) ?DUP IF >R THEN ;

: FILL ( addr n b -- ) -rot OVER + SWAP DO DUP I ! LOOP DROP ;
: ERASE ( addr n -- ) 0 FILL ;

// example: variable vector  :NONAME 1 2 3 + + . NONAME;  vector !
: :NONAME ( -- code-addr ) HERE ] ;
: NONAME; ?] IF RETURN 0 STATE ! THEN ; IMMEDIATE

// Return the number of words in the dictionary
: .wordcount. ( -- n ) 0 LAST
	BEGIN
		DUP MEM_LAST = IF 
			DROP . EXIT
		THEN

		SWAP 1+ SWAP

		DUP @ +
	REPEAT
	;

: .dictsize. MEM_LAST LAST - . ;
: .codesize. HERE 1001 - . ;
: .S DEPTH . '-' EMIT .BL DEPTH IF -1 DEPTH 1- 1- DO I PICK . -1 +LOOP THEN ;

: .dict. ( -- ) LAST
	BEGIN
		DUP MEM_LAST = IF 
			DROP EXIT
		THEN
		DUP . DUP 1+ @ . DUP HEAD>BODY . DUP HEAD>NAME COUNT TYPE .CR 
		DUP @ +
	REPEAT
	;

: words ( -- ) LAST
	BEGIN
		DUP MEM_LAST = IF 
			DROP EXIT
		THEN
		
		DUP HEAD>NAME COUNT TYPE .BL
		DUP @ +
	REPEAT
	;

: .collect. ( addr delim -- )
	>R DUP strclr
	SOURCE >IN @ 1+
	DO DUP I + @ DUP R@ = 
		IF 
			2DROP I 1+ >IN ! LEAVE 
		ELSE
			2 PICK str+ 
		THEN 
	LOOP
	R> 2DROP
	;

: ." PAD '"' .collect. ?] 
	IF 
	  PUSH HERE 0 , CALL [ PUSH ' COUNT , ] , CALL [ PUSH ' TYPE , ] , AGAIN HERE SWAP 0 , HERE SWAP !
		PAD string,
		HERE SWAP !
	ELSE 
		PAD [ CALL ' COUNT , CALL ' TYPE , ]
	THEN ; IMMEDIATE

: " PAD '"' .collect. ?] 
	IF 
	  PUSH HERE 3 + , AGAIN HERE 0 ,
		PAD string,
		HERE SWAP !
	ELSE 
		PAD
	THEN ; IMMEDIATE


// ********************************************************************************
// Arrays return the starting address of the array.
// This is a "counted" array, just like strings, so the first cell is the size.
// Use COUNT to get the number of elements and address of the first cell.
// Indexes go from 0 to n-1 (base 0)
// 10 array test    ... 
// 12 3 test >array ... stores 12 in position 3 of array 'test'
// 3 test array>    ... fetch value at position 3 from array 'test'
// ********************************************************************************

: ARRAY DUP 1+ ALLOCATE TUCK ! CREATE DICTP LAST , PUSH , RETURN ;
: ARRAY.Check.Bounds COUNT 1- 2 pick swap 0 swap between if -1 else ." index out of bounds." 0 then ;
: >ARRAY ( val pos array -- ) Array.Check.Bounds if + ! else 2drop then ;
: ARRAY> ( pos array -- val ) Array.Check.Bounds if + @ else 2drop then ;
: .ARRAY COUNT 0 DO DUP @ . 1+ LOOP DROP ;

// ********************************************************************************

: forget find-word ?dup if dup HEAD>BODY (HERE) ! DUP @ + (LAST) ! then ;
: forget.last  LAST HEAD>BODY (HERE) ! LAST DUP @ + (LAST) ! ;

: ?free last here - ;
: .free ?free . ;

// FILE stuff
: stdin  11 @ ;
: stdout 12 @ ;
: stderr 13 @ ;
: fopen.read.text " rt" fopen ;
: fopen.read.binary " rb" fopen ;
: fopen.write.text " wt" fopen ;
: fopen.write.binary " wb" fopen ;
: fread.line ." fread.line not implemented yet." ;

: src (source) @ ;
: readline src strclr begin stdin fgetc dup CR = if drop exit else dup emit src str+ then repeat ;

variable cmds 100 allot

: dbg.on .cr 1 15 ! ;
: dbg.off 0 15 ! .cr ;
: .num. 777777 . ;
: .ew. dup head>body swap 1+ @ if execute else ?] if 30 , , else execute then then ;
: .pw. pad .fw. ?dup if .ew. else .num. then ;
: .pl. begin source nip >in @ <= if exit else .word. pad @ if .pw. then then repeat ;
: forth begin ." 4th>" readline src " bye" strcmpi if exit else .BL 0 >IN ! .pl. ."  ok" .cr then repeat ;

break;
