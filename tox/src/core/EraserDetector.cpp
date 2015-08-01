#include <iostream>
#include <queue>
#include <set>
#include "EraserDetector.h"
#include "BackgroundModel.h"
#include "CalibratedKinect.h"
#include "TouchClassifier.h"

#define ERASERDETECTOR_CALIBFILE "data/touchcalib/eraser.txt"

EraserDetector::EraserDetector(CalibratedKinect* kinect, BackgroundModel* bgmodel) : _kinect(kinect), _bgmodel(bgmodel)
{
    std::cout << "EraserDetector: initializing..." << std::endl;

    int w = kinect->rawDepthImage()->width;
    int h = kinect->rawDepthImage()->height;

    // load default background model?
    if(!_bgmodel)
        _bgmodel = BackgroundModel::load();

    // loading parameters...
    _lo = 13;
    _hi = 75;
    _ero = 3;
    _dil = 3;
    _minArea = 2000;
    _maxArea = 10000;
    _coefficient = 0.7f;
    _loadFromFile();

    // images
    _blobs.allocate(w, h);    
    _tmp = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _tmp2 = cvCreateImage(cvSize(w,h), IPL_DEPTH_16U, 1);
    _labelImg = cvCreateImage(cvSize(w,h), IPL_DEPTH_LABEL, 1);

    if(!_tmp || !_tmp2 || !_labelImg) {
        std::cerr << "EraserDetector: can't allocate images" << std::endl;
        std::exit(1);
    }
}

EraserDetector::~EraserDetector()
{
    std::cout << "EraserDetector: releasing..." << std::endl;
    _saveToFile();
    cvReleaseImage(&_tmp);
    cvReleaseImage(&_tmp2);
    cvReleaseImage(&_labelImg);
}

void EraserDetector::update()
{
    // let us grab the current raw image from the kinect
//    cvZero(_tmp);
//    cvCopy(_kinect->rawDepthImage(), _tmp);//, _bgmodel->mask());
//return;
    
    // subtraction
    const IplImage* raw = _kinect->rawDepthImage();
    const unsigned short* ptr_sd; // bgmodel->standardDeviation
    const unsigned short* ptr_bg; // bgmodel->image
    unsigned short* ptr_tmp; // rawDepthImage AND bgmodel->mask
    const unsigned char* ptr_mask; // bg mask
    unsigned short* ptr_tmp2; // result...
    for(int y=0; y<_tmp->height; y++) {
        ptr_sd = (const unsigned short*)(_bgmodel->standardDeviation()->imageData + y * _bgmodel->standardDeviation()->widthStep);
        ptr_bg = (const unsigned short*)(_bgmodel->image()->imageData + y * _bgmodel->image()->widthStep);
        //ptr_bg = (const unsigned short*)(_bgmodel->mean()->imageData + y * _bgmodel->mean()->widthStep);
        ptr_mask = (const unsigned char*)(_bgmodel->mask()->imageData + y * _bgmodel->mask()->widthStep);
        ptr_tmp = (unsigned short*)(_tmp->imageData + y * _tmp->widthStep);
        ptr_tmp2 = (unsigned short*)(_tmp2->imageData + y * _tmp2->widthStep);
        for(int x=0; x<_tmp->width; x++) {
            ptr_tmp[x] = *((const unsigned short*)(raw->imageData + y * raw->widthStep) + x);
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
    cvErode(_blobs.getCvImage(), _blobs.getCvImage(), NULL, _ero);
    cvDilate(_blobs.getCvImage(), _blobs.getCvImage(), NULL, _dil);
    cvSmooth(_blobs.getCvImage(), _blobs.getCvImage(), CV_MEDIAN, 3);
    _blobs.flagImageChanged();

    // extracting & filtering the blobs
    cvReleaseBlobs(_cvb);
    cvb::cvLabel(_blobs.getCvImage(), _labelImg, _cvb);
    cvFilterByArea(_cvb, _minArea, _maxArea);
}

std::vector<EraserDetector::Eraser> EraserDetector::objects()
{
    std::vector<Eraser> result;

    for(cvb::CvBlobs::const_iterator it = _cvb.begin(); it != _cvb.end(); ++it) {
        Eraser p;
        cvb::CvBlob* blob = it->second;
        CvPoint2D32f q = _bgmodel->convertToSurfaceCoordinates( // TODO: project blob centroid on the table/surface before converting
            cvPoint(blob->centroid.x, blob->centroid.y)
        );

        p.x = blob->centroid.x; // in camera (image) coordinates
        p.y = blob->centroid.y;
        p.area = blob->area;
        p.surfaceX = q.x; // in surface ( [0,1]x[0,1] ) coordinates
        p.surfaceY = q.y;
        p.coefficient = _computeCircularityCoefficient(p);

        if(p.coefficient >= _coefficient)
            result.push_back(p);
    }

    return result;
}

void EraserDetector::_loadFromFile()
{
    std::ifstream f(ERASERDETECTOR_CALIBFILE);

    if(f.is_open()) {
        f >>
            _lo >>
            _hi >>
            _ero >>
            _dil >>
            _minArea >>
            _maxArea >>
            _coefficient
        ;
    }
    else
        std::cerr << "EraserDetector: can't load calib file." << std::endl;
}

void EraserDetector::_saveToFile() const
{
    std::ofstream f(ERASERDETECTOR_CALIBFILE);

    if(f.is_open()) {
        f <<
            _lo << " " <<
            _hi << " " <<
            _ero << " " <<
            _dil << " " <<
            _minArea << " " <<
            _maxArea << " " <<
            _coefficient
        ;
    }
    else
        std::cerr << "EraserDetector: can't save calib file." << std::endl;
}

double EraserDetector::_computeCircularityCoefficient(const Eraser& e)
{
    // will visit every pixel px of blob b, centered at (e.x, e.y)
    int w = _blobs.width;
    int h = _blobs.height;
    std::set<int> visited;
    std::queue<int> q;
    int px, x, y;
    int j, candidate, nx, ny;
    int positive = 0;
    IplImage* blobs = _blobs.getCvImage();

    q.push(px = e.y * w + e.x);
    visited.insert(px);
    while(!q.empty()) {
        // get the pixel px = (x, y)
        px = q.front();
        q.pop();
        x = px % w;
        y = (px - x) / w;

        // process the pixel
        if(double((x - e.x) * (x - e.x) + (y - e.y) * (y - e.y)) <= double(e.area) / M_PI)
            positive++;

        // get the neighbors
        int neighbors[4][2] = {
            { x + 1, y },
            { x - 1, y },
            { x, y + 1 },
            { x, y - 1 }
        };
        for(j=0; j<4; j++) {
            nx = neighbors[j][0];
            ny = neighbors[j][1];
            if(nx >= 0 && nx < w && ny >= 0 && ny < h) {
                if(0 < *((const unsigned char*)(blobs->imageData + ny * blobs->widthStep) + nx)) { // white px?
                    if(0 == visited.count(candidate = ny * w + nx)) { // not visited?
                        q.push(candidate);
                        visited.insert(candidate);
                    }
                }
            }
        }
    }

    // result
    return double(positive) / double(e.area);
}
