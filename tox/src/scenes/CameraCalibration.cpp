#include "CameraCalibration.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"


// ---------------------------------------
// Camera calibration scene
// ---------------------------------------
CameraCalibrationScene::CameraCalibrationScene(MyApplication* app, int numberOfBoards, int hsize, int vsize, float squareLengthInMeters, float screenshotInterval) : Scene(app), _numberOfBoards(numberOfBoards), _squareLength(squareLengthInMeters), _screenshotInterval(screenshotInterval)
{
    // remove old calibrations
    if(!application()->kinect()->isUsingInfrared()) {
        std::remove("data/camcalib/intrinsics_rgb.yaml");
        std::remove("data/camcalib/distortion_rgb.yaml");
    }
    else {
        std::remove("data/camcalib/intrinsics_ir.yaml");
        std::remove("data/camcalib/distortion_ir.yaml");
    }

    // all right...
    _patternSize = cvSize(hsize, vsize);
    _fnt.loadFont("DejaVuSans.ttf", 12);
    _timer = ofGetElapsedTimef();
}

CameraCalibrationScene::~CameraCalibrationScene()
{
    for(std::vector<CvPoint2D32f*>::iterator it = _acceptedBoards.begin(); it != _acceptedBoards.end(); ++it)
        delete[] (*it);
}

void CameraCalibrationScene::update()
{
    IplImage* img = application()->kinect()->colorImage()->getCvImage();
    bool takeAShot = (ofGetElapsedTimef() > _timer + _screenshotInterval);

    if(_numberOfBoards > 0) {
        CvPoint2D32f* corners = new CvPoint2D32f[_patternSize.width * _patternSize.height];
        int cornerCount = 0;

        int ret = cvFindChessboardCorners(
            img,
            _patternSize,
            corners,
            &cornerCount,
            CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE
        );

        if((ret != 0) && (cornerCount == _patternSize.width * _patternSize.height)) {
            IplImage* gray = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);
            cvCvtColor(img, gray, CV_RGB2GRAY);
            cvFindCornerSubPix(
                gray,
                corners,
                cornerCount,
                cvSize(11, 11),
                cvSize(-1, -1),
                cvTermCriteria(
                    CV_TERMCRIT_EPS + CV_TERMCRIT_ITER,
                    30,
                    0.1
                )
            );
            cvReleaseImage(&gray);
            cvDrawChessboardCorners(img, _patternSize, corners, cornerCount, ret);

            // accept it!!!!
            if(takeAShot) {
                _acceptedBoards.push_back(corners);
                _numberOfBoards--;
                _timer = ofGetElapsedTimef();
                std::cerr << "[camcalib] took a picture. " << _numberOfBoards << " more to go." << std::endl;
            }
            else
                delete[] corners;
        }
        else {
            std::cerr << "cvFindChessboardCorners() failed: cornerCount = " << cornerCount << std::endl;
            delete[] corners;
        }
    }
    else {
        std::cerr << "Calibrating..." << std::endl;
        _calibrate();
        std::cerr << "Done!" << std::endl;
        std::exit(0);
    }
}

void CameraCalibrationScene::draw()
{
    ofSetHexColor(0xFFFFFF);
    application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());

    ofEnableAlphaBlending();
    ofSetColor(0, 0, 0, 128);
    ofRect(0, 0, ofGetWidth(), 50);
    ofDisableAlphaBlending();

    std::stringstream ss;
    if(_numberOfBoards > 0)
        ss << "Ainda preciso de " << _numberOfBoards << " tabuleiro" << (_numberOfBoards != 1 ? "s" : "");
    else
        ss << "Calibrando...";
    ofSetHexColor(0xFFAA00);
    _fnt.drawString(ss.str(), 10, 30);
}

void CameraCalibrationScene::_calibrate()
{
    int nBoards = int(_acceptedBoards.size()); // number of boards
    int nCorners = _patternSize.width * _patternSize.height; // number of corners per board

    CvMat* imagePoints = cvCreateMat(nBoards * nCorners, 2, CV_32FC1);
    CvMat* objectPoints = cvCreateMat(nBoards * nCorners, 3, CV_32FC1);
    CvMat* pointCounts = cvCreateMat(nBoards, 1, CV_32SC1);
    CvMat* intrinsicMatrix = cvCreateMat(3, 3, CV_32FC1);
    CvMat* distortionCoeffs = cvCreateMat(5, 1, CV_32FC1);

    // fill the matrices
    for(int i=0; i<nBoards; i++) {
        const CvPoint2D32f* corners = _acceptedBoards[i];
        for(int j=0; j<nCorners; j++) {
            int row = i * nCorners + j;
            CV_MAT_ELEM(*imagePoints, float, row, 0) = corners[j].x;
            CV_MAT_ELEM(*imagePoints, float, row, 1) = corners[j].y;
            CV_MAT_ELEM(*objectPoints, float, row, 0) = float(int(j / _patternSize.width) * _squareLength);
            CV_MAT_ELEM(*objectPoints, float, row, 1) = float(int(j % _patternSize.height) * _squareLength);
            CV_MAT_ELEM(*objectPoints, float, row, 2) = 0.0f;
        }
        CV_MAT_ELEM(*pointCounts, int, i, 0) = nCorners;
    }

    // initialize output matrices
    cvSetIdentity(intrinsicMatrix); // set focal lengths := 1
    cvZero(distortionCoeffs); // in particular, set k3 := 0 (radial)

    // calibrate the camera
    cvCalibrateCamera2(
        objectPoints,
        imagePoints,
        pointCounts,
        cvGetSize(application()->kinect()->colorImage()->getCvImage()),
        intrinsicMatrix,
        distortionCoeffs,
        NULL,
        NULL,
        0 //| CV_CALIB_FIX_K3 //| CV_CALIB_USE_INTRINSIC_GUESS
    );

    // save
    if(!application()->kinect()->isUsingInfrared()) {
        cvSave("data/camcalib/intrinsics_rgb.yaml", intrinsicMatrix);
        cvSave("data/camcalib/distortion_rgb.yaml", distortionCoeffs);
    }
    else {
        cvSave("data/camcalib/intrinsics_ir.yaml", intrinsicMatrix);
        cvSave("data/camcalib/distortion_ir.yaml", distortionCoeffs);
    }

    // done!
    cvReleaseMat(&objectPoints);
    cvReleaseMat(&imagePoints);
    cvReleaseMat(&pointCounts);
    cvReleaseMat(&intrinsicMatrix);
    cvReleaseMat(&distortionCoeffs);
}


