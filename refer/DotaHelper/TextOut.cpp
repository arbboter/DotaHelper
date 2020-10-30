#include "Includes.h"
#include "TextOut.h"

DWORD GetLocalPlayer(DWORD gameBase)
{
    WORD rt;
    __asm
	{
		MOV EAX, gameBase;
		MOV EAX, DWORD PTR DS:[EAX+LOCAL_PLAYER_OFFSET];
		TEST EAX, EAX;
		JE err;
		MOV EAX, DWORD PTR DS:[EAX+0x28];
		MOV rt, AX;
    }

	return (DWORD)rt;
 
	err:
		return 0xF;
}

void WCTextOut(char* pszMessage, DWORD MsgType, float StayUpTime, DWORD gameBase)
{
	DWORD FXNTextToScreen = gameBase + TEXT_TO_CHAT_FUNCTION;
	DWORD plNum = GetLocalPlayer(gameBase);

	__asm
	{
		PUSH StayUpTime;
		PUSH MsgType;
		PUSH pszMessage;
		PUSH plNum;
		MOV EAX, gameBase;
		MOV EAX, DWORD PTR DS:[EAX+CHAT_UI_OFFSET];
		MOV ECX, EAX;
		CALL FXNTextToScreen;
	}
}