#include "Log.h"
#include <process.h>
#include <include/FileLib/File.h>
#include <include/ExceptionLib/Exception.h>
#include <include/CharacterLib/Character.h>
#include <include/ProcessLib/Process/Process.h>

#pragma comment(lib,"FileLib.lib")
#pragma comment(lib,"ExceptionLib.lib")
#pragma comment(lib,"CharacterLib.lib")
#pragma comment(lib,"ProcessLib.lib")
#pragma comment(lib,"user32") // GetWindowTextW

#define _SELF L"Log.cpp"
libTools::CLog::CLog() : _wsClientName(L"Empty"), _bRun(false), _bSync(false), _InfiniteSave(false)
{

}

libTools::CLog::~CLog()
{
	Release();
}

libTools::CLog& libTools::CLog::GetInstance()
{
	static CLog Log;
	return Log;
}

VOID libTools::CLog::Print(_In_ LPCWSTR pwszFunName, _In_ LPCWSTR pwszFileName, _In_ int nLine, _In_ int nLogOutputType, _In_ em_Log_Type emLogType, _In_ BOOL bMsgBox, _In_ LPCWSTR pwszFormat, ...)
{
	if (!_bRun)
		return;

	va_list		args;
	WCHAR		szBuffer[1024];
	va_start(args, pwszFormat);
	_vsnwprintf_s(szBuffer, _countof(szBuffer) - 1, _TRUNCATE, pwszFormat, args);
	va_end(args);


	LogContent LogContent_;
	LogContent_.emLogType = emLogType;
	LogContent_.uLine = nLine;
	LogContent_.wsFileName = pwszFileName;
	LogContent_.wsFunName = pwszFunName;
	LogContent_.wsContent = szBuffer;
	::GetLocalTime(&LogContent_.SysTime);

	if (nLogOutputType & em_Log_OutputType_File)
		AddSaveLogToQueue(LogContent_);
	if (nLogOutputType & em_Log_OutputType_Console)
		AddLogContentToQueue(LogContent_);

	if (emLogType == em_Log_Type::em_Log_Type_Exception)
	{
		SaveLog_Immediately(TRUE);
	}

	if (bMsgBox)
	{
		::MessageBoxW(NULL, szBuffer, _wsClientName.c_str(), NULL);
	}
}


VOID libTools::CLog::Release()
{
	_bRun = false;
	if (hSendExitEvent != NULL)
	{
		::WaitForSingleObject(hSendExitEvent, INFINITE);
		::CloseHandle(hSendExitEvent);
		hSendExitEvent = NULL;
	}
	if (hSaveLogEvent != NULL)
	{
		::WaitForSingleObject(hSaveLogEvent, INFINITE);
		::CloseHandle(hSaveLogEvent);
		hSaveLogEvent = NULL;
	}
}

VOID libTools::CLog::SetClientName(_In_ CONST std::wstring& cwsClientName, _In_ CONST std::wstring wsSaveLogPath)
{
	hSendExitEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
	hSaveLogEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);

	_wsClientName = cwsClientName;

	SYSTEMTIME CurrentSysTime;
	::GetLocalTime(&CurrentSysTime);

	_wsLogPath = wsSaveLogPath;

	_bRun = true;
	LOG_CF_D(L"------------Run[%s] Time=[%d-%d-%d %d:%d:%d] ----------------------------",
		cwsClientName.c_str(),
		static_cast<DWORD>(CurrentSysTime.wYear),
		static_cast<DWORD>(CurrentSysTime.wMonth),
		static_cast<DWORD>(CurrentSysTime.wDay),
		static_cast<DWORD>(CurrentSysTime.wHour),
		static_cast<DWORD>(CurrentSysTime.wMinute),
		static_cast<DWORD>(CurrentSysTime.wSecond));

	HANDLE hSendThread = reinterpret_cast<HANDLE>(::_beginthreadex(NULL, NULL, reinterpret_cast<_beginthreadex_proc_type>(_SendThread), this, NULL, NULL));
	if (hSendThread == NULL)
	{
		::MessageBoxW(NULL, L"CreateThread _SendThread Faild!", L"Error", NULL);
		return;
	}
	::CloseHandle(hSendThread);

	HANDLE hSaveThread = reinterpret_cast<HANDLE>(::_beginthreadex(NULL, NULL, reinterpret_cast<_beginthreadex_proc_type>(_SaveThread), this, NULL, NULL));
	if (hSaveThread == NULL)
	{
		::MessageBoxW(NULL, L"CreateThread _SaveThread Faild!", L"Error", NULL);
		return;
	}
	::CloseHandle(hSaveThread);
}

VOID libTools::CLog::SetClientName(_In_ CONST std::wstring& cwsClientName)
{
	_wsClientName = cwsClientName;
}

VOID libTools::CLog::SaveLog(_In_ CONST std::wstring& wsContent, _In_ BOOL bAppend)
{
	CException::InvokeAction(__FUNCTIONW__, [=]
	{
		std::wstring wsPath = _wsLogPath + _wsClientName + L".log";
		if (bAppend)
			CFile::AppendUnicodeFile(wsPath, wsContent);
		else
			CFile::WriteUnicodeFile(wsPath, wsContent);
	});
}

