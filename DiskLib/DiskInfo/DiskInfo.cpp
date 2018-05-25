#include "DiskInfo.h"
#include <filesystem>
#include <devioctl.h>
#include <ntdddisk.h>
#define _NTSCSI_USER_MODE_
#include <scsi.h>
#include <include/ProcessLib/Common/ResHandleManager.h>
#include <include/LogLib/Log.h>
#include <include/CharacterLib/Character.h>

#define _SELF L"DiskInfo.cpp"
BOOL libTools::CDiskInfo::GetDiskSerailNumber(_In_ WCHAR wchDisk, _Out_ std::wstring& wsSerailNumber)
{
	static WCHAR wszDeivePath[MAX_PATH] = { L"\\\\.\\?:" };
	wszDeivePath[4] = wchDisk;


	HANDLE hPhysicalDriveIOCTL = CreateFileW(wszDeivePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhysicalDriveIOCTL == NULL)
	{
		LOG_C_E(L"´ò¿ª´ÅÅÌ[%s]Ê§°Ü!", wszDeivePath);
		return FALSE;
	}

	STORAGE_PROPERTY_QUERY query;
	memset(&query, 0, sizeof(query));
	query.QueryType = PropertyStandardQuery;
	query.PropertyId = StorageDeviceProperty;


	PSTORAGE_DEVICE_DESCRIPTOR pdeviceDescriptor = new STORAGE_DEVICE_DESCRIPTOR;
	memset(pdeviceDescriptor, 0, sizeof(STORAGE_DEVICE_DESCRIPTOR));
	SetResDeleter(pdeviceDescriptor, [](PSTORAGE_DEVICE_DESCRIPTOR& pDescriptor) { delete pDescriptor; pDescriptor = nullptr; });
	SetResDeleter(hPhysicalDriveIOCTL, [](HANDLE& hFile) { ::CloseHandle(hFile); hFile = NULL; });


	DWORD dwRetLen = 0;
	if (!::DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), pdeviceDescriptor, sizeof(STORAGE_DEVICE_DESCRIPTOR), &dwRetLen, 0))
	{
		LOG_C_E(L"[%s] IOCTL_STORAGE_QUERY_PROPERTY = FALSE!", wszDeivePath);
		return FALSE;
	}


	DWORD returned = 0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER Buffer;
	ZeroMemory(&Buffer, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
	Buffer.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	Buffer.sptd.ScsiStatus = 0x0;
	Buffer.sptd.PathId = 0x0;
	Buffer.sptd.TargetId = 0x0;
	Buffer.sptd.Lun = 0x0;
	Buffer.sptd.CdbLength = 0xC;
	Buffer.sptd.SenseInfoLength = SPT_SENSE_LENGTH;
	Buffer.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	Buffer.sptd.DataTransferLength = 0x200;
	Buffer.sptd.TimeOutValue = 0x1A5E0;


	// Buffer
	CHAR szText[1024] = { 0 };
	Buffer.sptd.DataBuffer = szText;
	Buffer.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);


	// Fill 
	memset(Buffer.sptd.Cdb, 0, sizeof(Buffer.sptd.Cdb));
	Buffer.sptd.Cdb[0] = SCSIOP_BLANK;
	Buffer.sptd.Cdb[1] = 0x8;
	Buffer.sptd.Cdb[2] = 0xE;
	Buffer.sptd.Cdb[4] = 0x1;// (UCHAR)(sectorSize >> 8);  // Parameter List length
	Buffer.sptd.Cdb[8] = 0xA0;
	Buffer.sptd.Cdb[9] = 0xEC;


	if (!DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_SCSI_PASS_THROUGH_DIRECT, &Buffer, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER), &Buffer, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER), &returned, FALSE))
	{
		LOG_C_E(L"»ñÈ¡Ó²ÅÌ[%s] ÐòÁÐºÅÊ§°Ü", wszDeivePath);
		return FALSE;
	}


	FormatDiskSerialNumber(szText + 0x14);
	wsSerailNumber =libTools::CCharacter::ASCIIToUnicode(std::string(szText + 0x14));
	wsSerailNumber = libTools::CCharacter::Trim(wsSerailNumber);
	return TRUE;
}

