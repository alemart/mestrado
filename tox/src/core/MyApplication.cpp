#include <iostream>
#include <cmath>
#include <cstdlib>
#include "MyApplication.h"
#include "InputVideoStream.h"
#include "BackgroundModel.h"
#include "CalibratedKinect.h"
#include "TouchDetector.h"

#include "../scenes/BackgroundCalibration.h"
#include "../scenes/CameraCalibration.h"
#include "../scenes/CameraCorrespondenceCalibration.h"
#include "../scenes/Scene.h"
#include "../scenes/TouchDetection.h"
#include "../scenes/VideoPlayer.h"
#include "../scenes/VideoRecorder.h"
#include "../scenes/Paint.h"
#include "../scenes/Paint2.h"
#include "../scenes/Play.h"
#include "../scenes/Music.h"
#include "../scenes/TUIO.h"
#include "../scenes/WandCalibration.h"

// hflip / vflip
#define RM_VFLIP           1
#define RM_HFLIP           2
//#define RENDER_MODE        (RM_HFLIP | RM_VFLIP)   // useful for projector stuff?
#define RENDER_MODE        (0)


// tela: 68 x 52 = 3536 cm^2 = 0.35 m^2
static const char usage[] = 
    "Usage: ./tox SCENE_NAME (VIDEO_INPUT|openni|freenect|freenect_ir|webcam|null) [--fullscreen]\n"
    "\n"
    "where:\n"
    "   SCENE_NAME is one of the following:\n"
    "       bgcalib, wandcalib, touch, paint, paint2, tuio, play, music\n"
    "       videoplayer, videorecorder, camcalib_rgb, camcalib_ir, correspcalib.\n"
    "\n"
    "   VIDEO_INPUT is the name of the desired video input\n"
    "   (if not specified, the input will be provided by a kinect device)\n"
;




//
// constructors & destructors
//

MyApplication::MyApplication(int argc, char** argv) : _kinect(0), _currentScene(0), _nextScene(0)
{
    InputVideoStream* in;

    if(argc >= 2 && argc <= 3) {
        // define the actual input device
        if(argc == 3) {
            if(std::string(argv[2]) == "webcam")
                in = new WebcamInputVideoStream();
            else if(std::string(argv[2]) == "openni")
                in = new OpenNIInputVideoStream();
            else if(std::string(argv[2]) == "freenect_ir")
                in = new FreenectInputVideoStream(true);
            else if(std::string(argv[2]) == "freenect")
                in = new FreenectInputVideoStream(false);
            else if(std::string(argv[2]) == "null")
                in = new NullInputVideoStream();
            else
                in = new StoredInputVideoStream(argv[2]);
        }
        else
            in = new OpenNIInputVideoStream();

        // creating the calibrated kinect device...
        _kinect = new CalibratedKinect(in);

        // define the initial scene
        _initialSceneName = argv[1];
    }
    else
        _usage();
}



MyApplication::~MyApplication()
{
    if(_currentScene)
        delete _currentScene;

    if(_nextScene)
        delete _nextScene;

    delete _kinect;
}






//
// main routines
//

