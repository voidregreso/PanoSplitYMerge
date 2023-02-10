#include "framework.h"
#include "LogUtil.h"
#include "pano_api.h"
#include <thread>

float faceTransform[6][2] =
{
    {0, 0},
    {M_PI / 2, 0},
    {M_PI, 0},
    {-M_PI / 2, 0},
    {0, -M_PI / 2},
    {0, M_PI / 2}
};
const char chs[] = { 'f', 'r', 'b', 'l', 'u', 'd' };

inline void createCubeMapFace(const Mat& in, Mat& face, int faceId = 0, const int width = -1, const int height = -1)
{
    float inWidth = (float)in.cols;
    float inHeight = (float)in.rows;     // Obtener el numero de filas de la imagen

    // Allocate map
    Mat mapx(height, width, CV_32F);
    Mat mapy(height, width, CV_32F);             // Asignacion de los ejes X & Y de la imagen

    // Calculate adjacent (ak) and opposite (an) of the triangle that
    // is spanned from the sphere center to our cube face.
    const float an = sin(M_PI / 4);
    const float ak = cos(M_PI / 4);      // Calculo del centro de un triangulo tensor en una esfera adyacente a "ak" y opuesta a "an"

    const float ftu = faceTransform[faceId][0];
    const float ftv = faceTransform[faceId][1];

    // Para cada imagen, calcule las coordenadas de la fuente correspondientes
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            // Trazado de coordenadas en un plano
            float nx = (float)y / (float)height - 0.5f;
            float ny = (float)x / (float)width - 0.5f;

            nx *= 2;
            ny *= 2;

            // Map [-1, 1] plane coord to [-an, an]
            // thats the coordinates in respect to a unit sphere
            // that contains our box.
            nx *= an;
            ny *= an;

            float u, v;

            // Project from plane to sphere surface.
            if (ftv == 0) {
                // Center faces
                u = atan2(nx, ak);
                v = atan2(ny * cos(u), ak);
                u += ftu;
            }
            else if (ftv > 0) {
                // Bottom face
                float d = sqrt(nx * nx + ny * ny);
                v = M_PI / 2 - atan2(d, ak);
                u = atan2(ny, nx);
            }
            else {
                // Top face
                float d = sqrt(nx * nx + ny * ny);
                v = -M_PI / 2 + atan2(d, ak);
                u = atan2(-ny, nx);
            }

            // Map from angular coordinates to [-1, 1], respectively.
            u = u / (M_PI);
            v = v / (M_PI / 2);

            // Warp around, if our coordinates are out of bounds.
            while (v < -1) {
                v += 2;
                u += 1;
            }
            while (v > 1) {
                v -= 2;
                u += 1;
            }

            while (u < -1) {
                u += 2;
            }
            while (u > 1) {
                u -= 2;
            }

            // Map from [-1, 1] to in texture space
            u = u / 2.0f + 0.5f;
            v = v / 2.0f + 0.5f;

            u = u * (inWidth - 1);
            v = v * (inHeight - 1);

            mapx.at<float>(x, y) = u;
            mapy.at<float>(x, y) = v;
        }
    }

    // Recreate output image if it has wrong size or type.
    if (face.cols != width || face.rows != height ||
        face.type() != in.type()) {
        face = Mat(width, height, in.type());
    }

    // Do actual  using OpenCV's remap
    remap(in, face, mapx, mapy, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));
    if (faceId == 4) rotate(face, face, ROTATE_90_CLOCKWISE); // for top side, rotate clockwise 90
    if (faceId == 5) rotate(face, face, ROTATE_90_COUNTERCLOCKWISE); // for bottom side, rotate counter-clockwise 90 (270)
}

LPSTR CNext(LPCSTR ptr)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte(ptr[0]) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

LPSTR ExtractName(LPCSTR lpszPath)
{
    LPCSTR lastSlash = lpszPath;

    while (lpszPath && *lpszPath)
    {
        if ((*lpszPath == '\\' || *lpszPath == '/' || *lpszPath == ':') &&
            lpszPath[1] && lpszPath[1] != '\\' && lpszPath[1] != '/')
            lastSlash = lpszPath + 1;
        lpszPath = CNext(lpszPath);
    }
    return (LPSTR)lastSlash;
}

extern "C" BOOL split_sphere2cube(LPCSTR inFile, LPCSTR outFolder) {
    vector<thread> threads;
    Mat resImgs[6];
    try {
        Mat srcImg = imread(inFile);
        int w, h;
        if (srcImg.cols != srcImg.rows * 2) {
            logd("Not a standard sphere pano image");
            return FALSE;
        }
        w = h = srcImg.rows;
        for (int i = 0; i < 6; i++) {
            threads.push_back(thread([&srcImg, i, w, h, &resImgs] {
                Mat resImg;
                createCubeMapFace(srcImg, resImg, i, w, h);
                resImg.copyTo(resImgs[i]);
            }));
        }
        for (auto& t : threads) t.join();
        for (int i = 0; i < 6; i++) {
            string name = string(ExtractName(inFile));
            string outFile = name.substr(0, name.rfind(".")) + "_" + chs[i] + ".jpg";
            if (outFolder != NULL) outFile = string(outFolder) + "/" + outFile;
            imwrite(outFile, resImgs[i]);
        }
        return TRUE;
    }
    catch (exception& ex) {
        logd("Exception occurred: %s", const_cast<char*>(ex.what()));
        return FALSE;
    }
}