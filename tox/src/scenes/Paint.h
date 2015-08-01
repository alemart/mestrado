#ifndef _PAINT_H
#define _PAINT_H

#include "ofMain.h"
#include "Scene.h"
class TouchDetector;
class TouchTracker;

// paint scene
class PaintScene : public Scene
{
public:
    PaintScene(MyApplication* app);
    virtual ~PaintScene();

    virtual void update();
    virtual void draw();

    virtual void keyPressed(int key);

private:
    TouchDetector* _touchDetector;
    TouchTracker* _touchTracker;
    bool _clear;
    float _startTime;
};

#endif
