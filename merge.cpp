#include "framework.h"
#include "LogUtil.h"
#include "pano_api.h"

#include <thread>

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

Vec2f unit3DToUnit2D(double x, double y, double z, int faceIndex) {
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

    return Vec2f(x2D, y2D);
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

    Vec2f unit2D = unit3DToUnit2D(equirectUV.x, equirectUV.y, equirectUV.z,
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

        if (frontImg.cols != frontImg.rows || backImg.cols != backImg.rows || leftImg.cols != leftImg.rows
            || rightImg.cols != rightImg.rows || topImg.cols != topImg.rows || bottomImg.cols != bottomImg.rows) {
            logd("Height & weight of each image must be equal");
            return FALSE;
        }

        const int output_height = frontImg.rows - 1;
        const int output_width = output_height * 2;
        const int square_length = output_height;

        // Placeholder image for the result
        Mat destination(output_height, output_width, CV_8UC3,
            Scalar(255, 255, 255));

        parallel_for_(Range(0, destination.rows), [&](const Range& range) {
            for (int i = range.start; i < range.end; i++) {
                for (int j = 0; j < destination.cols; j++) {
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
                    Vec3b val;
                    if (cart.faceIndex == X_POS) {
                        val = frontImg.at<Vec3b>(cart.y, cart.x);
                    }
                    else if (cart.faceIndex == X_NEG) {
                        val = backImg.at<Vec3b>(cart.y, cart.x);
                    }
                    else if (cart.faceIndex == Y_POS) {
                        val = leftImg.at<Vec3b>(cart.y, cart.x);
                    }
                    else if (cart.faceIndex == Y_NEG) {
                        val = rightImg.at<Vec3b>(cart.y, cart.x);
                    }
                    else if (cart.faceIndex == Z_POS) {
                        val = topImg.at<Vec3b>(cart.y, cart.x);
                    }
                    else if (cart.faceIndex == Z_NEG) {
                        val = bottomImg.at<Vec3b>(cart.y, cart.x);
                    }

                    destination.at<Vec3b>(i, j) = val;
                }
            }
            });
        resize(destination, destination, Size(frontImg.cols * 2, frontImg.rows));
        imwrite(mergeFile, destination);
        return TRUE;
    }
    catch (exception& ex) {
        logd("Exception occurred: %s", const_cast<char*>(ex.what()));
        return FALSE;
    }
}

/*
void merge_cube2sphere(const Mat& cube, Mat& sphere, int square_length) {
    int chunk_size = cube.rows / std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < cube.rows; i += chunk_size) {
        int end = std::min(i + chunk_size, cube.rows);
        threads.emplace_back(merge_cube2sphere_chunk, std::ref(cube), std::ref(sphere),
            square_length, i, end);
    }
    for (auto& t : threads) {
        t.join();
    }
}

void merge_cube2sphere_chunk(const Mat& cube, Mat& sphere, int square_length,
    int start, int end) {
    double theta, phi;
    cart2D result;
    for (int y = start; y < end; y++) {
        for (int x = 0; x < cube.cols; x++) {
            theta = x * 2.0 * M_PI / square_length;
            phi = (y + 0.5) * M_PI / square_length;
            result = convertEquirectUVtoUnit2D(theta, phi, square_length);
            sphere.at<Vec3b>(y, x) = cube.at<Vec3b>(result.y, result.x + square_length * result.faceIndex);
        }
    }
}*/
