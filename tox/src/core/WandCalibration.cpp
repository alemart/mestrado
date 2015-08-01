#include <iostream>
#include "WandCalibration.h"

WandCalibration::WandCalibration(std::string calibfile) : _calibMatrix(0)
{
    if(!load(calibfile))
        reset();
    _tmp[0] = cvCreateMat(4, 1, CV_32FC1);
    _tmp[1] = cvCreateMat(3, 1, CV_32FC1);
}

WandCalibration::~WandCalibration()
{
    if(_calibMatrix)
        cvReleaseMat(&_calibMatrix);
    cvReleaseMat(&_tmp[0]);
    cvReleaseMat(&_tmp[1]);
}

ofVec3f WandCalibration::world2norm(ofVec3f worldCoords)
{
    CvMat* A = _calibMatrix;
    CvMat* x = _tmp[0];
    CvMat* b = _tmp[1];

    CV_MAT_ELEM(*x, float, 0, 0) = worldCoords.x;
    CV_MAT_ELEM(*x, float, 1, 0) = worldCoords.y;
    CV_MAT_ELEM(*x, float, 2, 0) = worldCoords.z;
    CV_MAT_ELEM(*x, float, 3, 0) = 1.0f;

    cvGEMM(A, x, 1.0f, NULL, 0.0f, b); // performs b = Ax

    return ofVec3f(
        CV_MAT_ELEM(*b, float, 0, 0),
        CV_MAT_ELEM(*b, float, 1, 0),
        CV_MAT_ELEM(*b, float, 2, 0)
    );
}

void WandCalibration::reset()
{
    if(_calibMatrix)
        cvReleaseMat(&_calibMatrix);
    _calibMatrix = cvCreateMat(3, 4, CV_32FC1);
    cvSetIdentity(_calibMatrix);
    std::cout << "[WandCalibration] calibration reset." << std::endl;
}

bool WandCalibration::load(std::string calibfile)
{
    if(_calibMatrix)
        cvReleaseMat(&_calibMatrix);

    std::cout << "[WandCalibration] will load '" << calibfile << "'..." << std::endl;
    if(!(_calibMatrix = (CvMat*)cvLoad(calibfile.c_str()))) {
        std::cout << "[WandCalibration] can't load '" << calibfile << "'." << std::endl;
        return false;
    }
    else {
        std::cout << "[WandCalibration] loaded '" << calibfile << "'." << std::endl;
        return true;
    }
}

bool WandCalibration::save(std::string calibfile)
{
    std::cout << "[WandCalibration] will save to '" << calibfile << "'..." << std::endl;
    if(_calibMatrix) {
        cvSave(calibfile.c_str(), _calibMatrix);
        std::cout << "[WandCalibration] saved to '" << calibfile << "'." << std::endl;
        return true;
    }
    else {
        std::cout << "[WandCalibration] can't save to '" << calibfile << "': no calibmatrix." << std::endl;
        return false;
    }
}

void WandCalibration::beginCalibration()
{
    reset();
    _correspondences.clear();
}

void WandCalibration::addCorrespondence(ofVec3f normalizedCoords, ofVec3f worldCoords)
{
    _correspondences.push_back(
        std::pair<ofVec3f, ofVec3f>( normalizedCoords, worldCoords )
    );
}

bool WandCalibration::endCalibration()
{
    if(_correspondences.size() >= 4) {
        int n = int(_correspondences.size());

        CvMat* A = cvCreateMat(3*n, 12, CV_32FC1);
        CvMat* x = cvCreateMat(12, 1, CV_32FC1);
        CvMat* b = cvCreateMat(3*n, 1, CV_32FC1);

        cvZero(A);
        cvZero(x);

        for(int j=0; j<n; j++) {
            ofVec3f& normalized = _correspondences[j].first;
            ofVec3f& world = _correspondences[j].second;

            CV_MAT_ELEM(*b, float, 3*j + 0, 0) = normalized.x;
            CV_MAT_ELEM(*b, float, 3*j + 1, 0) = normalized.y;
            CV_MAT_ELEM(*b, float, 3*j + 2, 0) = normalized.z;

            for(int i=0; i<3; i++) {
                CV_MAT_ELEM(*A, float, 3*j + i, 4*i + 0) = world.x;
                CV_MAT_ELEM(*A, float, 3*j + i, 4*i + 1) = world.y;
                CV_MAT_ELEM(*A, float, 3*j + i, 4*i + 2) = world.z;
                CV_MAT_ELEM(*A, float, 3*j + i, 4*i + 3) = 1.0f;
            }
        }

        cvSolve(A, b, x, CV_SVD); // find x such that || b - Ax || is minimized

        for(int j=0; j<3; j++) {
            for(int i=0; i<4; i++)
                CV_MAT_ELEM(*_calibMatrix, float, j, i) = CV_MAT_ELEM(*x, float, 4*j + i, 0);
        }

        return true;
    }
    else
        return false; // too few correspondences!
}
