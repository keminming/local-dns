/* CPU.cpp
 * CSCE 463 Sample Code 
 * by Dmitri Loguinov
 *
 * Obtains the current CPU utilization
 */
#pragma comment (lib,"Psapi.lib")
#include "stdafx.h"
#include "common.h"
#include "cpu.h"

// NOTE: link with Psapi.lib; this class uses the same functions as Task Manager

CPU::CPU (void)
{
	// NOTE: do not use unicode chars in the project; this will not compile! 
	// project->properties->general->character set = Not Set
	hDll = GetModuleHandle("ntdll.dll");
	if (hDll != NULL)
	{
		NtQuerySystemInformation = (NTSTATUS (__stdcall *)(SYSTEM_INFORMATION_CLASS 
			SystemInformationClass,
			PVOID SystemInformation,
			ULONG SystemInformationLength,
			PULONG ReturnLength)) GetProcAddress(hDll, "NtQuerySystemInformation");

		if (NtQuerySystemInformation != NULL)
		{
			SYSTEM_INFORMATION_CLASS query = SystemProcessorPerformanceInformation;
			NTSTATUS code = (NtQuerySystemInformation)(query, info, 
				sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)*MAX_CPU, &len);

			// how many CPUs
			this->cpus = len / sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

			printf ("found %d CPUs, total RAM %d MB\n", cpus, GetSystemRAM ());

			for (int i=0; i<cpus; i++)
			{
				this->idle[i] = info[i].IdleTime.QuadPart;
				this->kernel[i] = info[i].KernelTime.QuadPart;
				this->user[i] = info[i].UserTime.QuadPart;
			}
		}
	}

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, GetCurrentProcessId());
}

CPU::~CPU()
{
	if (hProcess != NULL)
		CloseHandle (hProcess);
}

// returns utilization * 100 
double CPU::GetCpuUtilization (double *array)
{
	SYSTEM_INFORMATION_CLASS query = SystemProcessorPerformanceInformation;

	if (hDll == NULL || NtQuerySystemInformation == NULL)
		return -1;

	NTSTATUS code = (NtQuerySystemInformation)(query, info, 
		sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)*MAX_CPU, &len);

	double average = 0;
	for (int i = 0; i < cpus; i++)
	{
		__int64 idle_dur = info[i].IdleTime.QuadPart - idle[i];
		__int64 kernel_dur = info[i].KernelTime.QuadPart - kernel[i];
		__int64 user_dur = info[i].UserTime.QuadPart - user[i];
		
		__int64 sys_time = kernel_dur + user_dur;

		if (sys_time == 0)		// called too often; need to wait between calls 
			return -2;

		double val = 100.0 * (sys_time - idle_dur) / sys_time;
		
		// store individual CPU utilization if desired by caller
		if (array != NULL)	
			array[i] = val;

		average += val;

		idle[i] = info[i].IdleTime.QuadPart;
		kernel[i] = info[i].KernelTime.QuadPart;
		user[i] = info[i].UserTime.QuadPart;
	}

	return average/cpus;
}

// return in megabytes
int CPU::GetProcessRAMUsage(void)
{
	if (hProcess != NULL)
	{
		PROCESS_MEMORY_COUNTERS pp;
		if (GetProcessMemoryInfo (hProcess, &pp, sizeof (PROCESS_MEMORY_COUNTERS)))
			// physical memory usage
			//return (int)floor(pp.WorkingSetSize/MEGABYTE + 0.5);
			// virtual memory usage
			return (int)floor(pp.PagefileUsage/MEGABYTE + 0.5);
		else
			return 0;
	}
	return 0;
}

int CPU::GetSystemRAM (void)
{
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx (&statex);

	// total virtual memory
	//return (int) (statex.ullTotalPageFile/MEGABYTE + 0.5); //(statex.ullTotalPhys/1e6);
	// total physical memory
	return (int) (statex.ullTotalPhys/MEGABYTE + 0.5); 
}

int CPU::GetSystemRAMUsage (void)
{
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx (&statex);

	// physical memory
	//return ((statex.ullTotalPhys-statex.ullAvailPhys)/1e6);
	// virtual memory
	return (int) ((statex.ullTotalPageFile -
		statex.ullAvailPageFile)/MEGABYTE + 0.5); 
}