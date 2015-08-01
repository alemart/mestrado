#ifndef _BACKGROUNDCALIBRATION_H
#define _BACKGROUNDCALIBRATION_H

#include <vector>
#include <string>
#include "Scene.h"
#include "ofMain.h"
#include "../core/ImageAccumulator.h"

// background calibration scene
class BackgroundCalibrationScene : public Scene
{
public:
    BackgroundCalibrationScene(MyApplication* app, int numberOfFramesToCollect = 90);
    virtual ~BackgroundCalibrationScene();

    virtual void update();
    virtual void draw();

    virtual void mousePressed(int x, int y, int button);
    virtual void keyPressed(int key);

private:
    bool _sampleBackground; // should i sample the bg?
    int _numberOfFramesToCollect;
    std::vector<ofPoint> _point;
    ofTrueTypeFont _fnt;
    ImageAccumulator _imgAccum;
    bool _showRGBImage;
};

#endif
