#include "Includes.h"
#include "GetWCGameState.h"

DWORD GetWCGameState(DWORD gameBase)
{
	return *(int*)(gameBase + GAME_STATE_OFFSET);
}

bool IsIngame(DWORD gameBase)
{
	/*DWORD dwGameState = GetWCGameState(gameBase);
	return (dwGameState >= SP_INGAME && dwGameState != BNET_VIEWING_PROFILE && dwGameState != BNET_VIEWING_PROFILE_2);*/
	return ((*(DWORD*)(gameBase + 0xAB5738)) == 4 && (*(DWORD*)(gameBase + 0xAB573C)) == 4);
}