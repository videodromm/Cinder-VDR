#pragma once
#include "cinder/Cinder.h"
#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header is seen, otherwise asio/websocketpp later in
// the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#if defined( CINDER_MSW )
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <string>

namespace videodromm
{
	// Identifies this computer, for settings that differ per machine (audio devices, ...) in json
	// files shared between PCs (the assets folder is synced with Syncthing).
	namespace machine
	{
		// Windows: the MachineGuid Windows generates at install time
		// (HKLM\SOFTWARE\Microsoft\Cryptography). Stable across renames and reboots, unique per
		// Windows install (a cloned disk image would share it). Falls back to the computer name.
		// Mac: the host name for now (TODO: IOPlatformUUID via IOKit, needs the IOKit framework).
		inline std::string getName() {
#if defined( CINDER_MSW )
			char name[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
			DWORD size = sizeof(name);
			if (::GetComputerNameA(name, &size)) return std::string(name, size);
			return "unknown";
#else
			char name[256] = { 0 };
			if (gethostname(name, sizeof(name) - 1) == 0) return std::string(name);
			return "unknown";
#endif
		}
		inline std::string getId() {
#if defined( CINDER_MSW )
			// KEY_WOW64_64KEY: read the 64-bit key even from a 32-bit build
			HKEY key = nullptr;
			if (::RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS) {
				char guid[64] = { 0 };
				DWORD size = sizeof(guid) - 1;
				DWORD type = 0;
				LONG result = ::RegQueryValueExA(key, "MachineGuid", nullptr, &type, reinterpret_cast<LPBYTE>(guid), &size);
				::RegCloseKey(key);
				if (result == ERROR_SUCCESS && type == REG_SZ && guid[0]) return std::string(guid);
			}
#endif
			return getName();
		}
	}
}
