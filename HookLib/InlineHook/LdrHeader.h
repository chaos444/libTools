#ifndef __LIBTOOLS_HOOKLIB_INLINEHOOK_LDRHEADER
#define __LIBTOOLS_HOOKLIB_INLINEHOOK_LDRHEADER

#pragma warning(disable:4005)
#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>

namespace libTools
{
	class CLdrHeader
	{
	public:
		CLdrHeader();
		~CLdrHeader() = default;

		// InlineHook(��ҪHook�ĺ�����ַ,���汻Hook�����ĵ�ַ,����ԭʼ��������ڵ�ַ)
		static BOOL WINAPI InlineHook(_In_ void *OrgProc, _In_	void *NewProc, _Out_ void **RealProc);

		// UnInlineHook(��Ҫ�ָ�Hook�ĺ�����ַ,ԭʼ��������ڵ�ַ)
		static VOID WINAPI UnInlineHook(_In_ void *OrgProc, _In_ void *RealProc);
	private:
		static BOOL WINAPI WriteReadOnlyMemory(LPBYTE	lpDest, LPBYTE	lpSource, ULONG	Length);

		static BOOL WINAPI GetPatchSize(IN	void *Proc, IN	DWORD dwNeedSize, OUT LPDWORD lpPatchSize);
	};
}


#endif // !__LIBTOOLS_HOOKLIB_INLINEHOOK_LDRHEADER
