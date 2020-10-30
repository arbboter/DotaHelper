#pragma once
#include <string>
using namespace std;
class CMapHack
{
public:
    BOOL Init();
    BOOL Source();

private:
    HANDLE  m_hProcHand;
    HMODULE m_hGameHand;
    DWORD   m_dwPid;
    DWORD   m_dwGameAddr;
    CString m_strProcPath;
    CString m_strGamePath;
private:
    // 提权访问其他进程
    BOOL EnableDebugPrivileges();
    BOOL InitGameHandle();
    DWORD GetProcPidByName(const char* pName);
    DWORD GetDllBaseAddr(const char* pName, DWORD dwPid);
    BOOL  GameMemoryWrite(DWORD dwOffset, const char* lpAddr, DWORD nSize);

private:
    static const string m_strProcName;
    static const string m_strGameDllName;
};

