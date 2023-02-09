#pragma once
#include <windows.h>

#if defined(INTERNAL_PANO)
#define MYAPI _declspec(dllexport)
#else
#define MYAPI _declspec(dllimport)
#endif

extern "C" {
	MYAPI BOOL split_sphere2cube(LPCSTR inFile, LPCSTR outFolder = NULL);
	// front, back, left, right, top, bottom
	MYAPI BOOL merge_cube2sphere(LPCSTR *cubesFile, LPCSTR mergeFile);
}