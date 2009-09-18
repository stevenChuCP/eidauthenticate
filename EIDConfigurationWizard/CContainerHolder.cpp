
#include <windows.h>
#include <tchar.h>
#include <credentialProvider.h>
#include "global.h"
#include "eidconfigurationwizard.h"
#include "../EIDCardLibrary/CContainer.h"
#include "../EIDCardLibrary/CertificateValidation.h"
#include "../EIDCardLibrary/StoredCredentialManagement.h"
#include "CContainerHolder.h"

#define CHECK_FAILED 0
#define CHECK_WARNING 1
#define CHECK_SUCCESS 2
#define CHECK_INFO 3

#define CHECK_USERNAME 0
#define CHECK_TRUST 1
#define CHECK_CRYPTO 2


#define ERRORTOTEXT(ERROR) case ERROR: LoadString( g_hinst,IDS_##ERROR, szName, dwSize);                 break;
BOOL GetTrustErrorMessage(DWORD dwError, PTSTR szName, DWORD dwSize)
{
    BOOL fReturn = TRUE;
	switch(dwError)
    {
		ERRORTOTEXT(CERT_TRUST_NO_ERROR)
		ERRORTOTEXT(CERT_TRUST_IS_NOT_TIME_VALID)
		ERRORTOTEXT(CERT_TRUST_IS_NOT_TIME_NESTED)
		ERRORTOTEXT(CERT_TRUST_IS_REVOKED)
		ERRORTOTEXT(CERT_TRUST_IS_NOT_SIGNATURE_VALID)
		ERRORTOTEXT(CERT_TRUST_IS_NOT_VALID_FOR_USAGE)
		ERRORTOTEXT(CERT_TRUST_IS_UNTRUSTED_ROOT)
		ERRORTOTEXT(CERT_TRUST_REVOCATION_STATUS_UNKNOWN)
		ERRORTOTEXT(CERT_TRUST_IS_CYCLIC)
		ERRORTOTEXT(CERT_TRUST_IS_PARTIAL_CHAIN)
		ERRORTOTEXT(CERT_TRUST_CTL_IS_NOT_TIME_VALID)
		ERRORTOTEXT(CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID)
		ERRORTOTEXT(CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE)
		default:                            
			fReturn = FALSE;
		break;
    }
	return fReturn;
} 

CContainerHolderTest::CContainerHolderTest(CContainer* pContainer)
{
	_pContainer = pContainer;
	_IsTrusted = IsTrusted();
	_SupportEncryption = SupportEncryption();
	_HasCurrentUserName = HasCurrentUserName();
}

CContainerHolderTest::~CContainerHolderTest()
{
	if (_pContainer)
	{
		delete _pContainer;
	}
}
void CContainerHolderTest::Release()
{
	delete this;
}

