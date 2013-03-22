// 463_hw_2.cpp : Defines the entry point for the console application.
//

/*
	KE Wang
	822002009
*/
#include "stdafx.h"
#include <string>
#include <conio.h>
using namespace std;

extern CRITICAL_SECTION statistic_lock;

int _tmain(int argc, _TCHAR* argv[])
{
	InitializeCriticalSection(&statistic_lock);

	if(is_integer(argv[1]))
	{
		try{
		Batch_resolver br("dns-in.txt");
		br.run(atoi(argv[1]));
		}
		catch(...)
		{
			printf("Error build batch resolver.\n");
		}
		
	}
	else
	{
		DNS dns;
		string answer;
		answer = dns.resolve(argv[1]);
		printf("%s\n",answer.c_str());
	}

	getch();
	DeleteCriticalSection(&statistic_lock);
	return 0;
}

