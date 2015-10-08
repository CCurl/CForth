: lists.4th ;

// lists ... they look like this ...
// int prev;         // addr of previous node, 0 = first
// int next;         // addr of next node, 0 = last
// int state;        // 1 = active, 0 = deleted
// int payload.size; // standard counted array
// int payload[];    //

: list.add.node ( payload last-node --- new-node )
	HERE >R , 0 , 1 , string,
	R@ @ ?DUP IF
		R@ SWAP 1+ !
	THEN
	R>
	;

: list.node.prev    ( node -- prev-node ) @ ;
: list.node.next    ( node -- next-node ) 1+ @ ;
: list.node.state   ( node -- state )     2+ ;
: list.node.payload ( node -- payload )   3 + ;
: list.node.state   ( node -- )           list.node.state 0 swap ! ;

: list.goto.last ( node -- last-node )
	begin
		dup list.node.next ?dup 
		if 
			nip
		else
			exit 
		then
	repeat
	;

: list.goto.first ( node -- last-node )
	begin
		dup list.node.prev ?dup 
		if 
			nip
		else
			exit 
		then
	repeat
	;

// NB: the payload is a standard counted array
: list.add ( payload list --- new-node ) list.goto.last list.add.node ;

// Create a new list like this ... first-payload list.new
: list.new ( payload --- new-list ) 0 list.add.node ;
