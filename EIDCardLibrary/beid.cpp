/*	EID Authentication
    Copyright (C) 2009 Vincent Le Toux

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include "EIDCardLibrary.h"
#include "Tracing.h"
#include "eidlib.h"
#include "eidlibException.h"

#include <DelayImp.h>
#pragma comment(lib, "Delayimp.lib")

// we do all this code to be able to handle if the beid dll is not installed
// on the system
// The trick is that we do Delay Dll Loading
// if the dll is not here, it will trigger a SEH exception
// but cpp try catch block don't catch this exception (sometimes in debug mode)
// so we have to add __try __except block
// and because the code is in cpp with possible exception,
// and because we can only have one exception handling
// we have to write 2 functions.

#ifndef  _M_X64
#pragma comment(lib,"../EIDCardLibrary/beid35libCpp")
// Tell the linker that my DLL should be delay loaded
//#pragma comment(linker, "/DelayLoad:beid35libCpp.dll")


LONG WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pExcPointers) {
   LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;
   PDelayLoadInfo pDelayLoadInfo =
    PDelayLoadInfo(pExcPointers->ExceptionRecord->ExceptionInformation[0]);

   switch (pExcPointers->ExceptionRecord->ExceptionCode) 
   {
   case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
      EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Dll %S was not found", pDelayLoadInfo->szDll);
      break;

   case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
      if (pDelayLoadInfo->dlp.fImportByName) 
	  {
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Function %S was not found in %S",
      	      pDelayLoadInfo->dlp.szProcName, pDelayLoadInfo->szDll);
      } 
	  else 
	  {
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Function ordinal %d was not found in %S",
      	      pDelayLoadInfo->dlp.dwOrdinal, pDelayLoadInfo->szDll);
      }
      break; 

   default:
      // Exception is not related to delay loading
      lDisposition = EXCEPTION_CONTINUE_SEARCH;
      break;
   }

   return(lDisposition);
}

// delayHookFunc - Delay load hooking function
// recover if the dll is not in the path
FARPROC WINAPI delayHookFailureFunc (unsigned dliNotify, PDelayLoadInfo pdli)
{
	UNREFERENCED_PARAMETER(pdli);
	FARPROC fp = NULL;   // Default return value
	HRESULT hr;
	EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Failure %d for %S",dliNotify, pdli->szDll);
	switch (dliNotify) {
	case dliFailLoadLib:
      // LoadLibrary failed.
      // In here a second attempt could be made to load the dll somehow.
      // If fp is still NULL, the ERROR_MOD_NOT_FOUND exception will be raised.
		PWSTR szPath = NULL;
		hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles ,0,NULL,&szPath);
		if (SUCCEEDED(hr))
		{
			szPath = (PWSTR) CoTaskMemRealloc(szPath,256 * sizeof(WCHAR));
			if (szPath)
			{
				wcscat_s(szPath,256,L"\\Belgium Identity Card\\beid35libCpp.dll");
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Loading %s",szPath);
				fp = (FARPROC) LoadLibraryW(szPath);
				CoTaskMemFree(szPath);
			}
		}
		break;
	}

	return(fp);
}

// __delayLoadHelper gets the hook function in here:
PfnDliHook __pfnDliFailureHook2 = delayHookFailureFunc;


#endif


		
#ifndef  _M_X64

using namespace eIDMW;

BOOL GetBEIDCertificateDataCpp(__in LPCTSTR szReaderName,__out LPTSTR *pszContainerName,
							__out PDWORD pdwKeySpec, __out PBYTE *ppbData, __out PDWORD pdwCount,
							__in_opt DWORD dwKeySpec = 0)
{
	BOOL fReturn = FALSE;
	BEID_ReaderSet *m_ReaderSet=NULL;
	BEID_ReaderContext *reader=NULL;
	try
	{
		m_ReaderSet = &BEID_ReaderSet::instance();
	#ifdef UNICODE
		PCHAR szReaderName2 = (PCHAR) EIDAlloc(wcslen(szReaderName) +1);
			
		WideCharToMultiByte(CP_ACP,0,szReaderName, -1, szReaderName2, wcslen(szReaderName)+1,NULL, NULL);
		reader = &m_ReaderSet->getReaderByName(szReaderName2);
		EIDFree(szReaderName2);
	#else
		reader = &m_ReaderSet->getReaderByName(szReaderName);
	#endif
		BEID_EIDCard& card = reader->getEIDCard();
		BEID_ByteArray cardInfo = card.getRawData_CardInfo();
		
		BEID_Certificates& certificates = card.getCertificates();
		BEID_Certificate *authenticationCertificate;
		if (dwKeySpec == AT_SIGNATURE)
		{
			authenticationCertificate = &(certificates.getSignature());
			*pdwKeySpec = AT_SIGNATURE;
		}
		else
		{
			authenticationCertificate = &(certificates.getAuthentication());
			*pdwKeySpec = AT_SIGNATURE;
		}
		
		BEID_ByteArray certificateData = authenticationCertificate->getCertData();
		*pdwCount = certificateData.Size();
		*ppbData = (PBYTE) EIDAlloc(*pdwCount);
		memcpy(*ppbData,certificateData.GetBytes(),*pdwCount);
		const unsigned char * pSerialNum = cardInfo.GetBytes();
		if (dwKeySpec == AT_SIGNATURE)
		{
			DWORD dwSize = sizeof(TEXT("Signature()"))+2*16*sizeof(TCHAR);
			*pszContainerName = (LPTSTR) EIDAlloc(dwSize);
			_stprintf_s(*pszContainerName,dwSize/sizeof(TCHAR),TEXT("Signature(%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)"),
				pSerialNum[0],pSerialNum[1],pSerialNum[2],pSerialNum[3],
				pSerialNum[4],pSerialNum[5],pSerialNum[6],pSerialNum[7],
				pSerialNum[8],pSerialNum[9],pSerialNum[10],pSerialNum[11],
				pSerialNum[12],pSerialNum[13],pSerialNum[14],pSerialNum[15]);
		}
		else
		{
			DWORD dwSize = sizeof(TEXT("Authentication()"))+2*16*sizeof(TCHAR);
			*pszContainerName = (LPTSTR) EIDAlloc(dwSize);
			_stprintf_s(*pszContainerName,dwSize/sizeof(TCHAR),TEXT("Authentication(%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)"),
				pSerialNum[0],pSerialNum[1],pSerialNum[2],pSerialNum[3],
				pSerialNum[4],pSerialNum[5],pSerialNum[6],pSerialNum[7],
				pSerialNum[8],pSerialNum[9],pSerialNum[10],pSerialNum[11],
				pSerialNum[12],pSerialNum[13],pSerialNum[14],pSerialNum[15]);

		}
		fReturn = TRUE;
	}
	catch(BEID_ExCardBadType &ex)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"This is not an eid card (0x%08x)",ex.GetError());
	}
	catch(BEID_ExNoCardPresent &ex)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No card present (0x%08x)",ex.GetError());
	}
	catch(BEID_ExNoReader &ex)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No reader found (0x%08x)",ex.GetError());
	}
	catch(BEID_Exception &ex)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"BEID_Exception exception (0x%08x)",ex.GetError());
	}
	catch ( const std::exception & e )
	{
		// affiche "Exemple d'exception"
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception %S", e.what());
	}
	catch(...)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception");
	}
	BEID_ReleaseSDK();
	return fReturn;
}


PCCERT_CONTEXT GetBEIDCertificateFromCspInfoCpp(__in PEID_SMARTCARD_CSP_INFO pCspInfo)
{
	PCCERT_CONTEXT pCertContext = NULL;
	LPTSTR szContainerName = pCspInfo->bBuffer + pCspInfo->nContainerNameOffset;
	LPTSTR szProviderName = pCspInfo->bBuffer + pCspInfo->nCSPNameOffset;
	LPTSTR szReaderName = pCspInfo->bBuffer + pCspInfo->nReaderNameOffset;

	BEID_ReaderSet *m_ReaderSet=NULL;
	BEID_ReaderContext *reader=NULL;
	try
	{
		m_ReaderSet = &BEID_ReaderSet::instance();
	#ifdef UNICODE
		PCHAR szReaderName2 = (PCHAR) EIDAlloc(wcslen(szReaderName) +1);
			
		WideCharToMultiByte(CP_ACP,0,szReaderName, -1, szReaderName2, wcslen(szReaderName)+1,NULL, NULL);
		reader = &m_ReaderSet->getReaderByName(szReaderName2);
		EIDFree(szReaderName2);
	#else
		reader = &m_ReaderSet->getReaderByName(szReaderName);
	#endif
		BEID_EIDCard& card = reader->getEIDCard();
		BEID_ByteArray cardInfo = card.getRawData_CardInfo();
		
		BEID_Certificates& certificates = card.getCertificates();
		BEID_Certificate *authenticationCertificate;
		/*if (pCspInfo->KeySpec == AT_SIGNATURE)
		{
			authenticationCertificate = &(certificates.getSignature());
		}
		else
		{*/
			authenticationCertificate = &(certificates.getAuthentication());
		//}
		
		BEID_ByteArray certificateData = authenticationCertificate->getCertData();
		pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING, certificateData.GetBytes(), certificateData.Size()); 
		if (pCertContext) {
			// save reference to CSP (else we can't access private key)
			CRYPT_KEY_PROV_INFO KeyProvInfo = {0};
			KeyProvInfo.pwszProvName = szProviderName;
			KeyProvInfo.pwszContainerName = szContainerName;
			KeyProvInfo.dwProvType = PROV_RSA_FULL;
			KeyProvInfo.dwKeySpec = pCspInfo->KeySpec;

			CertSetCertificateContextProperty(pCertContext,CERT_KEY_PROV_INFO_PROP_ID,0,&KeyProvInfo);
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Certificate OK");

		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Unable to CertCreateCertificateContext : %d",GetLastError());
		}
	}
	catch(BEID_ExCardBadType &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"This is not an eid card (0x%08x)",ex.GetError());
	}
    catch(BEID_ExNoCardPresent &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No card present (0x%08x)",ex.GetError());
	}
    catch(BEID_ExNoReader &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No reader found (0x%08x)",ex.GetError());
	}
	catch(BEID_Exception &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"BEID_Exception exception (0x%08x)",ex.GetError());
	}
	catch ( const std::exception & e )
	{
		// affiche "Exemple d'exception"
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception %S", e.what());
	}
	catch(...)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception");
	}
	BEID_ReleaseSDK();
	return pCertContext;
}


