#include "Includes.h"
#include "Main.h"
#include "IniReader.h"
#include "IniWriter.h"
#include "TextOut.h"
#include "GetWCGameState.h"
#include "CloakDll.h"
#include "ManaBar.h"

using namespace std;
#pragma comment(lib, "psapi.lib")

//////////////////////////////////////////////////////////////////////////////////////////////

HHOOK keyboardHook;
HINSTANCE warInst;
HANDLE hWar;

HANDLE hThread;
DWORD hThreadId = 0;

DWORD GameBase;

DWORD dDistanceOffset = 0, currentDistance;

DWORD PlayerColors[16];
DWORD RegTemp10, RegTemp11, RegTemp12, RegTemp13, RegTemp14, RegTemp15, RegTemp16, RegTemp17, 
	RegTemp20, RegTemp21, RegTemp22, RegTemp23, RegTemp24, RegTemp25, RegTemp26, RegTemp27,
	RegTemp;

DWORD pClassOffset, ColorUnit, ColoredInviOriCall, ColoredInviRet,
	RuneNotifierBack, MessageFuncBack, MessageFuncOriJmp, RoshanNotifierOriCall, RoshanNotifierBack;

float runeXTop = -2400, runeYTop = 1600, runeXBot = 2970, runeYBot = -2850, runeDuration = 5,
	roshanX = 2450, roshanY = -730, roshanDuration = 5;

int hkSwitchMaphack, hkZoomIn, hkZoomOut, zoomWithMouse;

bool Injected = false, MaphackEnabled = false, IsWarcraftIngame = false;

char* ChatText;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef void	(__cdecl *pPingMinimap)					(float* x, float* y, float* duration);
typedef void 	(__cdecl *pPingMinimapEx)				(float* x, float* y, float* duration, int red, int green, int blue, bool extraEffects);

pPingMinimap	PingMinimap;
pPingMinimapEx	PingMinimapEx;

void InitJassNatives();

//////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
	
	if (nCode == HC_ACTION)
	{
		if (wParam == hkSwitchMaphack && (BYTE)((lParam & 0xFF000000) >> 30) == 0
				&& (BYTE)((lParam & 0xFF000000) >> 31) == 0)
		{
			if (IsIngame(GameBase))
			{
				//If any features were enabled before, disable them now and vice versa.
				if (MaphackEnabled)
				{
					featurelist features;
					features.revealUnitsMainmap = false;
					features.revealUnitsMinimap = false;
					features.removeFogMainmap = false;
					features.removeFogMinimap = false;
					features.showSkillsCooldowns = false;
					features.makeUnitsClickable = false;
					features.showEnemyPings = false;
					features.greyHP = false;
					features.colorInvisible = false;
					features.enableTrading = false;
					features.showResources = false;
					features.protectFromAH = false;
					features.roshanNotifier = false;
					features.runeNotifier = false;
					features.distance = 0;
					features.angle = 0;
					features.rotation = 0;
					PatchFeatures(features);
					delete &features;

					WCTextOut("|cFF6699CCDotaHelper|r: All features |cFF0000AAOFF|r!", CHAT_RECIPIENT_PRIVATE, 1, GameBase);

					MaphackEnabled = false;
				}
				else
				{
					featurelist features;
					features.revealUnitsMainmap = true;
					features.revealUnitsMinimap = true;
					features.removeFogMainmap = 2;
					features.removeFogMinimap = 2;
					features.showSkillsCooldowns = true;
					features.makeUnitsClickable = true;
					features.showEnemyPings = true;
					features.greyHP = true;
					features.colorInvisible = true;
					features.enableTrading = true;
					features.showResources = true;
					features.protectFromAH = true;
					features.roshanNotifier = true;
					features.runeNotifier = true;
					features.distance = 0;
					features.angle = 0;
					features.rotation = 0;
					PatchFeatures(features);
					delete &features;

					WCTextOut("|cFF6699CCDotaHelper|r: All features |cFFAA0000ON|r!", CHAT_RECIPIENT_PRIVATE, 1, GameBase);

					MaphackEnabled = true;
				}
			}
		}
		else if (wParam == hkZoomIn && (BYTE)((lParam & 0xFF000000) >> 30) == 0
				&& (BYTE)((lParam & 0xFF000000) >> 31) == 0)
		{
			if (IsIngame(GameBase))
			{
				char text[64];
				
				float distance = *(float*)(GameBase + CAMERA_DISTANCE) - 150.0f;
				SetCameraDistance(distance);

				sprintf_s(text, "|cFF6699CCDotaHelper|r: Camera distance changed to %d.", (int)distance);
				WCTextOut(text, CHAT_RECIPIENT_PRIVATE, 1, GameBase);

				return 1;
			}
		}
		else if (wParam == hkZoomOut && (BYTE)((lParam & 0xFF000000) >> 30) == 0
				&& (BYTE)((lParam & 0xFF000000) >> 31) == 0)
		{
			if (IsIngame(GameBase))
			{
				char text[64];
				
				float distance = *(float*)(GameBase + CAMERA_DISTANCE) + 150.0f;
				SetCameraDistance(distance);

				sprintf_s(text, "|cFF6699CCDotaHelper|r: Camera distance changed to %d.", (int)distance);
				WCTextOut(text, CHAT_RECIPIENT_PRIVATE, 1, GameBase);
				
				return 1;
			}
		}
	}
	
	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

