
// War3MapHack.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"		// 主符号


// CWar3MapHackApp:
// 有关此类的实现，请参阅 War3MapHack.cpp
//

class CWar3MapHackApp : public CWinApp
{
public:
	CWar3MapHackApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CWar3MapHackApp theApp;
