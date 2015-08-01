#include <iostream>
#include "TouchDetector.h"
#include "BackgroundModel.h"
#include "CalibratedKinect.h"
#include "TouchClassifier.h"

#define TOUCHDETECTOR_CALIBFILE "data/touchcalib/params.txt"



TouchDetector::TouchDetector(CalibratedKinect* kinect, BackgroundModel* bgmodel) : TouchDetector(kinect, TouchClassifier::QUALITY_BEST, bgmodel)
{
}

TouchDetector::TouchDetector(CalibratedKinect* kinect, TouchClassifier::TouchClassifierQuality classifierQuality, BackgroundModel* bgmodel) : _kinect(kinect), _bgmodel(bgmodel)
{
    std::cout << "TouchDetector: initializing..." << std::endl;

    int w = kinect->rawDepthImage()->width;
    int h = kinect->rawDepthImage()->height;

    // load default background model?
    if(!_bgmodel)
        _bgmodel = BackgroundModel::load();

    // loading parameters...
    _lo = 12;
    _hi = 22;
    _dil1 = 1;
    _ero1 = 2;
    _dil2 = 3;
    _ero2 = 0;
    _minArea = 25;
    _maxArea = 150;
    _radius = 0.01f;
    _theta = 10.0f;
    _loadFromFile();

    // images
    _bg.allocate(w, h);    
    _cur.allocate(w, h);    
    _sub.allocate(w, h);    
    _blobs.allocate(w, h);    
    _blacknwhite.allocate(w, h);
    _blacknwhitewobg.allocate(w, h);
    _coloredwobg.allocate(w, h);
    _binarywobg.allocate(w, h);
    _tmp = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _tmp2 = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _labelImg = cvCreateImage(cvSize(w,h), IPL_DEPTH_LABEL, 1);

    if(!_tmp || !_tmp2 || !_labelImg) {
        std::cerr << "TouchDetector: can't allocate images" << std::endl;
        std::exit(1);
    }

    // we need the background...
    cvConvertScale(_bgmodel->image(), _bg.getCvImage(), 255.0f / 2047.0f);
    //cvDilate(_bg.getCvImage(), _bg.getCvImage(), NULL, 1);
    _bg.flagImageChanged();

    // the classifier
    _classifier = new TouchClassifier(this, classifierQuality);
}

TouchDetector::~TouchDetector()
{
    std::cout << "TouchDetector: releasing..." << std::endl;
    _saveToFile();
    cvReleaseImage(&_tmp);
    cvReleaseImage(&_tmp2);
    cvReleaseImage(&_labelImg);
    delete _bgmodel;
    delete _classifier;
}

void TouchDetector::_loadFromFile()
{
    std::ifstream f(TOUCHDETECTOR_CALIBFILE);

    if(f.is_open()) {
        f >>
            _lo >>
            _hi >>
            _dil1 >>
            _ero1 >>
            _dil2 >>
            _ero2 >>
            _minArea >>
            _maxArea >>
            _radius >>
            _theta
        ;
    }
    else
        std::cerr << "TouchDetector: can't load calib file." << std::endl;
}

void TouchDetector::_saveToFile() const
{
    std::ofstream f(TOUCHDETECTOR_CALIBFILE);

    if(f.is_open()) {
        f <<
            _lo << " " <<
            _hi << " " <<
            _dil1 << " " <<
            _ero1 << " " <<
            _dil2 << " " <<
            _ero2 << " " <<
            _minArea << " " <<
            _maxArea << " " <<
            _radius << " " <<
            _theta
        ;
    }
    else
        std::cerr << "TouchDetector: can't save calib file." << std::endl;
}

