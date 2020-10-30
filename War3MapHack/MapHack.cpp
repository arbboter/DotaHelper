#include "pch.h"
#include "MapHack.h"
#include <TlHelp32.h>

// 静态变量初始化
const string CMapHack::m_strProcName = "war3.exe";
const string CMapHack::m_strGameDllName = "game.dll";

//const string CMapHack::m_strProcName = "launchy.exe";
//const string CMapHack::m_strGameDllName = "calcy.dll";

BOOL CMapHack::Init()
{
    // 变量初始化
    m_hProcHand = NULL;
    m_hGameHand = NULL;
    m_dwPid = 0;
    m_dwGameAddr = NULL;
    m_strProcPath = "";
    m_strGamePath = "";

    // 提权
    BOOL bRet = EnableDebugPrivileges();
    if (!bRet)
    {
        MessageBox(NULL, _T("设置程序权限失败"), _T("权限错误"), MB_ICONERROR | MB_OK);
        return bRet;
    }
    // 初始化目标
    bRet = InitGameHandle();
    if (!bRet)
    {
        MessageBox(NULL, _T("初始化目标进程失败，请确认程序已运行"), _T("找不到程序"), MB_ICONERROR | MB_OK);
        return bRet;
    }
    return bRet;
}

BOOL CMapHack::EnableDebugPrivileges()
{
    // 得到进程的令牌句柄
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
    {
        return FALSE;
    }
    // 查询进程的权限
    TOKEN_PRIVILEGES tkp;
    tkp.PrivilegeCount = 1;
    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
    // 修改进程权限
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(token, FALSE, &tkp, sizeof(tkp), NULL, NULL))
    {
        return FALSE;
    }
    CloseHandle(token);
    return TRUE;
}

BOOL CMapHack::InitGameHandle()
{
    // 获取进程ID
    if (GetProcPidByName(m_strProcName.c_str()) <= 0)
    {
        return FALSE;
    }
    // 打开进程
    auto hProc = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_TERMINATE | PROCESS_VM_OPERATION
        | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, m_dwPid);
    if (hProc == NULL)
    {
        return FALSE;
    }
    m_hProcHand = hProc;
    // 获取DLL基地址
    if (GetDllBaseAddr(m_strGameDllName.c_str(), m_dwPid) <= 0)
    {
        return FALSE;
    }
    return TRUE;
}

DWORD CMapHack::GetProcPidByName(const char* pName)
{
    PROCESSENTRY32 pe;
    // 参考msdn，主要是获得windows当前的任务的一个snap（快照）
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pe.dwSize = sizeof(PROCESSENTRY32);
    // 检索上一步获得的windows的快照的每个进程。First ,next 函数 
    if (!Process32First(hSnapshot, &pe))
    {
        return 0;
    }
    CString strFindName(pName);
    strFindName.MakeUpper();
    do
    {
        pe.dwSize = sizeof(PROCESSENTRY32);
        CString strCurName = pe.szExeFile;
        strCurName.MakeUpper();
        // 其中参数pe里面有进程信息如name，即在任务管理器里面看到的名字，如qq.exe 
        if (strFindName == strCurName)
        {
            // 记下这个ID，也及时我们要得到的进程的ID 
            m_dwPid = pe.th32ProcessID;
            m_strProcPath = pe.szExeFile;
            break;
        }
        if (Process32Next(hSnapshot, &pe) == FALSE)
            break;
    } while (1);
    CloseHandle(hSnapshot);
    return m_dwPid;
}

DWORD CMapHack::GetDllBaseAddr(const char* pName, DWORD dwPid)
{
    HANDLE hSnap = NULL;
    MODULEENTRY32 me32;
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
    me32.dwSize = sizeof(MODULEENTRY32);
    CString strFindName(pName);
    strFindName.MakeUpper();
    CString strCurName;
    if (Module32First(hSnap, &me32))
    {
        do
        {
            strCurName = me32.szModule;
            strCurName.MakeUpper();
            if (strCurName == strFindName)
            {
                m_strGamePath = me32.szExePath;
                m_hGameHand = me32.hModule;
                m_dwGameAddr = (DWORD)me32.modBaseAddr;
                CloseHandle(hSnap);
                return m_dwGameAddr;
            }
        } while (Module32Next(hSnap, &me32));
    }
    CloseHandle(hSnap);
    return 0;
}
BOOL CMapHack::GameMemoryWrite(DWORD dwOffset, const char* lpAddr, DWORD nSize)
{
    auto pAddr = (LPVOID)(m_dwGameAddr + lpAddr);
    return WriteProcessMemory(m_hProcHand, pAddr, lpAddr, 1, &nSize);
}


