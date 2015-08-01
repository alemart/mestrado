#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <stack>
#include "Canvas.h"
#include "../WallOS.h"

Canvas::Canvas(WallOS* system) : WallApp(system, "Canvas")
{
}

Canvas::~Canvas()
{
    _saveToDisk();
}

void Canvas::setup()
{
    _loadFromDisk();
}

void Canvas::update()
{
    for(TUIO::TuioObject* t : touchPoints()) {
        std::cout << "sessid = " << t->getSessionID() << " [type: " << t->getSymbolID() << "]" << std::endl;
        if(_isMarker(t->getSymbolID())) {
            // add a new point
            Point p;
            p.position = ofVec2f(t->getX(), t->getY());
            p.color = _touchType2color(t->getSymbolID());
            p.tuioSessionId = t->getSessionID();
            p.next = -1;
            _points.push_back(p);

            // connect it to the correct path
            int prev = _findIndexOfPreviousPointHavingIndex(_points.size() - 1);
            if(prev >= 0) {
                Point& q = _points[prev];
                if(q.position.distance(p.position) < 0.2) // noise?
                    q.next = _points.size() - 1;
            }
        }
        else if(t->getSymbolID() == TouchType.ERASER) {
            EraserSpot es;
            es.position = ofVec2f(t->getX(), t->getY());
            es.tuioSessionId = t->getSessionID();
            _eraserSpots.push_back(es);
        }
        else if(t->getSymbolID() == TouchType.FINGER) {
        }
    }
}

void Canvas::draw()
{
    std::priority_queue<Renderer*, std::vector<Renderer*>, RendererComparator> pq;

    // schedule curves
    std::map<int,bool> visited;
    for(int k=0; k<_points.size(); k++) {
        if(!visited[k]) {
            std::vector<ofVec2f> v;
            ofColor c = _points[k].color;
            int z = _points[k].tuioSessionId;

            std::stack<int> s;
            s.push(k);
            while(!s.empty()) {
                int top = s.top();
                Point& p = _points[top];
                s.pop();
                if(!visited[top]) {
                    v.push_back(p.position);
                    if(p.next > 0 && !visited[p.next])
                        s.push(p.next);
                    visited[top] = true;
                    z = p.tuioSessionId;
                }
            }

            pq.push(new CurveRenderer(z, v, c));
        }
    }

    // schedule erasers
    for(int k=0; k<_eraserSpots.size(); k++) {
        EraserSpot& es = _eraserSpots[k];
        pq.push(new EraserRenderer(es.tuioSessionId, es.position, 110));
    }

    // render everything
    while(!pq.empty()) {
        Renderer* r = pq.top();
        pq.pop();
        r->render(system(), _camera);
        delete r;
    }
}



// ===================

const char* Canvas::_filepath = "data/canvas.txt";

void Canvas::_loadFromDisk()
{
/*
    std::ifstream f(_filepath);
    if(f.is_open()) {
        float x, y;
        std::string line;

        // camera
        std::getline(f, line);
        std::stringstream ss(line);
        ss >> x >> y;
        _camera.position = ofVec2f(x, y);

        // blank line
        std::getline(f, line);
        if(line != "")
            std::cout << "[app.Canvas] WARNING: invalid format for " << _filepath << std::endl;

        // load the points
        while(std::getline(f, line) && line != "") {
            unsigned int r, g, b;
            std::stringstream ss(line);
//std::cout << "[] " << ss.str() << std::endl;
            ss >> r >> g >> b; // read color of a path
//std::cout << "read color " << r << "," << g << "," << b << std::endl;

            Point p, *last = 0;
            while(std::getline(f, line) && line != "") {
                float x, y;
                std::stringstream ss(line);
//std::cout << "{} " << ss.str() << std::endl;
                ss >> x >> y; // read position of a point
//std::cout << "read position " << x << "," << y << std::endl;

                p.position = ofVec2f(x, y);
                p.color = ofColor(r, g, b);
                p.next = 0;
                p.previous = last;
                p.tuioSessionId = -1;

                _points.push_back(p);
                if(last)
                    last->next = &_points[_points.size()-1];
                last = &_points[_points.size()-1];
            }
        }
    }
    else
        std::cout << "[app.Canvas] WARNING: can't read from " << _filepath << std::endl;
*/
}

void Canvas::_saveToDisk()
{
/*
    std::ofstream f(_filepath);
    if(f.is_open()) {
        // save camera
        f << float(_camera.position.x) << " " << float(_camera.position.y) << "\n\n";

        // save the points
        for(int i=0; i<int(_points.size()); i++) {
            Point* p = &_points[i];
std::cout << "writing pt " << i << std::endl;
            if(1 || !p->previous) { // first point in a path
                f << int(p->color.r) << " " << int(p->color.g) << " " << int(p->color.b) << "\n";
                std::cout << int(p->color.r) << " " << int(p->color.g) << " " << int(p->color.b) << "\n";
                do {
                    f << float(p->position.x) << " " << float(p->position.y) << "\n";
                    std::cout << float(p->position.x) << " " << float(p->position.y) << "\n";
                } while(0 != (p = p->next));
                f << "\n";
            }
        }
    }
    else
        std::cout << "[app.Canvas] WARNING: can't write to " << _filepath << std::endl;
*/
}

int Canvas::_findIndexOfPreviousPointHavingIndex(int idx)
{
    Point& p = _points[idx];
    int bestmatch = -1;

    for(int k = idx - 1; k >= 0; k--) {
        Point& q = _points[k];
        if(q.tuioSessionId == p.tuioSessionId && q.color == p.color) {
            bestmatch = k;
            break;
        }
    }

    return bestmatch;
}

ofColor Canvas::_touchType2color(int touchType) const
{
    switch(touchType) {
    case TouchType.MARKER_BLUE:
        return ofColor::fromHex(0x44AAFF);

    case TouchType.MARKER_GREEN:
        return ofColor::fromHex(0x33FF33);

    case TouchType.MARKER_YELLOW:
        return ofColor::fromHex(0xFFFF77);

    case TouchType.MARKER_MAGENTA:
        return ofColor::fromHex(0xFF0099);

    default:
        return ofColor::fromHex(0xFFFFFF);
    };
}

bool Canvas::_isMarker(int touchType) const
{
    return
        touchType == TouchType.MARKER_BLUE ||
        touchType == TouchType.MARKER_GREEN ||
        touchType == TouchType.MARKER_YELLOW ||
        touchType == TouchType.MARKER_MAGENTA
    ;
}


// ===========================================

void Canvas::CurveRenderer::render(WallOS* system, const Canvas::Camera& cam)
{
    ofPolyline line;
    ofSetLineWidth(10.0f);
    ofSetColor(_color);

    for(int k=0; k<_points.size(); k++) {
        ofVec2f transformedPosition = system->norm2win(_points[k] - cam.position);
        line.curveTo(transformedPosition);
    }

    line.draw();
}

void Canvas::EraserRenderer::render(WallOS* system, const Canvas::Camera& cam)
{
    ofSetColor(0, 0, 0, 32);
    ofVec2f position = system->norm2win(_spot - cam.position);
    ofCircle(position.x, position.y, _size);
}
