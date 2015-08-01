#include "CameraCorrespondenceCalibration.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/ImageAccumulator.h"


// ---------------------------------------
// Camera correspondence calibration scene
// ---------------------------------------
CameraCorrespondenceCalibrationScene::CameraCorrespondenceCalibrationScene(MyApplication* app, int hsize, int vsize, float squareLengthInMeters) : Scene(app), _squareLength(squareLengthInMeters), _foundChessboard(false), _state(WAIT_FOR_RGB_CHESSBOARD)
{
    _patternSize = cvSize(hsize, vsize);
    _rgbCorners = new CvPoint2D32f[hsize * vsize];
    _irCorners = new CvPoint2D32f[hsize * vsize];
    _corners = new CvPoint2D32f[hsize * vsize];
    _fnt.loadFont("DejaVuSans.ttf", 12);

    if(app->kinect()->isUsingInfrared())
        app->kinect()->toggleInfrared();
}

CameraCorrespondenceCalibrationScene::~CameraCorrespondenceCalibrationScene()
{
    delete[] _corners;
    delete[] _irCorners;
    delete[] _rgbCorners;
}

void CameraCorrespondenceCalibrationScene::update()
{
    IplImage* img = application()->kinect()->colorImage()->getCvImage();
    int cornerCount = 0;
    int ret = cvFindChessboardCorners(
        img,
        _patternSize,
        _corners,
        &cornerCount,
        CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE
    );
    
    _foundChessboard = (ret != 0) && (cornerCount == _patternSize.width * _patternSize.height);
    if(_foundChessboard) {
        IplImage* gray = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);
        cvCvtColor(img, gray, CV_RGB2GRAY);
        cvFindCornerSubPix(
            gray,
            _corners,
            cornerCount,
            cvSize(11, 11),
            cvSize(-1, -1),
            cvTermCriteria(
                CV_TERMCRIT_EPS + CV_TERMCRIT_ITER,
                30,
                0.1
            )
        );
        cvReleaseImage(&gray);
        cvDrawChessboardCorners(img, _patternSize, _corners, cornerCount, ret);
    }
}

void CameraCorrespondenceCalibrationScene::draw()
{
    ofSetHexColor(0xFFFFFF);
    application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());

    ofEnableAlphaBlending();
    ofSetColor(0, 0, 0, 128);
    ofRect(0, 0, ofGetWidth(), 50);
    ofDisableAlphaBlending();

    std::stringstream ss;
    switch(_state) {
    case WAIT_FOR_RGB_CHESSBOARD:
        ss << "Apos mostrar o tabuleiro, por favor, pressione espaco.";
        break;
    case WAIT_FOR_IR_CHESSBOARD:
        ss << "Otimo. Agora, mais uma vez. Mostre o tabuleiro e pressione espaco.";
        break;
    default:
        ss << "Por favor, aguarde...";
        break;
    }
    ofSetHexColor(0xFFAA00);
    _fnt.drawString(ss.str(), 10, 30);
}

void CameraCorrespondenceCalibrationScene::keyPressed(int key)
{
    int n = _patternSize.width * _patternSize.height;

    if(!(key == ' ' || key == OF_KEY_RETURN))
        return;

    switch(_state) {
    case WAIT_FOR_RGB_CHESSBOARD:
        if(_foundChessboard) {
            std::cerr << "[correspcalib] grabbed rgb corners" << std::endl;
            std::copy(_corners, _corners + n, _rgbCorners);
            application()->kinect()->toggleInfrared();
            _state = WAIT_FOR_IR_CHESSBOARD;
        }
        break;

    case WAIT_FOR_IR_CHESSBOARD:
        if(_foundChessboard) {
            std::cerr << "[correspcalib] grabbed ir corners" << std::endl;
            std::copy(_corners, _corners + n, _irCorners);
            application()->kinect()->toggleInfrared();

            std::cerr << "[correspcalib] calibrating..." << std::endl;
            _calibrate();
            std::cerr << "[correspcalib] done!" << std::endl;
            std::exit(0);

            break;
        }
    }
}

