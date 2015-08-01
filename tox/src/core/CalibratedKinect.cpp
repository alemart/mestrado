#include <thread>
#include <algorithm>
#include <iostream>
#include "CalibratedKinect.h"
#include "InputVideoStream.h"

// depth smoothing stuff
#define USE_DEPTHSMOOTHING // comment to disable
#define DEPTHSMOOTHING_NUMTHREADS 24
#define DEPTHSMOOTHING_INNERBAND_THRESHOLD 2
#define DEPTHSMOOTHING_OUTERBAND_THRESHOLD 2




CalibratedKinect::CalibratedKinect(InputVideoStream* videoStream) : _in(videoStream), _infrared(false), _colorImage(0), _grayscaleDepthImage(0), _rawDepthImage(0), _mapxRGB(0), _mapyRGB(0), _mapxIR(0), _mapyIR(0), _intrinsicsRGB(0), _intrinsicsIR(0), _distortionRGB(0), _distortionIR(0), _depthModel(0), _rotation(0), _translation(0), _undistortedColorImage(0), _undistortedRawDepthImage(0), _oldColorImage(0), _depthSmoothingIn(0), _depthSmoothingOut(0)
{
}

CalibratedKinect::~CalibratedKinect()
{
    delete _colorImage;
    delete _grayscaleDepthImage;
    cvReleaseImage(&_rawDepthImage);

    cvReleaseMat(&_depthModel);
    cvReleaseMat(&_rotation);
    cvReleaseMat(&_translation);

    if(_mapxRGB)
        cvReleaseImage(&_mapxRGB);

    if(_mapyRGB)
        cvReleaseImage(&_mapyRGB);

    if(_mapxIR)
        cvReleaseImage(&_mapxIR);

    if(_mapyIR)
        cvReleaseImage(&_mapyIR);

    if(_intrinsicsRGB)
        cvReleaseMat(&_intrinsicsRGB);

    if(_intrinsicsIR)
        cvReleaseMat(&_intrinsicsIR);

    if(_distortionRGB)
        cvReleaseMat(&_distortionRGB);

    if(_distortionIR)
        cvReleaseMat(&_distortionIR);

    cvReleaseImage(&_undistortedColorImage);
    cvReleaseImage(&_undistortedRawDepthImage);
    cvReleaseImage(&_oldColorImage);
    cvReleaseImage(&_depthSmoothingIn);
    cvReleaseImage(&_depthSmoothingOut);

    delete _in;
}

