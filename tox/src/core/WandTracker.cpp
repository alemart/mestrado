#include "WandTracker.h"
#include "ImageLabeler.h"
#include "CalibratedKinect.h"
#include "BackgroundModel.h"
#include "WandCalibration.h"

// number of framesteps a touch point can resist
// given that it is not being received by the sensor
#define TIMETOLIVE 6

unsigned WandTracker::_idAutoIncrement = 0;

WandTracker::WandTracker(CalibratedKinect* kinect, BackgroundModel* bgmodel, ImageLabeler* labeler, int theLookupRadius, int theMinArea, std::string color) :
    _kinect(kinect), _bgmodel(bgmodel), _labeler(labeler), _calibration(0), _lookupRadius(theLookupRadius), _minArea(theMinArea), _color(color), _isCalibrating(false), _ttl(0), _ttl2(0)
{
    // calibration
    _calibration = new WandCalibration();

    // determine the 3d plane of the interactive surface
    ofVec3f topleft = ofVec3f( bgmodel->topleft3DVertex().x, bgmodel->topleft3DVertex().y, bgmodel->topleft3DVertex().z );
    ofVec3f topright = ofVec3f( bgmodel->topright3DVertex().x, bgmodel->topright3DVertex().y, bgmodel->topright3DVertex().z );
    ofVec3f bottomright = ofVec3f( bgmodel->bottomright3DVertex().x, bgmodel->bottomright3DVertex().y, bgmodel->bottomright3DVertex().z );
    ofVec3f bottomleft = ofVec3f( bgmodel->bottomleft3DVertex().x, bgmodel->bottomleft3DVertex().y, bgmodel->bottomleft3DVertex().z );
    _plane = _computeBest3DPlane(topleft, topright, bottomright, bottomleft);
}

WandTracker::~WandTracker()
{
    delete _calibration;
}