void CameraCorrespondenceCalibrationScene::_calibrate()
{
    int n = _patternSize.width * _patternSize.height;

    CvMat* ir = cvCreateMat(n, 2, CV_32FC1);
    CvMat* rgb = cvCreateMat(n, 2, CV_32FC1);
    CvMat* ir2rgb = cvCreateMat(3, 3, CV_32FC1);
    CvMat* rgb2ir = cvCreateMat(3, 3, CV_32FC1);

    for(int i=0; i<n; i++) {
        CV_MAT_ELEM(*ir, float, i, 0) = _irCorners[i].x;
        CV_MAT_ELEM(*ir, float, i, 1) = _irCorners[i].y;
        CV_MAT_ELEM(*rgb, float, i, 0) = _rgbCorners[i].x;
        CV_MAT_ELEM(*rgb, float, i, 1) = _rgbCorners[i].y;
    }

    cvFindHomography(ir, rgb, ir2rgb); // so easy!!!!
    cvFindHomography(rgb, ir, rgb2ir);

    cvSave("data/camcalib/ir2rgb.yaml", ir2rgb);
    cvSave("data/camcalib/rgb2ir.yaml", rgb2ir);

    cvReleaseMat(&ir);
    cvReleaseMat(&rgb);
    cvReleaseMat(&ir2rgb);
    cvReleaseMat(&rgb2ir);
}


// ---------------------------------------
// Camera correspondence calibration scene 2
// ---------------------------------------
CameraCorrespondenceCalibrationScene2::CameraCorrespondenceCalibrationScene2(MyApplication* app) : Scene(app)
{
    _fnt.loadFont("DejaVuSans.ttf", 12);
    if(app->kinect()->isUsingInfrared())
        app->kinect()->toggleInfrared();
}

CameraCorrespondenceCalibrationScene2::~CameraCorrespondenceCalibrationScene2()
{
}

void CameraCorrespondenceCalibrationScene2::update()
{
}

void CameraCorrespondenceCalibrationScene2::draw()
{
    ofxCvImage* img = _colorCorners.size() < 4 ? application()->kinect()->colorImage() : application()->kinect()->grayscaleDepthImage();
    ofSetHexColor(0xFFFFFF);
    img->draw(0, 0, ofGetWidth(), ofGetHeight());

    ofEnableAlphaBlending();
    ofSetColor(0, 0, 0, 128);
    ofRect(0, 0, ofGetWidth(), 50);
    ofDisableAlphaBlending();

    std::stringstream ss;
    ss << "Preciso de 4 pontos";
    ofSetHexColor(0xFFAA00);
    _fnt.drawString(ss.str(), 10, 30);

    std::vector<ofVec2f>* vec = _colorCorners.size() < 4 ? &_colorCorners : &_depthCorners;
    for(int j=0; j<(int)vec->size(); j++)
        ofCircle((*vec)[j].x, (*vec)[j].y, 5);
}

void CameraCorrespondenceCalibrationScene2::mousePressed(int x, int y, int button)
{
    if(button != 0)
        return;

    if(_colorCorners.size() < 4) {
        _colorCorners.push_back( ofVec2f(x,y) );
        if(_colorCorners.size() == 4)
            std::cerr << "[correspcalib] grabbed color corners" << std::endl;
    }
    else if(_depthCorners.size() < 4) {
        _depthCorners.push_back( ofVec2f(x,y) );
        if(_depthCorners.size() == 4) {
            std::cerr << "[correspcalib] grabbed depth corners" << std::endl;
            std::cerr << "[correspcalib] calibrating..." << std::endl;
            _calibrate();
            std::cerr << "[correspcalib] done!" << std::endl;
            std::exit(1);
        }
    }
}

