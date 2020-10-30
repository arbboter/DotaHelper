#ifndef _MAIN_H_
#define _MAIN_H_

#define DLLIMPORT __declspec(dllexport)

#define CAMERA_AREA_OFFSET	0x936424
#define CAMERA_AREA_SIZE	64

#define CAMERA_DISTANCE		0x93645C
#define CAMERA_AOA			0x936438
#define CAMERA_ROTATION		0x936444

#define MAPHACK_AREA_OFFSET	0x2024AC
#define MAPHACK_AREA_SIZE	5546819

struct featurelist
{
	bool revealUnitsMainmap, revealUnitsMinimap, showSkillsCooldowns, makeUnitsClickable, 
		showEnemyPings, greyHP, colorInvisible, enableTrading, showResources,
		protectFromAH, roshanNotifier, runeNotifier, manabar;
	int	removeFogMainmap, removeFogMinimap,
		distance, angle, rotation;
};

void			PatchFeatures(featurelist features);

void			EnableDebugPriv();
DLLIMPORT void	MainProc();

void			HookChatFunc();
void			ColoredInviFunc();
void			GreyHPFunc();
void			RoshanNotifierFunc();
void			RuneNotifierFunc();

void			PlantDetourJMP(BYTE* source, const BYTE* destination, const int length);
void			PlantDetourCALL(BYTE* source, const BYTE* destination, const int length);

bool			StartsWith(char* source, char* compare);
bool			IsNumber(char* source);

void			SetCameraDistance(float distance);

#endif