BOOL SolveBEIDChallengeCpp(__in PCCERT_CONTEXT pCertContext, __in LPCWSTR Pin)
{
	PSTR szPin = NULL;
	BOOL fAuthenticated = FALSE;
	UNREFERENCED_PARAMETER(pCertContext);
	try
	{
		szPin = (PCHAR) EIDAlloc(wcslen(Pin) +1);
		WideCharToMultiByte(CP_ACP,0,Pin, -1, szPin, wcslen(Pin)+1,NULL, NULL);
		unsigned long ulRemaining=0xFFFF;

		BEID_ReaderContext &reader = ReaderSet.getReader();
		BEID_EIDCard &card = reader.getEIDCard();

		if(card.getPins().getPinByNumber(0).verifyPin(szPin,ulRemaining))
		{
			fAuthenticated = TRUE;
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"verify pin succeeded");
			
		}
		else
		{
			if(ulRemaining==0xFFFF)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"verify pin canceled");
			}
			else
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"verify pin failed (%d tries left)",ulRemaining );
			}
		}
	}
    catch(BEID_ExCardBadType &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"This is not an eid card (0x%08x)",ex.GetError());
	}
    catch(BEID_ExNoCardPresent &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No card present (0x%08x)",ex.GetError());
	}
    catch(BEID_ExNoReader &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No reader found (0x%08x)",ex.GetError());
	}
    catch(BEID_Exception &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"BEID_Exception exception (0x%08x)",ex.GetError());
	}
	catch ( const std::exception & e )
	{
		// affiche "Exemple d'exception"
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception %S", e.what());
	}
	catch(...)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception");
	}
	if(szPin)
		EIDFree(szPin);
	BEID_ReleaseSDK();
	return fAuthenticated;
}
#endif