void CameraCorrespondenceCalibrationScene2::_calibrate()
{
    int n = 4;
    int w = application()->kinect()->colorImage()->width;
    int h = application()->kinect()->colorImage()->height;

    CvMat* d = cvCreateMat(n, 2, CV_32FC1);
    CvMat* rgb = cvCreateMat(n, 2, CV_32FC1);
    CvMat* d2rgb = cvCreateMat(3, 3, CV_32FC1);
    CvMat* rgb2d = cvCreateMat(3, 3, CV_32FC1);

    for(int i=0; i<n; i++) {
        CV_MAT_ELEM(*d, float, i, 0) = _depthCorners[i].x * w / ofGetWidth();
        CV_MAT_ELEM(*d, float, i, 1) = _depthCorners[i].y * h / ofGetHeight();
        CV_MAT_ELEM(*rgb, float, i, 0) = _colorCorners[i].x * w / ofGetWidth();
        CV_MAT_ELEM(*rgb, float, i, 1) = _colorCorners[i].y * h / ofGetHeight();
    }

    cvFindHomography(d, rgb, d2rgb); // so easy!!!!
    cvFindHomography(rgb, d, rgb2d);

    cvSave("data/camcalib/d2rgb.yaml", d2rgb);
    cvSave("data/camcalib/rgb2d.yaml", rgb2d);

    cvReleaseMat(&d);
    cvReleaseMat(&rgb);
    cvReleaseMat(&d2rgb);
    cvReleaseMat(&rgb2d);
}




// ---------------------------------------
// Camera correspondence calibration scene 3
// ---------------------------------------
CameraCorrespondenceCalibrationScene3::CameraCorrespondenceCalibrationScene3(MyApplication* app, int hsize, int vsize) : Scene(app), _hsize(hsize), _vsize(vsize)
{
    _corners = new CvPoint2D32f[hsize * vsize];
    _fnt.loadFont("DejaVuSans.ttf", 12);
    if(!app->kinect()->isUsingInfrared())
        app->kinect()->toggleInfrared();
    _gray = cvCreateImage(cvGetSize( app->kinect()->colorImage()->getCvImage() ), IPL_DEPTH_8U, 1);
}

CameraCorrespondenceCalibrationScene3::~CameraCorrespondenceCalibrationScene3()
{
    cvReleaseImage(&_gray);
    delete[] _corners;
}

void CameraCorrespondenceCalibrationScene3::update()
{
    IplImage* img = application()->kinect()->colorImage()->getCvImage();

    int ret = cvFindChessboardCorners(
        img,
        cvSize(_hsize, _vsize),
        _corners,
        &_cornerCount,
        CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE
    );

    _foundChessboard = (_cornerCount == _hsize * _vsize && ret != 0);
    if(_foundChessboard) {
        cvCvtColor(img, _gray, CV_RGB2GRAY);
        cvFindCornerSubPix(
            _gray,
            _corners,
            _cornerCount,
            cvSize(11, 11),
            cvSize(-1, -1),
            cvTermCriteria(
                CV_TERMCRIT_EPS + CV_TERMCRIT_ITER,
                30,
                0.1
            )
        );
    }
}

void CameraCorrespondenceCalibrationScene3::draw()
{
    ofxCvImage* img = application()->kinect()->colorImage();
    cvDrawChessboardCorners(img->getCvImage(), cvSize(_hsize, _vsize), _corners, _cornerCount, _foundChessboard ? 1 : 0);
    ofSetHexColor(0xFFFFFF);
    img->draw(0, 0, ofGetWidth(), ofGetHeight());

    ofEnableAlphaBlending();
    ofSetColor(0, 0, 0, 128);
    ofRect(0, 0, ofGetWidth(), 50);
    ofDisableAlphaBlending();

    std::stringstream ss;
    ss << "ESPACO = tirar foto; C = calibrar; fotos RGB/IR: " << _rgbBoards.size() << "/" << _irBoards.size();
    ofSetHexColor(0xFFAA00);
    _fnt.drawString(ss.str(), 10, 30);
}