static DWORD WINAPI ThreadProc(LPVOID pvParam)
{
	for(;;)
	{
		if (IsIngame(GameBase) && !IsWarcraftIngame)
		{
			WCTextOut("|cFF6699CCDotaHelper|r by YourName loaded!", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
			IsWarcraftIngame = true;
		}
		else if (!IsIngame(GameBase) && IsWarcraftIngame)
			IsWarcraftIngame = false;

		if (!Injected && GetWCGameState(GameBase) == BNET_LOGON_SCREEN)
		{
			Injected = true;
			HideDLL(GetModuleHandle("Game.dll"));
			LoadLibraryEx("Game.dll", NULL, 0);
		}

		Sleep(10);
	}

	return 0;
}

DLLIMPORT void MainProc()
{
	// 配置文件
	fstream stream(INI_FILE_NAME, ios::in);
	
	// 如果配置文件不存在，创建一个默认的配置文件
	if (!stream.is_open())
	{
		ofstream newStream(INI_FILE_NAME, ios::out);
		newStream.close();

		CIniWriter writer(INI_FILE_NAME);

		//Create a .ini file containing all the data.
		writer.WriteInteger("Maphack", "RevealUnitsMainmap", 1);
		writer.WriteInteger("Maphack", "RevealUnitsMinimap", 1);
		writer.WriteInteger("Maphack", "RemoveFogMainmap", 2);
		writer.WriteInteger("Maphack", "RemoveFogMinimap", 2);
		writer.WriteInteger("Maphack", "ShowSkillsCooldowns", 1);
		writer.WriteInteger("Maphack", "MakeUnitsClickable", 1);
		writer.WriteInteger("Maphack", "ShowEnemyPings", 1);
		writer.WriteInteger("Maphack", "GreyHP", 1);
		writer.WriteInteger("Maphack", "ColorInvisible", 1);
		writer.WriteInteger("Maphack", "ShowResources", 1);
		writer.WriteInteger("Maphack", "RoshanNotifier", 1);
		writer.WriteInteger("Maphack", "RuneNotifier", 1);
		writer.WriteInteger("Maphack", "EnableTrading", 1);
		writer.WriteInteger("Maphack", "Manabar", 1);

		writer.WriteInteger("Camera", "Distance", 1650);
		writer.WriteInteger("Camera", "AngleOfAttack", 304);
		writer.WriteInteger("Camera", "Rotation", 90);

		writer.WriteInteger("Hotkeys", "SwitchMaphack", VK_F5);
		writer.WriteInteger("Hotkeys", "ZoomIn", VK_ADD);
		writer.WriteInteger("Hotkeys", "ZoomOut", VK_SUBTRACT);

		writer.WriteInteger("Misc", "ShowGUI", 1);
	}
	else
		stream.close();

    // 读取配置文件
	CIniReader reader(INI_FILE_NAME);

	// Create hooks and threads, and get Warcraft's handle
	hThread = CreateThread(NULL, NULL, ThreadProc, NULL, NULL, NULL);
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, warInst, GetCurrentThreadId());
	hWar = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());

	//Read out the hotkeys
	hkSwitchMaphack = reader.ReadInteger("Hotkeys", "SwitchMaphack", VK_F5);
	hkZoomIn = reader.ReadInteger("Hotkeys", "ZoomIn", VK_ADD);
	hkZoomOut = reader.ReadInteger("Hotkeys", "ZoomOut", VK_SUBTRACT);
	zoomWithMouse = reader.ReadInteger("Camera", "ScrollWithMouse", 0);

	//Initializing pointer variables
	pClassOffset = GameBase + LOCAL_PLAYER_OFFSET;
	ColorUnit = GameBase + 0x4D2FF0;
	ColoredInviOriCall = GameBase + 0x2AB4E0;
	ColoredInviRet = GameBase + 0x39976A;
	RuneNotifierBack = GameBase + 0x3BB77C;
	MessageFuncBack = GameBase + 0x2FC526;
	MessageFuncOriJmp = GameBase + 0x2FC7A4;
	RoshanNotifierOriCall = GameBase + 0x3BD180;
	RoshanNotifierBack = GameBase + 0x3C50AC;

	//Initializing player colors
	PlayerColors[0] = 0xFFFF0202;
	PlayerColors[1] = 0xFF0041FF;
	PlayerColors[2] = 0xFF1BE6D8;
	PlayerColors[3] = 0xFF530080;
	PlayerColors[4] = 0xFFFFFC00;
	PlayerColors[5] = 0xFFFE890D;
	PlayerColors[6] = 0xFF1FBF00;
	PlayerColors[7] = 0xFFE55AAF;
	PlayerColors[8] = 0xFF949596;
	PlayerColors[9] = 0xFF7DBEF1;
	PlayerColors[10] = 0xFF0F6145;
	PlayerColors[11] = 0xFF4D2903;
	PlayerColors[12] = 0xFF272727;
	PlayerColors[13] = 0xFF272727;
	PlayerColors[14] = 0xFF272727;
	PlayerColors[15] = 0xFF272727;

	//Hooking the ingame chat func
	PlantDetourJMP((BYTE*)(GameBase + 0x2FC51C), (BYTE*) HookChatFunc, 10);

	//Initializing the JASS natives.
	InitJassNatives();
	
	//Patching maphack offsets
	if (reader.ReadInteger("Maphack", "Manabar", 1) >= 1)
		InjectManabar(GameBase);

	featurelist features;
	features.revealUnitsMainmap = reader.ReadInteger("Maphack", "RevealUnitsMainmap", 1);
	features.revealUnitsMinimap = reader.ReadInteger("Maphack", "RevealUnitsMinimap", 1);
	features.removeFogMainmap = reader.ReadInteger("Maphack", "RemoveFogMainmap", 2);
	features.removeFogMinimap = reader.ReadInteger("Maphack", "RemoveFogMinimap", 2);
	features.showSkillsCooldowns = reader.ReadInteger("Maphack", "ShowSkillsCooldowns", 1);
	features.makeUnitsClickable =reader.ReadInteger("Maphack", "MakeUnitsClickable", 1);
	features.showEnemyPings = reader.ReadInteger("Maphack", "ShowEnemyPings", 1);
	features.greyHP = reader.ReadInteger("Maphack", "GreyHP", 1);
	features.colorInvisible = reader.ReadInteger("Maphack", "ColorInvisible", 1);
	features.enableTrading = reader.ReadInteger("Maphack", "EnableTrading", 1);
	features.showResources = reader.ReadInteger("Maphack", "ShowResources", 1);
	features.protectFromAH = reader.ReadInteger("Maphack", "ProtectFromAH", 1);
	features.roshanNotifier = reader.ReadInteger("Maphack", "RoshanNotifier", 1);
	features.runeNotifier = reader.ReadInteger("Maphack", "RuneNotifier", 1);

	features.distance = reader.ReadInteger("Camera", "Distance", 1650);
	features.angle = reader.ReadInteger("Camera", "AngleOfAttack", 304);
	features.rotation = reader.ReadInteger("Camera", "Rotation", 90);

	PatchFeatures(features);

	currentDistance = features.distance;
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
            // 禁用指定的DLL的DLL_THREAD_ATTACH和DLL_THREAD_DETACH通知，减小程序的工作集大小
			DisableThreadLibraryCalls(hinstDLL);
            // 提升本程序的操作权限
			EnableDebugPriv();

            // 获取HOOK目标DLL地址
			GameBase = (DWORD) GetModuleHandle("Game.dll");
			warInst = hinstDLL;

			MainProc();
			break;
		}
	}
	
	return TRUE;
}

