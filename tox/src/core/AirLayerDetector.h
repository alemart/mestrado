#ifndef _AIRLAYERDETECTOR_H
#define _AIRLAYERDETECTOR_H

#include <vector>
#include <string>
#include "ofxOpenCv.h"
#include "cvblob.h"

class BackgroundModel;
class CalibratedKinect;



class AirLayerDetector
{
public:
    struct Object
    {
        int x, y; // position (in depth image coordinates)
        int area; // the area
        float surfaceX, surfaceY; // values between 0 and 1, inclusive
        std::string type; // big, small (depending on the area)
    };

    AirLayerDetector(CalibratedKinect* kinect, BackgroundModel* bgmodel, int lo, int hi, int minArea, int maxArea, int classificationThreshold);
    ~AirLayerDetector();

    // detectionnnn
    void update();
    std::vector<Object> objects() const;

    // parameters
    inline int lo() const { return _lo; };
    inline void setLo(int lo) { _lo = lo >= 0 && lo < _hi ? lo : _lo; }
    inline int hi() const { return _hi; };
    inline void setHi(int hi) { _hi = hi >= _lo && hi <= 0xFFFF ? hi : _hi; }
    inline int minArea() const { return _minArea; };
    inline void setMinArea(int minArea) { _minArea = minArea >= 0 && minArea < _maxArea ? minArea : _minArea; }
    inline int maxArea() const { return _maxArea; };
    inline void setMaxArea(int maxArea) { _maxArea = maxArea >= _minArea ? maxArea : _maxArea; }
    inline int classificationThreshold() const { return _threshold; }
    inline void setClassificationThreshold(int threshold) { _threshold = /*!(threshold >= _lo && threshold <= _hi) ? _threshold :*/ threshold; }

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
    int _threshold; // classification (big, small)

    cvb::CvBlobs _cvb;

    IplImage* _tmp;
    IplImage* _tmp2;
    IplImage* _labelImg;

    ofxCvGrayscaleImage _blobs;
};

#endif
