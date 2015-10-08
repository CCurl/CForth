#include "stdafx.h"

#include <conio.h>
#include "ForthOS.h"

ForthOS::ForthOS(int memSize)
{
	SP = CSP = IP = RSP = 0;
	memorySize = memSize;
	input_fp = output_fp = NULL;

#ifdef _DEBUG
	memSize = MEMORY_SIZE;
	memory = dataSpace;
	memset(dataSpace, 0, memSize * sizeof(int));
#else
	memory = new int[memorySize];
	memset(memory, 0, memorySize * sizeof(int));
#endif

	memset(stack, 0, STACK_SIZE * sizeof(int));
	memset(cstack, 0, CSTACK_SIZE * sizeof(int));
	memset(rstack, 0, RSTACK_SIZE * sizeof(int));

	for (int i = 0; i <= I_RETURN; i++)
	{
		prims[i] = NULL;
	}

	prims[I_LITERAL] = &ForthOS::Literal;
	prims[I_STORE] = &ForthOS::Store;
	prims[I_FETCH] = &ForthOS::Fetch;
	prims[I_CALL] = &ForthOS::Call;
	prims[I_ONEPLUS] = &ForthOS::OnePlus;
	prims[I_ONEMINUS] = &ForthOS::OneMinus;
}

ForthOS::~ForthOS()
{
#ifndef _DEBUG
	free(memory);
#endif
}

int ForthOS::PUSH(int val) 
{
	if (SP >= STACK_SIZE)
	{
		SP = 0;
		MemSet(DEPTH_ADDRESS, SP);
		throw CString(_T("Stack full."));
	}
	stack[SP++] = val; 
	MemSet(DEPTH_ADDRESS, SP);
	return val;
}

int ForthOS::POP() // { return (SP > 0) ? stack[--SP] : 0; }
{
	if (SP < 1)
	{
		SP = 0;
		MemSet(DEPTH_ADDRESS, SP);
		throw CString(_T("Stack empty."));
	}
	MemSet(DEPTH_ADDRESS, --SP);
	return stack[SP];
}

int ForthOS::COMMA(int val)
{
	int here = MemGet(HERE_ADDRESS);
	MemSet(here++, val);
	MemSet(HERE_ADDRESS, here);
	return here;
}

int ForthOS::EXECUTE(int xt)
{
	IP = xt;
	bool err = false;

	try
	{
		IP = DoExecute();
	}
	catch (CString msg)
	{
		err = true;
		AppendOutput(msg);
	}
	catch (...)
	{
		err = true;
	}

	if (err)
	{
		SP = 0;
		RSP = 0;
		CSP = 0;
		IP = 0;
		MemSet(STATE_ADDRESS, 0);
		MemSet(IP, I_RETURN);
	}
	return IP;
}

