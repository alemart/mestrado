#ifndef _TOUCHDETECTOR_H
#define _TOUCHDETECTOR_H

#include <vector>
#include <string>
#include "ofxOpenCv.h"
#include "cvblob.h"
#include "TouchClassifier.h"

class BackgroundModel;
class CalibratedKinect;


class TouchDetector
{
public:
    struct TouchPoint
    {
        int x, y; // position (in depth image coordinates)
        int area; // the area
        float surfaceX, surfaceY; // position in surface coordinates ( [0,1]x[0,1] )
        std::string type; // class of the touch point

        int startX, startY; // initial position
        float surfaceStartX, surfaceStartY; // initial position (in surface coordinates)
        float speedX, speedY; // speed in pixels per second
        float surfaceSpeedX, surfaceSpeedY; // speed (surface coords)

        // internal
        bool _tracked;
        unsigned int _id;
        int _timeToLive;
    };

    TouchDetector(CalibratedKinect* kinect, BackgroundModel* bgmodel = 0);
    TouchDetector(CalibratedKinect* kinect, TouchClassifier::TouchClassifierQuality classifierQuality, BackgroundModel* bgmodel = 0);
    ~TouchDetector();

    // touuchhhhhh
    void update();
    std::vector<TouchPoint> touchPoints() const;

    // load/save from/to disk
    inline void load() { _loadFromFile(); }
    inline void save() const { _saveToFile(); }

    // touch detection - parameters
    inline int lo() const { return _lo; };
    inline void setLo(int lo) { _lo = lo >= 0 && lo < _hi ? lo : _lo; }
    inline int hi() const { return _hi; };
    inline void setHi(int hi) { _hi = hi >= _lo && hi <= 0xFFFF ? hi : _hi; }
    inline int dil1() const { return _dil1; };
    inline void setDil1(int dil1) { _dil1 = dil1 >= 0 ? dil1 : _dil1; }
    inline int dil2() const { return _dil2; };
    inline void setDil2(int dil2) { _dil2 = dil2 >= 0 ? dil2 : _dil2; }
    inline int ero1() const { return _ero1; };
    inline void setEro1(int ero1) { _ero1 = ero1 >= 0 ? ero1 : _ero1; }
    inline int ero2() const { return _ero2; };
    inline void setEro2(int ero2) { _ero2 = ero2 >= 0 ? ero2 : _ero2; }
    inline int minArea() const { return _minArea; };
    inline void setMinArea(int minArea) { _minArea = minArea >= 0 && minArea < _maxArea ? minArea : _minArea; }
    inline int maxArea() const { return _maxArea; };
    inline void setMaxArea(int maxArea) { _maxArea = maxArea >= _minArea ? maxArea : _maxArea; }
    inline float radius() const { return _radius; }
    inline void setRadius(float radius) { _radius = radius >= 0 ? radius : _radius; }
    inline float theta() const { return _theta; }
    inline void setTheta(float theta) { _theta = theta >= 0 ? theta : _theta; }

    // these are the images that are used during the processing...
    ofxCvGrayscaleImage* background() { return &_bg; }
    ofxCvGrayscaleImage* depthImage() { return &_cur; } // I have the bg mask applied on myself...
    ofxCvGrayscaleImage* depthImageWithoutBackground() { return &_sub; }
    ofxCvGrayscaleImage* blobs() { return &_blobs; }
    ofxCvGrayscaleImage* blacknwhiteImage() { return &_blacknwhite; }
    ofxCvGrayscaleImage* blacknwhiteImageWithoutBackground() { return &_blacknwhitewobg; }
    ofxCvColorImage* coloredImageWithoutBackground() { return &_coloredwobg; }
    ofxCvGrayscaleImage* binaryImageWithoutBackground() { return &_binarywobg; }

    // misc
    CalibratedKinect* kinect() const { return _kinect; }
    BackgroundModel* backgroundModel() const { return _bgmodel; }
    
private:
    CalibratedKinect* _kinect;
    BackgroundModel* _bgmodel;
    TouchClassifier* _classifier;

    int _lo, _hi; // cuboid (threshold)
    int _dil1, _dil2, _ero1, _ero2; // dilate & erode
    int _minArea, _maxArea; // blob filtering
    float _radius; // sphere radius (classification)
    float _theta; // theta (classification: elliptical boundary model)

    cvb::CvBlobs _cvb;

    IplImage* _tmp;
    IplImage* _tmp2;
    IplImage* _labelImg;

    ofxCvGrayscaleImage _bg;
    ofxCvGrayscaleImage _cur;
    ofxCvGrayscaleImage _sub;
    ofxCvGrayscaleImage _blobs;
    ofxCvGrayscaleImage _blacknwhite;
    ofxCvGrayscaleImage _blacknwhitewobg;
    ofxCvColorImage _coloredwobg;
    ofxCvGrayscaleImage _binarywobg;

    void _loadFromFile();
    void _saveToFile() const;
};

#endif
