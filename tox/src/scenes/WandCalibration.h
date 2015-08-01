#ifndef _WANDCALIBSCENE_H
#define _WANDCALIBSCENE_H

#include <string>
#include <set>
#include <map>
#include "TuioServer.h"
#include "TuioObject.h"
#include "TuioCursor.h"
#include "TuioTime.h"
#include "ofMain.h"
#include "Scene.h"
#include "../core/WandTracker.h"
class ImageLabeler;
class TouchDetector;

class WandCalibrationScene : public Scene
{
public:
    WandCalibrationScene(MyApplication* app);
    virtual ~WandCalibrationScene();

    virtual void update();
    virtual void draw();
    virtual void keyPressed(int key);

private: 
    TouchDetector* _touchDetector;
    ImageLabeler* _imageLabeler;
    WandTracker* _wandTracker;
    std::vector<WandTracker::Wand> _trackedWands;
    ofTrueTypeFont _fnt, _fnt2;
    bool _first;
};

#endif
