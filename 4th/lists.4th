forget lists.module

: Lists.Module ;

// lists ... they look like this ...
// int prev;     // addr of previous node, 0 = first
// int next;     // addr of next node, 0 = last
// int data;     // address of entry

: prevNode @ ;      //  ( node -- prev-node|0 )
: nextNode 1+ @ ;   //  ( node -- next-node|0 )
: node@ 1+ 1+ @ ;  //  ( node -- val )
: node! 1+ 1+ ! ;  //  ( val node -- )

: list.addNode ( last-node --- new-node )
	HERE >R , 0 , 0 , 
	R@ @ ?DUP IF
		R@ SWAP 1+ !
	THEN
	R>
	;

: list.deleteNode >R // ( this -- )
	R@ @ ?DUP IF
		1+ R@ 1+ @ SWAP ! // prevNode.next now points to this.next
	THEN

	R@ 1+ @ ?DUP IF
		R@ @ SWAP !      // nextNode.prev now points to this.prev
	THEN

	R> DROP
	;

: list.last ( node -- last-node )
	begin
		dup nextNode ?dup 
		if 
			nip
		else
			exit 
		then
	repeat
	;

: list.first ( node -- last-node )
	begin
		dup prevNode ?dup 
		if 
			nip
		else
			exit 
		then
	repeat
	;

// NB: the addr is stored as is in the list
: list.add ( list --- new-node ) list.last list.addNode ;
: list.new ( -- new-list ) 0 list.addNode ;
: list.dump begin ?dup if dup node@ . nextNode else exit then repeat ;

: list.findVal ( val list -- node|0 )
	begin
		dup node@ 2 pick =
		if 
			nip exit
		then
		nextnode dup 0=
		if
			nip exit
		then
	repeat
	;

// example ...
// variable tl 
// 111 list.new dup tl ! node!
// 222 tl @ list.add node!
// 333 tl @ list.add node!
// 444 tl @ list.add node!
// last tl @ list.add node!
// tl @ list.dump