int ForthOS::DoExecute()
{
	int addr = 0, arg1 = 0, arg2 = 0, val = 0;
	CStackEntry *cse = NULL;

	while (true)
	{
		INSTR_T instr = (INSTR_T)MemGet(IP++);
		if ((MemGet(DEBUGFLAG_ADDRESS) > 1) && (input_fp != NULL))
		{
			CString txt;
			DumpInstr(IP - 1, txt);
			AppendOutput(txt);
			AppendOutput(_T("\n"));
		}

		if (prims[instr])
		{
			(this->*prims[instr])();
			continue;
		}

		switch (instr)
		{
		case I_CALL:
			if ((MemGet(DEBUGFLAG_ADDRESS) == 1) && (input_fp != NULL))
			{
				CString txt;
				DumpInstr(IP - 1, txt);
				AppendOutput(txt);
				AppendOutput(_T("\n"));
			}
			addr = MemGet(IP++);
			RPUSH(IP);
			IP = addr;
			break;

		case I_RETURN:
			if ((MemGet(DEBUGFLAG_ADDRESS) == 1) && (input_fp != NULL))
			{
				CString txt;
				DumpInstr(IP - 1, txt);
				AppendOutput(txt);
				AppendOutput(_T("\n"));
			}
			if (RSP > 0)
				IP = RPOP();
			else
				return IP;
			break;

		case I_STORE:
			addr = POP();
			val = POP();
			MemSet(addr, val);
			break;

		case I_FETCH:
			addr = POP();
			PUSH(MemGet(addr));
			break;

		case I_LITERAL:
			PUSH(MemGet(IP++));
			break;

		case I_DROP:
			POP();
			break;

		case I_SWAP:
			arg2 = POP();
			arg1 = POP();
			PUSH(arg2);
			PUSH(arg1);
			break;

		case I_DUP:
			arg1 = POP();
			PUSH(arg1);
			PUSH(arg1);
			break;

		case I_ROT:
			arg2 = POP();
			arg1 = POP();
			val = POP();
			PUSH(arg1);
			PUSH(arg2);
			PUSH(val);
			break;

		case I_IF_RT:
			// If 0 (FALSE), then JUMP to (ELSE) or (THEN)
			addr = MemGet(IP++);
			val = POP();
			if (val == 0)
				IP = addr;
			break;

		case I_GOTO:
			IP = MemGet(IP);
			break;

		case I_DO:
			cse = CPUSH();
			cse->loop_start_addr = IP; // LOOP target address
			cse->loop_increment = 1; // Default increment
			cse->loop_index = POP(); // Index
			cse->loop_limit = POP(); // Limit
			break;

		case I_I:
			cse = CPOP(); // Push the index
			if (cse == NULL)
				throw "invalid DO construct.";
			CPUSH();
			PUSH(cse->loop_index);
			break;

		case I_LEAVE:
			cse = CPOP(); // Set Limit = Index
			if (cse == NULL)
				throw "invalid DO construct.";
			cse = CPUSH();
			cse->loop_limit = cse->loop_index;
			break;

		case I_PLUSLOOP:
			cse = CPOP(); // Index
			if (cse == NULL)
				throw "invalid DO construct.";
			CPUSH();
			cse->loop_increment = POP();
			// No break, fall through

		case I_LOOP:
			cse = CPOP(); // Index
			if (cse == NULL)
				throw "invalid DO construct.";
			if (cse->loop_increment == 0)
			{
				throw "invalid LOOP increment.";
			}
			cse->loop_index += cse->loop_increment;

			// Did it cross the limit?
			if (cse->Continue())
			{
				// Nope, go back to start
				CPUSH();
				IP = cse->loop_start_addr;
			}
			break;

		case I_ONEPLUS:
			PUSH(POP() + 1);
			break;

		case I_ONEMINUS:
			PUSH(POP() - 1);
			break;

		case I_PLUS:
			PUSH(POP() + POP());
			break;

		case I_MINUS:
			arg2 = POP();
			arg1 = POP();
			PUSH(arg1 - arg2);
			break;

		case I_MULT:
			PUSH(POP() * POP());
			break;

		case I_DIV:
			arg2 = POP();
			arg1 = POP();
			if (arg2 == 0)
				throw "divide by zero.";
			PUSH(arg1 / arg2);
			break;

		case I_EQ:
			PUSH((POP() == POP()) ? -1 : 0);
			break;

		case I_NEQ:
			PUSH((POP() != POP()) ? -1 : 0);
			break;

		case I_GT:
			PUSH((POP() < POP()) ? -1 : 0);
			break;

		case I_LT:
			PUSH((POP() > POP()) ? -1 : 0);
			break;

		case I_NOT:
			PUSH((POP() == 0) ? -1 : 0);
			break;

		case I_OVER:
			PUSH(1);
			// NO break, ... : OVER 1 PICK ;

		case I_PICK:
			arg1 = POP();
			arg2 = SP - arg1 - 1;
			PUSH(arg2 >= 0 ? stack[arg2] : 0);
			break;

		case I_DOT:
			val = POP();
			{
				CString x; 
				int base = MemGet(BASE_ADDRESS);
				if (base == 10)
					x.Format(_T("%d "), val);
				else if (base == 16)
					x.Format(_T("%X "), val);
				else
					FormatNumber(base, val, x);
				AppendOutput(x);
			}
			break;

		case I_EMIT:
			val = POP();
			AppendOutput((CHAR)val);
			break;

		case I_TO_R: // >R
			RPUSH(POP());
			break;

		case I_R_FROM: // R>
			PUSH(RPOP());
			break;

		case I_R_AT: // R@
			val = RPOP();
			PUSH(val);
			RPUSH(val);
			break;

		case I_DICTP: // 
			addr = MemGet(IP++);
			break;

		case I_FOPEN: 
			arg2 = POP(); // mode string
			arg1 = POP(); // filename
			{
				CString fn; MemToString(arg1, fn);
				CString mode; MemToString(arg2, mode);
				FILE *fp = NULL;
				CT2A asciiFn(fn);
				CT2A asciiMode(mode);
				fopen_s(&fp, asciiFn.m_psz, asciiMode.m_psz);
				if (fp == NULL)
				{
					PUSH(GetLastError());
				}
				PUSH((int)fp);
			}
			break;

		case I_FCLOSE:
			arg1 = POP(); // fp
			if (arg1 != 0)
			{
				fclose((FILE *)arg1);
			}
			break;

		case I_FGETC: // fp @ fgetc ( fp -- ch )
			arg1 = POP(); // fp
			if (arg1)
			{
				FILE *fp = (FILE *)arg1;
				if (fp == stdin)
					PUSH(_getch());
				else
					PUSH(fgetc(fp));
			}
			break;

		case I_FREAD: // returns number actually read on TOS
			arg1 = POP(); // fp
			arg2 = POP(); // requested length
			addr = POP(); // target-addr
			if (arg1 != 0)
			{
				char *buf = new char[arg2 + 1];
				FILE *fp = (FILE *)arg1; // fp
				int numRead = fread(buf, 1, arg2, fp);
				for (int i = 0; i < numRead; i++)
				{
					MemSet(addr++, buf[i]);
				}
				delete[] buf;
				PUSH(numRead);
			}
			else
			{
				PUSH(0);
			}
			break;

		case I_FWRITE: // data-buf COUNT fp @ fwrite
			val = POP(); // fp
			arg2 = POP(); // num
			arg1 = POP(); // addr
			if (val)
			{
				FILE *fp = (FILE *)val;
				for (int i = 0; i < arg2; i++)
				{
					fputc(MemGet(arg1++), fp);
				}
			}
		break;

		case 98: // data-buf COUNT fp @ fwrite
			addr = IP - 1;
			break;

		default:
			// Invalid instruction
			{
				   CString err; err.Format(_T("invalid instruction (%d) at %d."), instr, IP - 1);
				throw err;
			}
			break;
		}
	}

	return 0;
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
	while (val != COMPILE_BREAK)
	{
		MemSet(here++, val);
		if ((mode == MODE_BOOT) && ((val == I_IF_RT) || (val == I_GOTO)))
		{
			int offset = va_arg(code, int);
			MemSet(here++, here + offset);
		}
		val = va_arg(code, int);
	}
	va_end(code);
	MemSet(HERE_ADDRESS, here);
	return xt;
}

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
		I_CALL, xtHERE_ADDR, I_FETCH,
		I_RETURN, COMPILE_BREAK);
	int xtHERE = Create(StringToMem(INPUT_BUFFER, _T("HERE")), FLAG_IS_NORMAL, xt);

	// : .inc. DUP @ 1+ SWAP ! ;
	xt = Compile(MODE_BOOT,
		I_DUP, I_FETCH, I_ONEPLUS, I_SWAP, I_STORE,
		I_RETURN, COMPILE_BREAK);
	int xtInc = Create(StringToMem(INPUT_BUFFER, _T(".inc.")), FLAG_IS_NORMAL, xt);

	// : , HERE ! (HERE) .inc. ;
	xt = Compile(MODE_BOOT,
		I_CALL, xtHERE, I_STORE,
		I_LITERAL, HERE_ADDRESS, 
		I_CALL, xtInc,
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
		I_CALL, xtSourceAddr, I_FETCH,
		I_CALL, xtCount,
		I_RETURN, COMPILE_BREAK);
	int xtSource = Create(StringToMem(INPUT_BUFFER, _T("SOURCE")), FLAG_IS_NORMAL, xt);

	// .str+.
	xt = Compile(MODE_BOOT,
		I_DUP, I_CALL, xtInc, I_DUP, I_FETCH, I_PLUS, I_STORE,
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
		I_CALL, xtSource, I_CALL, xtToIN, I_FETCH,
		I_DO,
		/**/ I_DUP, I_I, I_PLUS, I_FETCH,
		/**/ I_DUP, I_LITERAL, 32, I_EQ,
		/**/ I_IF_RT, 7,
		/******/ I_DROP, I_CALL, xtToIN, I_CALL, xtInc,
		/**/ I_GOTO, 2,
		/******/ I_DROP, I_LEAVE,
		I_LOOP, I_DROP,

		// Initialize PAD
		I_LITERAL, 0, I_CALL, xtPad, I_STORE,

		// Copy to PAD until next space (or end of line)
		I_CALL, xtSource, I_CALL, xtToIN, I_FETCH,
		I_DO,
		/**/ I_DUP, I_I, I_PLUS, I_FETCH,
		/**/ I_DUP, I_LITERAL, 32, I_EQ,
		/**/ I_IF_RT, 4,
		/******/ I_DROP, I_LEAVE,
		/**/ I_GOTO, 8,
		/******/ I_CALL, xtPad, I_CALL, xtStrPlus,
		/******/ I_CALL, xtToIN, I_CALL, xtInc,
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
		I_CALL, xtLASTAddr, I_FETCH,
		I_RETURN, COMPILE_BREAK);
	int xtLAST = Create(StringToMem(INPUT_BUFFER, _T("LAST")), FLAG_IS_NORMAL, xt);

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
		I_CALL, xtWord,
		I_CALL, xtLAST, I_CALL, xtPad, I_FETCH, I_LITERAL, 4, I_PLUS, I_MINUS, I_LITERAL, LAST_ADDRESS, I_STORE,
		I_CALL, xtLAST, I_CALL, xtPad, I_FETCH, I_LITERAL, 3, I_PLUS, I_OVER, I_STORE, I_ONEPLUS,
		I_LITERAL, 0, I_OVER, I_STORE, I_ONEPLUS,
		I_CALL, xtHERE, I_OVER, I_STORE, I_ONEPLUS,
		I_CALL, xtPad, I_FETCH, I_OVER, I_STORE, I_ONEPLUS,
		I_CALL, xtPad, I_CALL, xtCount, I_TO_R, I_SWAP, I_R_FROM, I_LITERAL, 0,
		I_DO,
		/* */ I_OVER, I_I, I_PLUS, I_FETCH, I_OVER, I_STORE, I_ONEPLUS,
		I_LOOP,
		I_DROP, I_DROP,
		I_RETURN, COMPILE_BREAK);
	int xtCreate = Create(StringToMem(INPUT_BUFFER, _T("CREATE")), FLAG_IS_NORMAL, xt);

	// Built-in word ... : (COLON)
	xt = Compile(MODE_BOOT,
		I_CALL, xtCreate,
		I_LITERAL, I_DICTP, I_CALL, xtComma, 
		I_CALL, xtLAST, I_CALL, xtComma,
		I_LITERAL, 1, I_LITERAL, STATE_ADDRESS, I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T(":")), FLAG_IS_NORMAL, xt);

	// Built-In WORD: @ (FETCH) ... : ! STATE @ IF <I_FETCH> , ELSE [ <I_FETCH> , ] THEN ; IMMEDIATE
	xt = Compile(MODE_BOOT,
		I_LITERAL, STATE_ADDRESS, I_FETCH,
		I_IF_RT, 5,
		I_LITERAL, I_FETCH,
		I_CALL, xtComma,
		I_RETURN,
		I_FETCH,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("@")), FLAG_IS_IMMEDIATE, xt);

	// : ! STATE @ IF <I_STORE> , ELSE [ <I_STORE> , ] THEN ; IMMEDIATE
	xt = Compile(MODE_BOOT,
		I_LITERAL, STATE_ADDRESS, I_FETCH,
		I_IF_RT, 5,
		I_LITERAL, I_STORE,
		I_CALL, xtComma,
		I_RETURN,
		I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("!")), FLAG_IS_IMMEDIATE, xt);

	// : IMMEDIATE 1 LAST 1+ ! ;
	xt = Compile(MODE_BOOT,
		I_LITERAL, 1, I_CALL, xtLAST, I_ONEPLUS, I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("IMMEDIATE")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: ; (SEMI-COLON) ... : ; <RETURN> , 0 STATE ! ; IMMEDIATE
	xt = Compile(MODE_BOOT,
		I_LITERAL, I_RETURN,
		I_CALL, xtComma,
		I_LITERAL, 0, I_LITERAL, STATE_ADDRESS, I_STORE,
		I_RETURN, COMPILE_BREAK);
	int addrSemiColon = Create(StringToMem(INPUT_BUFFER, _T(";")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: IF ( -- c-addr )
	// Check the STATE; if interpreting, 
	xt = Compile(MODE_BOOTIF,
		I_LITERAL, I_IF_RT,
		I_CALL, xtComma,
		I_CALL, xtHERE,
		I_LITERAL, 0,
		I_CALL, xtComma,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("IF")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: ELSE ( c-addr -- c-addr )
	xt = Compile(MODE_BOOTIF,
		I_LITERAL, I_GOTO,
		I_CALL, xtComma,
		I_CALL, xtHERE,
		I_LITERAL, 0,
		I_CALL, xtComma,
		I_CALL, xtHERE,
		I_ROT,
		I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("ELSE")), FLAG_IS_IMMEDIATE, xt);

	// Built-In WORD: THEN ( c-addr -- )
	xt = Compile(MODE_BOOTIF,
		I_CALL, xtHERE,
		I_SWAP,
		I_STORE,
		I_RETURN, COMPILE_BREAK);
	Create(StringToMem(INPUT_BUFFER, _T("THEN")), FLAG_IS_IMMEDIATE, xt);
}

void ForthOS::GetWordName(CString& ret, int addr)
{
	ret.Empty();
	addr += 3;
	int len = MemGet(addr++);
	for (int i = 0; i < len; i++)
	{
		CHAR c = MemGet(addr++);
		ret.AppendChar(c);
	}
}

void ForthOS::ResolveCall(CString& ret, int addr)
{
	addr += 3;
	int len = MemGet(addr++);
	for (int i = 0; i < len; i++)
	{
		CHAR c = MemGet(addr++);
		ret.AppendChar(c);
	}
}

int ForthOS::DumpInstr(int xt, CString& ret)
{
	CString tmp;
	int addr = 0, val;
	ret.Format(_T("%04d: "), xt);

	INSTR_T instr = (INSTR_T)MemGet(xt++);
	switch (instr)
	{
	case I_CALL:
		addr = MemGet(xt++);
		ret.AppendFormat(_T("CALL %04d"), addr);
		addr = MemGet(addr + 1);
		GetWordName(tmp, addr);
		ret.Append(tmp);
		break;

	case I_RETURN:
		ret.AppendFormat(_T("I_RETURN"));
		break;

	case I_STORE:
		ret.AppendFormat(_T("I_STORE"));
		break;

	case I_FETCH:
		ret.AppendFormat(_T("I_FETCH"));
		break;

	case I_LITERAL:
		val = MemGet(xt++);
		ret.AppendFormat(_T("I_LITERAL %d"), val);
		break;

	case I_DROP:
		ret.AppendFormat(_T("I_DROP"));
		break;

	case I_SWAP:
		ret.AppendFormat(_T("I_SWAP"));
		break;

	case I_DUP:
		ret.AppendFormat(_T("I_DUP"));
		break;

	case I_ROT:
		ret.AppendFormat(_T("I_ROT"));
		break;

		// These are obsolete ...
		// case I_IF:
		// case I_ELSE:
		// case I_THEN:

	case I_IF_RT:
		ret.AppendFormat(_T("I_IF_RT"));
		// If FALSE, JUMP to ELSE or THEN
		addr = MemGet(xt++);
		ret.AppendFormat(_T(" (0 = IF GOTO %d)"), addr);
		break;

	case I_GOTO:
		addr = MemGet(xt++);
		ret.AppendFormat(_T("I_GOTO %04d"), addr);
		break;

	case I_DO:
		ret.AppendFormat(_T("I_DO"));
		break;

	case I_I:
		ret.AppendFormat(_T("I_I"));
		break;

	case I_LEAVE:
		ret.AppendFormat(_T("I_LEAVE"));
		break;

	case I_PLUSLOOP:
		ret.AppendFormat(_T("I_PLUSLOOP"));
		break;

	case I_LOOP:
		ret.AppendFormat(_T("I_LOOP"));
		break;

	case I_ONEPLUS:
		ret.AppendFormat(_T("I_ONEPLUS"));
		break;

	case I_ONEMINUS:
		ret.AppendFormat(_T("I_ONEMINUS"));
		break;

	case I_PLUS:
		ret.AppendFormat(_T("I_PLUS"));
		break;

	case I_MINUS:
		ret.AppendFormat(_T("I_MINUS"));
		break;

	case I_MULT:
		ret.AppendFormat(_T("I_MULT"));
		break;

	case I_DIV:
		ret.AppendFormat(_T("I_DIV"));
		break;

	case I_EQ:
		ret.AppendFormat(_T("I_EQ"));
		break;

	case I_NOT:
		ret.AppendFormat(_T("I_NOT"));
		break;

	case I_NEQ:
		ret.AppendFormat(_T("I_NEQ"));
		break;

	case I_GT:
		ret.AppendFormat(_T("I_GT"));
		break;

	case I_LT:
		ret.AppendFormat(_T("I_LT"));
		break;

	case I_OVER:
		ret.AppendFormat(_T("I_OVER"));
		break;

	case I_PICK:
		ret.AppendFormat(_T("I_PICK"));
		break;

	case I_DOT:
		ret.AppendFormat(_T("I_DOT"));
		break;

	case I_EMIT:
		ret.AppendFormat(_T("I_EMIT"));
		break;

	case I_TO_R:
		ret.AppendFormat(_T("I_TO_R (>R)"));
		break;

	case I_R_FROM:
		ret.AppendFormat(_T("I_R_FROM (R>)"));
		break;

	case I_R_AT:
		ret.AppendFormat(_T("I_R_AT (R@)"));
		break;

	case I_DICTP:
		addr = MemGet(xt++);
		ret.AppendFormat(_T("I_DICTP (%04d)"), addr);
		GetWordName(tmp, addr);
		ResolveCall(ret, addr);
		break;

	case I_FOPEN:
		ret.AppendFormat(_T("I_FOPEN"));
		break;

	case I_FCLOSE:
		ret.AppendFormat(_T("I_FCLOSE"));
		break;

	case I_FREAD:
		ret.AppendFormat(_T("I_FREAD"));
		break;

	case I_FWRITE:
		ret.AppendFormat(_T("I_FWRITE"));
		break;

	default:
		// Not an instruction
		ret.AppendFormat(_T(".INT %d"), instr);
		break;
	}
	return xt;
}

int ForthOS::DumpInstr_ASM(int xt, CString& ret)
{
	int addr = 0, val = 0;
	CString tmp, ldz = _T("\r\n        ");
	ret.Format(_T("a%05d: "), xt);

	INSTR_T instr = (INSTR_T)MemGet(xt++);
	switch (instr)
	{
	case I_CALL:
		addr = MemGet(xt++);
		ResolveCall(tmp, MemGet(addr + 1));
		ret.AppendFormat(_T("call a%05d ; %s"), addr, tmp);
		break;

	case I_RETURN:
		ret.AppendFormat(_T("ret"));
		break;

	case I_STORE:
		ret.AppendFormat(_T("call dsStore"));
		break;

	case I_FETCH:
		ret.AppendFormat(_T("call dsFetch"));
		break;

	case I_LITERAL:
		val = MemGet(xt++);
		ret.AppendFormat(_T("mov edx, %d ; LITERAL%scall dsPush"), val, ldz);
		break;

	case I_DROP:
		ret.AppendFormat(_T("call dsPop ; DROP"));
		break;

	case I_SWAP:
		ret.AppendFormat(_T("call dsSwap ; SWAP"), ldz, ldz);
		break;

	case I_DUP:
		ret.AppendFormat(_T("call dsDup ; DUP"), val, ldz);
		break;

	case I_ROT:
		ret.AppendFormat(_T("call dsRot ; ROT"));
		break;

	// These are obsolete ...
	// case I_IF:
	// case I_ELSE:
	// case I_THEN:

	case I_IF_RT:
		ret.AppendFormat(_T("NOP"));
		// If FALSE, JUMP to ELSE or THEN
		addr = MemGet(xt++);
		ret.AppendFormat(_T("; for now ... (0 = IF GOTO %d)"), addr);
		break;

	case I_GOTO:
		addr = MemGet(xt++);
		ret.AppendFormat(_T("jmp a%05d ; GOTO"), addr);
		break;

	case I_DO:
		ret.AppendFormat(_T("call fDo ; DO"));
		break;

	case I_I:
		ret.AppendFormat(_T("call fDo_I ; I"));
		break;

	case I_LEAVE:
		ret.AppendFormat(_T("call fDo_Leave ; LEAVE"));
		break;

	case I_PLUSLOOP:
		ret.AppendFormat(_T("call fDo_PlusLoop ; +LOOP"));
		break;

	case I_LOOP:
		ret.AppendFormat(_T("call fDo_Loop ; LOOP"));
		break;

	case I_ONEPLUS:
		ret.AppendFormat(_T("call fOnePlus ; 1+"));
		break;

	case I_ONEMINUS:
		ret.AppendFormat(_T("call fOneMinus ; 1-"));
		break;

	case I_PLUS:
		ret.AppendFormat(_T("call fPlus ; +"));
		break;

	case I_MINUS:
		ret.AppendFormat(_T("call fMinus ; -"));
		break;

	case I_MULT:
		ret.AppendFormat(_T("call fMult ; *"));
		break;

	case I_DIV:
		ret.AppendFormat(_T("call fDivide ; /"));
		break;

	case I_EQ:
		ret.AppendFormat(_T("call fEquals ; ="));
		break;

	case I_NOT:
		ret.AppendFormat(_T("call fNot ; NOT"));
		break;

	case I_NEQ:
		ret.AppendFormat(_T("call fNotEquals ; <>"));
		break;

	case I_GT:
		ret.AppendFormat(_T("call fGT ; >"));
		break;

	case I_LT:
		ret.AppendFormat(_T("call fLT ; <"));
		break;

	case I_OVER:
		ret.AppendFormat(_T("call fOver ; OVER"));
		break;

	case I_PICK:
		ret.AppendFormat(_T("call fPick ; PICK"));
		break;

	case I_DOT:
		ret.AppendFormat(_T("call fDot ; ."));
		break;

	case I_EMIT:
		ret.AppendFormat(_T("call fEmit ; EMIT"));
		break;
		
	case I_TO_R:
		ret.AppendFormat(_T("call fToR ; >R"));
		break;

	case I_R_FROM:
		ret.AppendFormat(_T("call fRFrom ; R>"));
		break;

	case I_R_AT:
		ret.AppendFormat(_T("call fRAt ; R@"));
		break;

	case I_DICTP:
		ret.Empty();
		if (!currentWordName.IsEmpty())
		{
			ret.AppendFormat(_T("; %s ENDP\r\n"), currentWordName);
		}
		addr = MemGet(xt++);
		GetWordName(tmp, addr);
		currentWordName = tmp;
		ret.AppendFormat(_T("\r\n; %s PROC\r\n"), currentWordName);
		ret.AppendFormat(_T("a%05d: "), xt-2);
		ret.AppendFormat(_T("mov eax, %d ; DICTP (%04d)"), addr, addr);
		break;

	case I_FOPEN:
		ret.AppendFormat(_T("call fFileOpen ; FOPEN"));
		break;

	case I_FCLOSE:
		ret.AppendFormat(_T("call fFileClose ; FCLOSE"));
		break;

	case I_FREAD:
		ret.AppendFormat(_T("call fFileRead ; FREAD"));
		break;

	case I_FWRITE:
		ret.AppendFormat(_T("call fFileWrite ; FWRITE"));
		break;

	default:
		// Not an instruction
		ret.AppendFormat(_T("NOP ; %d"), instr);
		break;
	}
	return xt;
}

int ForthOS::See(int xt, CString& ret, int stopHere)
{
	CString def;
	ret = _T("\r\n");

	while (xt < stopHere)
	{
		xt = DumpInstr_ASM(xt, def);
		ret.AppendFormat(_T("%s\r\n"), def);
	}
	return xt;
}

void ForthOS::DumpStack(CString& ret)
{
	ret.Format(_T("SP: %d  -  "), SP);
	for (int addr = 0; addr < SP; addr++)
	{
		ret.AppendFormat(_T("%d "), stack[addr]);
	}
}

void ForthOS::Dump(CString& ret)
{
	ret = _T("; ");

	for (int i = 0; i <= 15; i++)
	{
		if ((i % 5 == 0) && (i > 0))
			ret.Append(_T("\r\n; "));
		ret.AppendFormat(_T("[%02d: %05d] "), i, MemGet(i));
	}
	ret.Append(_T("\r\n"));

	int entryStart = MemGet(LAST_ADDRESS);
	int entrySize = MemGet(entryStart);
	while (entrySize != 0)
	{
		CString name;
		int addr = entryStart;
		entrySize = MemGet(addr++);
		if (entrySize == 0)
			continue; 
		int flags = MemGet(addr++);
		int isImmediate = (flags & 0x0001) != 0;
		int xt = MemGet(addr++);
		int nameLen = MemGet(addr++);
		if (addr == 8060)
			int sss = 4;
		for (int i = 0; i < nameLen; i++)
		{
			name.AppendChar((CHAR)MemGet(addr + i));
		}
		ret.AppendFormat(_T("\r\n; %04d, %2d, %d, %d, %d, \"%s\""), entryStart, entrySize, isImmediate, xt, nameLen, name);
		entryStart += (entrySize+1);
	}

	CString code;
	See(CODE_START, code, HERE());

	code.AppendFormat(_T("; %s ENDP\r\n"), currentWordName);
	currentWordName.Empty();
	ret.Append(code);
}

void ForthOS::AppendOutput(LPCTSTR text)
{
	if (output_fp != NULL)
		
		fputws(text, output_fp);
	else
		output.Append(text);
}

void ForthOS::AppendOutput(CHAR ch)
{
	if (output_fp != NULL)
		fputc(ch, output_fp);
	else
		output.AppendChar(ch);
}

int to_upper(int c)
{
	if (('a' <= c) && (c <= 'z'))
		return c - 32;
	return c;
}

int ForthOS::CompareStrings(int addr1, int addr2)
{
	int len1 = MemGet(addr1);
	int len2 = MemGet(addr2);
	if (len1 == len2)
	{
		for (int i = 1; i <= len1; i++)
		{
			int c1 = MemGet(addr1 + i);
			int c2 = MemGet(addr2 + i);
			if (to_upper(c1) != to_upper(c2))
				return 0;
		}
		return 1;
	}
	return 0;
}

// Returns start of code for the word, 0 if not found
int ForthOS::TICK(int nameAddr, bool& isImmediate)
{
	int entryStart = MemGet(LAST_ADDRESS);
	int entrySize = MemGet(entryStart);
	int memMax = MemGet(MEMLAST_ADDRESS);
	while (entryStart < memMax)
	{
		int addr = entryStart;
		entrySize = MemGet(addr++);
		if (entrySize == 0)
			continue;

		int flags = MemGet(addr++);
		isImmediate = (flags & 0x0001) != 0;
		int xt = MemGet(addr++);
		int wordName = addr;
		if (CompareStrings(addr, nameAddr))
		{
			return xt;
		}
		entryStart += (entrySize + 1);
	}
	return 0;
}

bool ForthOS::StringIsChar(int addr, int& val)
{
	val = 0;
	int len = MemGet(addr);
	if ((len == 3) && (MemGet(addr + 1) == '\'') && (MemGet(addr + 3) == '\''))
	{
		CHAR c = MemGet(addr + 2);
		val = c;
		return true;
	}
	return false;
}

bool ForthOS::StringIsNumber(CString str, int& val)
{
	CT2A ascii(str);
	char *end = NULL;
	int base = MemGet(BASE_ADDRESS);
	val = strtol(ascii.m_psz, &end, base);
	return (*end) == NULL;
}

bool ForthOS::StringIsNumber(int addr, int& val)
{
	val = 0;
	int len = MemGet(addr);
	bool isNegative = false;
	for (int i = 1; i <= len; i++)
	{
		CHAR c = MemGet(addr + i);
		if ((i == 1) && (c == '-'))
		{
			isNegative = true;
			continue;
		}
		if (c < '0' || c > '9')
		{
			return false;
		}
		int t = c - '0';
		val = (val * 10) + t;
	}
	if (isNegative)
		val = -val;
	return true;
}

bool ForthOS::StringIsString(int addr)
{
	int len = MemGet(addr);

	return ((len > 1) && (MemGet(addr + 1) == '"') && (MemGet(addr + len) == '"'));
}

int ForthOS::StringToMem(int addr, LPCTSTR val)
{
	int len = 0;
	while (*val)
	{
		++len;
		MemSet(addr + len, *(val++));
	}
	MemSet(addr, len);
	MemSet(addr + len + 1, ' '); // For now, this is needed by CREATE
	return addr;
}

int ForthOS::MemToString(int addr, CString& val)
{
	int len = MemGet(addr);
	val.Empty();
	for (int i = 1; i <= len; i++)
	{
		CHAR c = MemGet(addr + i);
		val += c;
	}
	return len;
}

int ForthOS::GetNextWord(int PAD)
{
	int toIN = MemGet(TOIN_ADDRESS);
	int source = MemGet(SOURCE_ADDRESS);
	int len = MemGet(source++);
	CHAR c;

	// 0 PAD !
	MemSet(PAD, 0);

	// Skip leading whitespace
	while (toIN < len)
	{
		c = MemGet(source + toIN);
		if (iswspace(c))
		{
			MemSet(TOIN_ADDRESS, ++toIN);
		}
		else
		{
			break;
		}
	}

	// Go until the next whitespace or the end of the line
	while (toIN < len)
	{
		// (source) @ 1+ >IN @ + @ .is-whitespace
		// IF LEAVE 
		c = MemGet(source + toIN);
		if (iswspace(c))
		{
			break;
		}
		else
		{
			// ELSE PAD @ 1+ SWAP ! PAD DUP @ + !
			MemSet(PAD, MemGet(PAD) + 1);
			MemSet(PAD + MemGet(PAD), c);

			// >IN @ 1+ >IN !
			MemSet(TOIN_ADDRESS, ++toIN);
		}
		//THEN
	}
	return PAD;
}

int ForthOS::ParseInput(LPCTSTR commands)
{
	inputStream = commands;
	CString name = commands;

	if (name == _T("load"))
	{
		Load();
		return 0;
	}

	if (name == _T("save"))
	{
		Save();
		return 0;
	}

	if (name == _T(".bs"))
	{
		BootStrap_FILE();
		return 0;
	}

	int source = StringToMem(MemGet(SOURCE_ADDRESS), inputStream);

	// 0 >IN !
	int toIN = 0; 
	MemSet(TOIN_ADDRESS, toIN);

	int len = MemGet(source++);
	int PAD = source + len + 10;
	int err = false;

	try
	{
		while (toIN < len)
		{
			GetNextWord(PAD);
			ExecuteWord(PAD);
			source = MemGet(SOURCE_ADDRESS);
			len = MemGet(source++);
			toIN = MemGet(TOIN_ADDRESS);
		}
	}
	catch (CString msg)
	{
		err = true;
		AppendOutput(msg);
	}
	catch (...)
	{
		err = true;
	}

	if (err)
	{
		SP = 0;
		RSP = 0;
		CSP = 0;
		MemSet(STATE_ADDRESS, 0);
	}

	// at end of the inputStream
	return toIN;
}

FILE *fileExists(LPCTSTR fileNme)
{
	FILE *fp = NULL;
	CT2A ascii(fileNme);
	fopen_s(&fp, ascii.m_psz, "rt");
	return fp;
}

void ForthOS::BootStrap_FILE()
{
	//FILE *fp = fileExists(_T("BootStrap.4th"));
	FILE *fp = fileExists(_T("ForthOS.4th"));

	if (fp == NULL)
	{
		//fp = fileExists(_T("..\\BootStrap.4th"));
		fp = fileExists(_T("..\\ForthOS.4th"));
	}

	if (fp)
	{
		Include(fp);
		fclose(fp);
	}
}

bool ForthOS::Include(int pad)
{
	bool ret = false;
	CString fileName;
	MemToString(pad, fileName);
	if (fileName.Right(4).CompareNoCase(_T(".4th")) != 0)
	{
		fileName.Append(_T(".4th"));
	}

	FILE *fp = fileExists(fileName);
	if (fp == NULL)
	{
		CString fn2; 
		fn2.Format(_T("..\\%s"), fileName);
		fp = fileExists(fn2);
	}
	if (fp)
	{
		ret = Include(fp);
		fclose(fp);
	}
	return ret;
}

bool ForthOS::Include(FILE *fp)
{
	if (fp == NULL)
	{
		return false;
	}

	CString line;
	int i = 0;
	DWORD numRead = 0;
	char buf[256];
	while (fgets(buf, sizeof(buf), fp) == buf)
	{
		line = buf;
		line.TrimRight();
		if (line.CompareNoCase(_T("break;")) == 0)
		{
			break;
		}
		else
		{
			ParseInput(line);
		}
	}
	return true;
}

void ForthOS::Load()
{
	FILE *fp = NULL;
	fopen_s(&fp, "session.txt", "rt");
	if (fp)
	{
		int i = 0;
		char buf[64];
		while (fgets(buf, sizeof(buf), fp) == buf)
		{
			int num = atoi(buf);
			MemSet(i++, num);
		}
		fclose(fp);
		AppendOutput(_T("Loaded."));
	}
}

void ForthOS::Save()
{
	FILE *fp = NULL;
	fopen_s(&fp, "session.txt", "wt");
	if (fp)
	{
		for (int i = 0; i < memorySize; i++)
		{
			fprintf(fp, "%d\n", MemGet(i));
		}
		fclose(fp);
		AppendOutput(_T("Saved."));
	}
}

// Primitives
void ForthOS::ExecuteWord(int PAD)
{
	CString name;
	MemToString(PAD, name);

	if (name.CompareNoCase(_T("include")) == 0)
	{
		GetNextWord(PAD);
		Include(PAD);
		return;
	}

	if (MemGet(PAD) == 0)
		return;

	bool isImmediate = false;
	int xt = TICK(PAD, isImmediate);

	if (xt != 0)
	{
		if (MemGet(STATE_ADDRESS) == STATE_COMPILING) // Compiling?
		{
			if (isImmediate) // Execute it NOW!
			{
				EXECUTE(xt);
			}
			else // Build it into the word's definition
			{
				Compile(MODE_RUN, I_CALL, xt, COMPILE_BREAK);
			}
		}
		else
		{
			EXECUTE(xt);
		}
		return;
	}

	int num = 0;
	if (StringIsNumber(name, num))
	{
		if (MemGet(STATE_ADDRESS) == STATE_COMPILING) // Compiling?
		{
			Compile(MODE_RUN, I_LITERAL, num, COMPILE_BREAK);
		}
		else
		{
			PUSH(num);
		}
		return;
	}

	if (StringIsChar(PAD, num))
	{
		if (MemGet(STATE_ADDRESS) == STATE_COMPILING) // Compiling?
		{
			Compile(MODE_RUN, I_LITERAL, num, COMPILE_BREAK);
		}
		else
		{
			PUSH(num);
		}
		return;
	}

	// QUIT logic
	CString msg; msg.Format(_T("'%s' not found."), name);
	throw msg;
}

void ForthOS::FormatNumber(int base, int num, CString& x)
{
	if (num == 0)
	{
		x = " 0";
		return;
	}

	if ((base < 2) || (base > 16))
		throw CString(_T("invalid BASE."));

	char *chars = "0123456789ABCDEF";
	char output[64], *cp = &output[63];
	*cp = (char)NULL;
	int q = num, r;
	while (q != 0)
	{
		r = q % base;
		q = q / base;
		*(--cp) = chars[r];
	}
	*(--cp) = ' ';
	x = cp;
}
