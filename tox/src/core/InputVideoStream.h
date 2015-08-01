#ifndef _INPUTVIDEOSTREAM_H
#define _INPUTVIDEOSTREAM_H

#include "ofxOpenCv.h"
#include <opencv/highgui.h>

// forward decls
class ofxKinect;
class ofxOpenNI;
class ImageAccumulator;

// video recorder
class InputVideoStream
{
public:
    InputVideoStream() { };
    virtual ~InputVideoStream() { };

    virtual void setup() = 0;
    virtual void update() = 0;
    virtual bool gotNewFrame() = 0;

    virtual const unsigned char* pixels() = 0; // 8 bits per pixel, 3 channels
    virtual const unsigned short* depthPixels() = 0; // 11 bits per pixel, 1 channel
    virtual int width() = 0;
    virtual int height() = 0;

    virtual void setCameraTiltAngle(float angle) { } // angle in degrees, from -30 to 30 (0 is flat)
    virtual float cameraTiltAngle() const { return 0.0f; }

private:
};

// null video stream
class NullInputVideoStream : public InputVideoStream
{
public:
    NullInputVideoStream();
    virtual ~NullInputVideoStream();

    virtual void setup();
    virtual void update();
    virtual bool gotNewFrame();

    virtual const unsigned char* pixels();
    virtual const unsigned short* depthPixels();
    virtual int width();
    virtual int height();

private:
    unsigned char* _pixels;
    unsigned short* _depthPixels;
};

// kinect with libfreenect
class FreenectInputVideoStream : public InputVideoStream
{
public:
    FreenectInputVideoStream(bool infrared = false);
    virtual ~FreenectInputVideoStream();

    virtual void setup();
    virtual void update();
    virtual bool gotNewFrame();

    virtual const unsigned char* pixels();
    virtual const unsigned short* depthPixels();
    virtual int width();
    virtual int height();

    virtual void setCameraTiltAngle(float angle);
    virtual float cameraTiltAngle() const;

private:
    ofxKinect* _kinect;
    bool _infrared;
};

// kinect with openni
class OpenNIInputVideoStream : public InputVideoStream
{
public:
    OpenNIInputVideoStream();
    virtual ~OpenNIInputVideoStream();

    virtual void setup();
    virtual void update();
    virtual bool gotNewFrame();

    virtual const unsigned char* pixels();
    virtual const unsigned short* depthPixels();
    virtual int width();
    virtual int height();

    virtual void setCameraTiltAngle(float angle);
    virtual float cameraTiltAngle() const;

private:
    ofxOpenNI* _openni;
};

// read video from disk
class StoredInputVideoStream : public InputVideoStream
{
public:
    StoredInputVideoStream(ImageAccumulator* colorSource, ImageAccumulator* depthSource);
    StoredInputVideoStream(std::string videoSequenceName);
    virtual ~StoredInputVideoStream();

    virtual void setup();
    virtual void update();
    virtual bool gotNewFrame();

    virtual const unsigned char* pixels();
    virtual const unsigned short* depthPixels();
    virtual int width();
    virtual int height();

private:
    ImageAccumulator* _colorSource;
    ImageAccumulator* _depthSource;
};

// regular webcam
class WebcamInputVideoStream : public InputVideoStream
{
public:
    WebcamInputVideoStream();
    virtual ~WebcamInputVideoStream();

    virtual void setup();
    virtual void update();
    virtual bool gotNewFrame();

    virtual const unsigned char* pixels();
    virtual const unsigned short* depthPixels();
    virtual int width();
    virtual int height();

private:
    CvCapture* _capture;
    CvSize _size;
    IplImage* _colorImage;
    IplImage* _depthImage;
};

#endif