void CalibratedKinect::setup()
{
    // setup video stream
    _in->setup();
    int w = _in->width();
    int h = _in->height();

    // allocate images
    _colorImage = new ofxCvColorImage;
    _grayscaleDepthImage = new ofxCvGrayscaleImage;
    _rawDepthImage = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _undistortedColorImage = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 3);
    _undistortedRawDepthImage = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _oldColorImage = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 3);
    _depthSmoothingIn = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_16U, 1);
    _depthSmoothingOut = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_16U, 1);

    if(!_colorImage || !_grayscaleDepthImage || !_rawDepthImage || !_undistortedColorImage || !_undistortedRawDepthImage || !_oldColorImage || !_depthSmoothingIn || !_depthSmoothingOut) {
        std::cerr << "CalibratedKinect: can't allocate images" << std::endl;
        std::exit(1);
    }

    _colorImage->allocate(w, h);
    _grayscaleDepthImage->allocate(w, h);
    cvZero(_rawDepthImage);
    cvZero(_undistortedColorImage);
    cvZero(_undistortedRawDepthImage);

    // will load calibration...
    _mapxRGB = _mapyRGB = _mapxIR = _mapyIR = 0;
    _intrinsicsRGB = _intrinsicsIR = 0;
    _depthModel = _rotation = _translation = 0;

    // loading calibration: depth model, rotation & translation
    if(!(_depthModel = (CvMat*)cvLoad("data/camcalib/depth_model.yaml"))) {
        std::cerr << "CalibratedKinect: can't find the 2x1 depth model vector." << std::endl;
        std::exit(1);
    }

    if(!(_rotation = (CvMat*)cvLoad("data/camcalib/rotation.yaml"))) {
        std::cerr << "CalibratedKinect: can't find the 3x3 rotation matrix." << std::endl;
        std::exit(1);
    }

    if(!(_translation = (CvMat*)cvLoad("data/camcalib/translation.yaml"))) {
        std::cerr << "CalibratedKinect: can't find the 3x1 translation vector." << std::endl;
        std::exit(1);
    }

    // loading calibration: intrinsics & distortion
    _intrinsicsRGB = (CvMat*)cvLoad("data/camcalib/intrinsics_rgb.yaml");
    if(_intrinsicsRGB) {
        _distortionRGB = (CvMat*)cvLoad("data/camcalib/distortion_rgb.yaml");
        if(_distortionRGB) {
            std::cerr << "CalibratedKinect: I've found the RGB camera calibration!" << std::endl;
            _mapxRGB = cvCreateImage(cvGetSize(_colorImage->getCvImage()), IPL_DEPTH_32F, 1);
            _mapyRGB = cvCreateImage(cvGetSize(_colorImage->getCvImage()), IPL_DEPTH_32F, 1);
            cvInitUndistortMap(_intrinsicsRGB, _distortionRGB, _mapxRGB, _mapyRGB);
        }
    }

    _intrinsicsIR = (CvMat*)cvLoad("data/camcalib/intrinsics_ir.yaml");
    if(_intrinsicsIR) {
        _distortionIR = (CvMat*)cvLoad("data/camcalib/distortion_ir.yaml");
        if(_distortionIR) {
            std::cerr << "CalibratedKinect: I've found the IR camera calibration!" << std::endl;
            _mapxIR = cvCreateImage(cvGetSize(_rawDepthImage), IPL_DEPTH_32F, 1);
            _mapyIR = cvCreateImage(cvGetSize(_rawDepthImage), IPL_DEPTH_32F, 1);
            cvInitUndistortMap(_intrinsicsIR, _distortionIR, _mapxIR, _mapyIR);
        }
    }

    // depth lookup
    int n = sizeof(_depthLookup) / sizeof(unsigned char);
    int nearClip = 400; // in mm
    int farClip = 1500;
    int t;

    for(int i=0; i<n; i++) {
        t = (255 * (i - nearClip)) / (farClip - nearClip);
        if(t < 0)
            t = 0;
        else if(t > 255)
            t = 255;
        _depthLookup[i] = t;
    }

    // rawDepth2meters lookup
    if(1) {
        int n = sizeof(_rawDepth2metersLookup) / sizeof(double);
        double alpha = CV_MAT_ELEM(*_depthModel, float, 0, 0);
        double beta = CV_MAT_ELEM(*_depthModel, float, 1, 0);
        for(int i=0; i<n; i++) {
            _rawDepth2metersLookup[i] = 1.0 / (alpha * double(i) + beta);
            //_rawDepth2metersLookup[i] = 0.1236 * tan(double(i) / 2842.5 + 1.1863);
        }
    }

    // depth2color lookup
    if(1) {
        ofVec2f v;
        for(int y=0; y<480; y++) {
            for(int x=0; x<640; x++) {
                v = worldCoords2colorCoords(
                        depthCoords2worldCoords(
                            ofVec2f(x, y)
                        )
                    );
                _depth2colorLookup[y][x][0] = (short)(v.x + 0.5f);
                _depth2colorLookup[y][x][1] = (short)(v.y + 0.5f);
            }
        }
    }
}