BOOL GetBEIDCertificateData(__in LPCTSTR szReaderName,__out LPTSTR *pszContainerName,
							__out PDWORD pdwKeySpec, __out PBYTE *ppbData, __out PDWORD pdwCount,
							__in_opt DWORD dwKeySpec = 0)
{
#ifndef  _M_X64
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Retrieving certificate");
	__try
	{
		return GetBEIDCertificateDataCpp(szReaderName, pszContainerName, pdwKeySpec, ppbData,pdwCount,dwKeySpec);
	}
	__except (DelayLoadDllExceptionFilter(GetExceptionInformation())) 
	{
	// Prepare to exit elegantly
		return FALSE;
	}
#else
	UNREFERENCED_PARAMETER(dwKeySpec);
	UNREFERENCED_PARAMETER(pdwCount);
	UNREFERENCED_PARAMETER(ppbData);
	UNREFERENCED_PARAMETER(pdwKeySpec);
	UNREFERENCED_PARAMETER(pszContainerName);
	UNREFERENCED_PARAMETER(szReaderName);
	return FALSE;
#endif
}

PCCERT_CONTEXT GetBEIDCertificateFromCspInfo(__in PEID_SMARTCARD_CSP_INFO pCspInfo)
{
	#ifndef  _M_X64
	__try
	{
		return GetBEIDCertificateFromCspInfoCpp(pCspInfo);
	}
	__except (DelayLoadDllExceptionFilter(GetExceptionInformation())) 
	{
	// Prepare to exit elegantly
		return NULL;
	}
#else
	UNREFERENCED_PARAMETER(pCspInfo);
	return NULL;
#endif
}

BOOL SolveBEIDChallenge(__in PCCERT_CONTEXT pCertContext, __in LPCWSTR Pin)
{
	#ifndef  _M_X64
	__try
	{
		return SolveBEIDChallengeCpp(pCertContext,Pin);
	}
	__except (DelayLoadDllExceptionFilter(GetExceptionInformation())) 
	{
	// Prepare to exit elegantly
		return FALSE;
	}
#else
	UNREFERENCED_PARAMETER(pCertContext);
	UNREFERENCED_PARAMETER(Pin);
	return FALSE;
#endif
}