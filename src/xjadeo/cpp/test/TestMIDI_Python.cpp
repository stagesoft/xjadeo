/**
 * TestMIDI_Python.cpp - MIDI implementation test using libmtcmaster Python interface
 * 
 * Alternative test using Python interface if C API is not available.
 * This uses Python C API to call libmtcmaster Python bindings.
 */

#include "sync/ALSASeqMIDIDriver.h"
#include "sync/MTCDecoder.h"
#include "utils/Logger.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>

#ifdef HAVE_PYTHON
#include <Python.h>
#endif

using namespace xjadeo;

// Test configuration
static const double TEST_FRAMERATE = 25.0;
static const int TEST_DURATION_SEC = 10;
static const int TEST_START_FRAME = 0;

#ifdef HAVE_PYTHON
/**
 * Python-based MTC generator using libmtcmaster Python interface
 */
class PythonMTCGenerator {
public:
    PythonMTCGenerator() : mtcObj_(nullptr), initialized_(false) {
        // Initialize Python if not already done
        if (!Py_IsInitialized()) {
            Py_Initialize();
        }
    }
    
    ~PythonMTCGenerator() {
        stop();
        if (mtcObj_) {
            Py_DECREF(mtcObj_);
        }
    }
    
    bool start(const std::string& portName, double framerate, int startFrame) {
        if (initialized_) {
            return false;
        }
        
        // Import mtcmaster module
        PyObject* module = PyImport_ImportModule("mtcmaster");
        if (!module) {
            PyErr_Print();
            std::cerr << "Failed to import mtcmaster module" << std::endl;
            return false;
        }
        
        // Get MtcMaster class
        PyObject* mtcClass = PyObject_GetAttrString(module, "MtcMaster");
        Py_DECREF(module);
        if (!mtcClass) {
            PyErr_Print();
            std::cerr << "Failed to get MtcMaster class" << std::endl;
            return false;
        }
        
        // Create MtcMaster instance
        PyObject* args = PyTuple_New(1);
        PyObject* portStr = PyUnicode_FromString(portName.c_str());
        PyTuple_SetItem(args, 0, portStr);
        
        mtcObj_ = PyObject_CallObject(mtcClass, args);
        Py_DECREF(args);
        Py_DECREF(mtcClass);
        
        if (!mtcObj_) {
            PyErr_Print();
            std::cerr << "Failed to create MtcMaster instance" << std::endl;
            return false;
        }
        
        // Set framerate
        PyObject* fpsObj = PyFloat_FromDouble(framerate);
        PyObject* result = PyObject_CallMethod(mtcObj_, "set_fps", "O", fpsObj);
        Py_DECREF(fpsObj);
        if (!result) {
            PyErr_Print();
            std::cerr << "Failed to set framerate" << std::endl;
            Py_DECREF(mtcObj_);
            mtcObj_ = nullptr;
            return false;
        }
        Py_DECREF(result);
        
        // Set start frame
        PyObject* frameObj = PyLong_FromLong(startFrame);
        result = PyObject_CallMethod(mtcObj_, "set_frame", "O", frameObj);
        Py_DECREF(frameObj);
        if (!result) {
            PyErr_Print();
            std::cerr << "Failed to set start frame" << std::endl;
            Py_DECREF(mtcObj_);
            mtcObj_ = nullptr;
            return false;
        }
        Py_DECREF(result);
        
        // Start MTC generation
        result = PyObject_CallMethod(mtcObj_, "start", nullptr);
        if (!result) {
            PyErr_Print();
            std::cerr << "Failed to start MTC generation" << std::endl;
            Py_DECREF(mtcObj_);
            mtcObj_ = nullptr;
            return false;
        }
        Py_DECREF(result);
        
        initialized_ = true;
        std::cout << "MTC generation started (Python) on port: " << portName << std::endl;
        std::cout << "Framerate: " << framerate << " fps, Start frame: " << startFrame << std::endl;
        return true;
    }
    
    void stop() {
        if (mtcObj_ && initialized_) {
            PyObject_CallMethod(mtcObj_, "stop", nullptr);
            initialized_ = false;
        }
    }
    
    bool isRunning() const { return initialized_; }
    
