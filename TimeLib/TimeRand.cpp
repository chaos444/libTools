#include "TimeRand.h"
#include "TimeTick.h"
#include <random>

int libTools::CTimeRand::GetRand(_In_ int nMinValue, _In_ int nMaxValue)
{
	//static UINT uSeed = static_cast<UINT>(CTimeTick::_GetTickCount64());
	//srand(static_cast<UINT>(uSeed));
	//uSeed = static_cast<UINT>(rand());
	INT uSeed = static_cast<int>(__rdtsc());
	return (nMinValue + uSeed % (nMaxValue - nMinValue + 1));
}

