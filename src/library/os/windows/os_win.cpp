#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cwctype>
#include <set>
#include <vector>
#include <licensecc/datatypes.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <winioctl.h>

#include "../../base/string_utils.h"
#include "../../base/logger.h"
#include "../os.h"
#include "../os_common.h"
#include "../cpu_info.hpp"
using namespace std;

FUNCTION_RETURN getMachineName(unsigned char identifier[6]) {
	FUNCTION_RETURN result = FUNC_RET_ERROR;
	char buffer[MAX_COMPUTERNAME_LENGTH + 1];
	int bufsize = MAX_COMPUTERNAME_LENGTH + 1;
	const BOOL cmpName = GetComputerName(buffer, (unsigned long*)&bufsize);
	if (cmpName) {
		strncpy(reinterpret_cast<char*>(identifier), buffer, 6);
		result = FUNC_RET_OK;
	}
	return result;
}

static bool getPhysicalDiskNumber(const char* drive, DWORD& diskNumber) {
	const std::string volumePath = std::string("\\\\.\\") + drive[0] + ':';
	HANDLE volume = CreateFileA(volumePath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
								   nullptr, OPEN_EXISTING, 0, nullptr);
	if (volume == INVALID_HANDLE_VALUE) {
		return false;
	}

	VOLUME_DISK_EXTENTS extents = {};
	DWORD bytesReturned = 0;
	const BOOL success = DeviceIoControl(volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0,
											 &extents, sizeof(extents), &bytesReturned, nullptr);
	CloseHandle(volume);
	// A volume spanning multiple extents needs a larger buffer; it has no unambiguous disk here.
	if (!success || extents.NumberOfDiskExtents != 1) {
		return false;
	}
	diskNumber = extents.Extents[0].DiskNumber;
	return true;
}

static bool getPhysicalDiskSerial(DWORD diskNumber, std::string& serial) {
	const std::string diskPath = "\\\\.\\PhysicalDrive" + std::to_string(diskNumber);
	HANDLE disk = CreateFileA(diskPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							   nullptr, OPEN_EXISTING, 0, nullptr);
	if (disk == INVALID_HANDLE_VALUE) {
		return false;
	}

	STORAGE_PROPERTY_QUERY query = {};
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;
	STORAGE_DESCRIPTOR_HEADER header = {};
	DWORD bytesReturned = 0;
	bool found = false;
	if (DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &header, sizeof(header),
						&bytesReturned, nullptr) && header.Size >= sizeof(STORAGE_DEVICE_DESCRIPTOR) &&
		header.Size <= 65536) {
		std::vector<unsigned char> buffer(header.Size);
		if (DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer.data(),
							header.Size, &bytesReturned, nullptr) && bytesReturned >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
			STORAGE_DEVICE_DESCRIPTOR descriptor = {};
			std::memcpy(&descriptor, buffer.data(), sizeof(descriptor));
			const DWORD offset = descriptor.SerialNumberOffset;
			if (offset >= offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties) && offset < bytesReturned) {
				const char* begin = reinterpret_cast<const char*>(buffer.data() + offset);
				const char* end = reinterpret_cast<const char*>(buffer.data() + bytesReturned);
				const char* terminator = std::find(begin, end, '\0');
				if (terminator != end) {
					serial.assign(begin, terminator);
					const size_t first = serial.find_first_not_of(" \t\r\n");
					const size_t last = serial.find_last_not_of(" \t\r\n");
					if (first != std::string::npos) {
						serial = serial.substr(first, last - first + 1);
						found = true;
					}
				}
			}
		}
	}
	CloseHandle(disk);
	return found;
}

static bool getVolumeSerial(const char* drive, std::string& serial) {
	DWORD volumeSerial = 0;
	if (!GetVolumeInformationA(drive, nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0)) {
		return false;
	}
	serial = std::to_string(volumeSerial);
	return true;
}

FUNCTION_RETURN getDiskInfos(std::vector<DiskInfo>& diskInfos) {
	char logicalDrives[MAX_PATH] = {};
	const DWORD length = GetLogicalDriveStringsA(MAX_PATH, logicalDrives);
	if (length == 0 || length >= MAX_PATH) {
		return FUNC_RET_NOT_AVAIL;
	}

	std::set<DWORD> seenDisks;
	for (const char* drive = logicalDrives; *drive; drive += std::strlen(drive) + 1) {
		if (GetDriveTypeA(drive) != DRIVE_FIXED) {
			continue;
		}
		DWORD diskNumber = 0;
		const bool diskMapped = getPhysicalDiskNumber(drive, diskNumber);
		if (diskMapped && seenDisks.count(diskNumber) != 0) {
			for (auto& diskInfo : diskInfos) {
				if (diskInfo.disk_phys_id_initialized && diskInfo.id == static_cast<int>(diskNumber) &&
					drive[0] == 'C') {
					diskInfo.preferred = true;
				}
			}
			continue;
		}
		std::string serial;
		const bool physicalSerialAvailable = diskMapped && getPhysicalDiskSerial(diskNumber, serial);
		DiskInfo diskInfo = {};
		diskInfo.id = diskMapped ? static_cast<int>(diskNumber) : -1;
		license::mstrlcpy(diskInfo.device, drive, sizeof(diskInfo.device));
		if (physicalSerialAvailable) {
			seenDisks.insert(diskNumber);
			diskInfo.disk_phys_id = serial;
			diskInfo.disk_phys_id_initialized = true;
		} else {
			LOG_WARN("Cannot read physical disk serial for %s; falling back to volume serial", drive);
			if (!getVolumeSerial(drive, serial)) {
				LOG_WARN("Cannot read volume serial for %s", drive);
				continue;
			}
			diskInfo.disk_uuid = serial;
			diskInfo.uuid_initialized = true;
		}
		diskInfo.preferred = (drive[0] == 'C');
		diskInfos.push_back(diskInfo);
	}
	return diskInfos.empty() ? FUNC_RET_NOT_AVAIL : FUNC_RET_OK;
}