void CameraCorrespondenceCalibrationScene3::keyPressed(int key)
{
    if(key == ' ') {
        if(_foundChessboard) {
            bool ir = application()->kinect()->isUsingInfrared();
            CvPoint2D32f* crn = new CvPoint2D32f[_cornerCount];
            std::copy(_corners, _corners + _cornerCount, crn);
            (ir ? _irBoards : _rgbBoards).push_back(
                application()->kinect()->colorImage()->getCvImage()
            );
            (ir ? _irCorners : _rgbCorners).push_back(
                crn
            );
            std::cerr << "[correspcalib] took a " << (ir ? "ir" : "rgb") << " photo." << std::endl;
            application()->kinect()->toggleInfrared();
        }
        else
            std::cerr << "[correspcalib] can't take a photo: where's the chessboard?" << std::endl;
    }
    else if(key == 'c') {
        if(_rgbBoards.size() == _irBoards.size() && _rgbBoards.size() >= 2) {
            std::cerr << "[correspcalib] calibrating..." << std::endl;
            _calibrate();
            std::cerr << "[correspcalib] done!" << std::endl;
            std::exit(0);
        }
        else
            std::cerr << "[correspcalib] can't calibrate: I need (rgb photos = ir photos) >= 2" << std::endl;
    }
}

void CameraCorrespondenceCalibrationScene3::_calibrate()
{
    int rgb = 0, ir = 0;

    for(std::vector<IplImage*>::iterator it = _rgbBoards.begin(); it != _rgbBoards.end(); ++it) {
        std::stringstream ss;
        ss << "data/correspcalib/rgb" << (++rgb);
        cvSave((ss.str() + ".yaml").c_str(), *it);
    }

    for(std::vector<IplImage*>::iterator it = _irBoards.begin(); it != _irBoards.end(); ++it) {
        std::stringstream ss;
        ss << "data/correspcalib/ir" << (++ir);
        cvSave((ss.str() + ".yaml").c_str(), *it);
    }

    std::ofstream fr("data/correspcalib/rgb_corners.txt");
    if(fr.is_open()) {
        fr << _rgbCorners.size() << "\n";
        fr << _hsize << " " << _vsize << "\n\n";
        for(std::vector<CvPoint2D32f*>::iterator it = _rgbCorners.begin(); it != _rgbCorners.end(); ++it) {
            CvPoint2D32f* corners = (*it);
            for(int i=0; i<_hsize * _vsize; i++)
                fr << corners[i].x << " " << corners[i].y << " ";
            fr << "\n";
        }
    }

    std::ofstream fi("data/correspcalib/ir_corners.txt");
    if(fi.is_open()) {
        fi << _irCorners.size() << "\n";
        fi << _hsize << " " << _vsize << "\n\n";
        for(std::vector<CvPoint2D32f*>::iterator it = _irCorners.begin(); it != _irCorners.end(); ++it) {
            CvPoint2D32f* corners = (*it);
            for(int i=0; i<_hsize * _vsize; i++)
                fi << corners[i].x << " " << corners[i].y << " ";
            fi << "\n";
        }
    }

    std::ofstream fc("data/correspcalib/chessboard_corners.txt");
    if(fc.is_open()) {
        int n = _rgbCorners.size();
        fc << n << "\n";
        fc << _hsize << " " << _vsize << "\n\n";
        for(int i=0; i<n; i++) {
            for(int j=0; j<_hsize * _vsize; j++)
                fc << int(j / _hsize) << " " << int(j % _hsize) << " ";
            fc << "\n";
        }
    }
}


