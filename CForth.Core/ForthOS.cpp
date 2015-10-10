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

	prims[I_FETCH] = &ForthOS::FETCH;
	prims[I_STORE] = &ForthOS::STORE;
	prims[I_LITERAL] = &ForthOS::LITERAL;
	prims[I_DUP] = &ForthOS::DUP;
	prims[I_SWAP] = &ForthOS::SWAP;
	prims[I_DROP] = &ForthOS::DROP;
	prims[I_PICK] = &ForthOS::PICK;
	prims[I_ROT] = &ForthOS::ROT;
	prims[I_ONEPLUS] = &ForthOS::ONEPLUS;
	prims[I_PLUS] = &ForthOS::PLUS;
	prims[I_MINUS] = &ForthOS::MINUS;
	prims[I_MULT] = &ForthOS::MULT;
	prims[I_DIV] = &ForthOS::DIV;
	prims[I_EQ] = &ForthOS::EQ;
	prims[I_LT] = &ForthOS::LT;
	prims[I_GT] = &ForthOS::GT;
	prims[I_TO_R] = &ForthOS::TO_R;
	prims[I_R_FROM] = &ForthOS::R_FROM;
	prims[I_R_AT] = &ForthOS::R_AT;
	prims[I_IF_RT] = &ForthOS::IF_RT;
	prims[I_DO] = &ForthOS::DO;
	prims[I_I] = &ForthOS::I;
	prims[I_LEAVE] = &ForthOS::LEAVE;
	prims[I_LOOP] = &ForthOS::LOOP;
	prims[I_PLUSLOOP] = &ForthOS::PLUSLOOP;
	prims[I_GOTO] = &ForthOS::GOTO;
	prims[I_AND] = &ForthOS::AND;
	prims[I_OR] = &ForthOS::OR;
	prims[I_XOR] = &ForthOS::XOR;
	prims[I_EMIT] = &ForthOS::EMIT;
	prims[I_DICTP] = &ForthOS::DICTP;
	prims[I_FOPEN] = &ForthOS::FOPEN;
	prims[I_FCLOSE] = &ForthOS::FCLOSE;
	prims[I_FREAD] = &ForthOS::FREAD;
	prims[I_FWRITE] = &ForthOS::FWRITE;
	prims[I_FGETC] = &ForthOS::FGETC;
}

ForthOS::~ForthOS()
{
#ifndef _DEBUG
	free(memory);
#endif
}

