#ifndef _TOUCHDETECTION_H
#define _TOUCHDETECTION_H

#include "ofMain.h"
#include "Scene.h"
class TouchDetector;
class TouchTracker;
class EraserDetector;

// touch detection scene
class TouchDetectionScene : public Scene
{
public:
    TouchDetectionScene(MyApplication* app);
    virtual ~TouchDetectionScene();

    virtual void update();
    virtual void draw();

    virtual void keyPressed(int key);

private:
    TouchDetector* _touchDetector;
    TouchTracker* _touchTracker;
    EraserDetector* _eraserDetector;

    ofTrueTypeFont _fnt;
    bool _displayDepthImage;
};

#endif
