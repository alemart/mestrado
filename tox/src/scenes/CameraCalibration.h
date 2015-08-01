#ifndef _CAMERACALIBRATION_H
#define _CAMERACALIBRATION_H

#include <vector>
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "Scene.h"

class CameraCalibrationScene : public Scene
{
public:
    CameraCalibrationScene(MyApplication* app, int numberOfBoards = 20, int hsize = 7, int vsize = 10, float squareLengthInMeters = 0.024f, float screenshotInterval = 2.0f);
    virtual ~CameraCalibrationScene();

    virtual void update();
    virtual void draw();

private:
    CvSize _patternSize;
    int _numberOfBoards;
    float _squareLength;
    std::vector<CvPoint2D32f*> _acceptedBoards;
    void _calibrate();

    float _timer, _screenshotInterval;
    ofTrueTypeFont _fnt;
};

#endif
