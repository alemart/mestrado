#ifndef _CAMERACORRESPONDENCECALIBRATION_H
#define _CAMERACORRESPONDENCECALIBRATION_H

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "Scene.h"

// camera correspondence calibration scene
class CameraCorrespondenceCalibrationScene : public Scene
{
public:
    CameraCorrespondenceCalibrationScene(MyApplication* app, int hsize = 7, int vsize = 10, float squareLengthInMeters = 0.024f);
    virtual ~CameraCorrespondenceCalibrationScene();

    virtual void update();
    virtual void draw();
    virtual void keyPressed(int key);

private:
    ofTrueTypeFont _fnt;

    CvSize _patternSize;
    float _squareLength;

    CvPoint2D32f* _rgbCorners;
    CvPoint2D32f* _irCorners;
    CvPoint2D32f* _corners;

    bool _foundChessboard;
    enum { WAIT_FOR_RGB_CHESSBOARD, WAIT_FOR_IR_CHESSBOARD } _state;

    virtual void _calibrate();
};

// camera correspondence calibration scene 2
class CameraCorrespondenceCalibrationScene2 : public Scene
{
public:
    CameraCorrespondenceCalibrationScene2(MyApplication* app);
    virtual ~CameraCorrespondenceCalibrationScene2();

    virtual void update();
    virtual void draw();
    virtual void mousePressed(int x, int y, int button);

private:
    ofTrueTypeFont _fnt;
    std::vector<ofVec2f> _colorCorners;
    std::vector<ofVec2f> _depthCorners;

    virtual void _calibrate();
};

// camera correspondence calibration scene 3
class CameraCorrespondenceCalibrationScene3 : public Scene
{
public:
    CameraCorrespondenceCalibrationScene3(MyApplication* app, int hsize = 7, int vsize = 10);
    virtual ~CameraCorrespondenceCalibrationScene3();

    virtual void update();
    virtual void draw();
    virtual void keyPressed(int key);

private:
    ofTrueTypeFont _fnt;
    IplImage* _gray;

    std::vector<IplImage*> _rgbBoards;
    std::vector<IplImage*> _irBoards;
    std::vector<CvPoint2D32f*> _rgbCorners;
    std::vector<CvPoint2D32f*> _irCorners;

    int _cornerCount;
    CvPoint2D32f* _corners;

    virtual void _calibrate();

    int _hsize, _vsize;
    bool _foundChessboard;
};

// camera correspondence calibration scene calc
class CameraCorrespondenceCalibrationSceneCalc : public Scene
{
public:
    CameraCorrespondenceCalibrationSceneCalc(MyApplication* app);
    virtual ~CameraCorrespondenceCalibrationSceneCalc();

    virtual void update();
    virtual void draw();

private:
};

#endif