DWORD CContainerHolderTest::GetIconIndex()
{
	if (_IsTrusted && _HasCurrentUserName)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
BOOL CContainerHolderTest::IsTrusted()
{
	BOOL fReturn = FALSE;
	PCCERT_CONTEXT pCertContext = _pContainer->GetCertificate();
	if (pCertContext)
	{
		fReturn = IsTrustedCertificate(pCertContext);
		_dwTrustError = GetLastError();
		CertFreeCertificateContext(pCertContext);
	}
	return fReturn;
}
BOOL CContainerHolderTest::SupportEncryption()
{
	BOOL fReturn = FALSE;
	PCCERT_CONTEXT pCertContext = _pContainer->GetCertificate();
	if (pCertContext)
	{
		fReturn = CanEncryptPassword(NULL,0,pCertContext);
		CertFreeCertificateContext(pCertContext);
	}
	return fReturn;
}

BOOL CContainerHolderTest::HasCurrentUserName()
{
	TCHAR szUserName[1024] = TEXT("");
	DWORD dwSize = ARRAYSIZE(szUserName);
	GetUserName(szUserName, &dwSize);
	return _tcscmp(_pContainer->GetUserName(),szUserName)==0;
}

CContainer* CContainerHolderTest::GetContainer()
{
	return _pContainer;
}

int CContainerHolderTest::GetCheckCount()
{
	return 3;
}
int CContainerHolderTest::GetImage(DWORD dwCheckNum)
{
	
	switch(dwCheckNum)
	{
	case CHECK_USERNAME: 
		if (_HasCurrentUserName)
			return CHECK_SUCCESS;
		else
			return CHECK_FAILED;
		break;
	case CHECK_TRUST: 
		if (_IsTrusted)
			return CHECK_SUCCESS;
		else
			return CHECK_FAILED;
		break;
	case CHECK_CRYPTO: 
		if (_SupportEncryption)
			return CHECK_SUCCESS;
		else
			return CHECK_WARNING;
		break;
	}
	return 0;
}
PTSTR CContainerHolderTest::GetDescription(DWORD dwCheckNum)
{
	DWORD dwWords = 1024;
	PTSTR szDescription = (PTSTR) malloc(dwWords * sizeof(TCHAR));
	if (!szDescription) return NULL;
	szDescription[0] = 0;
	switch(dwCheckNum)
	{
	case CHECK_USERNAME: 
		if (_HasCurrentUserName)
			LoadString(g_hinst,IDS_04USERNAMEOK,szDescription, dwWords);
		else
			LoadString(g_hinst,IDS_04USERNAMENOK,szDescription, dwWords);
		break;
	case CHECK_TRUST: 
		if (_IsTrusted)
			LoadString(g_hinst,IDS_04TRUSTOK,szDescription, dwWords);
		else
		{
			if (!GetTrustErrorMessage(_dwTrustError,szDescription,dwWords))
			{
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,_dwTrustError,0,szDescription,dwWords,NULL);
			}
		}
		break;
	case CHECK_CRYPTO: 
		if (_SupportEncryption)
			LoadString(g_hinst,IDS_04ENCRYPTIONOK,szDescription, dwWords);
		else
			LoadString(g_hinst,IDS_04ENCRYPTIONNOK,szDescription, dwWords);
		break;
	}
	return szDescription;
}

PTSTR CContainerHolderTest::GetSolveDescription(DWORD dwCheckNum)
{
	DWORD dwWords = 1024;
	PTSTR szDescription = (PTSTR) malloc(dwWords * sizeof(TCHAR));
	if (!szDescription) return NULL;
	szDescription[0] = 0;
	switch(dwCheckNum)
	{
	case CHECK_USERNAME: 
		if (!_HasCurrentUserName)
		{
			LoadString(g_hinst,IDS_04USERNAMERENAME,szDescription, dwWords);
		}
		break;
	case CHECK_TRUST: 
		if (!_IsTrusted)
		{
			switch (_dwTrustError)
			{
			case CERT_TRUST_IS_PARTIAL_CHAIN:
				LoadString(g_hinst,IDS_04TRUSTMAKETRUSTED,szDescription, dwWords);
				break;
			case CERT_TRUST_IS_NOT_VALID_FOR_USAGE:
				LoadString(g_hinst,IDS_04TRUSTENABLENOEKU,szDescription, dwWords);
				break;
			}
		}
		break;
	}
	return szDescription;
}

