#include <unistd.h>
#include "ofxOpenCv.h"
#include "ofxKinect.h"
#include "ofxOpenNI.h"
#include "ImageAccumulator.h"
#include "InputVideoStream.h"




FreenectInputVideoStream::FreenectInputVideoStream(bool infrared) : _infrared(infrared)
{
    _kinect = new ofxKinect();
}

FreenectInputVideoStream::~FreenectInputVideoStream()
{
    delete _kinect;
}

void FreenectInputVideoStream::setup()
{
    _kinect->setRegistration(true);
    _kinect->init(_infrared);
    _kinect->open();
}

void FreenectInputVideoStream::update()
{
    _kinect->update();
}

bool FreenectInputVideoStream::gotNewFrame()
{
    return _kinect->isFrameNew();
}

const unsigned char* FreenectInputVideoStream::pixels()
{
    return _kinect->getPixels();
}

const unsigned short* FreenectInputVideoStream::depthPixels()
{
    return _kinect->getRawDepthPixels();
}

int FreenectInputVideoStream::width()
{
    return _kinect->width;
}

int FreenectInputVideoStream::height()
{
    return _kinect->height;
}

void FreenectInputVideoStream::setCameraTiltAngle(float angle)
{
    angle = std::max(-30.0f, std::min(30.0f, angle));
    _kinect->setCameraTiltAngle(angle);
}

float FreenectInputVideoStream::cameraTiltAngle() const
{
    return _kinect->getCurrentCameraTiltAngle();
}









OpenNIInputVideoStream::OpenNIInputVideoStream()
{
    _openni = new ofxOpenNI();
}

OpenNIInputVideoStream::~OpenNIInputVideoStream()
{
    std::cout << "deleting _openni ..." << std::endl;
    _openni->disableCalibratedRGBDepth();
    delete _openni;
}

void OpenNIInputVideoStream::setup()
{
    while(!_openni->setupFromXML("openni/config/ofxopenni_config.xml", false)) {
        std::cout << "Can't _openni->setupFromXML()... Trying again in " << std::flush;
        for(int i=15; i>0; i--) {
            std::cout << i << " " << std::flush;
            usleep(1000000);
        }
        std::cout << std::endl;
    }

    _openni->enableCalibratedRGBDepth();
}

void OpenNIInputVideoStream::update()
{
    _openni->update();
}

bool OpenNIInputVideoStream::gotNewFrame()
{
    return _openni->isNewFrame();
}

const unsigned char* OpenNIInputVideoStream::pixels()
{
    return _openni->getRGBPixels().getPixels();
}

const unsigned short* OpenNIInputVideoStream::depthPixels()
{
    return _openni->getDepthRawPixels().getPixels();
}

int OpenNIInputVideoStream::width()
{
    return _openni->getWidth();
}

int OpenNIInputVideoStream::height()
{
    return _openni->getHeight();
}

void OpenNIInputVideoStream::setCameraTiltAngle(float angle)
{
    std::cerr << "[error] OpenNIInputVideoStream::setCameraTiltAngle() is not supported!" << std::endl;
}

float OpenNIInputVideoStream::cameraTiltAngle() const
{
    std::cerr << "[error] OpenNIInputVideoStream::cameraTiltAngle() is not supported!" << std::endl;
    return 0.0f;
}









StoredInputVideoStream::StoredInputVideoStream(ImageAccumulator* colorSource, ImageAccumulator* depthSource)
{
    _colorSource = colorSource;
    _depthSource = depthSource;
}

StoredInputVideoStream::StoredInputVideoStream(std::string videoSequenceName)
{
    _colorSource = new ImageAccumulator(videoSequenceName + "_rgb");
    _depthSource = new ImageAccumulator(videoSequenceName + "_d");
}

StoredInputVideoStream::~StoredInputVideoStream()
{
    delete _colorSource;
    delete _depthSource;
}