void EnableDebugPriv()
{
    HANDLE hToken;
    LUID sedebugnameValue;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return;

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME,&sedebugnameValue))
	{
        CloseHandle(hToken);
        return;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = sedebugnameValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof tkp, NULL, NULL))
        CloseHandle(hToken);
} 

void __declspec(naked) HookChatFunc()
{
	__asm
	{
		MOV ESI, EAX;
		TEST ESI, ESI;
		JE orije;
		PUSHAD;
		MOV ChatText, EAX;
	}
	
	if (_stricmp(ChatText, "/mh on") == 0)
	{
		featurelist features;
		features.revealUnitsMainmap = true;
		features.revealUnitsMinimap = true;
		features.removeFogMainmap = 2;
		features.removeFogMinimap = 2;
		features.showSkillsCooldowns = true;
		features.makeUnitsClickable = true;
		features.showEnemyPings = true;
		features.greyHP = true;
		features.colorInvisible = true;
		features.enableTrading = true;
		features.showResources = true;
		features.protectFromAH = true;
		features.roshanNotifier = true;
		features.runeNotifier = true;
		features.distance = 0;
		features.angle = 0;
		features.rotation = 0;
		PatchFeatures(features);
		
		goto popback;
	}
	else if (_stricmp(ChatText, "/mh off") == 0)
	{
		featurelist features;
		features.revealUnitsMainmap = false;
		features.revealUnitsMinimap = false;
		features.removeFogMainmap = false;
		features.removeFogMinimap = false;
		features.showSkillsCooldowns = false;
		features.makeUnitsClickable = false;
		features.showEnemyPings = false;
		features.greyHP = false;
		features.colorInvisible = false;
		features.enableTrading = false;
		features.showResources = false;
		features.protectFromAH = false;
		features.roshanNotifier = false;
		features.runeNotifier = false;
		features.distance = 0;
		features.angle = 0;
		features.rotation = 0;
		PatchFeatures(features);
		
		goto popback;
	}
	else if (_stricmp(ChatText, "/terminate") == 0)
		TerminateProcess(hWar, 0);
	else if (StartsWith(ChatText, "/distance "))
	{
		char* szDistance = ChatText + strlen("/distance ");
		if (IsNumber(szDistance))
		{
			float distance = atof(szDistance);
			if (distance == 0)
				distance = 1;
			SetCameraDistance(distance);
		}
		goto popback;
	}

	__asm
	{
		POPAD;
		JMP MessageFuncBack;
	}

orije:
	__asm JMP MessageFuncOriJmp;

popback:
	__asm
	{
		POPAD;
		JMP MessageFuncOriJmp;
	}
}