void CalibratedKinect::update()
{
    _in->update();

    if(_in->gotNewFrame()) {
        int w = _in->width();
        int h = _in->height();

        // retrieve color image
        if(_infrared) {
            // convert infrared image to RGB
            IplImage* out = _colorImage->getCvImage();
            const unsigned char* in = _in->pixels();
            for(int y=0; y<h; y++) {
                unsigned char *pc = (unsigned char*)(out->imageData + y * out->widthStep);
                const unsigned char *pg = in + y * w;
                for(int x=0; x<w; x++) {
                    pc[3*x + 0] = pg[x];
                    pc[3*x + 1] = pg[x];
                    pc[3*x + 2] = pg[x];
                }
            }
            _colorImage->flagImageChanged();
        }
        else
            _colorImage->setFromPixels(_in->pixels(), w, h);

        // retrieve depth image
        const unsigned short *in = _in->depthPixels();
        IplImage* grayscaleDepthImage = _grayscaleDepthImage->getCvImage();
        for(int y=0; y<h; y++) {
            const unsigned short *src = in + y * w;
            unsigned short *dst = (unsigned short*)(_rawDepthImage->imageData + y * _rawDepthImage->widthStep);
            unsigned char *dst2 = (unsigned char*)(grayscaleDepthImage->imageData + y * grayscaleDepthImage->widthStep);
            for(int x=0; x<w; x++) {
                dst[x] = src[x] & 0x7FF; // 11 bpp
                dst2[x] = _depthLookup[dst[x]]; // FIXME
            }
        }

        // FIXME: use if kinect->setRegistration(true)
#ifdef USE_DEPTHSMOOTHING
        auto interp = cv::INTER_NEAREST;
        cvResize(_rawDepthImage, _depthSmoothingIn, interp);
        _depthSmoothing(_depthSmoothingIn, _depthSmoothingOut, DEPTHSMOOTHING_INNERBAND_THRESHOLD, DEPTHSMOOTHING_OUTERBAND_THRESHOLD, DEPTHSMOOTHING_NUMTHREADS);
        cvResize(_depthSmoothingOut, _rawDepthImage, interp);
        //cvDilate(_rawDepthImage, _rawDepthImage, NULL, 1);
        //cvErode(grayscaleDepthImage, grayscaleDepthImage, NULL, 1);
#endif

        // we modified its cvImage() directly
        _grayscaleDepthImage->flagImageChanged();

        // getting undistorted images
        cvCopy(_colorImage->getCvImage(), _undistortedColorImage);
        cvCopy(_rawDepthImage, _undistortedRawDepthImage);

        // undistort color image
        if(!_infrared) {
            if(_mapxRGB && _mapyRGB) {
                IplImage* ic = cvCloneImage(_colorImage->getCvImage());
                cvRemap(ic, _colorImage->getCvImage(), _mapxRGB, _mapyRGB);
                cvReleaseImage(&ic);
            }
        }
        else {
            if(_mapxIR && _mapyIR) {
                IplImage* ic = cvCloneImage(_colorImage->getCvImage());
                cvRemap(ic, _colorImage->getCvImage(), _mapxIR, _mapyIR);
                cvReleaseImage(&ic);
            }
        }

        // undistort depth image
#if 1
        if(_mapxIR && _mapyIR) {
            IplImage* id = cvCloneImage(_grayscaleDepthImage->getCvImage());
            IplImage* ir = cvCloneImage(_rawDepthImage);
            cvRemap(id, _grayscaleDepthImage->getCvImage(), _mapxIR, _mapyIR);
            cvRemap(ir, _rawDepthImage, _mapxIR, _mapyIR);
            cvReleaseImage(&ir);
            cvReleaseImage(&id);
        }
#endif

        // backup (in case the scene needs to paint the color image...)
        cvCopy(_colorImage->getCvImage(), _oldColorImage, NULL);
    }
    else {
        cvCopy(_oldColorImage, _colorImage->getCvImage(), NULL);
        _colorImage->flagImageChanged();
    }
}








ofxCvImage* CalibratedKinect::colorImage() const
{
    return _colorImage;
}

ofxCvImage* CalibratedKinect::grayscaleDepthImage() const
{
    return _grayscaleDepthImage;
}

const IplImage* CalibratedKinect::rawDepthImage() const
{
    return _rawDepthImage;
}










void CalibratedKinect::toggleInfrared()
{
    FreenectInputVideoStream* k = dynamic_cast<FreenectInputVideoStream*>(_in);
    if(k) { // gambiarra
        _infrared = !_infrared;
        delete _in;
        _in = new FreenectInputVideoStream(_infrared);
        _in->setup();
        _in->update();
    }
}

bool CalibratedKinect::isUsingInfrared() const
{
    return _infrared;
}

void CalibratedKinect::setTiltAngle(float angle)
{
    _in->setCameraTiltAngle(angle);
}

float CalibratedKinect::tiltAngle() const
{
    return _in->cameraTiltAngle();
}
















unsigned short CalibratedKinect::rawDepthAt(int x, int y) const
{
    if(x >= 0 && x < _rawDepthImage->width && y >= 0 && y < _rawDepthImage->height) {
        unsigned short* ptr = (unsigned short*)(_rawDepthImage->imageData + y * _rawDepthImage->widthStep);
        return ptr[x];
    }
    else
        return 0;
}

unsigned short CalibratedKinect::rawDepthAt(ofVec2f v) const
{
    return rawDepthAt(v.x, v.y);
}

