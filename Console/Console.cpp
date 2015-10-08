// Console.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <signal.h>
#include "Console.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// -------------------------------------------------------------------
// Standard Console app stuff
CWinApp theApp;
using namespace std;
// -------------------------------------------------------------------

#define IS_CONSOLE
#include "..\CForth.Core\ForthOS.h"

FILE *fp = NULL;
char line[128];
ForthOS *myOS = NULL;

int init()
{
	if (myOS != NULL)
	{
		delete myOS;
		myOS = NULL;
	}

	myOS = new ForthOS(MEMORY_SIZE);

	fp = stdin;
	myOS->input_fp = stdin;
	myOS->output_fp = stdout;

	myOS->BootStrap();

	return 0;
}

int go()
{
	while (true)
	{
		line[0] = (char)NULL;

		if (fp == stdin)
		{
			fputs("\nCForth>", stdout);
		}
		try
		{
			if (fgets(line, sizeof(line), fp) != line)
				continue;
		}
		catch (int e)
		{
			fprintf(stderr, "exception %d caught!", e);
			continue;
		}
		catch (...)
		{
			fprintf(stderr, "exception caught!");
			continue;
		}
		int len = strlen(line);
		if (line[len - 1] == '\n')
		{
			line[--len] = (char)NULL;
		}

		CString input(line);
		if (input.CompareNoCase(_T("bb")) == 0)
		{
			input = _T("bye");
		}

		if (input.CompareNoCase(_T("bye")) == 0)
		{
			return 0;
		}

		if (input.CompareNoCase(_T("restart")) == 0)
		{
			init();
			strcpy_s(line, sizeof(line), "include forthos");
			len = 3;
		}

		if (input.CompareNoCase(_T("dump")) == 0)
		{
			CString ret;
			myOS->Dump(ret);
			fputws(ret, stdout);
			continue;
		}

		myOS->ParseInput(CString(line));

		if (fp == stdin)
		{
			fputs(" ok", stdout);
		}
	}
}


int init();
int startup();
void SignalHandler(int action);

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	if (!startup())
		return 1;

	CString arg, cmd;
	myOS = NULL;
	init();

	for (int i = 1; i < argc; i++)
	{
		arg = argv[i];
		if ((arg[0] == '-') || (arg[0] == '/'))
		{
			arg = arg.Mid(1);
			if (arg.CompareNoCase(_T("?")) == 0)
			{
				printf("usage: fcon -? -bs -include:fileName\n");
			}
			else if (arg.CompareNoCase(_T("bs")) == 0)
			{
				cmd.Format(_T("include forthos"), arg);
				printf("%S ...", cmd);
				myOS->ParseInput(cmd);
				printf("\n", cmd);
			}
			else if (arg.Left(8).CompareNoCase(_T("include:")) == 0)
			{
				arg = arg.Mid(8);
				cmd.Format(_T("include %s"), arg);
				printf("%S ...", cmd);
				myOS->ParseInput(cmd);
				printf("\n", cmd);
			}
		}
	}

	return go();
}

int startup()
{
	HMODULE hModule = ::GetModuleHandle(NULL);
	if (hModule == NULL)
	{
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		return 0;
	}
	// initialize MFC and print and error on failure
	if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		return 0;
	}

	signal(SIGABRT, SignalHandler);
	signal(SIGBREAK, SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGINT, SignalHandler);

	return 1;
}

void SignalHandler(int action)
{
	fprintf(stderr, "Signal %d caught!", action);
	init();
	go();
	exit(1);
}