BOOL libTools::CDiskInfo::IsUsbDiver(_In_ WCHAR wchDisk)
{
	static WCHAR wszDeivePath[MAX_PATH] = { L"\\\\.\\?:" };
	wszDeivePath[4] = wchDisk;


	HANDLE hPhysicalDriveIOCTL = CreateFileW(wszDeivePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhysicalDriveIOCTL == NULL)
	{
		LOG_C_E(L"´ò¿ª´ÅÅÌ[%s]Ê§°Ü!", wszDeivePath);
		return FALSE;
	}

	STORAGE_PROPERTY_QUERY query;
	memset(&query, 0, sizeof(query));
	query.QueryType = PropertyStandardQuery;
	query.PropertyId = StorageDeviceProperty;


	PSTORAGE_DEVICE_DESCRIPTOR pdeviceDescriptor = new STORAGE_DEVICE_DESCRIPTOR;
	memset(pdeviceDescriptor, 0, sizeof(STORAGE_DEVICE_DESCRIPTOR));
	SetResDeleter(pdeviceDescriptor, [](PSTORAGE_DEVICE_DESCRIPTOR& pDescriptor) { delete pDescriptor; pDescriptor = nullptr; });
	SetResDeleter(hPhysicalDriveIOCTL, [](HANDLE& hFile) { ::CloseHandle(hFile); hFile = NULL; });


	DWORD dwRetLen = 0;
	if (!::DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), pdeviceDescriptor, sizeof(STORAGE_DEVICE_DESCRIPTOR), &dwRetLen, 0))
	{
		LOG_C_E(L"[%s] IOCTL_STORAGE_QUERY_PROPERTY = FALSE!", wszDeivePath);
		return FALSE;
	}


	return pdeviceDescriptor->BusType == 0x7;
}

WCHAR libTools::CDiskInfo::GetSystemDrive()
{
	WCHAR wszDisk[MAX_PATH] = { 0 };
	::GetSystemDirectoryW(wszDisk, MAX_PATH);
	return wszDisk[0];
}

BOOL libTools::CDiskInfo::GetSystemPhysicalDrive(_Out_ std::wstring& wsSysPhyicalDrive)
{
	DWORD dwDeviceNumber = 0;
	if (!GetPhysicalDiskNumber(GetSystemDrive(), dwDeviceNumber))
		return FALSE;


	wsSysPhyicalDrive = FormatPhysicalDiskNumber(dwDeviceNumber);
	return TRUE;
}

BOOL libTools::CDiskInfo::GetPhysicalDiskNumber(_In_ WCHAR wchDisk, _Out_ DWORD& dwDeviceNumber)
{
	HANDLE hDevice = ::CreateFileW(libTools::CCharacter::MakeFormatText(L"\\\\.\\%c:", wchDisk).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		LOG_C_E(L"CreateFileW [Disk:%c] = INVALID_HANDLE_VALUE", wchDisk);
		return FALSE;
	}


	SetResDeleter(hDevice, [](HANDLE& hFile) { ::CloseHandle(hFile); hFile = NULL; });


	STORAGE_DEVICE_NUMBER	sdNumber		= { 0 };
	DWORD					dwBytesReturned = 0;
	if (DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdNumber, sizeof(sdNumber), &dwBytesReturned, NULL))
	{
		dwDeviceNumber = sdNumber.DeviceNumber;
		return TRUE;
	}
	return FALSE;
}

BOOL libTools::CDiskInfo::IsBasicDisk(_In_ WCHAR wchDisk)
{
	return ::GetDriveTypeW(libTools::CCharacter::MakeFormatText(L"%c:\\", wchDisk).c_str()) == DRIVE_FIXED;
}

UINT libTools::CDiskInfo::GetDiskSize(_In_ WCHAR wchDisk)
{
	std::wstring wsPath = L"";
	wsPath.append(1, wchDisk);


	std::experimental::filesystem::v1::path GameDisk(wsPath);
	auto uMaxSpace = std::experimental::filesystem::v1::space(GameDisk).capacity;
	return static_cast<UINT>(uMaxSpace / 1024 / 1024 / 1024);
}

std::wstring libTools::CDiskInfo::FormatPhysicalDiskNumber(_In_ DWORD dwDeviceNumber)
{
	return libTools::CCharacter::MakeFormatText(L"\\\\.\\PhysicalDrive%d", dwDeviceNumber);
}

BOOL libTools::CDiskInfo::GetPhysicalDiskNumber(_In_ WCHAR wchDisk, _Out_ std::wstring& wsPhysicalDiskNumber)
{
	DWORD dwDeviceNumber;
	if (!GetPhysicalDiskNumber(wchDisk, dwDeviceNumber))
	{
		return FALSE;
	}


	wsPhysicalDiskNumber = FormatPhysicalDiskNumber(dwDeviceNumber);
	return TRUE;
}
