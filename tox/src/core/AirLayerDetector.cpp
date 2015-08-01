#include <iostream>
#include "AirLayerDetector.h"
#include "BackgroundModel.h"
#include "CalibratedKinect.h"
#include "TouchClassifier.h"



AirLayerDetector::AirLayerDetector(CalibratedKinect* kinect, BackgroundModel* bgmodel, int lo, int hi, int minArea, int maxArea, int classificationThreshold) : _kinect(kinect), _bgmodel(bgmodel), _lo(lo), _hi(hi), _minArea(minArea), _maxArea(maxArea), _threshold(classificationThreshold)
{
    std::cout << "AirLayerDetector: initializing..." << std::endl;

    int w = kinect->rawDepthImage()->width;
    int h = kinect->rawDepthImage()->height;

    // images
    _blobs.allocate(w, h);    
    _tmp = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _tmp2 = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _labelImg = cvCreateImage(cvSize(w,h), IPL_DEPTH_LABEL, 1);

    if(!_tmp || !_tmp2 || !_labelImg) {
        std::cerr << "AirLayerDetector: can't allocate images" << std::endl;
        std::exit(1);
    }
}

AirLayerDetector::~AirLayerDetector()
{
    std::cout << "AirLayerDetector: releasing..." << std::endl;
    cvReleaseImage(&_tmp);
    cvReleaseImage(&_tmp2);
    cvReleaseImage(&_labelImg);
    delete _bgmodel;
}

void AirLayerDetector::update()
{
    // let us grab the current raw image from the kinect
    cvZero(_tmp);
    cvCopy(_kinect->rawDepthImage(), _tmp);//, _bgmodel->mask());
    
    // subtraction
    const unsigned short* ptr_sd; // bgmodel->standardDeviation
    const unsigned short* ptr_bg; // bgmodel->image
    const unsigned short* ptr_tmp; // rawDepthImage AND bgmodel->mask
    const unsigned char* ptr_mask; // bg mask
    unsigned short* ptr_tmp2; // result...
    for(int y=0; y<_tmp->height; y++) {
        ptr_sd = (const unsigned short*)(_bgmodel->standardDeviation()->imageData + y * _bgmodel->standardDeviation()->widthStep);
        ptr_bg = (const unsigned short*)(_bgmodel->image()->imageData + y * _bgmodel->image()->widthStep);
        //ptr_bg = (const unsigned short*)(_bgmodel->mean()->imageData + y * _bgmodel->mean()->widthStep);
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
                ptr_tmp2[x] = 0;
        }
    }

    // binary threshold
    cvInRangeS(_tmp2, cvScalar(_lo), cvScalar(_hi), _blobs.getCvImage());

    // noise...
    cvDilate(_blobs.getCvImage(), _blobs.getCvImage(), NULL, 2);
    cvSmooth(_blobs.getCvImage(), _blobs.getCvImage(), CV_MEDIAN, 3);
    _blobs.flagImageChanged();

    // extracting & filtering the blobs
    cvReleaseBlobs(_cvb);
    cvb::cvLabel(_blobs.getCvImage(), _labelImg, _cvb);
    cvFilterByArea(_cvb, _minArea, _maxArea);
}

std::vector<AirLayerDetector::Object> AirLayerDetector::objects() const
{
    std::vector<Object> result;

    for(cvb::CvBlobs::const_iterator it = _cvb.begin(); it != _cvb.end(); ++it) {
        Object p;
        cvb::CvBlob* blob = it->second;
        CvPoint2D32f q = _bgmodel->convertToSurfaceCoordinates( // TODO: project blob centroid on the table/surface before converting
            cvPoint(blob->centroid.x, blob->centroid.y)
        );

        p.x = blob->centroid.x; // in camera (image) coordinates
        p.y = blob->centroid.y;
        p.area = blob->area;
        p.surfaceX = q.x; // in surface ( [0,1]x[0,1] ) coordinates
        p.surfaceY = q.y;
        p.type = (int(blob->area) >= _threshold ? "big" : "small");

        result.push_back(p);
    }

    return result;
}
