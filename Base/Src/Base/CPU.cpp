#include "Base/CPU.h"
#include "Base/Base.h"

#ifdef WIN32
#include <Windows.h>
#endif

namespace Public
{
namespace Base
{
CPU::CPU()
{
}

CPU::~CPU()
{
}
#ifdef WIN32
typedef BOOL(WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

static unsigned int CountSetBits(unsigned long bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
	DWORD i;

	for (i = 0; i <= LSHIFT; ++i)
	{
		bitSetCount += ((bitMask & bitTest) ? 1 : 0);
		bitTest /= 2;
	}

	return bitSetCount;
}

bool CPU::getCpuCount(unsigned int &logiccpu, unsigned int &corecpu)
{
	LPFN_GLPI glpi;
	glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
	if (NULL == glpi)
	{
		logerror("GetLogicalProcessorInformation is not supported.\n");
		return false;
	}
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	DWORD returnLength = 0;
	while (true)
	{
		DWORD rc = glpi(buffer, &returnLength);
		if (rc == TRUE)
		{
			break;
		}

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			return false;
		}
		if (buffer)
		{
			free(buffer);
		}
		buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
	}

	ptr = buffer;
	DWORD byteOffset = 0;
	DWORD logicalProcessorCount = 0;
	DWORD processorCoreCount = 0;

	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
	{
		switch (ptr->Relationship)
		{
		case RelationProcessorCore:
			processorCoreCount++;
			logicalProcessorCount += CountSetBits((unsigned long)ptr->ProcessorMask);
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}
	free(buffer);
	logiccpu = logicalProcessorCount;
	corecpu = processorCoreCount;
	return true;
}

bool CPU::limit(unsigned int scale)
{
	unsigned int totalcount = 0;
	unsigned int corecpu = 0;
	if (!getCpuCount(totalcount, corecpu) || totalcount == 0)
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		DWORD_PTR totalcount = info.dwNumberOfProcessors; // 只有1核不能限制
	}
	unsigned int idecount = (unsigned int)(totalcount / scale);
	if (idecount <= 0 || totalcount <= 1 || scale <= 1) // sacle为1，无需限制，比例不对，无法限制
	{
		return false;
	}
	DWORD_PTR dwBit = 0;
	DWORD_PTR systembit = sizeof(DWORD_PTR) * 8;
	if (totalcount > systembit)
	{
		unsigned int usecount = (unsigned int)(totalcount - idecount);
		idecount = (unsigned int)systembit - usecount;
	}

	if ((int)idecount < 1)
	{
		dwBit = (2LL << ((unsigned long long)systembit - 1)) - 1;
	}
	else
	{
		dwBit = ~((2LL << ((DWORD_PTR)idecount - 1)) - 1);
	}

	totalcount = (int)((float)totalcount / 4 + 0.5) * 4;
	if (totalcount > 64)
	{
		totalcount = 64;
	}
	DWORD_PTR dwTotalbit = ((2ULL << (totalcount - 1)) - 1);
	DWORD_PTR dwCpuMask = 0;
	dwCpuMask = dwCpuMask | dwBit & dwTotalbit;
	loginfo("logiccpu:%d cpuMask:0x%X", totalcount, dwCpuMask);
	if (SetProcessAffinityMask(GetCurrentProcess(), dwCpuMask) == FALSE)
	{
		return false;
	}
	return true;
}
#else
bool CPU::getCpuCount(unsigned int &logiccpu, unsigned int &corecpu)
{
	return false;
}
bool CPU::limit(unsigned int scale)
{
	return false;
}
#endif
} // namespace Base
} // namespace Public
