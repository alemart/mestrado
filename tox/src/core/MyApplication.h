#ifndef _MYAPPLICATION_H
#define _MYAPPLICATION_H

#include <string>
#include "ofMain.h"

class Scene;
class CalibratedKinect;

class MyApplication : public ofBaseApp
{
public:
    MyApplication(int argc, char **argv);
    virtual ~MyApplication();

    // main routines
    void setup();
    void update();
    void draw();

    // openframeworks (events)
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    // scene management
    void setScene(Scene* scene);
    Scene* scene() const;

    // kinect
    CalibratedKinect* kinect() const;

    // window title
    void setDefaultWindowTitle();
    void setWindowTitle(std::string str);

private:
    CalibratedKinect* _kinect;
    std::string _title;

    Scene* _currentScene;
    Scene* _nextScene;
    std::string _initialSceneName;

    void _usage() const;
};

#endif
