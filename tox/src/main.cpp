//
//      _____              
//      __  /__________  __
//      _  __/  __ \_  |/_/
//      / /_ / /_/ /_>  <  
//      \__/ \____//_/|_|  
//
//     Multitouch surfaces
//     <powered by Kinect>
//
//
// by Alexandre Martins <alemart@ime.usp.br>
// latin.ime.usp.br, 2013
//

#include "ofMain.h"
#include "ofAppGlutWindow.h"
#include "core/MyApplication.h"

int main(int argc, char* argv[])
{
    bool fs = false; // gambiarra p/ fullscreen
    if(argc > 1 && std::string(argv[argc-1]) == "--fullscreen") {
        fs = true;
        argc--;
    }

    ofAppGlutWindow window;
    window.setGlutDisplayString("rgba single samples>=4 depth");
    //ofSetupOpenGL(&window, 1280, 960, fs ? OF_FULLSCREEN : OF_WINDOW);
    ofSetupOpenGL(&window, 1024, 768, fs ? OF_FULLSCREEN : OF_WINDOW);

    ofRunApp(new MyApplication(argc, argv));
    return 0;
}
