#include "DiskInfo.h"
#include <filesystem>
#include <devioctl.h>
#include <ntdddisk.h>
#define _NTSCSI_USER_MODE_
#include <scsi.h>
#include <include/ProcessLib/Common/ResHandleManager.h>
#include <include/LogLib/Log.h>
#include <include/CharacterLib/Character.h>
#include <iostream>

#define _SELF L"DiskInfo.cpp"
BOOL libTools::CDiskInfo::GetDiskSerailNumber_BySCSI(_In_ WCHAR wchDisk, _Out_ std::wstring& wsSerailNumber)
{
	static WCHAR wszDeivePath[MAX_PATH] = { L"\\\\.\\?:" };
	wszDeivePath[4] = wchDisk;


	HANDLE hPhysicalDriveIOCTL = CreateFileW(wszDeivePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhysicalDriveIOCTL == NULL)
	{
		LOG_C_E(L"¥Úø™¥≈≈Ã[%s] ß∞‹!", wszDeivePath);
		return FALSE;
	}


	DWORD returned = 0;
	SCSI_PASS_THROUGH_WITH_BUFFERS Buffer;

	CONST static std::vector<DWORD> VecTarget = { 0xA0, 0xB0 };
	for (auto& Target : VecTarget)
	{
		ZeroMemory(&Buffer, sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
		Buffer.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
		Buffer.Spt.PathId = 0;
		Buffer.Spt.TargetId = 0;
		Buffer.Spt.Lun = 0;
		Buffer.Spt.SenseInfoLength = 24;
		Buffer.Spt.DataIn = SCSI_IOCTL_DATA_IN;
		Buffer.Spt.DataTransferLength = IDENTIFY_BUFFER_SIZE;
		Buffer.Spt.TimeOutValue = 0x1A5E0;
		Buffer.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf);
		Buffer.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, SenseBuf);


		// Fill 
		memset(Buffer.Spt.Cdb, 0, sizeof(Buffer.Spt.Cdb));
		Buffer.Spt.CdbLength = 12;
		Buffer.Spt.Cdb[0] = 0xA1;
		Buffer.Spt.Cdb[1] = (4 << 1) | 0;
		Buffer.Spt.Cdb[2] = (1 << 3) | (1 << 2) | 2;
		Buffer.Spt.Cdb[3] = 0;
		Buffer.Spt.Cdb[4] = 1;
		Buffer.Spt.Cdb[5] = 0;
		Buffer.Spt.Cdb[6] = 0;
		Buffer.Spt.Cdb[7] = 0;
		Buffer.Spt.Cdb[8] = static_cast<UCHAR>(Target);
		Buffer.Spt.Cdb[9] = ID_CMD;


		UINT uLength = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf) + Buffer.Spt.DataTransferLength;
		if (!DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_SCSI_PASS_THROUGH, &Buffer, sizeof(SCSI_PASS_THROUGH), &Buffer, uLength, &returned, FALSE))
		{
			LOG_C_E(L"ªÒ»°”≤≈Ã[%s] –Ú¡–∫≈ ß∞‹", wszDeivePath);
			return FALSE;
		}


		ATA_IDENTIFY_DEVICE AtaIdentifyDevice;
		memcpy(&AtaIdentifyDevice, Buffer.DataBuf, sizeof(AtaIdentifyDevice));

		FormatDiskSerialNumber(AtaIdentifyDevice.SerialNumber);
		wsSerailNumber = libTools::CCharacter::ASCIIToUnicode(AtaIdentifyDevice.SerialNumber);
		wsSerailNumber = libTools::CCharacter::Trim(wsSerailNumber);
		if (!wsSerailNumber.empty())
			return TRUE;
	}

	return FALSE;
}

