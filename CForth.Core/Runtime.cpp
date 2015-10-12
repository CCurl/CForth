#include "stdafx.h"

#include <conio.h>
#include "ForthOS.h"

FILE *fileExists(LPCTSTR fn);

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
	while (true)
	{
		if ((MemGet(DEBUGFLAG_ADDRESS) > 1) && (input_fp != NULL))
		{
			CString txt;
			DumpInstr(IP, txt);
			AppendOutput(txt);
			AppendOutput(_T("\n"));
		}

		INSTR_T instr = (INSTR_T)MemGet(IP++);
		if ((instr >= I_FETCH) && (instr <= I_FGETC) && (prims[instr]))
		{
			(this->*prims[instr])();
			continue;
		}

		if (instr == I_BREAK)
		{
			continue;
		}

		if (instr == I_RETURN)
		{
			if ((MemGet(DEBUGFLAG_ADDRESS) == 1) && (input_fp != NULL))
			{
				CString txt;
				DumpInstr(IP - 1, txt);
				AppendOutput(txt);
				AppendOutput(_T("\n"));
			}
			if (RSP > 0)
			{
				IP = RPOP();
			}
			else
			{
				return IP;
			}
		}
		else
		{
			if ((instr >= CODE_START) && (instr < HERE()))
			{
				if ((MemGet(DEBUGFLAG_ADDRESS) == 1) && (output_fp != NULL))
				{
					CString txt;
					DumpInstr(IP - 1, txt);
					AppendOutput(txt);
					AppendOutput(_T("\n"));
				}
				RPUSH(IP);
				IP = instr;
			}
			else
			{
				// Invalid instruction
				CString err; err.Format(_T("invalid instruction (%d) at %d."), instr, IP - 1);
				throw err;
			}
		}
	}

	return 0;
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