FUNCTION_RETURN getModuleName(char buffer[MAX_PATH]) {
	FUNCTION_RETURN result = FUNC_RET_OK;
	const DWORD wres = GetModuleFileName(NULL, buffer, MAX_PATH);
	if (wres == 0) {
		result = FUNC_RET_ERROR;
	}
	return result;
}

/*
 * GetSystemIdForPublisher is an *undocumented* API (not declared in any public
 * Windows SDK header). It is exported from kernel32.dll (forwarded to
 * api-ms-win-core-sysinfo-l1-2-3.dll) only on Windows 10 1703 (Creators Update)
 * and later, and returns a 16-byte GUID-like identifier tied to the Windows
 * installation.
 *
 * We declare the prototype ourselves and resolve it at runtime with
 * GetProcAddress, so that on older Windows 10 builds (1507/1511/1607), where the
 * export does not exist, the module still loads and we can fall back to the
 * registry instead of failing at load time. Requires no special privileges.
 */
typedef BOOL(WINAPI* PFN_GET_SYSTEM_ID_FOR_PUBLISHER)(BYTE* buffer, DWORD* buffer_size);

static FUNCTION_RETURN getSystemIdForPublisher(std::string& identifier) {
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (hKernel32 == NULL) {
		return FUNC_RET_NOT_AVAIL;
	}
	PFN_GET_SYSTEM_ID_FOR_PUBLISHER pfn =
		reinterpret_cast<PFN_GET_SYSTEM_ID_FOR_PUBLISHER>(GetProcAddress(hKernel32, "GetSystemIdForPublisher"));
	if (pfn == NULL) {
		// Not available before Windows 10 1703.
		return FUNC_RET_NOT_AVAIL;
	}
	BYTE systemId[16] = {0};
	DWORD systemIdSize = sizeof(systemId);
	if (!pfn(systemId, &systemIdSize)) {
		LOG_DEBUG("GetSystemIdForPublisher failed, error: %lu", (unsigned long)GetLastError());
		return FUNC_RET_NOT_AVAIL;
	}
	identifier = guidToString(systemId);
	return FUNC_RET_OK;
}

static std::string wideStringToString(const std::wstring& wstr) {
	if (wstr.empty()) {
		return std::string();
	}
	const int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
	if (size <= 0) {
		return std::string();
	}
	std::string str(static_cast<size_t>(size), '\0');
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], size, NULL, NULL);
	return str;
}

static FUNCTION_RETURN readRegistryString(HKEY rootKey, const wchar_t* subKey, const wchar_t* valueName,
										  std::string& out) {
	HKEY hKey = NULL;
	LONG result = RegOpenKeyExW(rootKey, subKey, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS) {
		LOG_DEBUG("RegOpenKeyExW(%ls) failed, error: %ld", subKey, result);
		return FUNC_RET_NOT_AVAIL;
	}
	DWORD type = 0;
	DWORD dataSize = 0;
	result = RegQueryValueExW(hKey, valueName, NULL, &type, NULL, &dataSize);
	if (result != ERROR_SUCCESS || type != REG_SZ || dataSize == 0) {
		RegCloseKey(hKey);
		return FUNC_RET_NOT_AVAIL;
	}
	std::wstring value(dataSize / sizeof(wchar_t), L'\0');
	result = RegQueryValueExW(hKey, valueName, NULL, NULL, reinterpret_cast<LPBYTE>(&value[0]), &dataSize);
	RegCloseKey(hKey);
	if (result != ERROR_SUCCESS) {
		return FUNC_RET_NOT_AVAIL;
	}
	// Strip anything after the terminating NUL and any trailing whitespace.
	std::wstring::size_type end = value.find(L'\0');
	std::wstring trimmed = value.substr(0, end);
	while (!trimmed.empty() && iswspace(trimmed[trimmed.size() - 1])) {
		trimmed.erase(trimmed.size() - 1);
	}
	if (trimmed.empty()) {
		return FUNC_RET_NOT_AVAIL;
	}
	out = wideStringToString(trimmed);
	return out.empty() ? FUNC_RET_NOT_AVAIL : FUNC_RET_OK;
}

FUNCTION_RETURN getOsSpecificIdentifier(std::string& identifier) {
	// 1) Preferred: undocumented GetSystemIdForPublisher (Windows 10 1703+),
	//    resolved at runtime so older Windows 10 builds fall through cleanly.
	FUNCTION_RETURN ret = getSystemIdForPublisher(identifier);
	if (ret == FUNC_RET_OK) {
		return ret;
	}
	ret = readRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid", identifier);
	if (ret == FUNC_RET_OK) {
		return ret;
	}
	ret = readRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductId",
							 identifier);
	return ret;
}
