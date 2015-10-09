#include "stdafx.h"

#include <conio.h>
#include "ForthOS.h"

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

CString instrNames[I_FGETC+1] = 
{
	_T(""),
	_T("I_FETCH"),
	_T("I_STORE"),
	_T("I_LITERAL"),
	_T("I_DUP"),
	_T("I_SWAP"),
	_T("I_DROP"),
	_T("I_PICK"),
	_T("I_ROT"),
	_T("I_ONEPLUS"),
	_T("I_PLUS"),
	_T("I_MINUS"),
	_T("I_MULT"),
	_T("I_DIV"),
	_T("I_EQ"),
	_T("(obsolete NEQ)"),
	_T("I_LT"),
	_T("I_GT"),
	_T("I_TO_R"),
	_T("I_R_FROM"),
	_T("I_R_AT"),
	_T("I_IF_RT"),
	_T("I_DO"),
	_T("I_I"),
	_T("I_LEAVE"),
	_T("I_LOOP"),
	_T("I_PLUSLOOP"),
	_T("I_GOTO"),
	_T("(obsolete DOT)"),
	_T("(obsolete TYPE)"),
	_T("(obsolete CALL)"),
	_T("(obsolete OVER)"),
	_T("I_EMIT"),
	_T("I_DICTP"),
	_T("(obsolete -1)"),
	_T("(obsolete 0=)"),
	_T("I_FOPEN"),
	_T("I_FCLOSE"),
	_T("I_FREAD"),
	_T("I_FWRITE"),
	_T("I_FGETC"),
};

int ForthOS::DumpInstr(int xt, CString& ret)
{
	CString tmp;
	int addr = xt, val;

	INSTR_T instr = (INSTR_T)MemGet(xt++);
	ret.Format(_T("%04d: %02d "), addr, instr);
	if ((instr > 0) && (instr <= I_FGETC) && (!instrNames[instr].IsEmpty()))
	{
		ret.AppendFormat(_T("     ; %s"), instrNames[instr]);
	}

	switch (instr)
	{
	case I_LITERAL:
		val = MemGet(xt++);
		ret.Format(_T("%04d: %02d "), addr, instr);
		ret.AppendFormat(_T("%4d ; %s (PUSH %d)"), val, instrNames[instr], val);
		//ret.AppendFormat(_T("%d"), val);
		break;

	case I_IF_RT:
		ret.Format(_T("%04d: %02d "), addr, instr);
		val = MemGet(xt++);
		ret.AppendFormat(_T("%4d ; %s"), val, instrNames[instr]);
		ret.AppendFormat(_T(" (0 = IF GOTO %d)"), val);
		break;

	case I_GOTO:
		ret.Format(_T("%04d: %02d "), addr, instr);
		addr = MemGet(xt++);
		ret.AppendFormat(_T("%4d ; %s "), addr, instrNames[instr]);
		ret.AppendFormat(_T("%04d"), addr);
		break;

	case I_RETURN:
		ret.AppendFormat(_T("     ; %s"), _T("RETURN"));
		break;

	case I_DICTP:
		addr = MemGet(xt++);
		ResolveCall(tmp, addr);
		ret.Format(_T("\r\n%04d: %2d %04d ; %s \"%s\""), xt-2, instr, addr, instrNames[instr], tmp);
		break;

	default:
		if ((instr >= CODE_START) && (instr < HERE()))
		{
			// ret.AppendFormat(_T(""), instr);
			addr = MemGet(instr+1);
			GetWordName(tmp, addr);
			//ResolveCall(ret, addr);
			ret.AppendFormat(_T("   ; %s"), tmp);
			//ret.Append(tmp);
		}
		else
		{
			if ((instr < 1) || (instr > I_FGETC) && (instr != I_RETURN))
			{
				ret.Format(_T("%04d: %-4d "), addr, instr);
				ret.AppendFormat(_T("   ; .INT %d"), instr);
			}
		}
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
	//case I_CALL:
	//	addr = MemGet(xt++);
	//	ResolveCall(tmp, MemGet(addr + 1));
	//	ret.AppendFormat(_T("call a%05d ; %s"), addr, tmp);
	//	break;

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

	case I_GT:
		ret.AppendFormat(_T("call fGT ; >"));
		break;

	case I_LT:
		ret.AppendFormat(_T("call fLT ; <"));
		break;

	case I_PICK:
		ret.AppendFormat(_T("call fPick ; PICK"));
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
		xt = DumpInstr(xt, def);
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

FILE *fileExists(LPCTSTR fileNme)
{
	FILE *fp = NULL;
	CT2A ascii(fileNme);
	fopen_s(&fp, ascii.m_psz, "rt");
	return fp;
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