//
// FIXME: this will track only ONE yellow wand
//
void WandTracker::update(float dt)
{
    std::set<Wand> candidates;
    auto acceptableColors = []() -> std::set<std::string> {
        std::set<std::string> s;
        s.insert("marker_yellow");
        return s;
    };

    // tracking stuff
    _oldWands = _wands;
    _wands.clear();
    for(const Wand& w : _oldWands)
        candidates.insert(w);

    //
    // track wand_yellow's
    //
#if 0
    static unsigned _ = 0, __ = 30; // wtf?! this introduces hickups on the app...
    if(_oldWands.size() > 0) {
        // have we got wands already?
        for(const Wand& oldWand : _oldWands) {
            ImageLabeler::ConnectedComponent blob = _labeler->getBlobAt(oldWand.position.imageCoords, acceptableColors());
            if(blob.area >= _minArea) {
                if(blob.centerOfMass.distance(oldWand.position.imageCoords) <= _lookupRadius) {
                    // build a new wand
                    Wand wand;
                    wand.id = oldWand.id;
                    wand.area = blob.area;
                    _fillWandPositions(&wand, blob.centerOfMass);
                    _fillWandSpeeds(&wand, oldWand, dt);

                    // determine the type of the wand
                    if((oldWand.type == "wand_yellow" || oldWand.type == "wand_yellow:active") && (blob.dominantClass() == "marker_yellow"))
                        wand.type = (blob.classCount["marker_magenta"] > wand.area/8) ? "wand_yellow" : "wand_yellow:active"; // FIXME
                    else if((oldWand.type == "wand_magenta" || oldWand.type == "wand_magenta:active") && (blob.dominantClass() == "marker_magenta"))
                        wand.type = (blob.classCount["marker_magenta"] > wand.area/3) ? "wand_magenta" : "wand_magenta:active"; // FIXME
                    else if((oldWand.type == "wand_blue" || oldWand.type == "wand_blue:active") && (blob.dominantClass() == "marker_blue"))
                        wand.type = (blob.classCount["marker_blue"] > wand.area/3) ? "wand_blue" : "wand_blue:active"; // FIXME
                    else if((oldWand.type == "wand_green" || oldWand.type == "wand_green:active") && (blob.dominantClass() == "marker_green"))
                        wand.type = (blob.classCount["marker_green"] > wand.area/3) ? "wand_green" : "wand_green:active"; // FIXME
                    else
                        continue; // invalid wand...
std::cout << "classCount[marker_yellow] = " << blob.classCount["marker_yellow"] << ", classCount[marker_magenta] = " << blob.classCount["marker_magenta"] << std::endl;
                    _wands.push_back(wand);
                }
            }
        }
    }
    else if(0 == (_++) % __) {
        // grab new wands (this is SLOW)
        std::vector<ImageLabeler::ConnectedComponent> blobs = _labeler->getAllBlobs(acceptableColors());
        for(ImageLabeler::ConnectedComponent& blob : blobs) {
            // TODO: min/max area filter?
            if(blob.area >= _minArea) {
                // build a new wand
                Wand wand;
                wand.id = ++_idAutoIncrement;
                wand.area = blob.area;
                _fillWandPositions(&wand, blob.centerOfMass);

                // determine the type of the wand
                if(blob.dominantClass() == "marker_yellow")
                    wand.type = "wand_yellow";
                else if(blob.dominantClass() == "marker_magenta")
                    wand.type = "wand_magenta";
                else if(blob.dominantClass() == "marker_blue")
                    wand.type = "wand_blue";
                else if(blob.dominantClass() == "marker_green")
                    wand.type = "wand_green";
                else
                    continue; // invalid wand...

                // tracking (obsolete now)
                /*float mindist = _lookupRadius;
                const Wand* bestCandidate = 0;
                for(std::set<Wand>::iterator it = candidates.begin(); it != candidates.end(); ++it) {
                    float dist = (it->position.imageCoords).distance(wand.position.imageCoords);
                    if(dist <= mindist) {
                        mindist = dist;
                        bestCandidate = &(*it);
                    }
                }
                if(bestCandidate != 0) {
                    Wand prev = *bestCandidate;
                    wand.id = prev.id;
                    _fillWandSpeeds(&wand, prev, dt);
                    candidates.erase(prev);
                }*/

                // done!
                _wands.push_back(wand);
            }
        }
    }
#else
    std::vector<ImageLabeler::ConnectedComponent> acceptedBlobs;
    std::vector<ImageLabeler::ConnectedComponent> blobs = _labeler->getAllBlobs(acceptableColors());

    // filter valid blobs
    for(ImageLabeler::ConnectedComponent& blob : blobs) {
        if(blob.area >= _minArea)
            acceptedBlobs.push_back(blob);
    }

    // build the wand
    if(acceptedBlobs.size() > 0) {
        Wand wand;
        bool success = false;

        // setup the wand
        if(_color == "yellow") {
            if(acceptedBlobs.size() >= 2) {
                wand.type = std::string("wand_") + _color;
                wand.area = acceptedBlobs[0].area + acceptedBlobs[1].area;
                success = _fillWandPositions(&wand, (acceptedBlobs[0].centerOfMass + acceptedBlobs[1].centerOfMass) / 2);
            }
            else {
                wand.type = std::string("wand_") + _color + ":active";
                wand.area = acceptedBlobs[0].area;
                success = _fillWandPositions(&wand, acceptedBlobs[0].centerOfMass);
            }
        }
        else {
            wand.type = "wand_" + _color;
            wand.area = acceptedBlobs[0].area;
            success = _fillWandPositions(&wand, acceptedBlobs[0].centerOfMass);
        }

        // get the old wand!
        if(!success && _oldWands.size() > 0)
            wand = _oldWands[0];

        // track the wand
        if(_oldWands.size() > 0) {
            Wand& oldWand = _oldWands[0];
            wand.id = oldWand.id;
            _fillWandSpeeds(&wand, oldWand, dt);

            // important for TUIO: give a new ID if type changes
            if(oldWand.type != wand.type)
                wand.id = ++_idAutoIncrement;
        }
        else
            wand.id = ++_idAutoIncrement;


        // done!
        _wands.push_back(wand);
    }

    // time-to-live
    if(_wands.size() == 0) {
        if(_ttl++ < TIMETOLIVE)
            _wands = _oldWands;
        else
            _ttl = 0;
    }
    else
        _ttl = 0;

    if(_wands.size() > 0 && _oldWands.size() > 0 && _wands[0].type != _oldWands[0].type) {
        if(_ttl2++ < TIMETOLIVE) {
            _wands[0].id = _oldWands[0].id;
            _wands[0].type = _oldWands[0].type;
        }
        else
            _ttl2 = 0;
    }
    else
        _ttl2 = 0;
#endif

    // calibration
    if(_isCalibrating) {
        if(_wands.size() > 0) {
            Wand& w = _wands[0];
            ofVec3f p = _kinect->depthCoords2worldCoords(w.position.imageCoords);
            _calibsample.push_back(CalibrationSample(p));
            std::cout << "wand " + _color + " world pos " << p.x << ", " << p.y << ", " << p.z << std::endl;
        }
    }
}








