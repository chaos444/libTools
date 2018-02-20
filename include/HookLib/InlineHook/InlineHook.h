#ifndef __LIBTOOLS_HOOKLIB_INLINEHOOK_INLINEHOOK_H__
#define __LIBTOOLS_HOOKLIB_INLINEHOOK_INLINEHOOK_H__

#include <Windows.h>

namespace libTools
{
	class CInlineHook
	{
	public:
		struct HookContent
		{
			DWORD dwHookAddr;				//Hook�ĵ�ַ
			DWORD dwFunAddr;				//�Լ������ĵ�ַ
			UINT  uNopCount;				//ҪNop�����ֽ���
			DWORD dwOldCode1;				//��ԭ�Ĵ���1
			DWORD dwOldCode2;				//��ԭ�Ĵ���2
			DWORD dwFlag;					// ����
			HookContent()
			{
				Release();
			}
			VOID Release()
			{
				dwHookAddr = dwFunAddr = dwOldCode1 = dwOldCode2 = dwFlag = 0;
				uNopCount = 0;
			}
		};
	public:
		CInlineHook() = default;
		~CInlineHook() = default;

		static BOOL Hook(_In_ _Out_ HookContent* pHookContent);

		static BOOL UnHook(_In_ _Out_ HookContent* pHookContent);

		// Hook(��ҪHook�ĺ�����ַ,���汻Hook�����ĵ�ַ,����ԭʼ��������ڵ�ַ)
		static BOOL Hook(_In_ void *OrgProc, _In_ void *NewProc, _Out_ void **RealProc);

		// UnHook(��Ҫ�ָ�Hook�ĺ�����ַ,ԭʼ��������ڵ�ַ)
		static BOOL UnHook(_In_ void *OrgProc, _In_ void *RealProc);
	private:

	};
}

#endif // !__LIBTOOLS_HOOKLIB_INLINEHOOK_INLINEHOOK_H__
