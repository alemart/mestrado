#ifndef _PAINT2_H
#define _PAINT2_H

#include <vector>
#include <string>
#include "ofMain.h"
#include "Scene.h"
class TouchDetector;
class TouchTracker;

// paint2 scene
class Paint2Scene : public Scene
{
public:
    Paint2Scene(MyApplication* app);
    virtual ~Paint2Scene();

    virtual void update();
    virtual void draw();

    virtual void keyPressed(int key);

private:
    TouchDetector* _touchDetector;
    TouchTracker* _touchTracker;
    ofImage _canvas;
    bool _debug;

    ofVec2f _cameraPosition;
    struct Segment
    {
        float currX, currY, prevX, prevY;
        std::string color;
    };
    std::vector<Segment> _segments;

    void _drawSegment(Segment* s);
};

#endif