void __declspec(naked) ColoredInviFunc()
{
	__asm
	{
		PUSHAD;

		MOV EDX, pClassOffset;
		MOV EDX, DWORD PTR DS:[EDX];
		XOR EAX, EAX;
		MOV AL, BYTE PTR DS:[EDX+0x28];
		MOV EDX, DWORD PTR DS:[ESI];

		PUSH 0x4;
		PUSH 0x0;
		PUSH EAX;
		MOV EAX, DWORD PTR DS:[EDX+0xFC];
		MOV ECX, ESI;
		CALL EAX;
		CMP EAX, 0x1;
		JE back;

		XOR EAX, EAX;
		MOV AL, BYTE PTR DS:[ESI+0x5F];
		CMP AL, 0x1;
		JNZ back;

		MOV ECX, ESI;
		PUSH ECX;
		MOV EAX, ESP;
		MOV DWORD PTR DS:[EAX], 0xFFCC33CC;
		MOV ECX, DWORD PTR DS:[ECX+0x28];
		XOR EDX, EDX;
		CALL ColorUnit;

	back:
		POPAD;
		CALL ColoredInviOriCall;
		JMP ColoredInviRet;
	}
}

void __declspec(naked) GreyHPFunc()
{
	__asm
	{
		PUSHAD;

		MOV EDX, pClassOffset;
		MOV EDX,DWORD PTR DS:[EDX];
		XOR EAX, EAX;
		MOV AL, BYTE PTR DS:[EDX+0x28];
		MOV EDX, DWORD PTR DS:[ESI];

		PUSH 0x4;
		PUSH 0x0;
		PUSH EAX;
		MOV EAX, DWORD PTR DS:[EDX+0xFC];
		MOV ECX, ESI;
		CALL EAX;
		CMP EAX, 0x1;
		JE visible;

		POPAD;

		MOV EAX, DWORD PTR SS:[ESP+0x4];
		OR DWORD PTR DS:[EAX], 0xDD00FF;
		MOV DL, BYTE PTR DS:[EAX+0x3];
		MOV BYTE PTR DS:[ECX+0x68], DL;
		MOV DL, BYTE PTR DS:[EAX];
		MOV BYTE PTR DS:[ECX+0x69], DL;
		MOV DL, BYTE PTR DS:[EAX+0x1];
		MOV BYTE PTR DS:[ECX+0x6A], DL;
		MOV DL, BYTE PTR DS:[EAX+0x2];
		MOV BYTE PTR DS:[ECX+0x6B], DL;

		MOV EDX, DWORD PTR DS:[ECX];
		MOV EAX, DWORD PTR DS:[EDX+0x24];
		CALL EAX;
		RET 0x4;
		
	visible:
		POPAD;

		MOV EAX, DWORD PTR SS:[ESP+0x4];
		MOV DL, BYTE PTR DS:[EAX+0x3];
		MOV BYTE PTR DS:[ECX+0x68], DL;
		MOV DL, BYTE PTR DS:[EAX];
		MOV BYTE PTR DS:[ECX+0x69], DL;
		MOV DL, BYTE PTR DS:[EAX+0x1];
		MOV BYTE PTR DS:[ECX+0x6A], DL;
		MOV DL, BYTE PTR DS:[EAX+0x2];
		MOV BYTE PTR DS:[ECX+0x6B], DL;

		MOV EDX, DWORD PTR DS:[ECX];
		MOV EAX, DWORD PTR DS:[EDX+0x24];
		CALL EAX;
		RET 0x4;
	}
}

