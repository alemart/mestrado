#ifndef _VIDEOPLAYER_H
#define _VIDEOPLAYER_H

#include "Scene.h"
class ofxCvImage;

// Video player scene
class VideoPlayerScene : public Scene
{
public:
    VideoPlayerScene(MyApplication* app);
    virtual ~VideoPlayerScene();

    virtual void update();
    virtual void draw();

    virtual void keyPressed(int key);

private:
    ofxCvImage* _cameraImage;
};

#endif