BOOL libTools::CDiskInfo::GetDiskSerialNumber_ByIdentify(_In_ WCHAR wchDisk, _Out_ std::wstring& wsSerailNumber)
{
#define  DFP_GET_VERSION			0x00074080
#define  DFP_RECEIVE_DRIVE_DATA		0x0007C088
#define  IDE_ATAPI_IDENTIFY			0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY			0xEC  //  Returns ID sector for ATA.

	struct GETVERSIONOUTPARAMS
	{
		BYTE bVersion;      // Binary driver version.
		BYTE bRevision;     // Binary driver revision.
		BYTE bReserved;     // Not used.
		BYTE bIDEDeviceMap; // Bit map of IDE devices.
		DWORD fCapabilities; // Bit mask of driver capabilities.
		DWORD dwReserved[4]; // For future use.
	};


	DWORD dwDeviceNumber = 0;
	if (!libTools::CDiskInfo::GetPhysicalDiskNumber(wchDisk, dwDeviceNumber))
		return FALSE;

	std::wstring wsPhyicalDiskNumber = libTools::CDiskInfo::FormatPhysicalDiskNumber(dwDeviceNumber);
	HANDLE hPhysicalDriveIOCTL = ::CreateFileW(wsPhyicalDiskNumber.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
		return FALSE;


	SetResDeleter(hPhysicalDriveIOCTL, [](HANDLE& h) { ::CloseHandle(h); });



	GETVERSIONOUTPARAMS VersionParams = { 0 };
	DWORD               cbBytesReturned = 0;
	if (!::DeviceIoControl(hPhysicalDriveIOCTL, DFP_GET_VERSION, NULL, 0, &VersionParams, sizeof(VersionParams), &cbBytesReturned, NULL) || VersionParams.bIDEDeviceMap <= 0)
		return FALSE;

	SENDCMDINPARAMS		scip = { 0 };
	BYTE				bIDCmd = (VersionParams.bIDEDeviceMap >> dwDeviceNumber & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY; // IDE or ATAPI IDENTIFY cmd
	BYTE				IdOutCmd[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1] = { 0 };


	SENDCMDINPARAMS* pSCIP = reinterpret_cast<SENDCMDINPARAMS *>(IdOutCmd);
	pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
	pSCIP->irDriveRegs.bFeaturesReg = 0;
	pSCIP->irDriveRegs.bSectorCountReg = 1;
	pSCIP->irDriveRegs.bCylLowReg = 0;
	pSCIP->irDriveRegs.bCylHighReg = 0;


	// Compute the drive number.
	pSCIP->irDriveRegs.bDriveHeadReg = 0xA0 | ((dwDeviceNumber & 1) << 4);


	// The command can either be IDE identify or ATAPI identify.
	pSCIP->irDriveRegs.bCommandReg = bIDCmd;
	pSCIP->bDriveNumber = static_cast<BYTE>(dwDeviceNumber);
	pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;


	if (!::DeviceIoControl(hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA, (LPVOID)pSCIP, sizeof(SENDCMDINPARAMS) - 1, (LPVOID)IdOutCmd, sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1, &cbBytesReturned, NULL))
		return FALSE;


	CHAR szSerialNumber[1024] = { 0 };
	USHORT* Buffer = reinterpret_cast<USHORT *>((reinterpret_cast<SENDCMDOUTPARAMS *>(IdOutCmd))->bBuffer);


	// Max SerialNumber Size = 20
	for (int i = 10, Pos = 0; i <= 19; ++i)
	{
		szSerialNumber[Pos++] = static_cast<CHAR>(Buffer[i] / 256);
		szSerialNumber[Pos++] = static_cast<CHAR>(Buffer[i] % 256);
	}

	wsSerailNumber = libTools::CCharacter::ASCIIToUnicode(std::string(szSerialNumber));
	libTools::CCharacter::Trim(wsSerailNumber);
	return !wsSerailNumber.empty();
}

BOOL libTools::CDiskInfo::IsUsbDiver(_In_ WCHAR wchDisk)
{
	static WCHAR wszDeivePath[MAX_PATH] = { L"\\\\.\\?:" };
	wszDeivePath[4] = wchDisk;


	HANDLE hPhysicalDriveIOCTL = CreateFileW(wszDeivePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhysicalDriveIOCTL == NULL)
	{
		LOG_C_E(L"¥Úø™¥≈≈Ã[%s] ß∞‹!", wszDeivePath);
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
	std::wstring wsPath = L"C:\\";
	wsPath[0] = wchDisk;


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