void __declspec(naked) RoshanNotifierFunc()
{
	__asm
	{
		CALL RoshanNotifierOriCall;
		
		MOV RegTemp10, EAX;
		MOV RegTemp11, EBX;
		MOV RegTemp12, ECX;
		MOV RegTemp13, EDX;
		MOV RegTemp14, ESI;
		MOV RegTemp15, EDI;
		MOV RegTemp16, EBP;
		MOV RegTemp17, ESP;

		MOV EAX, DWORD PTR DS:[ESP+0x10];
		CMP EAX, 0x6E30304C;
		JNE back;
	}

	if (!IsIngame(GameBase))
		goto back;

	WCTextOut("|cFF6699CCDotaHelper|r: Roshan respawned!", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
	PingMinimapEx(&roshanX, &roshanY, &roshanDuration, 0xFF, 0xFF, 0xFF, false);
	
	back:
	__asm
	{
		MOV EAX, RegTemp10;
		MOV EBX, RegTemp11;
		MOV ECX, RegTemp12;
		MOV EDX, RegTemp13;
		MOV ESI, RegTemp14;
		MOV EDI, RegTemp15;
		MOV EBP, RegTemp16;
		MOV ESP, RegTemp17;
		JMP RoshanNotifierBack;
	}
}

void __declspec(naked) RuneNotifierFunc()
{
	__asm
	{
		MOV EAX, DWORD PTR SS:[ESP+0xC];
		MOV EDX, DWORD PTR SS:[ESP+0x8];
		MOV ECX, DWORD PTR SS:[ESP+0x4];

		MOV RegTemp20, EAX;
		MOV RegTemp21, EBX;
		MOV RegTemp22, ECX;
		MOV RegTemp23, EDX;
		MOV RegTemp24, ESI;
		MOV RegTemp25, EDI;
		MOV RegTemp26, EBP;
		MOV RegTemp27, ESP;

		CMP DWORD PTR DS:[EAX], 0x44CE0000;
			JE top;
		CMP DWORD PTR DS:[EAX], 0xC5310000;
			JE bot;

		JMP RuneNotifierBack;

	top:
		CMP ECX, 0x49303037
			JE illuTop;
        CMP ECX, 0x49303036
			JE hasteTop;
		CMP ECX, 0x4930304B
			JE ddamageTop;
		CMP ECX, 0x49303038
			JE regenTop;
		CMP ECX, 0x4930304A
			JE inviTop;
		JMP RuneNotifierBack;

	bot:
		CMP ECX,0x49303037
			JE illuBot;
        CMP ECX,0x49303036
			JE hasteBot;
        CMP ECX,0x4930304B
			JE ddamageBot;
        CMP ECX,0x49303038
			JE regenBot;
        CMP ECX,0x4930304A
			JE inviBot;
        JMP RuneNotifierBack;
	}

	illuTop:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFFFFFC01Illusion|r rune spawned at the |cFFCC99FFtop|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXTop, &runeYTop, &runeDuration, 0xFF, 0xFC, 0x01, false);
		goto fin;
	illuBot:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFFFFFC01Illusion|r rune spawned at the |cFFFFFFCCbot|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXBot, &runeYBot, &runeDuration, 0xFF, 0xFC, 0x01, false);
		goto fin;

	hasteTop:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFFFF0303Haste|r rune spawned at the |cFFCC99FFtop|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXTop, &runeYTop, &runeDuration, 0xFF, 0x03, 0x03, false);
		goto fin;
	hasteBot:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFFFF0303Haste|r rune spawned at the |cFFFFFFCCbot|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXBot, &runeYBot, &runeDuration, 0xFF, 0x03, 0x03, false);
		goto fin;

	ddamageTop:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFF0042FFDouble Damage|r rune spawned at the |cFFCC99FFtop|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXTop, &runeYTop, &runeDuration, 0x00, 0x42, 0xFF, false);
		goto fin;
	ddamageBot:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFF0042FFDouble Damage|r rune spawned at the |cFFFFFFCCbot|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXBot, &runeYBot, &runeDuration, 0x00, 0x42, 0xFF, false);
		goto fin;

	regenTop:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFF00FF00Regeneration|r rune spawned at the |cFFCC99FFtop|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXTop, &runeYTop, &runeDuration, 0x00, 0xFF, 0x00, false);
		goto fin;
	regenBot:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFF00FF00Regeneration|r rune spawned at the |cFFFFFFCCbot|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXBot, &runeYBot, &runeDuration, 0x00, 0xFF, 0x00, false);
		goto fin;

	inviTop:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFF99CCFFInvisibility|r rune spawned at the |cFFCC99FFtop|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXTop, &runeYTop, &runeDuration, 0x99, 0xCC, 0xFF, false);
		goto fin;
	inviBot:
		WCTextOut("|cFF6699CCDotaHelper|r: |cFF99CCFFInvisibility|r rune spawned at the |cFFFFFFCCbot|r spot.", CHAT_RECIPIENT_PRIVATE, 5, GameBase);
		PingMinimapEx(&runeXBot, &runeYBot, &runeDuration, 0x99, 0xCC, 0xFF, false);
		goto fin;

	fin:
	__asm
	{
		MOV EAX, RegTemp20;
		MOV EBX, RegTemp21;
		MOV ECX, RegTemp22;
		MOV EDX, RegTemp23;
		MOV ESI, RegTemp24;
		MOV EDI, RegTemp25;
		MOV EBP, RegTemp26;
		MOV ESP, RegTemp27;
		
		JMP RuneNotifierBack;
	}
}