ofColor CalibratedKinect::colorAt(int x, int y) const
{
    const IplImage* c = colorImage()->getCvImage();
    if(x >= 0 && x < c->width && y >= 0 && y < c->height) {
        unsigned char* ptr = (unsigned char*)(c->imageData + y * c->widthStep);
        return ofColor(ptr[3*x+0], ptr[3*x+1], ptr[3*x+2]);
    }
    else
        return ofColor(0, 0, 0);
}

ofColor CalibratedKinect::colorAt(ofVec2f v) const
{
    return colorAt(v.x, v.y);
}

double CalibratedKinect::rawDepth2meters(unsigned short rawDepth) const
{
    if(rawDepth < 2047)
        return _rawDepth2metersLookup[rawDepth];
    else
        return 0.0;
}

ofVec3f CalibratedKinect::depthCoords2worldCoords(float x, float y, unsigned short rawDepthAtXY) const
{
    if(_intrinsicsIR) {
        float fx = CV_MAT_ELEM(*_intrinsicsIR, float, 0, 0);
        float fy = CV_MAT_ELEM(*_intrinsicsIR, float, 1, 1);
        float cx = CV_MAT_ELEM(*_intrinsicsIR, float, 0, 2);
        float cy = CV_MAT_ELEM(*_intrinsicsIR, float, 1, 2);

        double z = rawDepth2meters(rawDepthAtXY);
        y = (y - cy) * z / fy;
        x = (x - cx) * z / fx;

        return ofVec3f(x, y, z);
    }
    else {
        std::cerr << "CalibratedKinect - can't convert from depth to world coords: no intrinsic IR matrix!" << std::endl;
        return ofVec3f(0, 0, 0);
    }
}

ofVec3f CalibratedKinect::depthCoords2worldCoords(ofVec2f depthCoords) const
{
    return depthCoords2worldCoords(depthCoords.x, depthCoords.y, rawDepthAt(depthCoords));
}

ofVec2f CalibratedKinect::worldCoords2colorCoords(ofVec3f worldCoords) const
{
    if(_intrinsicsRGB) {
        CvMat* theAdjustedWorldCoords = cvCreateMat(3, 1, CV_32FC1);
        CvMat* theWorldCoords = cvCreateMat(3, 1, CV_32FC1);
        ofVec3f adjustedWorldCoords;

        CV_MAT_ELEM(*theWorldCoords, float, 0, 0) = worldCoords.x;
        CV_MAT_ELEM(*theWorldCoords, float, 1, 0) = worldCoords.y;
        CV_MAT_ELEM(*theWorldCoords, float, 2, 0) = worldCoords.z;

        cvGEMM(_rotation, theWorldCoords, 1.0, _translation, 1.0, theAdjustedWorldCoords, 0); // adjWorld = R world + T

        adjustedWorldCoords.x = CV_MAT_ELEM(*theAdjustedWorldCoords, float, 0, 0);
        adjustedWorldCoords.y = CV_MAT_ELEM(*theAdjustedWorldCoords, float, 1, 0);
        adjustedWorldCoords.z = CV_MAT_ELEM(*theAdjustedWorldCoords, float, 2, 0);

        cvReleaseMat(&theWorldCoords);
        cvReleaseMat(&theAdjustedWorldCoords);

        if(fabs(adjustedWorldCoords.z) >= 1e-5) {
            float fx = CV_MAT_ELEM(*_intrinsicsRGB, float, 0, 0);
            float fy = CV_MAT_ELEM(*_intrinsicsRGB, float, 1, 1);
            float cx = CV_MAT_ELEM(*_intrinsicsRGB, float, 0, 2);
            float cy = CV_MAT_ELEM(*_intrinsicsRGB, float, 1, 2);

            float x = (adjustedWorldCoords.x * fx / adjustedWorldCoords.z) + cx;
            float y = (adjustedWorldCoords.y * fy / adjustedWorldCoords.z) + cy;

            return ofVec2f(x, y);
        }
        else {
            std::cerr << "CalibratedKinect - can't convert from world to color coords: adjustedWorldCoords.z is too small!" << std::endl;
            return ofVec2f(0, 0);
        }
    }
    else {
        std::cerr << "CalibratedKinect - can't convert from world to color coords: no intrinsic RGB matrix!" << std::endl;
        return ofVec2f(0, 0);
    }
}

