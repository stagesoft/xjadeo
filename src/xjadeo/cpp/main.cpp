#include "XjadeoApplication.h"
#include <iostream>

int main(int argc, char** argv) {
    xjadeo::XjadeoApplication app;
    
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    return app.run();
}