void StoredInputVideoStream::setup()
{
    _colorSource->rewind();
    _depthSource->rewind();

    if(_colorSource->count() == 0 || _depthSource->count() == 0) {
        std::cerr << "invalid video stream." << std::endl;
        std::exit(1);
    }
}

void StoredInputVideoStream::update()
{
    _colorSource->next();
    _depthSource->next();
}

bool StoredInputVideoStream::gotNewFrame()
{
    return pixels() && depthPixels();
}

const unsigned char* StoredInputVideoStream::pixels()
{
    const IplImage* img = _colorSource->current();
    return img && img->nChannels == 3 ? (unsigned char*)(img->imageData) : 0;
}

const unsigned short* StoredInputVideoStream::depthPixels()
{
    const IplImage* img = _depthSource->current();
    return img && img->nChannels == 1 ? (unsigned short*)(img->imageData) : 0;
}

int StoredInputVideoStream::width()
{
    const IplImage* img = _colorSource->current();
    return img ? img->width : 0;
}

int StoredInputVideoStream::height()
{
    const IplImage* img = _colorSource->current();
    return img ? img->height : 0;
}







WebcamInputVideoStream::WebcamInputVideoStream() : _capture(0), _colorImage(0), _depthImage(0)
{
//    _capture = cvCreateCameraCapture(-1);
    _capture = cvCaptureFromCAM(CV_CAP_ANY);
    if(!_capture) {
        std::cerr << "WebcamInputVideoStream: can't create camera capture!" << std::endl;
        std::exit(1);
    }

    _size = cvSize(
        (int)cvGetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH),
        (int)cvGetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT)
    );
    std::cerr << "WebcamInputVideoStream: recording (" << _size.width << "x" << _size.height << ")..." << std::endl;
}

WebcamInputVideoStream::~WebcamInputVideoStream()
{
    cvReleaseImage(&_depthImage);
    cvReleaseCapture(&_capture);
}

void WebcamInputVideoStream::setup()
{
    _colorImage = cvQueryFrame(_capture);
    cvCvtColor(_colorImage, _colorImage, CV_BGR2RGB);
    _depthImage = cvCreateImage(cvGetSize(_colorImage), IPL_DEPTH_16U, 1);
    cvZero(_depthImage);
}

void WebcamInputVideoStream::update()
{
    IplImage *frame = cvRetrieveFrame(_capture);
    if(frame != NULL) {
        _colorImage = frame;
        cvCvtColor(_colorImage, _colorImage, CV_BGR2RGB);
    }
}

bool WebcamInputVideoStream::gotNewFrame()
{
    return cvGrabFrame(_capture) != 0;
}

const unsigned char* WebcamInputVideoStream::pixels()
{
    return (unsigned char*)_colorImage->imageData;
}

const unsigned short* WebcamInputVideoStream::depthPixels()
{
    return (unsigned short*)_depthImage->imageData;
}

int WebcamInputVideoStream::width()
{
    return _size.width;
}

int WebcamInputVideoStream::height()
{
    return _size.height;
}







NullInputVideoStream::NullInputVideoStream()
{
    _pixels = new unsigned char[3 * width() * height()];
    _depthPixels = new unsigned short[width() * height()];
}

NullInputVideoStream::~NullInputVideoStream()
{
    if(_pixels)
        delete[] _pixels;

    if(_depthPixels)
        delete[] _depthPixels;
}

void NullInputVideoStream::setup()
{
    std::fill(_pixels, _pixels + 3 * width() * height(), 0);
    std::fill(_depthPixels, _depthPixels + width() * height(), 0);
}

void NullInputVideoStream::update()
{
    ;
}

bool NullInputVideoStream::gotNewFrame()
{
    return true;
}

const unsigned char* NullInputVideoStream::pixels()
{
    return _pixels;
}

const unsigned short* NullInputVideoStream::depthPixels()
{
    return _depthPixels;
}

int NullInputVideoStream::width()
{
    return 640;
}

int NullInputVideoStream::height()
{
    return 480;
}