ofVec2f CalibratedKinect::depthCoords2colorCoords(ofVec2f depthCoords) const
{
    if(depthCoords.x >= 0 && depthCoords.x < (640-1) && depthCoords.y >= 0 && depthCoords.y < (480-1)) {
        int x = (int)(depthCoords.x + 0.5f);
        int y = (int)(depthCoords.y + 0.5f);
        // we have 0 <= x <= 639 and 0 <= y <= 479... therefore...
        return ofVec2f(_depth2colorLookup[y][x][0], _depth2colorLookup[y][x][1]);
    }
    else {
        return worldCoords2colorCoords(
                    depthCoords2worldCoords(
                        depthCoords
                    )
                );
    }
}

ofVec2f CalibratedKinect::worldCoords2depthCoords(ofVec3f worldCoords) const
{
    if(_intrinsicsIR) {
        float fx = CV_MAT_ELEM(*_intrinsicsIR, float, 0, 0);
        float fy = CV_MAT_ELEM(*_intrinsicsIR, float, 1, 1);
        float cx = CV_MAT_ELEM(*_intrinsicsIR, float, 0, 2);
        float cy = CV_MAT_ELEM(*_intrinsicsIR, float, 1, 2);

        float x = (worldCoords.x * fx / worldCoords.z) + cx;
        float y = (worldCoords.y * fy / worldCoords.z) + cy;

        return ofVec2f(x, y);
    }
    else {
        std::cerr << "CalibratedKinect - can't convert from world to depth coords: no intrinsic IR matrix!" << std::endl;
        return ofVec2f(0, 0);
    }
}















//
// undistorted images
//
const IplImage* CalibratedKinect::undistortedColorImage() const
{
    return _undistortedColorImage;
}

const IplImage* CalibratedKinect::undistortedRawDepthImage() const
{
    return _undistortedRawDepthImage;
}




//
// depth smoothing
// http://www.codeproject.com/Articles/317974/KinectDepthSmoothing
//
void CalibratedKinect::_depthSmoothing(IplImage* in, IplImage* out, int innerBandThreshold, int outerBandThreshold, int numberOfThreads)
{
    int w = in->width;
    int h = in->height;

    auto filterLines = [&] (int yBegin, int yEnd) {
        int x, y, xi, yi, xs, ys, j;
        unsigned short raw, *ptr, depth, frequency;
        for(y=yBegin; y<yEnd; y++) {
            ptr = (unsigned short*)(in->imageData + y * in->widthStep);
            for(x=0; x<w; x++) {
                if(ptr[x] == 0) { // we can filter this
                    int outerBandCount = 0, innerBandCount = 0;
                    unsigned short f[24][2] = { // the filter array
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // raw depth
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // frequency
                    };
                    for(yi = -2; yi <= 2; yi++) {
                        for(xi = -2; xi <= 2; xi++) {
                            if(xi != 0 || yi != 0) {
                                xs = x + xi;
                                ys = y + yi;
                                if(xs >= 0 && xs < w && ys >= 0 && ys < h) {
                                    raw = *((unsigned short*)(in->imageData + ys * in->widthStep) + xs);
                                    for(j=0; j<24; j++) {
                                        if(f[j][0] == raw) {
                                            f[j][1]++; // f[j][0] is a candidate of the statistical mode of the (inner|outer) band
                                            break;
                                        }
                                        else if(f[j][0] == 0) {
                                            f[j][0] = raw;
                                            f[j][1]++;
                                            break;
                                        }
                                    }
                                    if(yi == 2 || yi == -2 || xi == 2 || xi == -2)
                                        outerBandCount++;
                                    else
                                        innerBandCount++;
                                }
                            }
                        }
                    }
                    if(innerBandCount >= innerBandThreshold || outerBandCount >= outerBandThreshold) {
                        depth = frequency = 0;
                        for(j=0; j<24; j++) {
                            if(f[j][1] > frequency) {
                                depth = f[j][0];
                                frequency = f[j][1];
                            }
                            else if(f[j][0] == 0) // not needed; saves time?
                                break;
                        }
                        *((unsigned short*)(out->imageData + y * out->widthStep) + x) = depth;
                    }
                    else
                        *((unsigned short*)(out->imageData + y * out->widthStep) + x) = ptr[x];
                }
                else
                    *((unsigned short*)(out->imageData + y * out->widthStep) + x) = ptr[x];
            }
        }
    };

    std::vector<std::thread> threads;
    for(int k=0; k<numberOfThreads; k++)
        threads.push_back(std::thread(filterLines, k*h/numberOfThreads, (k+1)*h/numberOfThreads));
    for(auto& thread : threads)
        thread.join();
}
