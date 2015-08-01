#ifndef _VIDEORECORDER_H
#define _VIDEORECORDER_H

#include <string>
#include "ofMain.h"
#include "Scene.h"
#include "../core/ImageAccumulator.h"

// Video recorder scene
// records both RGB and Depth images
class VideoRecorderScene : public Scene
{
public:
    VideoRecorderScene(MyApplication* app, const std::string& sequenceName, int numberOfFramesToCollect = 180);
    virtual ~VideoRecorderScene();

    virtual void update();
    virtual void draw();

private:
    std::string _seqName;
    int _numberOfFramesToCollect;
    ofTrueTypeFont _fnt;
    ImageAccumulator _imgAccum[2];
};

#endif