    int64_t getCurrentFrame() const {
        if (mtcObj_) {
            PyObject* result = PyObject_CallMethod(mtcObj_, "get_frame", nullptr);
            if (result) {
                int64_t frame = PyLong_AsLongLong(result);
                Py_DECREF(result);
                return frame;
            }
        }
        return -1;
    }
    
private:
    PyObject* mtcObj_;
    bool initialized_;
};
#endif

/**
 * Test MTC reception using Python interface
 */
bool test_MTC_Reception_Python() {
    std::cout << "\n=== Testing MTC Reception (Python) ===" << std::endl;
    
#ifdef HAVE_PYTHON
    PythonMTCGenerator generator;
    
    auto driver = std::make_unique<ALSASeqMIDIDriver>();
    
    if (!driver->isSupported()) {
        std::cerr << "ALSA Sequencer not supported" << std::endl;
        return false;
    }
    
    driver->setFramerate(TEST_FRAMERATE);
    driver->setVerbose(true);
    driver->setClockAdjustment(false);
    
    std::cout << "Opening ALSA MIDI driver..." << std::endl;
    if (!driver->open("-1")) {
        std::cerr << "Failed to open ALSA MIDI driver" << std::endl;
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (!driver->isConnected()) {
        std::cerr << "MIDI driver not connected. Please connect manually." << std::endl;
        driver->close();
        return false;
    }
    
    std::string mtcPort = "128:0";
    std::cout << "Starting MTC generation (Python) on port: " << mtcPort << std::endl;
    
    if (!generator.start(mtcPort, TEST_FRAMERATE, TEST_START_FRAME)) {
        std::cerr << "Failed to start MTC generation" << std::endl;
        driver->close();
        return false;
    }
    
    std::cout << "Testing MTC reception for " << TEST_DURATION_SEC << " seconds..." << std::endl;
    std::cout << std::setw(10) << "Time" << std::setw(15) << "Generated" 
              << std::setw(15) << "Received" << std::setw(15) << "Diff" << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    int64_t lastReceivedFrame = -1;
    int frameCount = 0;
    int errorCount = 0;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        if (elapsed >= TEST_DURATION_SEC) {
            break;
        }
        
        int64_t receivedFrame = driver->pollFrame();
        int64_t generatedFrame = generator.getCurrentFrame();
        
        if (receivedFrame >= 0) {
            if (receivedFrame != lastReceivedFrame) {
                frameCount++;
                int64_t diff = receivedFrame - generatedFrame;
                
                std::cout << std::setw(10) << elapsed << "s"
                          << std::setw(15) << generatedFrame
                          << std::setw(15) << receivedFrame
                          << std::setw(15) << diff << std::endl;
                
                if (std::abs(diff) > 2) {
                    errorCount++;
                    std::cerr << "Warning: Large frame difference: " << diff << std::endl;
                }
                
                lastReceivedFrame = receivedFrame;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    generator.stop();
    driver->close();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Frames received: " << frameCount << std::endl;
    std::cout << "Errors: " << errorCount << std::endl;
    
    if (frameCount == 0) {
        std::cerr << "ERROR: No frames received!" << std::endl;
        return false;
    }
    
    if (errorCount > frameCount / 10) {
        std::cerr << "ERROR: Too many frame errors!" << std::endl;
        return false;
    }
    
    std::cout << "Test PASSED" << std::endl;
    return true;
    
#else
    std::cerr << "Python support not available" << std::endl;
    return false;
#endif
}

// Include the MTCDecoder test from TestMIDI.cpp
extern bool test_MTCDecoder();

int main(int argc, char** argv) {
    std::cout << "xjadeo MIDI Test Suite (Python interface)" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    bool allPassed = true;
    
    // Test 1: MTCDecoder
    if (!test_MTCDecoder()) {
        allPassed = false;
    }
    
    // Test 2: MTC Reception using Python
    if (argc > 1 && strcmp(argv[1], "--skip-mtc") != 0) {
        if (!test_MTC_Reception_Python()) {
            allPassed = false;
        }
    }
    
    if (allPassed) {
        std::cout << "\n=== All tests PASSED ===" << std::endl;
        return 0;
    } else {
        std::cerr << "\n=== Some tests FAILED ===" << std::endl;
        return 1;
    }
}

