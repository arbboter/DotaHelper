#include "IniWriter.h"
#include "Includes.h" 

CIniWriter::CIniWriter(char* szFileName)
{
	 memset(m_szFileName, 0x00, 255);
	 memcpy(m_szFileName, szFileName, strlen(szFileName));
}

void CIniWriter::WriteInteger(char* szSection, char* szKey, int iValue)
{
	 char szValue[255];
	 sprintf_s(szValue, "%d", iValue);
	 WritePrivateProfileString(szSection,  szKey, szValue, m_szFileName); 
}

void CIniWriter::WriteFloat(char* szSection, char* szKey, float fltValue)
{
	 char szValue[255];
	 sprintf_s(szValue, "%f", fltValue);
	 WritePrivateProfileString(szSection,  szKey, szValue, m_szFileName); 
}

void CIniWriter::WriteBoolean(char* szSection, char* szKey, bool bolValue)
{
	 char szValue[255];
	 sprintf_s(szValue, "%s", bolValue ? "True" : "False");
	 WritePrivateProfileString(szSection,  szKey, szValue, m_szFileName); 
}

void CIniWriter::WriteString(char* szSection, char* szKey, char* szValue)
{
	WritePrivateProfileString(szSection,  szKey, szValue, m_szFileName);
}