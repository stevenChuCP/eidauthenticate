#include <windows.h>
#include <tchar.h>
#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/Tracing.h"

/** Used to append a string to a multi string reg key */
void AppendValueToMultiSz(HKEY hKey,PTSTR szKey, PTSTR szValue, PTSTR szData)
{
	HKEY hkResult;
	DWORD Status;
	Status=RegOpenKeyEx(hKey,szKey,0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE,&hkResult);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		return;
	}
	DWORD RegType;
	DWORD RegSize;
	PTSTR Buffer = NULL;
	PTSTR Pointer;
	RegSize = 0;
	Status = RegQueryValueEx( hkResult,szValue,NULL,&RegType,NULL,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		RegCloseKey(hkResult);
		return;
	}
	RegSize += (_tcslen(szData) + 1 ) * sizeof(TCHAR);
	Buffer = (PTSTR) malloc(RegSize);
	if (!Buffer)
	{
		MessageBoxWin32(GetLastError());
		RegCloseKey(hkResult);
		return;
	}
	Status = RegQueryValueEx( hkResult,szValue,NULL,&RegType,(LPBYTE)Buffer,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		RegCloseKey(hkResult);
		free(Buffer);
		return;
	}

	char bFound = FALSE;
	Pointer = Buffer;
	while (*Pointer) 
	{
		if (_tcscmp(Pointer,szData)==0) {
			bFound = TRUE;
			break;
		}
		Pointer = Pointer + _tcslen(Pointer) + 1;
	}
	if (bFound == FALSE) {
		// add the data
		_tcscat_s(Pointer, _tcslen(szData) + 1, szData);
		Status = RegSetValueEx(hkResult, szValue, 0, RegType, (PBYTE) Buffer, RegSize);
		if (Status != ERROR_SUCCESS) {
			MessageBoxWin32(Status);
		}
	}
	free(Buffer);
	RegCloseKey(hkResult);
}

/** Used to Remove a string to a multi string reg key */
void RemoveValueFromMultiSz(HKEY hKey, PTSTR szKey, PTSTR szValue, PTSTR szData)
{
	HKEY hkResult;
	DWORD Status;
	Status=RegOpenKeyEx(hKey,szKey,0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE,&hkResult);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		return;
	}
	DWORD RegType;
	DWORD RegSize;
	PTSTR BufferIn = NULL;
	PTSTR BufferOut = NULL;
	PTSTR PointerIn;
	PTSTR PointerOut;
	RegSize = 0;
	Status = RegQueryValueEx( hkResult,szValue,NULL,&RegType,NULL,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		RegCloseKey(hkResult);
		return;
	}
	BufferIn = (PTSTR) malloc(RegSize);
	if (!BufferIn)
	{
		MessageBoxWin32(GetLastError());
		RegCloseKey(hkResult);
		return;
	}
	BufferOut = (PTSTR) malloc(RegSize);
	if (!BufferIn)
	{
		MessageBoxWin32(GetLastError());
		free(BufferIn);
		RegCloseKey(hkResult);
		return;
	}
	Status = RegQueryValueEx( hkResult,szValue,NULL,&RegType,(LPBYTE)BufferIn,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		free(BufferIn);
		free(BufferOut);
		RegCloseKey(hkResult);
		return;
	}

	PointerIn = BufferIn;
	PointerOut = BufferOut;
	while (*PointerIn) 
	{
		if (_tcscmp(PointerIn,szData)!=0) {
			_tcscpy_s(PointerOut,RegSize, PointerIn);
			PointerOut = PointerOut + _tcslen(PointerOut) + 1;
		}
		PointerIn = PointerIn + _tcslen(PointerIn) + 1;
	}
	Status = RegSetValueEx(hkResult, szValue, 0, RegType, (PBYTE) PointerOut, RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
	}
	free(BufferIn);
	free(BufferOut);
	RegCloseKey(hkResult);
}

/** Installation and uninstallation routine
*/