bool ForthOS::Include(int pad)
{
	bool ret = false;
	CString fileName;

	MemToString(pad, fileName);
	FILE *fp = fileExists(fileName);

	if (fp == NULL)
	{
		if (fileName.Right(4).CompareNoCase(_T(".4th")) != 0)
		{
			fileName.Append(_T(".4th"));
			fp = fileExists(fileName);
		}
	}

	if (fp != NULL)
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
				//Compile(MODE_RUN, I_CALL, xt, COMPILE_BREAK);
				Compile(MODE_RUN, xt, COMPILE_BREAK);
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

void ForthOS::FETCH()
{
	PUSH(MemGet(POP()));
}

// pops a val off the specified stack
void ForthOS::SPOP()
{
	int stack = POP();
	int depth = MemGet(stack);
	if (depth > 0)
	{
		int val = MemGet(stack + depth);
		PUSH(val);
		MemSet(stack, depth - 1);
	}
}

// pushes a val onto the specified stack
void ForthOS::SPUSH()
{
	int stack = POP();
	int val = POP();
	int depth = MemGet(stack)+1;
	MemSet(stack + depth, val);
	MemSet(stack, depth);
}

void ForthOS::STORE()
{
	int addr = POP();
	int val = POP(); 
	MemSet(addr, val);
}

void ForthOS::LITERAL()
{
	PUSH(MemGet(IP++));
}

void ForthOS::DUP()
{
	int val = POP();
	PUSH(val);
	PUSH(val);
}

void ForthOS::SWAP()
{
	int arg2 = POP();
	int arg1 = POP();
	PUSH(arg2);
	PUSH(arg1);
}

void ForthOS::DROP()
{
	POP();
}

void ForthOS::PICK()
{
	int arg1 = POP();
	int arg2 = SP - arg1 - 1;
	if (arg2 >= 0)
	{
		PUSH(stack[arg2]);
	}
	else
	{
		throw CString(_T("PICK operand out of range."));
	}
}

void ForthOS::ROT()
{
	int arg2 = POP();
	int arg1 = POP();
	int arg0 = POP();
	PUSH(arg1);
	PUSH(arg2);
	PUSH(arg0);
}

void ForthOS::ONEPLUS()
{
	PUSH(POP() + 1);
}

void ForthOS::PLUS()
{
	PUSH(POP() + POP());
}

void ForthOS::MINUS()
{
	int arg2 = POP();
	int arg1 = POP();
	PUSH(arg1 - arg2);
}

void ForthOS::MULT()
{
	PUSH(POP() * POP());
}

void ForthOS::DIV()
{
	int arg2 = POP();
	int arg1 = POP();
	if (arg2 == 0)
		throw CString(_T("divide by zero."));
	PUSH(arg1 / arg2);
}

void ForthOS::EQ()
{
	int arg2 = POP();
	int arg1 = POP();
	PUSH((arg1 == arg2) ? -1 : 0);
}

void ForthOS::LT()
{
	PUSH((POP() > POP()) ? -1 : 0);
}

void ForthOS::GT()
{
	PUSH((POP() < POP()) ? -1 : 0);
}

void ForthOS::TO_R()
{
	RPUSH(POP());
}

void ForthOS::R_FROM()
{
	PUSH(RPOP());
}

void ForthOS::R_AT()
{
	int arg1 = PUSH(RPOP());
	RPUSH(arg1);
}

void ForthOS::IF_RT()
{
	// If 0 (FALSE), then JUMP to (ELSE) or (THEN)
	int addr = MemGet(IP++);
	int val = POP();
	if (val == 0)
		IP = addr;
}

void ForthOS::DO()
{
	CStackEntry *cse = CPUSH();
	cse->loop_start_addr = IP; // LOOP target address
	cse->loop_increment = 1; // Default increment
	cse->loop_index = POP(); // Index
	cse->loop_limit = POP(); // Limit
}

void ForthOS::I()
{
	CStackEntry *cse = CPOP(); // Push the index
	if (cse == NULL)
		throw "invalid DO construct.";
	CPUSH();
	PUSH(cse->loop_index);
}

void ForthOS::LEAVE()
{
	CStackEntry *cse = CPOP(); // Set Limit = Index
	if (cse == NULL)
		throw "invalid DO construct.";
	cse = CPUSH();
	cse->loop_limit = cse->loop_index;
}

void ForthOS::LOOP()
{
	CStackEntry *cse = CPOP(); // Index
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
}

void ForthOS::PLUSLOOP()
{
	CStackEntry *cse = CPOP(); // Index
	if (cse == NULL)
		throw "invalid DO construct.";
	CPUSH();
	cse->loop_increment = POP();
	LOOP();
}

void ForthOS::GOTO()
{
	IP = MemGet(IP);
}

void ForthOS::AND()
{
	int arg1 = POP();
	int arg2 = POP();
	PUSH(arg1 & arg2);
}

void ForthOS::OR()
{
	int arg1 = POP();
	int arg2 = POP();
	PUSH(arg1 | arg2);
}

void ForthOS::XOR()
{
	int arg1 = POP();
	int arg2 = POP();
	PUSH(arg1 ^ arg2);
}

void ForthOS::EMIT()
{
	int val = POP();
	AppendOutput((CHAR)val);
}

void ForthOS::DICTP()
{
	IP++;
}

void ForthOS::FOPEN()
{
	int arg2 = POP(); // mode string
	int arg1 = POP(); // filename
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

void ForthOS::FCLOSE()
{
	int arg1 = POP(); // fp
	if (arg1 != 0)
	{
		fclose((FILE *)arg1);
	}
}

void ForthOS::FREAD()
{
	int arg1 = POP(); // fp
	int arg2 = POP(); // requested length
	int addr = POP(); // target-addr
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
}

void ForthOS::FWRITE()
{
	int val = POP(); // fp
	int arg2 = POP(); // num
	int arg1 = POP(); // addr
	if (val)
	{
		FILE *fp = (FILE *)val;
		for (int i = 0; i < arg2; i++)
		{
			fputc(MemGet(arg1++), fp);
		}
	}
}

void ForthOS::FGETC()
{
	int arg1 = POP(); // fp
	if (arg1)
	{
		FILE *fp = (FILE *)arg1;
		if (fp == stdin)
			PUSH(_getch());
		else
			PUSH(fgetc(fp));
	}
}


