#ifndef _ERASERDETECTOR_H
#define _ERASERDETECTOR_H

#include <vector>
#include <string>
#include "ofxOpenCv.h"
#include "cvblob.h"

class BackgroundModel;
class CalibratedKinect;



class EraserDetector
{
public:
    struct Eraser
    {
        int x, y; // position (in depth image coordinates)
        int area; // the area
        float surfaceX, surfaceY; // values between 0 and 1, inclusive
        float coefficient; // circularity (in [0, 1])

        int startX, startY; // initial position
        float surfaceStartX, surfaceStartY; // initial position (in surface coordinates)
        float speedX, speedY; // speed in pixels per second
        float surfaceSpeedX, surfaceSpeedY; // speed (surface coords)

        // internal
        unsigned int _id;
        int _timeToLive;
    };

    EraserDetector(CalibratedKinect* kinect, BackgroundModel* bgmodel = 0);
    ~EraserDetector();

    // detectionnnn
    void update();
    std::vector<Eraser> objects();

    // parameters
    inline int lo() const { return _lo; };
    inline void setLo(int lo) { _lo = lo >= 0 && lo < _hi ? lo : _lo; }
    inline int hi() const { return _hi; };
    inline void setHi(int hi) { _hi = hi >= _lo && hi <= 0xFFFF ? hi : _hi; }
    inline int minArea() const { return _minArea; };
    inline void setMinArea(int minArea) { _minArea = minArea >= 0 && minArea < _maxArea ? minArea : _minArea; }
    inline int maxArea() const { return _maxArea; };
    inline void setMaxArea(int maxArea) { _maxArea = maxArea >= _minArea ? maxArea : _maxArea; }
    inline int ero() const { return _ero; }
    inline void setEro(int ero) { _ero = ero >= 0 ? ero : _ero; }
    inline int dil() const { return _dil; }
    inline void setDil(int dil) { _dil = dil >= 0 ? dil : _dil; }
    inline float classificationCoefficient() const { return _coefficient; }
    inline void setClassificationCoefficient(float coefficient) { _coefficient = coefficient; }

    // load/save from/to disk
    inline void load() { _loadFromFile(); }
    inline void save() const { _saveToFile(); }

    // these are the images that are used during the processing...
    ofxCvGrayscaleImage* blobs() { return &_blobs; }

    // misc
    CalibratedKinect* kinect() const { return _kinect; }
    BackgroundModel* backgroundModel() const { return _bgmodel; }
    
private:
    CalibratedKinect* _kinect;
    BackgroundModel* _bgmodel;

    int _lo, _hi; // cuboid (threshold)
    int _minArea, _maxArea; // blob filtering
    int _ero, _dil; // morphology
    float _coefficient; // classification

    cvb::CvBlobs _cvb;

    IplImage* _tmp;
    IplImage* _tmp2;
    IplImage* _labelImg;

    ofxCvGrayscaleImage _blobs;

    void _loadFromFile();
    void _saveToFile() const;

    double _computeCircularityCoefficient(const Eraser& e);
};

#endif
