#include "Lock.h"

void CMutexLock::Lock()
{
	_Lock.lock();
}

void CMutexLock::UnLock()
{
	_Lock.unlock();
}

CThreadLock::CThreadLock()
{
	::InitializeCriticalSection(&_Lock);
}

CThreadLock::~CThreadLock()
{
	::DeleteCriticalSection(&_Lock);
}

void CThreadLock::Lock()
{
	::EnterCriticalSection(&_Lock);
}

void CThreadLock::UnLock()
{
	::LeaveCriticalSection(&_Lock);
}
