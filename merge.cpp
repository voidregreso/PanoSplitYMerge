#include "framework.h"
#include "LogUtil.h"
#include "pano_api.h"

enum cube_faces { X_POS, X_NEG, Y_POS, Y_NEG, Z_POS, Z_NEG };

struct cart3D {
    double x;
    double y;
    double z;
    int faceIndex;
};

struct cart2D {
    double x;
    double y;
    int faceIndex;
};

cv::Vec2f unit3DToUnit2D(double x, double y, double z, int faceIndex) {
    double x2D, y2D;

    if (faceIndex == X_POS) { // X+
        x2D = y + 0.5;
        y2D = z + 0.5;
    }
    else if (faceIndex == X_NEG) { // X-
        x2D = (y * -1) + 0.5;
        y2D = z + 0.5;
    }
    else if (faceIndex == Y_POS) { // Y+
        x2D = x + 0.5;
        y2D = z + 0.5;
    }
    else if (faceIndex == Y_NEG) { // Y-
        x2D = (x * -1) + 0.5;
        y2D = z + 0.5;
    }
    else if (faceIndex == Z_POS) { // Z+
        x2D = y + 0.5;
        y2D = (x * -1) + 0.5;
    }
    else { // Z-
        x2D = y + 0.5;
        y2D = x + 0.5;
    }

    y2D = 1 - y2D;

    return cv::Vec2f(x2D, y2D);
}

cart3D projectX(double theta, double phi, int sign) {
    cart3D result;

    result.x = sign * 0.5;
    result.faceIndex = sign == 1 ? X_POS : X_NEG;
    double rho = result.x / (cos(theta) * sin(phi));
    result.y = rho * sin(theta) * sin(phi);
    result.z = rho * cos(phi);
    return result;
}

cart3D projectY(double theta, double phi, int sign) {
    cart3D result;

    result.y = -sign * 0.5;
    result.faceIndex = sign == 1 ? Y_POS : Y_NEG;
    double rho = result.y / (sin(theta) * sin(phi));
    result.x = rho * cos(theta) * sin(phi);
    result.z = rho * cos(phi);
    return result;
}

cart3D projectZ(double theta, double phi, int sign) {
    cart3D result;

    result.z = sign * 0.5;
    result.faceIndex = sign == 1 ? Z_POS : Z_NEG;
    double rho = result.z / cos(phi);
    result.x = rho * cos(theta) * sin(phi);
    result.y = rho * sin(theta) * sin(phi);
    return result;
}

cart2D convertEquirectUVtoUnit2D(double theta, double phi, int square_length) {
    // Calculate the unit vector
    double x = cos(theta) * sin(phi);
    double y = sin(theta) * sin(phi);
    double z = cos(phi);

    // Find the maximum value in the unit vector
    double maximum = max(abs(x), max(abs(y), abs(z)));
    double xx = x / maximum;
    double yy = -y / maximum;
    double zz = z / maximum;

    // Project ray to cube surface
    cart3D equirectUV;
    if (xx == 1 or xx == -1) {
        equirectUV = projectX(theta, phi, xx);
    }
    else if (yy == 1 or yy == -1) {
        equirectUV = projectY(theta, phi, yy);
    }
    else {
        equirectUV = projectZ(theta, phi, zz);
    }

    cv::Vec2f unit2D = unit3DToUnit2D(equirectUV.x, equirectUV.y, equirectUV.z,
        equirectUV.faceIndex);

    unit2D[0] *= square_length;
    unit2D[1] *= square_length;

    cart2D result;
    result.x = (int)unit2D[0];
    result.y = (int)unit2D[1];
    result.faceIndex = equirectUV.faceIndex;

    return result;
}

// front, back, left, right, top, bottom
extern "C" BOOL merge_cube2sphere(LPCSTR *cubesFile, LPCSTR mergeFile) {
    if (cubesFile == NULL) return FALSE;
    try {
        Mat frontImg = imread(cubesFile[0]); // +X
        Mat backImg = imread(cubesFile[1]); // -X
        Mat leftImg = imread(cubesFile[2]); // +Y
        Mat rightImg = imread(cubesFile[3]); // -Y
        Mat topImg = imread(cubesFile[4]); // +Z
        Mat bottomImg = imread(cubesFile[5]); // -Z

        const int output_height = floor((frontImg.rows + leftImg.rows + topImg.rows + backImg.rows + rightImg.rows + bottomImg.rows) / 6);
        const int output_width = output_height * 2;
        const int square_length = output_height;

        // Placeholder image for the result
        Mat destination(output_height, output_width, CV_8UC3,
            Scalar(255, 255, 255));

#pragma omp parallel for
        for (int j = 0; j < destination.cols; j++) {
            for (int i = 0; i < destination.rows; i++) {
                // 2. Get the normalised u,v coordinates for the current pixel
                double U = (double)j / (output_width - 1); // 0..1
                double V = (double)i / (output_height - 1); // No need for 1-... as the image output
                // needs to start from the top anyway.
                double theta = U * 2 * M_PI;
                double phi = V * M_PI;
                // 4. calculate the 3D cartesian coordinate which has been projected to
                // a cubes face
                cart2D cart = convertEquirectUVtoUnit2D(theta, phi, square_length);

                // 5. use this pixel to extract the colour
                cv::Vec3b val;
                if (cart.faceIndex == X_POS) {
                    val = frontImg.at<cv::Vec3b>(cart.y, cart.x);
                }
                else if (cart.faceIndex == X_NEG) {
                    val = backImg.at<cv::Vec3b>(cart.y, cart.x);
                }
                else if (cart.faceIndex == Y_POS) {
                    val = leftImg.at<cv::Vec3b>(cart.y, cart.x);
                }
                else if (cart.faceIndex == Y_NEG) {
                    val = rightImg.at<cv::Vec3b>(cart.y, cart.x);
                }
                else if (cart.faceIndex == Z_POS) {
                    val = topImg.at<cv::Vec3b>(cart.y, cart.x);
                }
                else if (cart.faceIndex == Z_NEG) {
                    val = bottomImg.at<cv::Vec3b>(cart.y, cart.x);
                }

                destination.at<cv::Vec3b>(i, j) = val;
            }
        }
        imwrite(mergeFile, destination);
        return TRUE;
    }
    catch (exception& ex) {
        logd("Exception occurred: %s", const_cast<char*>(ex.what()));
        return FALSE;
    }
}