BOOL CMapHack::Source()
{
    BOOL bRet = TRUE;
    // 大地图显示单位
    bRet &= GameMemoryWrite(0x74D1B9, "\xB2\x00\x90\x90\x90\x90", 6);

    // 显示隐形单位 
    bRet &= GameMemoryWrite(0x39EBBC, "\x75", 1);
    bRet &= GameMemoryWrite(0x3A2030, "\x90\x90", 2);
    bRet &= GameMemoryWrite(0x3A20DB, "\x8B\xC0", 2);
    // 显示物品
    bRet &= GameMemoryWrite(0x28357C, "\x40\xC3", 2);
    // 小地图 去除迷雾
    bRet &= GameMemoryWrite(0x3A201B, "\xEB", 1);
    bRet &= GameMemoryWrite(0x40A864, "\x90\x90", 2);
    // 小地图显示单位
    bRet &= GameMemoryWrite(0x357065, "\x90\x90", 2);
    // 换了种方法绕过检测
    // PATCH(0x361F7C,"\x00",1);
    // 敌方信号
    bRet &= GameMemoryWrite(0x361F7C, "\xC1\x90\x90\x90", 4);
    // 他人提示
    bRet &= GameMemoryWrite(0x43F9A6, "\x3B", 1);
    bRet &= GameMemoryWrite(0x43F9A9, "\x85", 1);
    bRet &= GameMemoryWrite(0x43F9B9, "\x3B", 1);
    bRet &= GameMemoryWrite(0x43F9BC, "\x85", 1);
    // 敌方头像
    bRet &= GameMemoryWrite(0x3345E9, "\x39\xC0\x0F\x85", 4);
    // 盟友头像
    bRet &= GameMemoryWrite(0x371700, "\xE8\x3B\x28\x03\x00\x85\xC0\x0F\x85\x8F\x02\x00\x00\xEB\xC9\x90\x90\x90\x90", 19);
    // 资源面板
    bRet &= GameMemoryWrite(0x371700, "\xE8\x3B\x28\x03\x00\x85\xC0\x0F\x84\x8F\x02\x00\x00\xEB\xC9\x90\x90\x90\x90", 19);
    // 允许交易
    bRet &= GameMemoryWrite(0x36058A, "\x90", 1);
    bRet &= GameMemoryWrite(0x36058B, "\x90", 1);
    // 显示技能
    bRet &= GameMemoryWrite(0x34E8E2, "\xB8\xC8\x00\x00", 4);
    bRet &= GameMemoryWrite(0x34E8E7, "\x90", 1);
    bRet &= GameMemoryWrite(0x34E8EA, "\xB8\x64\x00\x00", 4);
    bRet &= GameMemoryWrite(0x34E8EF, "\x90", 1);
    // 技能CD
    bRet &= GameMemoryWrite(0x2031EC, "\x90\x90\x90\x90\x90\x90", 6);
    bRet &= GameMemoryWrite(0x34FDE8, "\x90\x90", 2);
    // 资源条 野外显血 视野外点击
    bRet &= GameMemoryWrite(0x28ECFE, "\xEB", 1);
    bRet &= GameMemoryWrite(0x34FE26, "\x90\x90\x90\x90", 4);
    // 无限取消
    bRet &= GameMemoryWrite(0x285CBC, "\x90\x90", 2);
    bRet &= GameMemoryWrite(0x285CD2, "\xEB", 1);
    // 过-MH
    bRet &= GameMemoryWrite(0x57BA7C, "\xEB", 1);
    bRet &= GameMemoryWrite(0x5B2D77, "\x03", 1);
    bRet &= GameMemoryWrite(0x5B2D8B, "\x03", 1);
    // 反-AH
    bRet &= GameMemoryWrite(0x3C84C7, "\xEB\x11", 2);
    bRet &= GameMemoryWrite(0x3C84E7, "\xEB\x11", 2);
    // 分辨幻影
    bRet &= GameMemoryWrite(0x3C6EDC, "\xB8\xFF\x00\x00\x00\xEB", 6);
    bRet &= GameMemoryWrite(0x3CC3B2, "\xEB", 1);
    bRet &= GameMemoryWrite(0x362391, "\x3B", 1);
    bRet &= GameMemoryWrite(0x362394, "\x85", 1);
    bRet &= GameMemoryWrite(0x39A51B, "\x90\x90\x90\x90\x90\x90", 6);
    bRet &= GameMemoryWrite(0x39A52E, "\x90\x90\x90\x90\x90\x90\x90\x90\x33\xC0\x40", 11);
    return bRet;
}
