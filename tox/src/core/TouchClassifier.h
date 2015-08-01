#ifndef _TOUCHCLASSIFIER_H
#define _TOUCHCLASSIFIER_H

#include <string>
#include <vector>
#include "ofMain.h"
#include "ofxOpenCv.h"

class TouchDetector;
class ColorClassifier;

class TouchClassifier
{
public:
    enum TouchClassifierQuality { QUALITY_BEST, QUALITY_FAST, QUALITY_FASTEST };
    TouchClassifier(TouchDetector* touchDetector, TouchClassifierQuality quality = QUALITY_BEST);
    ~TouchClassifier();

    void update();

    std::string classify(ofVec2f touchPosition, double sphereRadiusInMeters = 0.01, double theta = 10.0); // theta ::= elliptical boundary model (color classifier)

private:
    TouchDetector* _touchDetector;
    ColorClassifier* _colorClassifier;
    IplImage* _transformedColorImage;
    TouchClassifierQuality _quality;
    
    std::vector<int> _coloredPixelsInsideTheSphere(ofVec3f center, double radius);
    std::string _classifyPixels(const std::vector<int>& s, double theta);
    void _markPixel(int x, int y);
    int _quality2angleincr(TouchClassifierQuality quality);

    double _sin[360], _cos[360];
};

#endif