std::vector<WandTracker::Wand> WandTracker::getWands() const
{
    return _wands;
}


void WandTracker::setLookupRadius(int theLookupRadius)
{
    _lookupRadius = theLookupRadius;
}

int WandTracker::lookupRadius() const
{
    return _lookupRadius;
}

void WandTracker::setMinArea(int theMinArea)
{
    _minArea = theMinArea;
}

int WandTracker::minArea() const
{
    return _minArea;
}

WandTracker::Plane WandTracker::_computeBest3DPlane(ofVec3f p0, ofVec3f p1, ofVec3f p2, ofVec3f p3)
{
    Plane plane = { 0.0f, 0.0, 0.0f, 1.0f }; // we enforce d = 1
    CvMat* M = cvCreateMat(4, 3, CV_32FC1);
    CvMat* v = cvCreateMat(3, 1, CV_32FC1);
    CvMat* b = cvCreateMat(4, 1, CV_32FC1);
    ofVec3f p[4] = { p0, p1, p2, p3 };

    // debug
    for(int i=0; i<4; i++) {
        ofVec2f q = _kinect->worldCoords2depthCoords(p[i]);
        std::cout << "[wandtracker] " << (i+1) << "" <<
                     (i+1 == 1 ? "st" : (i+1 == 2 ? "nd" : (i+1 == 3 ? "rd" : "th"))) << " " <<
                     "plane vertex:\n            " <<
                     "(" << p[i].x << ", " << p[i].y << ", " << p[i].z << ") => " <<
                     "(" << q.x << ", " << q.y << ")" << std::endl;
    }

    // initialize matrix M
    for(int i=0; i<4; i++) {
        CV_MAT_ELEM(*M, float, i, 0) = p[i].x;
        CV_MAT_ELEM(*M, float, i, 1) = p[i].y;
        CV_MAT_ELEM(*M, float, i, 2) = p[i].z;
    }

    // intialize vector v
    cvZero(v);

    // initialize vector b
    for(int i=0; i<4; i++)
        CV_MAT_ELEM(*b, float, i, 0) = -plane.d;

    // solve Mv = b for v (least-squares)
    cvSolve(M, b, v, CV_SVD);

    // get the plane data
    plane.a = CV_MAT_ELEM(*v, float, 0, 0);
    plane.b = CV_MAT_ELEM(*v, float, 1, 0);
    plane.c = CV_MAT_ELEM(*v, float, 2, 0);

    // done!
    cvReleaseMat(&M);
    cvReleaseMat(&v);
    cvReleaseMat(&b);
    return plane;
}



std::string WandTracker::_namefix(std::string s) const
{
    if(s.substr(0, 9) == "projetor_")
        return s.substr(9);
    else if(s == "eraser_green")
        return "marker_green";
    else
        return s;
}



bool WandTracker::_fillWandPositions(Wand* w, ofVec2f blobCenterOfMass)
{
#if 1
    ofVec3f q = _kinect->depthCoords2worldCoords(blobCenterOfMass);
    ofVec3f u = _calibration->world2norm(q);
    if(u.z >= 1 || u.z <= 0.07)
        return false;

    // position in depth-image coords
    w->position.imageCoords = blobCenterOfMass;

    // position in normalized [0,1]^3 space
    w->position.coords = u;

    // i dont know if the following computations are accurate...

    // projects the wand onto the interactive surface (gets depth-image coords)
    ofVec3f n = ofVec3f( _plane.a, _plane.b, _plane.c );
    ofVec3f p = q - ((q.dot(n) + _plane.d) / (n.length() * n.length())) * n; // projects q onto _plane
    w->position.projectedImageCoords = _kinect->worldCoords2depthCoords(p);

    // projected wand in [0,1]^3 space
    CvPoint2D32f xy2 = _bgmodel->convertToSurfaceCoordinates(cvPoint(w->position.projectedImageCoords.x, w->position.projectedImageCoords.y));
    w->position.projectedCoords = ofVec3f(xy2.x, xy2.y, 0.0f);

    return true;
#else
    // position in depth-image coords
    w->position.imageCoords = blobCenterOfMass;

    // projects the wand onto the interactive surface (gets depth-image coords)
    ofVec3f q = _kinect->depthCoords2worldCoords( w->position.imageCoords );
    ofVec3f n = ofVec3f( _plane.a, _plane.b, _plane.c );
    ofVec3f p = q - ((q.dot(n) + _plane.d) / (n.length() * n.length())) * n; // projects q onto _plane
    w->position.projectedImageCoords = _kinect->worldCoords2depthCoords(p);

    // position in [0,1]^3 space. Is this computation correct?
    //CvPoint2D32f xy = _bgmodel->convertToSurfaceCoordinates(cvPoint(w->position.imageCoords.x, w->position.imageCoords.y));
    CvPoint2D32f xy = _bgmodel->convertToSurfaceCoordinates(cvPoint(w->position.projectedImageCoords.x, w->position.projectedImageCoords.y));
    float d = q.distance(p); // d(q, _plane) = | ax + by + cz | / sqrt(a^2 + b^2 + c^2 + d^2)
    float maxHeight = 100.0f; // FIXME
    w->position.coords = ofVec3f(xy.x, xy.y, d / maxHeight);

    // projected wand in [0,1]^3 space
    CvPoint2D32f xy2 = _bgmodel->convertToSurfaceCoordinates(cvPoint(w->position.projectedImageCoords.x, w->position.projectedImageCoords.y));
    w->position.projectedCoords = ofVec3f(xy2.x, xy2.y, 0.0f);
#endif
}