void PlantDetourJMP(BYTE* source, const BYTE* destination, const int length)
{
	BYTE* jump = (BYTE*) malloc(length + 5);

	DWORD oldProtection;
	VirtualProtect(source, length, PAGE_EXECUTE_READWRITE, &oldProtection);
	memcpy(jump, source, length);

	jump[length] = 0xE9;
	*(DWORD*)(jump + length) = (DWORD)((source + length) - (jump + length)) - 5;

	source[0] = 0xE9;
	*(DWORD*)(source + 1) = (DWORD)(destination - source) - 5;

	for(int i = 5; i < length; i++)
		source[i] = 0x90;

	VirtualProtect(source, length, oldProtection, &oldProtection);
}

void PlantDetourCALL(BYTE* source, const BYTE* destination, const int length)
{
	BYTE* jump = (BYTE*) malloc(length + 5);

	DWORD oldProtection;
	VirtualProtect(source, length, PAGE_EXECUTE_READWRITE, &oldProtection);
	memcpy(jump + 3, source, length);

	jump[0] = 0x58;
	jump[1] = 0x59;
	jump[2] = 0x50;
	jump[length+3] = 0xE9;
	*(DWORD*)(jump+length+4) = (DWORD)((source+length) - (jump+length+3)) - 5;

	source[0] = 0xE8;
	*(DWORD*)(source+1) = (DWORD)(destination - (source)) - 5;

	for( int i=5; i < length; i++ )
		source[i] = 0x90;

	VirtualProtect(source, length, oldProtection, &oldProtection);
}