void TouchDetector::update()
{
    // update the classifier
    _classifier->update();

    // let us grab the current raw image from the kinect
    //cvZero(_tmp);
    cvCopy(_kinect->rawDepthImage(), _tmp);//, _bgmodel->mask());
    cvConvertScale(_tmp, _cur.getCvImage(), 255.0f / 2047.0f);
    _cur.flagImageChanged();
    
    // subtraction
    int r, g, b;
    const unsigned char* ptr_color; // colored camera
    unsigned char* ptr_blacknwhite; // blacknwhite image
    unsigned char* ptr_blacknwhitewobg; // blacknwhite image (without background)
    unsigned char* ptr_coloredwobg; // colored image (without background)
    unsigned char* ptr_binarywobg; // binary image (without background)
    const unsigned short* ptr_sd; // bgmodel->standardDeviation
    const unsigned short* ptr_bg; // bgmodel->image
    const unsigned short* ptr_tmp; // rawDepthImage
    const unsigned char* ptr_mask; // bg mask
    unsigned short* ptr_tmp2; // result...
    for(int y=0; y<_tmp->height; y++) {
        ptr_color = (const unsigned char*)(_kinect->colorImage()->getCvImage()->imageData + y * _kinect->colorImage()->getCvImage()->widthStep);
        ptr_blacknwhite = (unsigned char*)(_blacknwhite.getCvImage()->imageData + y * _blacknwhite.getCvImage()->widthStep);
        ptr_blacknwhitewobg = (unsigned char*)(_blacknwhitewobg.getCvImage()->imageData + y * _blacknwhitewobg.getCvImage()->widthStep);
        ptr_coloredwobg = (unsigned char*)(_coloredwobg.getCvImage()->imageData + y * _coloredwobg.getCvImage()->widthStep);
        ptr_binarywobg = (unsigned char*)(_binarywobg.getCvImage()->imageData + y * _binarywobg.getCvImage()->widthStep);
        ptr_sd = (const unsigned short*)(_bgmodel->standardDeviation()->imageData + y * _bgmodel->standardDeviation()->widthStep);
        ptr_bg = (const unsigned short*)(_bgmodel->image()->imageData + y * _bgmodel->image()->widthStep);
        //ptr_bg = (const unsigned short*)(_bgmodel->mean()->imageData + y * _bgmodel->mean()->widthStep);
        ptr_bg = (const unsigned short*)(_bgmodel->unmaskedImage()->imageData + y * _bgmodel->unmaskedImage()->widthStep);
        ptr_mask = (const unsigned char*)(_bgmodel->mask()->imageData + y * _bgmodel->mask()->widthStep);
        ptr_tmp = (const unsigned short*)(_tmp->imageData + y * _tmp->widthStep);
        ptr_tmp2 = (unsigned short*)(_tmp2->imageData + y * _tmp2->widthStep);
        for(int x=0; x<_tmp->width; x++) {
            // perform the background subtraction
            if(ptr_mask[x] > 0 && ptr_tmp[x] > 25 && ptr_tmp[x] < 2047) { // FIXME: magic numbers?
                if(ptr_bg[x] >= ptr_tmp[x]) {
                    // inverse depth (kinect)
                    ptr_tmp2[x] = ptr_bg[x] - ptr_tmp[x];

                    // avoid jitter
                    if(ptr_tmp2[x] < 3 * ptr_sd[x])
                        ptr_tmp2[x] = 0;
                }
                else
                    ptr_tmp2[x] = 0;
            }
            else
                ptr_tmp2[x] = (ptr_tmp[x] > 25 && ptr_tmp[x] < 2047 && ptr_bg[x] - ptr_tmp[x] >= _lo) ? 0xFFFF : 0;

            // rgb channels
            r = ptr_color[3*x];
            g = ptr_color[3*x+1];
            b = ptr_color[3*x+2];

            // blacknwhite image
            ptr_blacknwhite[x] = (unsigned char)(std::min(255, int(0.3f * r + 0.59f * g + 0.11f * b))); // color to grayscale

            // images without background
            //if(((int)ptr_bg[x] - (int)ptr_tmp[x] >= (int)_lo))
            if(ptr_tmp2[x] >= _lo) {
                ptr_blacknwhitewobg[x] = ptr_blacknwhite[x];
                ptr_coloredwobg[3*x + 0] = ptr_color[3*x + 0];
                ptr_coloredwobg[3*x + 1] = ptr_color[3*x + 1];
                ptr_coloredwobg[3*x + 2] = ptr_color[3*x + 2];
                ptr_binarywobg[x] = (ptr_blacknwhitewobg[x] > 0) ? 255 : 0;
            }
            else {
                ptr_blacknwhitewobg[x] = 0;
                ptr_coloredwobg[3*x + 0] = 0;
                ptr_coloredwobg[3*x + 1] = 0;
                ptr_coloredwobg[3*x + 2] = 0;
                ptr_binarywobg[x] = 0;
            }

            // gambiarra
            if(ptr_tmp2[x] == 0xFFFF)
                ptr_tmp2[x] = 0;
        }
    }

    // okay, let's get the subtraction result
    cvConvertScale(_tmp2, _sub.getCvImage(), (255.0f / 2047.0f) * 5);
    cvErode(_blacknwhitewobg.getCvImage(), _blacknwhitewobg.getCvImage(), NULL, 2);
    cvDilate(_blacknwhitewobg.getCvImage(), _blacknwhitewobg.getCvImage(), NULL, 1);
    cvErode(_coloredwobg.getCvImage(), _coloredwobg.getCvImage(), NULL, 2);
    cvDilate(_coloredwobg.getCvImage(), _coloredwobg.getCvImage(), NULL, 1);
    _sub.flagImageChanged();
    _blacknwhite.flagImageChanged();
    _blacknwhitewobg.flagImageChanged();
    _coloredwobg.flagImageChanged();
    _binarywobg.flagImageChanged();

    // binary threshold
    cvInRangeS(_tmp2, cvScalar(_lo), cvScalar(_hi), _blobs.getCvImage());

    // noise...
    cvDilate(_blobs.getCvImage(), _blobs.getCvImage(), NULL, _dil1);
    cvErode(_blobs.getCvImage(), _blobs.getCvImage(), NULL, _ero1);
    cvDilate(_blobs.getCvImage(), _blobs.getCvImage(), NULL, _dil2);
    cvErode(_blobs.getCvImage(), _blobs.getCvImage(), NULL, _ero2);
    cvSmooth(_blobs.getCvImage(), _blobs.getCvImage(), CV_MEDIAN, 3);
    _blobs.flagImageChanged();

    // extracting & filtering the blobs
    cvReleaseBlobs(_cvb);
    cvb::cvLabel(_blobs.getCvImage(), _labelImg, _cvb);
    cvFilterByArea(_cvb, _minArea, _maxArea);
}

std::vector<TouchDetector::TouchPoint> TouchDetector::touchPoints() const
{
    std::vector<TouchPoint> result;

    for(cvb::CvBlobs::const_iterator it = _cvb.begin(); it != _cvb.end(); ++it) {
        TouchPoint p;
        cvb::CvBlob* blob = it->second;
        CvPoint2D32f q = _bgmodel->convertToSurfaceCoordinates(
            cvPoint(blob->centroid.x, blob->centroid.y)
        );

        p.x = blob->centroid.x; // in camera (image) coordinates
        p.y = blob->centroid.y;
        p.area = blob->area;
        p.surfaceX = q.x; // in surface ( [0,1]x[0,1] ) coordinates
        p.surfaceY = q.y;
        p.type = _classifier->classify( ofVec2f(p.x, p.y), _radius, _theta ); // the tracker may change this

        p.speedX = p.speedY = 0.0f; // the tracker will set all this
        p.surfaceSpeedX = p.surfaceSpeedY = 0.0f;
        p.startX = p.startY = 0.0f;
        p.surfaceStartX = p.surfaceStartY = 0.0f;
        p._tracked = false;
        p._id = 0;
        p._timeToLive = 0;

        result.push_back(p);
    }

    return result;
}