void MyApplication::setup()
{
    ofBackgroundHex(0xFFFFFF);
    ofSetFrameRate(60);
    setDefaultWindowTitle();

    // kinect
    _kinect->setup();

    // initial scene
    std::string sceneName(_initialSceneName);
    if(sceneName == "touch")
        setScene(new TouchDetectionScene(this));
    else if(sceneName == "bgcalib")
        setScene(new BackgroundCalibrationScene(this));
    else if(sceneName == "wandcalib")
        setScene(new WandCalibrationScene(this));
    else if(sceneName == "paint")
        setScene(new PaintScene(this));
    else if(sceneName == "paint2")
        setScene(new Paint2Scene(this));
    else if(sceneName == "play")
        setScene(new PlayScene(this));
    else if(sceneName == "music")
        setScene(new MusicScene(this));
    else if(sceneName == "tuio")
        setScene(new TUIOScene(this));
    else if(sceneName == "videoplayer")
        setScene(new VideoPlayerScene(this));
    else if(sceneName == "videorecorder") {
        std::string name;
        int frames;

        do {
            std::cout << "Please enter the name of the video: " << std::flush;
            std::cin >> name;
        } while(name == "");

        do { 
            std::cout << "Please enter the desired number of frames (e.g., 180): " << std::flush;
            std::cin >> frames;
        } while(frames <= 0);

        setScene(new VideoRecorderScene(this, name, frames));
    }
    else if(sceneName == "camcalib_rgb" || sceneName == "camcalib_ir") {
        if(sceneName == "camcalib_ir")
            _kinect->toggleInfrared();
        setScene(new CameraCalibrationScene(this));
    }
    else if(sceneName == "correspcalib")
        setScene(new CameraCorrespondenceCalibrationScene3(this));
    else if(sceneName == "correspcalib_calc")
        setScene(new CameraCorrespondenceCalibrationSceneCalc(this));
    else
        _usage();
}

void MyApplication::update()
{
    // fps
    if(_title == "-")
        ofSetWindowTitle(std::string("fps: ") + ofToString(ofGetFrameRate(), 2));
    else
        ofSetWindowTitle(_title);

    // update the kinect
    _kinect->update();

    // scene management
    if(_nextScene) {
        if(_currentScene)
            delete _currentScene;
        _currentScene = _nextScene;
        _nextScene = 0;
    }
    if(_currentScene)
        _currentScene->update();
}

void MyApplication::draw()
{
    // scene mgmt
    if(_currentScene) {
        ofPushMatrix();
        ofTranslate((RENDER_MODE & RM_HFLIP) ? float(ofGetWidth()) : 0.0f, (RENDER_MODE & RM_VFLIP) ? float(ofGetHeight()) : 0.0f);
        ofScale((RENDER_MODE & RM_HFLIP) ? -1.0f : 1.0f, (RENDER_MODE & RM_VFLIP) ? -1.0f : 1.0f);
        _currentScene->draw();
        ofPopMatrix();
    }

    glFlush();
}








//
// events (from openframeworks)
//

void MyApplication::keyPressed(int key)
{
    if(key == OF_KEY_F12)
        _kinect->toggleInfrared();

    if(_currentScene)
        _currentScene->keyPressed(key);
}

void MyApplication::keyReleased(int key)
{
    if(_currentScene)
        _currentScene->keyReleased(key);
}

void MyApplication::mouseMoved(int x, int y)
{
    if(_currentScene)
        _currentScene->mouseMoved(x, y);
}

void MyApplication::mouseDragged(int x, int y, int button)
{
    if(_currentScene)
        _currentScene->mouseDragged(x, y, button);
}

void MyApplication::mousePressed(int x, int y, int button)
{
    if(_currentScene)
        _currentScene->mousePressed(x, y, button);
}

void MyApplication::mouseReleased(int x, int y, int button)
{
    if(_currentScene)
        _currentScene->mouseReleased(x, y, button);
}

void MyApplication::windowResized(int w, int h)
{
    ;
}

void MyApplication::gotMessage(ofMessage msg)
{
    ;
}

void MyApplication::dragEvent(ofDragInfo dragInfo)
{
    ;
}








//
// scene management
//

void MyApplication::setScene(Scene* scene)
{
    if(_nextScene)
        delete _nextScene;

    _nextScene = scene;
}


Scene* MyApplication::scene() const
{
    return _currentScene;
}




//
// kinect
//

CalibratedKinect* MyApplication::kinect() const
{
    return _kinect;
}



//
// window title
//
void MyApplication::setWindowTitle(std::string title)
{
    _title = title;
}

void MyApplication::setDefaultWindowTitle()
{
    _title = "-";
}



//
// misc
//

void MyApplication::_usage() const
{
    std::cout << usage << std::endl;
    std::exit(0);
}