DWORD WINAPI libTools::CLog::_SendThread(LPVOID lpParm)
{
	auto pTestLog = reinterpret_cast<CLog*>(lpParm);
	LogContent LogContent_;
	while (pTestLog->_bRun)
	{
		if (!pTestLog->ExistLogServer())
		{
			pTestLog->ClearLogConttent();
			::Sleep(1000);
			continue;
		}

		auto StartTick = std::chrono::system_clock::now();
		while (pTestLog->_bRun && (std::chrono::system_clock::now() - StartTick).count() < 3000 * 10000)
		{
			if (!pTestLog->GetLogContentForQueue(LogContent_))
			{
				::Sleep(50);
				continue;
			}

			pTestLog->SendLogToForm(LogContent_);
		}
	}
	::SetEvent(pTestLog->hSendExitEvent);
	return 0;
}

VOID libTools::CLog::AddLogContentToQueue(_In_ CONST LogContent& LogContent_)
{
	std::lock_guard<std::mutex> _Lock(_MtxFormLogContent);
	CException::InvokeAction(__FUNCTIONW__, [&]
	{
		if (_bSync)
		{
			SendLogToForm(LogContent_);
		}
		else
		{
			_QueueLogContent.push_back(LogContent_);
		}
	});
}

BOOL libTools::CLog::GetLogContentForQueue(_Out_ LogContent& LogContent_)
{
	std::lock_guard<std::mutex> _Lock(_MtxFormLogContent);
	return CException::InvokeFunc<BOOL>(__FUNCTIONW__, [&]
	{
		if (!_QueueLogContent.empty())
		{
			LogContent_ = _QueueLogContent.front();
			_QueueLogContent.erase(_QueueLogContent.begin());
			return TRUE;
		}
		return FALSE;
	});
}

VOID libTools::CLog::ClearLogConttent()
{
	std::lock_guard<std::mutex> _Lock(_MtxFormLogContent);
	CException::InvokeAction(__FUNCTIONW__, [&]
	{
		_QueueLogContent.clear();
	});
}

VOID libTools::CLog::AddSaveLogToQueue(_In_ CONST LogContent& LogContent_)
{
	std::lock_guard<std::mutex> _Lock(_MtxSaveLogContent);
	CException::InvokeAction(__FUNCTIONW__, [&]
	{
		_QueueSaveLogContent.push_back(LogContent_);
	});
}

VOID libTools::CLog::GetSaveLogContentForQueue(_Out_ std::deque<LogContent>& VecLogContent)
{
	std::lock_guard<std::mutex> _Lock(_MtxSaveLogContent);
	CException::InvokeAction(__FUNCTIONW__, [&]
	{
		VecLogContent = _QueueSaveLogContent;
		_QueueSaveLogContent.clear();
	});
}

VOID libTools::CLog::SaveLog_Immediately(_In_ BOOL bAppend)
{
	static std::mutex _Mtx;
	std::lock_guard<std::mutex> _Lock(_Mtx);

	CException::InvokeAction(__FUNCTIONW__, [&]
	{
		std::deque<LogContent> VecLogContent;
		GetSaveLogContentForQueue(VecLogContent);
		if (!VecLogContent.empty())
		{
			std::wstring wsContent;
			for (auto& itm : VecLogContent)
				wsContent += FormatLogContent(itm);

			SaveLog(wsContent, bAppend);
		}
		ClearLogFile();
	});
}

VOID libTools::CLog::ClearLogFile()
{
	if (!_InfiniteSave)
	{
		std::wstring wsPath = _wsLogPath + _wsClientName + L".log";

		ULONG uLen = 0;
		if (CFile::FileExist(wsPath) && CFile::ReadAsciiFileLen(wsPath, uLen) && uLen >= 20 * 1024 * 1024)
		{
			CFile::WriteUnicodeFile(wsPath, L"");
		}
	}
}

VOID libTools::CLog::SetSyncSendLog() _NOEXCEPT
{
	_bSync = true;
}

VOID libTools::CLog::SetInfiniteSave() _NOEXCEPT
{
	_InfiniteSave = true;
}

UINT libTools::CLog::FormLogSize()
{
	std::lock_guard<std::mutex> _Lock(_MtxFormLogContent);
	return _QueueLogContent.size();
}

UINT libTools::CLog::SaveLogSize()
{
	std::lock_guard<std::mutex> _Lock(_MtxSaveLogContent);
	return _QueueSaveLogContent.size();
}

VOID libTools::CLog::Clear()
{
	std::lock_guard<std::mutex> Lock1(_MtxFormLogContent);
	std::lock_guard<std::mutex> Lock2(_MtxSaveLogContent);

	_QueueLogContent.clear();
	_QueueSaveLogContent.clear();
}

