#include <cmath>
#include <map>
#include "TouchClassifier.h"
#include "TouchDetector.h"
#include "CalibratedKinect.h"
#include "ColorClassifier.h"


// ---------------------------------------------------
// comment/uncomment me !!!
// ---------------------------------------------------
// if you want to collect samples
//#define ACQUIRE_SAMPLES_OF_CLASS       "projetor_finger"

// if you want to retrain the classifier...
//#define RETRAIN_THE_CLASSIFIER
// ---------------------------------------------------



TouchClassifier::TouchClassifier(TouchDetector* touchDetector, TouchClassifierQuality quality) : _touchDetector(touchDetector), _quality(quality)
{
    _transformedColorImage = cvCreateImage(
        cvGetSize(touchDetector->kinect()->colorImage()->getCvImage()),
        IPL_DEPTH_8U,
        3
    );

    double deg2rad = M_PI / 180.0;
    for(int i=0; i<360; i++) {
        _sin[i] = std::sin(i * deg2rad);
        _cos[i] = std::cos(i * deg2rad);
    }

    _colorClassifier = new ColorClassifier();
}

TouchClassifier::~TouchClassifier()
{
#ifdef RETRAIN_THE_CLASSIFIER
    std::cout << "training the color classifier..." << std::endl;
    ColorClassifier::CrossValidator cv(3); //cv(60);
    ColorClassifier::KnownSampleSet trainingSet;
    cv.retrieveTrainingSamples(trainingSet);
    _colorClassifier->train(trainingSet);
#endif
    delete _colorClassifier;
    cvReleaseImage(&_transformedColorImage);
}

void TouchClassifier::update()
{
    cvCvtColor(
        _touchDetector->kinect()->colorImage()->getCvImage(),
        _transformedColorImage,
        CV_RGB2Lab
    );
}

std::string TouchClassifier::classify(ofVec2f touchPosition, double sphereRadiusInMeters, double theta)
{
    CalibratedKinect* kinect = _touchDetector->kinect();
    ofVec3f sphereCenter = kinect->depthCoords2worldCoords(touchPosition);
    std::vector<int> s = _coloredPixelsInsideTheSphere(sphereCenter, sphereRadiusInMeters);
    return _classifyPixels(s, theta);
}

std::vector<int> TouchClassifier::_coloredPixelsInsideTheSphere(ofVec3f center, double radius)
{
    std::vector<int> s;
    CalibratedKinect* kinect = _touchDetector->kinect();
    IplImage* surface = _touchDetector->depthImageWithoutBackground()->getCvImage();
    
    const int da = _quality2angleincr(_quality);//5; // 3; // angle step
    const int lo = (float)_touchDetector->lo() * (255.0f / 2047.0f) * 5.0f; // FIXME

    double r = radius;
    int x, y, w = kinect->colorImage()->width;
    for(int phi = 0; phi < 360; phi += da) {
        for(int theta = 0; theta < 180; theta += da) {
            ofVec3f p(
                center.x + r * _sin[theta] * _cos[phi],
                center.y + r * _sin[theta] * _sin[phi],
                center.z + r * _cos[theta]
            );
            ofVec2f q(
                kinect->worldCoords2depthCoords(p)
            );
            x = (int)(q.x + 0.5f);
            y = (int)(q.y + 0.5f);

            if(*((unsigned char*)(surface->imageData + y * surface->widthStep) + x) > lo) {
                q = kinect->depthCoords2colorCoords(q); // this is fast !!!
                x = (int)(q.x + 0.5f);
                y = (int)(q.y + 0.5f);
                s.push_back( x + w * y );
            }
        }
    }

    return s;
}

std::string TouchClassifier::_classifyPixels(const std::vector<int>& s, double theta)
{
#if defined(ACQUIRE_SAMPLES_OF_CLASS)
    static int collectSample = 0;
    std::string filepath;

    if((++collectSample) % 10 == 0) {
        while(true) {
            static int id = 0;
            std::stringstream ss;
            ss << "color/samples/" << ACQUIRE_SAMPLES_OF_CLASS << "." << (id++) << ".txt";
            if(!ofFile::doesFileExist(filepath = ss.str()))
               break;
        }
    }
    else
        filepath = "color/garbage.tmp";

    std::ofstream f((std::string("data/") + filepath).c_str());
#endif

    std::string theClass;
    int x, y, w = _touchDetector->kinect()->colorImage()->width;
    std::vector<ColorClassifier::ChromaPair> cluster;

    for(std::vector<int>::const_iterator it = s.begin(); it != s.end(); ++it) {
        x = (*it) % w;
        y = (*it) / w;
        //_markPixel(x, y);

        unsigned char* data = (unsigned char*)(_transformedColorImage->imageData + y * _transformedColorImage->widthStep);
        unsigned char a = data[3*x + 1];
        unsigned char b = data[3*x + 2];
        cluster.push_back( ColorClassifier::ChromaPair(a, b) );

#if defined(ACQUIRE_SAMPLES_OF_CLASS)
        if(f.is_open())
            f << int(a) << " " << int(b) << "\n";
        /*short hue = (data[3*x + 0] * 2) % 360;
        unsigned char sat = data[3*x + 1];
        float hx = sat * _cos[hue];
        float hy = sat * _sin[hue];
        if(f.is_open())
            f << hx << " " << hy << "\n";*/
#endif
    }

    return _colorClassifier->classify(cluster, theta);
}

void TouchClassifier::_markPixel(int x, int y)
{
    unsigned char* line = (unsigned char*)(_touchDetector->kinect()->colorImage()->getCvImage()->imageData + y * _touchDetector->kinect()->colorImage()->getCvImage()->widthStep);
    line[3*x + 0] = 0xFF;
}

int TouchClassifier::_quality2angleincr(TouchClassifierQuality quality)
{
    switch(quality) {
    case QUALITY_BEST:
        return 5;
    case QUALITY_FAST:
        return 30;
    case QUALITY_FASTEST:
        return 45;
    default:
        return 5;
    }
}
