#ifndef __LIBTOOLS_PROCESS_KEYBOARDMSG_KEYBOARDMSG_H__
#define __LIBTOOLS_PROCESS_KEYBOARDMSG_KEYBOARDMSG_H__

#include <Windows.h>

namespace libTools
{
	class CKeyboardMsg
	{
	public:
		CKeyboardMsg() = default;
		~CKeyboardMsg() = default;

		// ģ�ⰴ��
		static BOOL		SimulationKey(_In_  HWND hWnd, _In_ CHAR dwASCII, _In_opt_ DWORD dwTime = 10);

		// ��ȡ������
		static DWORD	GetVirKey(_In_ CHAR dwASCII);

		// ģ�����
		static BOOL		SimulationMouse(_In_ int x, _In_ int y);

		// ģ�����
		static BOOL		SimulationRightMouse(_In_ int x, _In_ int y);

		// �����ı�
		static BOOL		SendAscii(_In_ WCHAR data, _In_  BOOL shift);

		// �����ı�
		static BOOL		SendUnicode(_In_ WCHAR data);

		// ģ�ⰴ��
		static BOOL		SendKey(_In_ WORD wVk);

		// �����ı�
		static BOOL		SendKeys(_In_ LPCWSTR data);

		// �����ı�
		static BOOL		SendKeys(_In_ LPCSTR data);
	private:

	};
}

#endif // !__LIBTOOLS_PROCESS_KEYBOARDMSG_KEYBOARDMSG_H__
