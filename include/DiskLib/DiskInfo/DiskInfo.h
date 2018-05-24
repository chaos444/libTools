#ifndef __LIBTOOLS_DISKLIB_DISKINFO_DISKINFO_H__
#define __LIBTOOLS_DISKLIB_DISKINFO_DISKINFO_H__

#include <Windows.h>
#include <string>
#include <ntddscsi.h>

namespace libTools
{
	class CDiskInfo
	{
	private:
	private:
#define SPT_CDB_LENGTH  32
#define SPT_SENSE_LENGTH  32
#define SPTWB_DATA_LENGTH  512

		typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS
		{
			SCSI_PASS_THROUGH spt;
			ULONG             Filler;      // realign buffers to double word boundary
			UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
			UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
		} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

		typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER
		{
			SCSI_PASS_THROUGH_DIRECT sptd;
			ULONG             Filler;      // realign buffer to double word boundary
			UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
		} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;
	public:
		CDiskInfo() = default;
		~CDiskInfo() = default;

		// Get Disk SerialNumber
		static BOOL GetDiskSerailNumber(_In_ WCHAR wchDisk, _Out_ std::wstring& wsSerailNumber);

		// Is USB
		static BOOL IsUsbDiver(_In_ WCHAR wchDisk);


		//
		static WCHAR GetSystemDrive();

		//
		static BOOL GetSystemPhysicalDrive(_Out_ std::wstring& wsSysPhyicalDrive);


		//
		static BOOL GetPhysicalDiskNumber(_In_ WCHAR wchDisk, _Out_ DWORD& dwDeviceNumber);


		//
		static std::wstring FormatPhysicalDiskNumber(_In_ DWORD dwDeviceNumber);


		//
		static BOOL GetPhysicalDiskNumber(_In_ WCHAR wchDisk, _Out_ std::wstring& wsPhysicalDiskNumber);


		// 
		static BOOL IsBasicDisk(_In_ WCHAR wchDisk);
	private:
		template<typename T>
		static VOID FormatDiskSerialNumber(_In_ T* pszSeralNumber)
		{
			while (*pszSeralNumber != '\0')
			{
				std::swap(*pszSeralNumber, *(pszSeralNumber + 1));
				pszSeralNumber += 2;
			}
		}
	};
}


#endif // !__LIBTOOLS_DISKLIB_DISKINFO_DISKINFO_H__