void WandTracker::_fillWandSpeeds(Wand* w, const Wand& previousWand, float dt)
{
    w->speed.coords = (w->position.coords - previousWand.position.coords) / dt;
    w->speed.projectedCoords = (w->position.projectedCoords - previousWand.position.projectedCoords) / dt;
    w->speed.imageCoords = (w->position.imageCoords - previousWand.position.imageCoords) / dt;
    w->speed.projectedImageCoords = (w->position.projectedImageCoords - previousWand.position.projectedImageCoords) / dt;
}

bool WandTracker::isCalibrating() const
{
    return _isCalibrating;
}

void WandTracker::startCalibrationProcedure()
{
    _calibsample.clear();
    _isCalibrating = true;
    _calibration->reset();
}

WandCalibration* WandTracker::calibration() const
{
    return _calibration;
}

bool WandTracker::endCalibrationProcedure(std::string calibfile)
{
    _isCalibrating = false;
    if(_calibsample.size() == 0) return false;
    ofVec3f q(_calibsample[0]);
    if(calibfile == "") calibfile = DEFAULT_WANDCALIB_FILE;
    bool ret;

    // obtaining bounding box
    float xmax = 0, xmin = 1, ymax = 0, ymin = 1, zmax = 0, zmin = 1;
    for(const CalibrationSample& sample : _calibsample) {
        xmax = std::max(xmax, sample.x);
        xmin = std::min(xmin, sample.x);
        ymax = std::max(ymax, sample.y);
        ymin = std::min(ymin, sample.y);
        zmax = std::max(zmax, sample.z);
        zmin = std::min(zmin, sample.z);
    }

    // world coords
    ofVec3f topleftback(xmin, ymin, zmax);
    ofVec3f toprightback(xmax, ymin, zmax);
    ofVec3f bottomleftback(xmin, ymax, zmax);
    ofVec3f bottomrightback(xmax, ymax, zmax);
    ofVec3f topleftfront(xmin, ymin, zmin);
    ofVec3f toprightfront(xmax, ymin, zmin);
    ofVec3f bottomleftfront(xmin, ymax, zmin);
    ofVec3f bottomrightfront(xmax, ymax, zmin);

    // normalized coords
    ofVec3f ntopleftback(0, 0, 0);
    ofVec3f ntoprightback(1, 0, 0);
    ofVec3f nbottomleftback(0, 1, 0);
    ofVec3f nbottomrightback(1, 1, 0);
    ofVec3f ntopleftfront(0, 0, 1);
    ofVec3f ntoprightfront(1, 0, 1);
    ofVec3f nbottomleftfront(0, 1, 1);
    ofVec3f nbottomrightfront(1, 1, 1);

    // calibrate it!
    _calibration->load(calibfile);
    _calibration->beginCalibration();
    _calibration->addCorrespondence(ntopleftback, topleftback);
    _calibration->addCorrespondence(ntoprightback, toprightback);
    _calibration->addCorrespondence(nbottomleftback, bottomleftback);
    _calibration->addCorrespondence(nbottomrightback, bottomrightback);
    _calibration->addCorrespondence(ntopleftfront, topleftfront);
    _calibration->addCorrespondence(ntoprightfront, toprightfront);
    _calibration->addCorrespondence(nbottomleftfront, bottomleftfront);
    _calibration->addCorrespondence(nbottomrightfront, bottomrightfront);
    ret = _calibration->endCalibration();
    ret = ret && _calibration->save(calibfile);

    // done!
    return ret;
}
