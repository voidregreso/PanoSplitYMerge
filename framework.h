// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <opencv2/imgproc.hpp>    
#include <opencv2/core.hpp>          
#include <opencv2/highgui.hpp>   
#include <cmath>
#include <omp.h>
#include <iostream>

using namespace std;
using namespace cv;

#define INTERNAL_PANO 1

#if defined(_DEBUG)
#pragma comment(lib, "ittnotifyd.lib") // needed by static-link only
#pragma comment(lib, "IlmImfd.lib") // needed by static-link only
#pragma comment(lib, "libjpeg-turbod.lib") // needed by static-link only
#pragma comment(lib, "libopenjp2d.lib") // needed by static-link only
#pragma comment(lib, "libpngd.lib") // needed by static-link only
#pragma comment(lib, "libtiffd.lib") // needed by static-link only
#pragma comment(lib, "libwebpd.lib") // needed by static-link only
#pragma comment(lib, "opencv_calib3d460d.lib") // needed by static-link only
#pragma comment(lib, "opencv_core460d.lib")
#pragma comment(lib, "opencv_flann460d.lib") // needed by static-link only
#pragma comment(lib, "opencv_highgui460d.lib")
#pragma comment(lib, "opencv_imgcodecs460d.lib") // needed by static-link only
#pragma comment(lib, "opencv_imgproc460d.lib") // needed by static-link only
#pragma comment(lib, "opencv_stitching460d.lib")
#pragma comment(lib, "opencv_video460d.lib") // needed by static-link only
#pragma comment(lib, "opencv_videoio460d.lib")
#pragma comment(lib, "opencv_videostab460d.lib")
#pragma comment(lib, "zlibd.lib") // needed by static-link only
#else
#pragma comment(lib, "ittnotify.lib") // needed by static-link only
#pragma comment(lib, "IlmImf.lib") // needed by static-link only
#pragma comment(lib, "libjpeg-turbo.lib") // needed by static-link only
#pragma comment(lib, "libopenjp2.lib") // needed by static-link only
#pragma comment(lib, "libpng.lib") // needed by static-link only
#pragma comment(lib, "libtiff.lib") // needed by static-link only
#pragma comment(lib, "libwebp.lib") // needed by static-link only
#pragma comment(lib, "opencv_calib3d460.lib") // needed by static-link only
#pragma comment(lib, "opencv_core460.lib")
#pragma comment(lib, "opencv_flann460.lib") // needed by static-link only
#pragma comment(lib, "opencv_highgui460.lib")
#pragma comment(lib, "opencv_imgcodecs460.lib") // needed by static-link only
#pragma comment(lib, "opencv_imgproc460.lib") // needed by static-link only
#pragma comment(lib, "opencv_stitching460.lib")
#pragma comment(lib, "opencv_video460.lib") // needed by static-link only
#pragma comment(lib, "opencv_videoio460.lib")
#pragma comment(lib, "opencv_videostab460.lib")
#pragma comment(lib, "zlib.lib") // needed by static-link only
#endif

const float M_PI = 3.14159265358979323846f;