void EIDAuthenticationPackageDllRegister()
{
	AppendValueToMultiSz(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Lsa"), TEXT("Authentication Packages"), AUTHENTICATIONPACKAGENAMET);
}

void EIDAuthenticationPackageDllUnRegister()
{
	RemoveValueFromMultiSz(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Lsa"), TEXT("Authentication Packages"), AUTHENTICATIONPACKAGENAMET);
}

void EIDPasswordChangeNotificationDllRegister()
{
	AppendValueToMultiSz(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Lsa"), TEXT("Notification Packages"), TEXT("EIDPasswordChangeNotification"));
}

void EIDPasswordChangeNotificationDllUnRegister()
{
	RemoveValueFromMultiSz(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Lsa"), TEXT("Notification Packages"), TEXT("EIDPasswordChangeNotification"));
}


void EIDCredentialProviderDllRegister()
{
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		NULL, REG_SZ, TEXT("EidCredentialProvider"),sizeof(TEXT("EidCredentialProvider")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		NULL, REG_SZ, TEXT("EidCredentialProvider"),sizeof(TEXT("EidCredentialProvider")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}\\InprocServer32"),
		NULL, REG_SZ, TEXT("EidCredentialProvider.dll"),sizeof(TEXT("EidCredentialProvider.dll")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}\\InprocServer32"),
		TEXT("ThreadingModel"),REG_SZ, TEXT("Apartment"),sizeof(TEXT("Apartment")));
}

void EIDCredentialProviderDllUnRegister()
{
	RegDeleteTree(HKEY_CLASSES_ROOT, TEXT("CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"));
	RegDeleteTree(HKEY_LOCAL_MACHINE, 
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"));
}

void EIDConfigurationWizardDllRegister()
{
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"),
		NULL,REG_SZ, TEXT("EIDConfigurationWizard"),sizeof(TEXT("EIDConfigurationWizard")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"), 
		NULL, REG_SZ, TEXT("EIDConfigurationWizard"),sizeof(TEXT("EIDConfigurationWizard")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"),
		TEXT("System.ApplicationName"),REG_SZ, TEXT("EID.EIDConfigurationWizard"),sizeof(TEXT("EID.EIDConfigurationWizard")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"),
		TEXT("System.ControlPanel.Category"),REG_SZ, TEXT("9.1"),sizeof(TEXT("9.1")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"),
		TEXT("LocalizedString"),REG_EXPAND_SZ, TEXT("Smart Card Logon"),sizeof(TEXT("Smart Card Logon")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"),
		TEXT("InfoTip"),REG_EXPAND_SZ, TEXT("Smart Card Logon"),sizeof(TEXT("Smart Card Logon")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}\\DefaultIcon"),
		NULL,REG_EXPAND_SZ, TEXT("%SystemRoot%\\system32\\imageres.dll,-58"),
			sizeof(TEXT("%SystemRoot%\\system32\\imageres.dll,-58")));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}\\Shell\\Open\\Command"),
		NULL,REG_EXPAND_SZ, TEXT("%SystemRoot%\\system32\\EIDConfigurationWizard.exe"),
			sizeof(TEXT("%SystemRoot%\\system32\\EIDConfigurationWizard.exe")));


}

void EIDConfigurationWizardDllUnRegister()
{
	RegDeleteTree(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"));
	RegDeleteTree(HKEY_CLASSES_ROOT, TEXT("CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}"));
}

void EnableLogging()
{
	DWORD qdwValue[2] = {0,0};
	DWORD dwValue;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("Guid"), REG_SZ, TEXT("{B4866A0A-DB08-4835-A26F-414B46F3244C}"),sizeof(TEXT("{B4866A0A-DB08-4835-A26F-414B46F3244C}")));
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("FileName"), REG_SZ, TEXT("C:\\Windows\\System32\\LogFiles\\WMI\\EIDCredentialProvider.etl"),sizeof(TEXT("C:\\Windows\\System32\\LogFiles\\WMI\\EIDCredentialProvider.etl")));
	dwValue = 8;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("FileMax"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 1;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("Start"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 8;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("BufferSize"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 0;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("FlushTimer"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 0;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("MaximumBuffers"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 0;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("MinimumBuffers"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 1;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("ClockType"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 64;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("MaxFileSize"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 1300;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("LogFileMode"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 5;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("FileCounter"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 0;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"), 
		TEXT("Status"), REG_SZ,&dwValue,sizeof(DWORD));


	dwValue = 1;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		TEXT("Enabled"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 5;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		TEXT("EnableLevel"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 0;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		TEXT("EnableProperty"), REG_SZ,&dwValue,sizeof(DWORD));
	dwValue = 1;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		TEXT("Status"), REG_SZ,&dwValue,sizeof(DWORD));
	
	dwValue = 1;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		TEXT("MatchAllKeyword"), REG_SZ,qdwValue,sizeof(REG_QWORD));
	dwValue = 1;
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}"), 
		TEXT("MatchAnyKeyword"), REG_SZ,qdwValue,sizeof(REG_QWORD));
	
}

void DisableLogging()
{
	RegDeleteTree(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider"));
}