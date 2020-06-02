#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#pragma warning(disable: 4800)
#pragma warning(disable: 4244)

#include <Windows.h>
#include <Tlhelp32.h>
#include <ShellAPI.h>
#include <Psapi.h>
#include <iostream>
#include <fstream>


#define INI_FILE_NAME   ".\\DotaHelpler.ini"