BOOL CContainerHolderTest::Solve(DWORD dwCheckNum)
{
	BOOL fReturn = FALSE;
	DWORD dwError = 0;
	switch(dwCheckNum)
	{
	case CHECK_USERNAME:
		//WinExec("control /name Microsoft.UserAccounts /page pageRenameMyAccount", SW_NORMAL);
		{
			
			
				
			if (IsElevated())
			{
				RenameAccount(_pContainer->GetUserName());
			}
			else
			{
				SHELLEXECUTEINFO shExecInfo;
				TCHAR szName[1024];
				TCHAR szParameter[50] = TEXT("RENAMEUSER ");
				GetModuleFileName(GetModuleHandle(NULL),szName, ARRAYSIZE(szName));
				_tcscat_s(szParameter, ARRAYSIZE(szParameter), _pContainer->GetUserName());

				shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

				shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
				shExecInfo.hwnd = NULL;
				shExecInfo.lpVerb = TEXT("runas");
				shExecInfo.lpFile = szName;
				shExecInfo.lpParameters = szParameter;
				shExecInfo.lpDirectory = NULL;
				shExecInfo.nShow = SW_NORMAL;
				shExecInfo.hInstApp = NULL;

				if (!ShellExecuteEx(&shExecInfo))
				{
					dwError = GetLastError();
				}
				else
				{
					if (WaitForSingleObject(shExecInfo.hProcess, INFINITE) == WAIT_OBJECT_0)
					{
						fReturn = TRUE;
					}
					else
					{
						dwError = GetLastError();
					}
				}
			}
		}
		fReturn = TRUE;
		break;
	case CHECK_TRUST:
		switch (_dwTrustError)
		{
		case CERT_TRUST_IS_PARTIAL_CHAIN:
			if (IsElevated())
			{
				PCCERT_CONTEXT pCertContext = _pContainer->GetCertificate();
				fReturn = MakeTrustedCertifcate(pCertContext);
				CertFreeCertificateContext(pCertContext);
			}
			else
			{
				//elevate
				SHELLEXECUTEINFO shExecInfo;
				TCHAR szName[1024];
				TCHAR szParameters[8000] = TEXT("TRUST ");
				GetModuleFileName(GetModuleHandle(NULL),szName, ARRAYSIZE(szName));
				PCCERT_CONTEXT pCertContext = _pContainer->GetCertificate();
				DWORD dwSize = ARRAYSIZE(szParameters) - 6;
				if (CryptBinaryToString(pCertContext->pbCertEncoded,pCertContext->cbCertEncoded, CRYPT_STRING_BASE64, szParameters + 6,&dwSize))
				{
					
					shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

					shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
					shExecInfo.hwnd = NULL;
					shExecInfo.lpVerb = TEXT("runas");
					shExecInfo.lpFile = szName;
					shExecInfo.lpParameters = szParameters;
					shExecInfo.lpDirectory = NULL;
					shExecInfo.nShow = SW_NORMAL;
					shExecInfo.hInstApp = NULL;

					if (!ShellExecuteEx(&shExecInfo))
					{
						dwError = GetLastError();
					}
					else
					{
						if (WaitForSingleObject(shExecInfo.hProcess, INFINITE) == WAIT_OBJECT_0)
						{
							fReturn = TRUE;
						}
						else
						{
							dwError = GetLastError();
						}
					}
				}
				else
				{
					dwError = GetLastError();
				}
				CertFreeCertificateContext(pCertContext);
			}
			break;
		case CERT_TRUST_IS_NOT_VALID_FOR_USAGE:
			if (IsElevated())
			{
				DWORD dwValue = 1;
				RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
					TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\SmartCardCredentialProvider"),
					TEXT("AllowCertificatesWithNoEKU"), REG_DWORD, &dwValue,sizeof(dwValue));
			}
			else
			{
				SHELLEXECUTEINFO shExecInfo;
				TCHAR szName[1024];
				GetModuleFileName(GetModuleHandle(NULL),szName, ARRAYSIZE(szName));
				shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

				shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
				shExecInfo.hwnd = NULL;
				shExecInfo.lpVerb = TEXT("runas");
				shExecInfo.lpFile = szName;
				shExecInfo.lpParameters = TEXT("ENABLENOEKU");
				shExecInfo.lpDirectory = NULL;
				shExecInfo.nShow = SW_NORMAL;
				shExecInfo.hInstApp = NULL;

				if (!ShellExecuteEx(&shExecInfo))
				{
					dwError = GetLastError();
				}
				else
				{
					if (WaitForSingleObject(shExecInfo.hProcess, INFINITE) == WAIT_OBJECT_0)
					{
						fReturn = TRUE;
					}
					else
					{
						dwError = GetLastError();
					}
				}
			}
			break;
		}
	}
	SetLastError(dwError);
	return fReturn;
}