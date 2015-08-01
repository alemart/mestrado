#include <iostream>
#include <cstdlib>
#include "WallOS.h"
#include "ofAppGlutWindow.h"

int main(int argc, char* argv[])
{
    if(argc == 1 || argc == 2) {
        ofAppGlutWindow window;
        ofSetupOpenGL(&window, 1280, 1024, OF_WINDOW);//FULLSCREEN);
        ofRunApp(new WallOS(argc == 2 ? atoi(argv[2]) : 3333));
    }
    else {
        std::cerr << "Usage: " << argv[0] << " [port = 3333]" << std::endl;
        std::cerr << " e.g.: " << argv[0] << " 3333" << std::endl;
    }

    return 0;
}
