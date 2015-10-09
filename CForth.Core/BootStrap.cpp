#include "stdafx.h"
#include "ForthOS.h"

// Dictionary entry structure ...
// int entrySize; // a counted array; name length + 3
// int flags;     // 1 = IMMEDIATE
// int xt;        // address of the start the of code
// int nameLength;
// int name[nameLength];
int ForthOS::Create(int name, int imm_flag, int xt)
{
	int name_len = MemGet(name++);
	int entry_len = name_len + 3;
	int entry_here = LAST() - entry_len - 1;

	MemSet(LAST_ADDRESS, entry_here);
	MemSet(xt + 1, entry_here);
	MemSet(entry_here++, entry_len);
	MemSet(entry_here++, imm_flag);
	MemSet(entry_here++, xt);
	MemSet(entry_here++, name_len);
	for (int i = 0; i < name_len; i++)
	{
		MemSet(entry_here++, MemGet(name++));
	}

	return xt;
}

void ForthOS::BootStrap()
{
	int xt = 0;

	// Reset Here pointer
	// MemSet(HERE_ADDRESS, CORE_START);
	MemSet(LAST_ADDRESS, memorySize - 1);
	MemSet(MEMLAST_ADDRESS, memorySize - 1);
	MemSet(HERE_ADDRESS, CODE_START);
	MemSet(BASE_ADDRESS, 10); // Decimal
	MemSet(SOURCE_ADDRESS, INPUT_BUFFER);
	MemSet(STDIN_ADDRESS, (int)stdin);
	MemSet(STDOUT_ADDRESS, (int)stdout);
	MemSet(STDERR_ADDRESS, (int)stderr);

	MemSet(MemGet(LAST_ADDRESS), 0);

	// : (HERE) 1 ;
	xt = Compile(MODE_BOOT,
		I_LITERAL, HERE_ADDRESS,
		I_RETURN, COMPILE_BREAK);
	int xtHERE_ADDR = Create(StringToMem(INPUT_BUFFER, _T("(HERE)")), FLAG_IS_NORMAL, xt);

	// : HERE (HERE) @ ;
	xt = Compile(MODE_BOOT,
		xtHERE_ADDR, I_FETCH,
		I_RETURN, COMPILE_BREAK);
	int xtHERE = Create(StringToMem(INPUT_BUFFER, _T("HERE")), FLAG_IS_NORMAL, xt);

	// : .inc. DUP @ 1+ SWAP ! ;
	xt = Compile(MODE_BOOT,
		I_DUP, I_FETCH, I_ONEPLUS, I_SWAP, I_STORE,
		I_RETURN, COMPILE_BREAK);
	int xtInc = Create(StringToMem(INPUT_BUFFER, _T(".inc.")), FLAG_IS_NORMAL, xt);

	// : , HERE ! (HERE) .inc. ;
	xt = Compile(MODE_BOOT,
		xtHERE, I_STORE,
		I_LITERAL, HERE_ADDRESS,
		xtInc,
		I_RETURN, COMPILE_BREAK);
	xtComma = Create(StringToMem(INPUT_BUFFER, _T(",")), FLAG_IS_NORMAL, xt);

	// : COUNT DUP 1+ SWAP @ ;
	xt = Compile(MODE_BOOT,
		I_DUP, I_ONEPLUS, I_SWAP, I_FETCH,
		I_RETURN, COMPILE_BREAK);
	int xtCount = Create(StringToMem(INPUT_BUFFER, _T("COUNT")), FLAG_IS_NORMAL, xt);

	// : (SOURCE) 5 ;
	xt = Compile(MODE_BOOT,
		I_LITERAL, SOURCE_ADDRESS,
		I_RETURN, COMPILE_BREAK);
	int xtSourceAddr = Create(StringToMem(INPUT_BUFFER, _T("(SOURCE)")), FLAG_IS_NORMAL, xt);

	// : SOURCE (SOURCE) @ ;
	xt = Compile(MODE_BOOT,
		xtSourceAddr, I_FETCH,
		xtCount,
		I_RETURN, COMPILE_BREAK);
	int xtSource = Create(StringToMem(INPUT_BUFFER, _T("SOURCE")), FLAG_IS_NORMAL, xt);

	// .str+.
	xt = Compile(MODE_BOOT,
		I_DUP, xtInc, I_DUP, I_FETCH, I_PLUS, I_STORE,
		I_RETURN, COMPILE_BREAK);
	int xtStrPlus = Create(StringToMem(INPUT_BUFFER, _T(".str+.")), FLAG_IS_NORMAL, xt);

	// >IN
	xt = Compile(MODE_BOOT,
		I_LITERAL, TOIN_ADDRESS,
		I_RETURN, COMPILE_BREAK);
	int xtToIN = Create(StringToMem(INPUT_BUFFER, _T(">IN")), FLAG_IS_NORMAL, xt);

	// Built-In WORD: PAD
	xt = Compile(MODE_BOOT,
		I_LITERAL, PAD_ADDRESS,
		I_RETURN, COMPILE_BREAK);
	int xtPad = Create(StringToMem(INPUT_BUFFER, _T("PAD")), FLAG_IS_NORMAL, xt);

	// WORD: .word.
	xt = Compile(MODE_BOOT,
		// Skip leading blanks
		xtSource, xtToIN, I_FETCH,
		I_DO,
		/**/ I_DUP, I_I, I_PLUS, I_FETCH,
		/**/ I_DUP, I_LITERAL, 32, I_EQ,
		/**/ I_IF_RT, 5,
		/******/ I_DROP, xtToIN, xtInc,
		/**/ I_GOTO, 2,
		/******/ I_DROP, I_LEAVE,
		I_LOOP, I_DROP,

		// Initialize PAD
		I_LITERAL, 0, xtPad, I_STORE,

		// Copy to PAD until next space (or end of line)
		xtSource, xtToIN, I_FETCH,
		I_DO,
		/**/ I_DUP, I_I, I_PLUS, I_FETCH,
		/**/ I_DUP, I_LITERAL, 32, I_EQ,
		/**/ I_IF_RT, 4,
		/******/ I_DROP, I_LEAVE,
		/**/ I_GOTO, 4,
		/******/ xtPad, xtStrPlus,
		/******/ xtToIN, xtInc,
		I_LOOP, I_DROP,

		// All done
		I_RETURN, COMPILE_BREAK);
	int xtWord = Create(StringToMem(INPUT_BUFFER, _T(".word.")), FLAG_IS_NORMAL, xt);

	// : (LAST) 2 ;
	xt = Compile(MODE_BOOT,
		I_LITERAL, LAST_ADDRESS,
		I_RETURN, COMPILE_BREAK);
	int xtLASTAddr = Create(StringToMem(INPUT_BUFFER, _T("(LAST)")), FLAG_IS_NORMAL, xt);

	// : LAST (LAST) @ ;
	xt = Compile(MODE_BOOT,
		xtLASTAddr, I_FETCH,
		I_RETURN, COMPILE_BREAK);
	int xtLAST = Create(StringToMem(INPUT_BUFFER, _T("LAST")), FLAG_IS_NORMAL, xt);

	// : OVER 1 PICK ;
	xt = Compile(MODE_BOOT,
		I_LITERAL, 1, I_PICK,
		I_RETURN, COMPILE_BREAK);
	int xtOVER = Create(StringToMem(INPUT_BUFFER, _T("OVER")), FLAG_IS_NORMAL, xt);

	/*
	* : CREATE
	*     WORD
	*     LAST PAD @ 4 + - (LAST) ! // new LAST
	*     LAST PAD @ 3 + OVER ! 1+ // len
	*     0 OVER ! 1+ // not immediate
	*     HERE OVER ! 1+ // XT
	*     PAD @ OVER ! 1+ // LEN
	*     PAD COUNT >R SWAP R> 0 DO OVER I + @ OVER ! 1+ LOOP // chars in name
	*     DROP DROP ;
	*/
	xt = Compile(MODE_BOOT,
		xtWord,
		xtLAST, xtPad, I_FETCH, I_LITERAL, 4, I_PLUS, I_MINUS, I_LITERAL, LAST_ADDRESS, I_STORE,
		xtLAST, xtPad, I_FETCH, I_LITERAL, 3, I_PLUS, xtOVER, I_STORE, I_ONEPLUS,
		I_LITERAL, 0, xtOVER, I_STORE, I_ONEPLUS,
		xtHERE, xtOVER, I_STORE, I_ONEPLUS,
		xtPad, I_FETCH, xtOVER, I_STORE, I_ONEPLUS,
		xtPad, xtCount, I_TO_R, I_SWAP, I_R_FROM, I_LITERAL, 0,
		I_DO,
		/* */ xtOVER, I_I, I_PLUS, I_FETCH, xtOVER, I_STORE, I_ONEPLUS,
		I_LOOP,
		I_DROP, I_DROP,
		I_RETURN, COMPILE_BREAK);
	int xtCreate = Create(StringToMem(INPUT_BUFFER, _T("CREATE")), FLAG_IS_NORMAL, xt);

	// Built-in word ... : (COLON)
	xt = Compile(MODE_BOOT,
		xtCreate,
		I_LITERAL, I_DICTP, xtComma, xtLAST, xtComma,
		I_LITERAL, 1, I_LITERAL, STATE_ADDRESS, I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T(":")), FLAG_IS_NORMAL, xt);

	// Built-In WORD: @ (FETCH) ... : ! STATE @ IF <I_FETCH> , ELSE [ <I_FETCH> , ] THEN ; IMMEDIATE
	xt = Compile(MODE_BOOT,
		I_LITERAL, STATE_ADDRESS, I_FETCH,
		I_IF_RT, 4,
		I_LITERAL, I_FETCH,
		xtComma,
		I_RETURN,
		I_FETCH,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("@")), FLAG_IS_IMMEDIATE, xt);

	// : ! STATE @ IF <I_STORE> , ELSE [ <I_STORE> , ] THEN ; IMMEDIATE
	xt = Compile(MODE_BOOT,
		I_LITERAL, STATE_ADDRESS, I_FETCH,
		I_IF_RT, 4,
		I_LITERAL, I_STORE,
		xtComma,
		I_RETURN,
		I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("!")), FLAG_IS_IMMEDIATE, xt);

	// : IMMEDIATE 1 LAST 1+ ! ;
	xt = Compile(MODE_BOOT,
		I_LITERAL, 1, xtLAST, I_ONEPLUS, I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("IMMEDIATE")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: ; (SEMI-COLON) ... : ; <RETURN> , 0 STATE ! ; IMMEDIATE
	xt = Compile(MODE_BOOT,
		I_LITERAL, I_RETURN,
		xtComma,
		I_LITERAL, 0, I_LITERAL, STATE_ADDRESS, I_STORE,
		I_RETURN, COMPILE_BREAK);
	int addrSemiColon = Create(StringToMem(INPUT_BUFFER, _T(";")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: IF ( -- c-addr )
	// Check the STATE; if interpreting, 
	xt = Compile(MODE_BOOTIF,
		I_LITERAL, I_IF_RT,
		xtComma,
		xtHERE,
		I_LITERAL, 0,
		xtComma,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("IF")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: ELSE ( c-addr -- c-addr )
	xt = Compile(MODE_BOOTIF,
		I_LITERAL, I_GOTO,
		xtComma,
		xtHERE,
		I_LITERAL, 0,
		xtComma,
		xtHERE,
		I_ROT,
		I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("ELSE")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: THEN ( c-addr -- )
	xt = Compile(MODE_BOOTIF,
		xtHERE,
		I_SWAP,
		I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("THEN")), FLAG_IS_IMMEDIATE, xt);
}

int ForthOS::Compile(int mode, ...)
{
	int here = HERE();
	int xt = here;

	if ((mode == MODE_BOOT) || (mode == MODE_BOOTIF))
	{
		MemSet(here++, I_DICTP);
		MemSet(here++, LAST());
	}

	va_list code;
	va_start(code, mode);
	int val = va_arg(code, int);
	do
	{
		MemSet(here++, val);
		if ((mode == MODE_BOOT) && ((val == I_IF_RT) || (val == I_GOTO)))
		{
			int offset = va_arg(code, int);
			MemSet(here++, here + offset);
		}
		val = va_arg(code, int);
	} while (val != COMPILE_BREAK);

	va_end(code);
	MemSet(HERE_ADDRESS, here);
	return xt;
}
