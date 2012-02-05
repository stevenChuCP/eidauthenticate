#include <windows.h>
#include <tchar.h>
#include <credentialprovider.h>

#include "EIDConfigurationWizardXP.h"
#include "globalXP.h"

#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/Registration.h"
#include "../EIDCardLibrary/CertificateUtilities.h"
#include "../EIDCardLibrary/CertificateValidation.h"
#include "../EIDCardLibrary/XPCompatibility.h"

#pragma comment(lib,"comctl32")
#pragma comment(lib,"Netapi32")
#pragma comment(lib,"Winscard")
#pragma comment(lib,"Scarddlg")

#ifdef UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

INT_PTR CALLBACK	WndProc_01MAIN(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_02ENABLE(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_03NEW(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_04CHECKS(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_05PASSWORD(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_06TESTRESULTOK(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_07TESTRESULTNOTOK(HWND, UINT, WPARAM, LPARAM);

BOOL fHasAlreadySmartCardCredential = FALSE;
BOOL fShowNewCertificatePanel;
BOOL fGotoNewScreen = FALSE;
HINSTANCE g_hinst;
WCHAR szReader[256];
DWORD dwReaderSize = ARRAYSIZE(szReader);
WCHAR szCard[256];
DWORD dwCardSize = ARRAYSIZE(szCard);
WCHAR szUserName[256];
DWORD dwUserNameSize = ARRAYSIZE(szUserName);
WCHAR szPassword[256];
DWORD dwPasswordSize = ARRAYSIZE(szPassword);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	// check that the authentication package is loaded
	if (!IsEIDPackageAvailable())
	{
		TCHAR szMessage[256] = TEXT("");
		LoadString(g_hinst,IDS_EIDNOTAVAILABLE, szMessage, ARRAYSIZE(szMessage));
		MessageBox(NULL,szMessage,TEXT("Error"),MB_ICONERROR);
		return -1;
	}
	// check that the user is not connected to a domain
	if (IsCurrentUserBelongToADomain())
	{
		TCHAR szMessage[2000] = TEXT("");
		LoadString(g_hinst,IDS_NODOMAINACCOUNT, szMessage, ARRAYSIZE(szMessage));
		MessageBox(NULL,szMessage,TEXT("Error"),MB_ICONERROR);
		return -1;
	}
	g_hinst = hInstance;
	int iNumArgs;
	LPWSTR *pszCommandLine =  CommandLineToArgvW(lpCmdLine,&iNumArgs);

	DWORD dwSize = dwUserNameSize;
	GetUserName(szUserName,&dwSize);

	if (iNumArgs >= 1)
	{
		
		if (_tcscmp(pszCommandLine[0],TEXT("NEW_USERNAME")) == 0)
		{
			fGotoNewScreen = TRUE;
			if (iNumArgs > 1)
			{
				_tcscpy_s(szUserName,dwUserNameSize, pszCommandLine[1]);
			}
		}
		else if (_tcscmp(pszCommandLine[0],TEXT("DIALOGREMOVEPOLICY")) == 0)
		{
			DialogRemovePolicy();
			return 0;
		} 
		else if (_tcscmp(pszCommandLine[0],TEXT("DIALOGFORCEPOLICY")) == 0)
		{
			DialogForceSmartCardLogonPolicy();
			return 0;
		} 
		else if (_tcscmp(pszCommandLine[0],TEXT("ENABLESIGNATUREONLY")) == 0)
		{
			DWORD dwValue = 1;
			RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
				TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\SmartCardCredentialProvider"),
				TEXT("AllowSignatureOnlyKeys"), REG_DWORD, &dwValue,sizeof(dwValue));
			return 0;
		}
		else if (_tcscmp(pszCommandLine[0],TEXT("ENABLENOEKU")) == 0)
		{
			DWORD dwValue = 1;
			RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
				TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\SmartCardCredentialProvider"),
				TEXT("AllowCertificatesWithNoEKU"), REG_DWORD, &dwValue,sizeof(dwValue));
			return 0;
		}
		else if (_tcscmp(pszCommandLine[0],TEXT("TRUST")) == 0)
		{
			if (iNumArgs < 2)
			{
				return 0;
			}
			DWORD dwSize = 0;
			CryptStringToBinary(pszCommandLine[1],0,CRYPT_STRING_BASE64,NULL,&dwSize,NULL,NULL);
			PBYTE pbCertificate = (PBYTE) EIDAlloc(dwSize);
			CryptStringToBinary(pszCommandLine[1],0,CRYPT_STRING_BASE64,pbCertificate,&dwSize,NULL,NULL);
			PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,pbCertificate, dwSize);
			if (pCertContext)
			{
				MakeTrustedCertifcate(pCertContext);
				CertFreeCertificateContext(pCertContext);
			}
			EIDFree(pbCertificate);
			return 0;
		} 
	}

	HPROPSHEETPAGE ahpsp[7];
	TCHAR szTitle[256] = TEXT("");
	fHasAlreadySmartCardCredential = TRUE;

	PROPSHEETPAGE psp = { sizeof(psp) };   
	psp.hInstance = hInstance;
	psp.dwFlags =  PSP_USEHEADERTITLE;
	psp.lParam = 0;//(LPARAM) &wizdata;
	
	LoadString(g_hinst,IDS_TITLE0, szTitle, ARRAYSIZE(szTitle));
	psp.pszHeaderTitle = szTitle;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_01MAIN);
	psp.pfnDlgProc = WndProc_01MAIN;
	ahpsp[0] = CreatePropertySheetPage(&psp);

	LoadString(g_hinst,IDS_TITLE1, szTitle, ARRAYSIZE(szTitle));
	psp.pszHeaderTitle = szTitle;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_02ENABLE);
	psp.pfnDlgProc = WndProc_02ENABLE;
	ahpsp[1] = CreatePropertySheetPage(&psp);

	LoadString(g_hinst,IDS_TITLE2, szTitle, ARRAYSIZE(szTitle));
	psp.pszHeaderTitle = szTitle;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_03NEW);
	psp.pfnDlgProc = WndProc_03NEW;
	ahpsp[2] = CreatePropertySheetPage(&psp);

	LoadString(g_hinst,IDS_TITLE3, szTitle, ARRAYSIZE(szTitle));
	psp.pszHeaderTitle = szTitle;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_04CHECKS);
	psp.pfnDlgProc = WndProc_04CHECKS;
	ahpsp[3] = CreatePropertySheetPage(&psp);

	LoadString(g_hinst,IDS_TITLE4, szTitle, ARRAYSIZE(szTitle));
	psp.pszHeaderTitle = szTitle;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_05PASSWORD);
	psp.pfnDlgProc = WndProc_05PASSWORD;
	ahpsp[4] = CreatePropertySheetPage(&psp);

	LoadString(g_hinst,IDS_TITLE5, szTitle, ARRAYSIZE(szTitle));
	psp.pszHeaderTitle = szTitle;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_06TESTRESULTOK);
	psp.pfnDlgProc = WndProc_06TESTRESULTOK;
	ahpsp[5] = CreatePropertySheetPage(&psp);

	LoadString(g_hinst,IDS_TITLE5, szTitle, ARRAYSIZE(szTitle));
	psp.pszHeaderTitle = szTitle;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_07TESTRESULTNOTOK);
	psp.pfnDlgProc = WndProc_07TESTRESULTNOTOK;
	ahpsp[6] = CreatePropertySheetPage(&psp);

	PROPSHEETHEADER psh;
	psh.dwSize = sizeof(psh);
	psh.hInstance = hInstance;
	psh.hwndParent = NULL;
	psh.phpage = ahpsp;
	psh.dwFlags = PSH_WIZARD | PSH_USEHICON ;
	psh.pszbmWatermark = 0;
	psh.pszbmHeader = 0;
	psh.nStartPage = 1;
	psh.nPages = ARRAYSIZE(ahpsp);
	psh.hIcon = NULL;
	LoadString(g_hinst,IDS_CAPTION, szTitle, ARRAYSIZE(szTitle));
	psh.pszCaption = szTitle;

	HMODULE hDll = LoadLibrary(TEXT("shell32.dll") );
	if (hDll)
	{
		psh.hIcon = LoadIcon(hDll, MAKEINTRESOURCE(13));
		FreeLibrary(hDll);
	}

	fHasAlreadySmartCardCredential = LsaEIDHasStoredCredential(NULL);
	if (fGotoNewScreen)
	{
		if (AskForCard(szReader, dwReaderSize, szCard, dwCardSize))
		{
			psh.nStartPage = 2;
		}
		else
		{
			psh.nStartPage = 1;
		}
	}
	else if (fHasAlreadySmartCardCredential)
	{
		// 01MAIN
		psh.nStartPage = 0;
	}
	else
	{
		// 02ENABLE
		psh.nStartPage = 1;
	}
	INT_PTR rc = PropertySheet(&psh);
	if (rc == -1)
	{
		MessageBoxWin32(GetLastError());
	}
	//_CrtDumpMemoryLeaks();
    return 0;

}