void PatchFeatures(featurelist features)
{
	DWORD oldProtect;
	VirtualProtect((void*)(GameBase + MAPHACK_AREA_OFFSET), MAPHACK_AREA_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
	
	//Units mainmap:主地图
	if (features.revealUnitsMainmap)
	{
		//Units：单位
		*(WORD*)(GameBase + 0x3A136B) = 0x9090;
		*(WORD*)(GameBase + 0x3A128C) = 0x9090;
		//Illusions：幻像
		*(WORD*)(GameBase + 0x28282C) = 0xC340;
		//Invis：隐身
		*(WORD*)(GameBase + 0x3997AB) = 0x9090;
		*(DWORD*)(GameBase + 0x3997AD) = 0x90909090;
		*(DWORD*)(GameBase + 0x3997BE) = 0x90909090;
		*(DWORD*)(GameBase + 0x3997C2) = 0x90909090;
		*(WORD*)(GameBase + 0x3997C6) = 0xC033;
		*(BYTE*)(GameBase + 0x3997C8) = 0x40;
		//Items/Runes etc：标记、蓝量等
		*(BYTE*)(GameBase + 0x3A12AB) = 0xEB;
	}
	else
	{
		//Units
		*(WORD*)(GameBase + 0x3A136B) = 0xCA23;
		*(WORD*)(GameBase + 0x3A128C) = 0xCA23;
		//Illusions
		*(WORD*)(GameBase + 0x28282C) = 0xCCC3;
		//Invis
		*(WORD*)(GameBase + 0x3997AB) = 0x978B;
		*(DWORD*)(GameBase + 0x3997AD) = 0x00000198;
		*(DWORD*)(GameBase + 0x3997BE) = 0x5500B70F;
		*(DWORD*)(GameBase + 0x3997C2) = 0xF7E85650;
		*(WORD*)(GameBase + 0x3997C6) = 0x007B;
		*(BYTE*)(GameBase + 0x3997C8) = 0x00;
		//Items/Runes etc
		*(BYTE*)(GameBase + 0x3A12AB) = 0x75;
	}
	
	//Units minimap：小地图的英雄单位
	if (features.revealUnitsMinimap)
		*(WORD*)(GameBase + 0x36120C) = 0x00;
	else
		*(WORD*)(GameBase + 0x36120C) = 0x01;

	//Ud blight：开了地图战争迷雾
	if (features.removeFogMainmap > 0 && features.removeFogMinimap > 0)
		*(WORD*)(GameBase + 0x4069D2) = 0x4190;
	else
		*(WORD*)(GameBase + 0x4069D2) = 0xC823;
	
	//Fog mainmap：大地图战争迷雾
	if (features.removeFogMainmap == 2)
	{
		//Disable full view
		*(WORD*)(GameBase + 0x74C7E9) = 0x908A;
		*(DWORD*)(GameBase + 0x74C7EB) = GameBase + 0xAA1E4C;
		//Enable player view
		*(DWORD*)(GameBase + 0x74C7C1) = 0x000FFF68;
		*(BYTE*)(GameBase + 0x74C7C5) = 0x00;
	}
	else if (features.removeFogMainmap == 1)
	{
		//Disable player view
		*(DWORD*)(GameBase + 0x74C7C1) = 0x2824548B;
		*(BYTE*)(GameBase + 0x74C7C5) = 0x52;
		//Enable full view
		*(WORD*)(GameBase + 0x74C7E9) = 0x00B2;
		*(DWORD*)(GameBase + 0x74C7EB) = 0x90909090;
	}
	else
	{
		//Disable full view
		*(WORD*)(GameBase + 0x74C7E9) = 0x908A;
		*(DWORD*)(GameBase + 0x74C7EB) = GameBase + 0xAA1E4C;
		//Disable player view
		*(DWORD*)(GameBase + 0x74C7C1) = 0x2824548B;
		*(BYTE*)(GameBase + 0x74C7C5) = 0x52;
	}
	
	//Fog minimap：小地图战争迷雾
	if (features.removeFogMinimap == 2)
	{
		*(WORD*)(GameBase + 0x356289) = 0xC021;
		*(WORD*)(GameBase + 0x3562F5) = 0x0188;
	}
	else if (features.removeFogMinimap == 1)
	{
		*(WORD*)(GameBase + 0x3562F5) = 0x9090;
		*(WORD*)(GameBase + 0x356289) = 0xC223;
	}
	else
	{
		*(WORD*)(GameBase + 0x3562F5) = 0x0188;
		*(WORD*)(GameBase + 0x356289) = 0xC223;
	}
	
	//Show skills and cooldowns：显示技能和冷却
	if (features.showSkillsCooldowns)
	{
		*(WORD*)(GameBase + 0x2024AC) = 0x9090;
		*(DWORD*)(GameBase + 0x2024AE) = 0x90909090;
		*(BYTE*)(GameBase + 0x28DFAE) = 0xEB;
		*(WORD*)(GameBase + 0x34F078) = 0x9090;
		*(BYTE*)(GameBase + 0x34F0B9) = 0x00;
	}
	else
	{
		*(WORD*)(GameBase + 0x2024AC) = 0x840F;
		*(DWORD*)(GameBase + 0x2024AE) = 0x0000015F;
		*(BYTE*)(GameBase + 0x28DFAE) = 0x75;
		*(WORD*)(GameBase + 0x34F078) = 0x0874;
		*(BYTE*)(GameBase + 0x34F0B9) = 0x08;
	}
	
	//Make units clickable：启用单位可点
	if (features.makeUnitsClickable)
	{
		*(WORD*)(GameBase + 0x284F6C) = 0x9090;
		*(BYTE*)(GameBase + 0x284F82) = 0xEB;
	}
	else
	{
		*(WORD*)(GameBase + 0x284F6C) = 0x2A74;
		*(BYTE*)(GameBase + 0x284F82) = 0x75;
	}
	
	//Show enemy pings：显示敌方的pings
	if (features.showEnemyPings)
	{
		*(BYTE*)(GameBase + 0x43EC66) = 0x3B;
		*(BYTE*)(GameBase + 0x43EC69) = 0x85;
		*(BYTE*)(GameBase + 0x43EC79) = 0x3B;
		*(BYTE*)(GameBase + 0x43EC7C) = 0x85;
	}
	else
	{
		*(BYTE*)(GameBase + 0x43EC66) = 0x85;
		*(BYTE*)(GameBase + 0x43EC69) = 0x84;
		*(BYTE*)(GameBase + 0x43EC79) = 0x85;
		*(BYTE*)(GameBase + 0x43EC7C) = 0x84;
	}
	
	//Show resources
	if (features.showResources)
		*(WORD*)(GameBase + 0x35F81A) = 0x9090;
	else
		*(WORD*)(GameBase + 0x35F81A) = 0x08EB;
	
	//Enable trading
	if (features.enableTrading)
	{
		*(DWORD*)(GameBase + 0x34DB72) = 0x0000C8B8;
		*(WORD*)(GameBase + 0x34DB76) = 0x9000;
		*(DWORD*)(GameBase + 0x34DB7A) = 0x000064B8;
		*(WORD*)(GameBase + 0x34DB7E) = 0x9000;
	}
	else
	{
		*(DWORD*)(GameBase + 0x34DB72) = 0x016C878B;
		*(WORD*)(GameBase + 0x34DB76) = 0x0000;
		*(DWORD*)(GameBase + 0x34DB7A) = 0x0168878B;
		*(WORD*)(GameBase + 0x34DB7E) = 0x0000;
	}
	
	//Grey HP under fog
	if (features.greyHP)
		PlantDetourCALL((BYTE*)(GameBase + 0x3649C2), (BYTE*) GreyHPFunc, 5);
	else
	{
		*(BYTE*)(GameBase + 0x3649C2) = 0xE8;
		*(DWORD*)(GameBase + 0x3649C3) = 0x002A9B49;
	}

	//Color invisible
	if (features.colorInvisible)
		PlantDetourJMP((BYTE*)(GameBase + 0x399765), (BYTE*) ColoredInviFunc, 5);
	else
	{
		*(BYTE*)(GameBase + 0x399765) = 0xE8;
		*(DWORD*)(GameBase + 0x399766) = 0xFFF11D76;
	}
	
	//Roshan notifier：刷肉山提示
	if (features.roshanNotifier)
		PlantDetourJMP((BYTE*)(GameBase + 0x3C50A7), (BYTE*) RoshanNotifierFunc, 5);
	else
	{
		*(BYTE*)(GameBase + 0x3C50A7) = 0xE8;
		*(DWORD*)(GameBase + 0x3C50A8) = 0xFFFF80D4;
	}
	
	//Rune notfiier：标记提示
	if (features.runeNotifier)
		PlantDetourJMP((BYTE*)(GameBase + 0x3BB770), (BYTE*) RuneNotifierFunc, 10);
	else
	{
		*(DWORD*)(GameBase + 0x3BB770) = 0x0C24448B;
		*(DWORD*)(GameBase + 0x3BB774) = 0x0824548B;
		*(WORD*)(GameBase + 0x3BB778) = 0x4C8B;
	}
	
	VirtualProtect((void*)(GameBase + MAPHACK_AREA_OFFSET), MAPHACK_AREA_SIZE, oldProtect, &oldProtect);
	
	//Patching camera offsets
	VirtualProtect((void*)(GameBase + CAMERA_AREA_OFFSET), CAMERA_AREA_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);

	if (features.distance > 0)
		*(float*)(GameBase + CAMERA_DISTANCE) = (float) features.distance;

	if (features.angle > 0)
		*(float*)(GameBase + CAMERA_AOA) = (float) features.angle;

	if (features.rotation > 0)
		*(float*)(GameBase + CAMERA_ROTATION) = (float) features.rotation;

	VirtualProtect((void*)(GameBase + CAMERA_AREA_OFFSET), CAMERA_AREA_SIZE, oldProtect, &oldProtect);
}

void InitJassNatives()
{
	PingMinimap = (pPingMinimap)(GameBase + 0x3B4420);
	PingMinimapEx = (pPingMinimapEx)(GameBase + 0x3B8430);
}

bool StartsWith(char* source, char* compare)
{
	if (strlen(source) < strlen(compare))
		return false;

	int CharCount = strlen(compare);
	for (int i = 0; i < CharCount; i++)
	   if (source[i] != compare[i])
			return false;

	return true;
}

bool IsNumber(char* source)
{
	int length = strlen(source);
	for(int i = 0; i < length; i++)
		if (source[i] < '0' || source[i] > '9')
			return false;
	return true;
}

void SetCameraDistance(float distance)
{
	DWORD oldProtect;
	HWND wndWar = FindWindow("Warcraft III", NULL);
	VirtualProtect((void*)(GameBase + CAMERA_AREA_OFFSET), CAMERA_AREA_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);

	if (distance > 0)
		*(float*)(GameBase + CAMERA_DISTANCE) = distance;

	PostMessage(wndWar, WM_SYSKEYDOWN, VK_PRIOR, 0);
    PostMessage(wndWar, WM_SYSKEYDOWN, VK_NEXT, 0);

	VirtualProtect((void*)(GameBase + CAMERA_AREA_OFFSET), CAMERA_AREA_SIZE, oldProtect, &oldProtect);
}