std::wstring libTools::CLog::FormatLogContent(_In_ CONST LogContent& Content) CONST
{
	return CCharacter::MakeFormatText(L"#Stack:\r\n  #Time:%02d:%02d:%02d  #Client:%s	#Level:%d #File:%s	#FunName:%s Line:%d	#Content:%s\r\n",
		static_cast<DWORD>(Content.SysTime.wHour),
		static_cast<DWORD>(Content.SysTime.wMinute),
		static_cast<DWORD>(Content.SysTime.wSecond),
		_wsClientName.c_str(),
		static_cast<DWORD>(Content.emLogType),
		Content.wsFileName.c_str(),
		Content.wsFunName.c_str(),
		Content.uLine,
		Content.wsContent.c_str());
}

BOOL libTools::CLog::IsSaveLog()
{
	std::lock_guard<std::mutex> _Lock(_MtxSaveLogContent);
	return _QueueSaveLogContent.size() >= MAX_SAVELOG_COUNT;
}

VOID libTools::CLog::ClearSaveLog()
{
	std::lock_guard<std::mutex> _Lock(_MtxSaveLogContent);
	_QueueSaveLogContent.erase(_QueueSaveLogContent.begin(), _QueueSaveLogContent.begin() + _QueueSaveLogContent.size() / 2);
}

VOID libTools::CLog::SendLogToForm(_In_ CONST LogContent& Content) CONST
{
	CException::InvokeAction(__FUNCTIONW__, [&]
	{
		auto wsText = libTools::CCharacter::MakeFormatText(L"%d\r\n%s\r\n%s\r\n%d\r\n%s\r\n%s",
			static_cast<int>(Content.emLogType),
			Content.wsFunName.c_str(),
			Content.wsFileName.c_str(),
			Content.uLine,
			Content.wsContent.c_str(),
			_wsClientName.c_str());

		std::shared_ptr<WCHAR> wsTextPtr(new WCHAR[wsText.length()], [](WCHAR* p) { delete[] p; });
		CCharacter::strcpy_my(wsTextPtr.get(), wsText.c_str());

		COPYDATASTRUCT cd = { 0 };
		cd.dwData = 0x4C6F67;
		cd.lpData = wsTextPtr.get();
		cd.cbData = static_cast<DWORD>((wsText.length() + 1) * 2);

		::SendMessageW(_LogFormContent.hWnd, WM_COPYDATA, reinterpret_cast<WPARAM>(&cd), reinterpret_cast<LPARAM>(&cd));
	});
}

BOOL CALLBACK libTools::CLog::EnumSetWinName(HWND hWnd, LPARAM lpParam)
{
#ifdef _WIN64
	if (IsWindow(hWnd) && IsWindowVisible(hWnd) && (GetWindowLong(hWnd, GWL_EXSTYLE)&WS_EX_TOOLWINDOW) != WS_EX_TOOLWINDOW)
#else
	if (IsWindow(hWnd) && IsWindowVisible(hWnd) && (GetWindowLong(hWnd, GWL_EXSTYLE)&WS_EX_TOOLWINDOW) != WS_EX_TOOLWINDOW && (GetWindowLong(hWnd, GWL_HWNDPARENT) == 0))
#endif // _WIN64
	{
		WCHAR wszWinText[64] = { 0 };
		WCHAR wszWinClass[64] = { 0 };

		if (GetClassNameW(hWnd, wszWinClass, _countof(wszWinClass) / sizeof(WCHAR)) > 0 && GetWindowTextW(hWnd, wszWinText, _countof(wszWinText)) > 0)
		{
			std::wstring wsWinClass = wszWinClass;
			if (wsWinClass.find(L"WindowsForms10.Window") != std::wstring::npos)//����ܱ��������,������
			{
				LogFormContent* pContent = reinterpret_cast<LogFormContent*>(lpParam);

				DWORD PID;
				::GetWindowThreadProcessId(hWnd, &PID);
				if (pContent != nullptr && PID == pContent->dwPid)
				{
					pContent->hWnd = hWnd;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

BOOL libTools::CLog::ExistLogServer()
{
	_LogFormContent.dwPid = CProcess::FindPidByProcName(L"Log.exe");
	if (_LogFormContent.dwPid != NULL)
	{
		_LogFormContent.hWnd = NULL;
		EnumWindows(EnumSetWinName, reinterpret_cast<LPARAM>(&_LogFormContent));
		return _LogFormContent.hWnd != NULL;
	}

	ZeroMemory(&_LogFormContent, sizeof(_LogFormContent));
	return FALSE;
}

DWORD WINAPI libTools::CLog::_SaveThread(LPVOID lpParm)
{
	auto pLog = reinterpret_cast<CLog*>(lpParm);
	LogContent LogContent_;
	while (pLog->_bRun)
	{
		if (pLog->_bSync)
		{
			pLog->SaveLog_Immediately(TRUE);
			::Sleep(500);
			continue;
		}

		if (!pLog->IsSaveLog())
		{
			::Sleep(500);
			continue;
		}

		pLog->ClearSaveLog();
	}
	::SetEvent(pLog->hSaveLogEvent);
	return 0;
}
