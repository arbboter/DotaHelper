#ifndef _TEXTOUT_H_
#define _TEXTOUT_H_

#define TEXT_TO_CHAT_FUNCTION	0x2FB250
#define CHAT_UI_OFFSET			0xAB4F80
#define LOCAL_PLAYER_OFFSET		0xAB65F4

enum EChatRecipients
{
	CHAT_RECIPIENT_ALL			= 0,
	CHAT_RECIPIENT_ALLIES		= 1,
	CHAT_RECIPIENT_OBSERVERS	= 2,
	CHAT_RECIPIENT_REFEREES	    = 2,
	CHAT_RECIPIENT_PRIVATE		= 3
};

DWORD GetLocalPlayer(DWORD gameBase);
void WCTextOut(char* pszMessage, DWORD MsgType, float StayUpTime, DWORD gameBase);

#endif