// ---------------------------------------
// Camera correspondence calibration scene calc
// ---------------------------------------
CameraCorrespondenceCalibrationSceneCalc::CameraCorrespondenceCalibrationSceneCalc(MyApplication* app) : Scene(app)
{
    int nBoards = 0, hsize = 0, vsize = 0;
    std::ifstream rgbFile("data/correspcalib/rgb_corners.txt");
    std::ifstream irFile("data/correspcalib/ir_corners.txt");
    std::ifstream chessFile("data/correspcalib/chessboard_corners.txt");

    if(rgbFile.is_open() && irFile.is_open() && chessFile.is_open()) {
        rgbFile >> nBoards >> hsize >> vsize; irFile >> nBoards >> hsize >> vsize; chessFile >> nBoards >> hsize >> vsize; // FIXME
        if(nBoards > 0 && hsize > 0 && vsize > 0) {
            int nCorners = hsize * vsize;

            // setup input matrices
            CvMat* rgbPoints = cvCreateMat(nCorners * nBoards, 2, CV_32FC1);
            CvMat* irPoints = cvCreateMat(nCorners * nBoards, 2, CV_32FC1);
            CvMat* chessboardPoints = cvCreateMat(nCorners * nBoards, 3, CV_32FC1);
            for(int i=0; i<nCorners * nBoards; i++) {
                float rgbX, rgbY, irX, irY, chessboardX, chessboardY;

                rgbFile >> rgbX >> rgbY;
                irFile >> irX >> irY;
                chessFile >> chessboardX >> chessboardY;

                CV_MAT_ELEM(*rgbPoints, float, i, 0) = rgbX;
                CV_MAT_ELEM(*rgbPoints, float, i, 1) = rgbY;
                CV_MAT_ELEM(*irPoints, float, i, 0) = irX;
                CV_MAT_ELEM(*irPoints, float, i, 1) = irY;
                CV_MAT_ELEM(*chessboardPoints, float, i, 0) = chessboardX;
                CV_MAT_ELEM(*chessboardPoints, float, i, 1) = chessboardY;
                CV_MAT_ELEM(*chessboardPoints, float, i, 2) = 0; // z = 0
            }

            CvMat* nPoints = cvCreateMat(nBoards, 1, CV_32SC1);
            for(int i=0; i<nBoards; i++)
                CV_MAT_ELEM(*nPoints, int, i, 0) = nCorners;

            // setup R and T
            CvMat* R = cvCreateMat(3, 3, CV_32FC1);
            CvMat* T = cvCreateMat(3, 1, CV_32FC1);

            // calibrate
            cvStereoCalibrate(
                chessboardPoints,
                rgbPoints,
                irPoints, // from ir to rgb
                nPoints,
                (CvMat*)cvLoad("data/camcalib/intrinsics_rgb.yaml"), // FIXME
                (CvMat*)cvLoad("data/camcalib/distortion_rgb.yaml"), // FIXME
                (CvMat*)cvLoad("data/camcalib/intrinsics_ir.yaml"), // FIXME
                (CvMat*)cvLoad("data/camcalib/distortion_ir.yaml"), // FIXME
                cvGetSize( application()->kinect()->colorImage()->getCvImage() ),
                R,
                T,
                NULL,
                NULL,
                cvTermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 100, 1e-5),
                CV_CALIB_FIX_INTRINSIC
            );
            cvSave("data/correspcalib/rotation.yaml", R);
            cvSave("data/correspcalib/translation.yaml", T);

            // release stuff
            cvReleaseMat(&T);
            cvReleaseMat(&R);
            cvReleaseMat(&nPoints);
            cvReleaseMat(&chessboardPoints);
            cvReleaseMat(&irPoints);
            cvReleaseMat(&rgbPoints);

            // done!
            std::cerr << "[correspcalib_calc] done!" << std::endl;
            std::exit(0);
        }
        else {
            std::cerr << "[correspcalib_calc] invalid [nBoards|hsize|vsize] in [rgb|ir|chessboard]_corners.txt" << std::endl;
            std::exit(1);
        }
    }
    else {
        std::cerr << "[correspcalib_calc] can't open [rgb|ir|chessboard]_corners.txt" << std::endl;
        std::exit(1);
    }
}

CameraCorrespondenceCalibrationSceneCalc::~CameraCorrespondenceCalibrationSceneCalc()
{
}

void CameraCorrespondenceCalibrationSceneCalc::update()
{
}

void CameraCorrespondenceCalibrationSceneCalc::draw()
{
}
