#ifndef _CALIBRATEDKINECT_H
#define _CALIBRATEDKINECT_H

#include "ofMain.h"
#include "ofxOpenCv.h"
class InputVideoStream;

class CalibratedKinect
{
public:
    CalibratedKinect(InputVideoStream* videoStream);
    virtual ~CalibratedKinect();

    void setup();
    void update(); // this should be called every frame

    ofxCvImage* colorImage() const; // 8 bpp, 3 channels
    ofxCvImage* grayscaleDepthImage() const; // 8 bpp, 1 channel
    const IplImage* rawDepthImage() const; // 11 bpp, 1 channel

    void toggleInfrared();
    bool isUsingInfrared() const; // is infrared cam being displayed in colorImage() ?

    double rawDepth2meters(unsigned short rawDepth) const;
    unsigned short rawDepthAt(int x, int y) const;
    unsigned short rawDepthAt(ofVec2f v) const;
    ofColor colorAt(int x, int y) const;
    ofColor colorAt(ofVec2f v) const;
    ofVec3f depthCoords2worldCoords(float x, float y, unsigned short rawDepthAtXY) const;
    ofVec3f depthCoords2worldCoords(ofVec2f depthCoords) const;
    ofVec2f worldCoords2colorCoords(ofVec3f worldCoords) const;
    ofVec2f depthCoords2colorCoords(ofVec2f depthCoords) const; // fast shortcut (uses a lookup table)
    ofVec2f worldCoords2depthCoords(ofVec3f worldCoords) const;

    const IplImage* undistortedColorImage() const;
    const IplImage* undistortedRawDepthImage() const;

    virtual void setTiltAngle(float angle); // angle in degrees, from -30 to 30 (0 is flat)
    virtual float tiltAngle() const;

private:
    InputVideoStream* _in;
    bool _infrared;

    ofxCvImage* _colorImage;
    ofxCvImage* _grayscaleDepthImage;
    IplImage* _rawDepthImage;

    IplImage* _mapxRGB;    // these
    IplImage* _mapyRGB;    // matrices
    IplImage* _mapxIR;     // may
    IplImage* _mapyIR;     // be NULL !
    CvMat* _intrinsicsRGB; // camera
    CvMat* _intrinsicsIR;  // calibration
    CvMat* _distortionRGB;
    CvMat* _distortionIR;

    CvMat* _depthModel; // 2x1 (alpha, beta)
    CvMat* _rotation; // 3x3, depth -- to --> color
    CvMat* _translation; // 3x1

    IplImage* _undistortedColorImage;
    IplImage* _undistortedRawDepthImage;

    unsigned char _depthLookup[2048];
    double _rawDepth2metersLookup[2048];
    short _depth2colorLookup[480][640][2];

    IplImage* _oldColorImage;
    IplImage* _depthSmoothingIn;
    IplImage* _depthSmoothingOut;

    void _depthSmoothing(IplImage* in, IplImage* out, int innerBandThreshold, int outerBandThreshold, int numberOfThreads);
};

#endif
