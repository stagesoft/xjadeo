#include "OSCRemoteControl.h"
#include "../XjadeoApplication.h"
#include "../layer/LayerManager.h"
#include <cstring>
#include <cstdio>
#include <sstream>

extern "C" {
#include <lo/lo_lowlevel.h>
}

namespace xjadeo {

static void oscErrorHandler(int num, const char* msg, const char* path) {
    fprintf(stderr, "liblo server error %d in path %s: %s\n", num, path ? path : "unknown", msg);
}

OSCRemoteControl::OSCRemoteControl(XjadeoApplication* app, LayerManager* layerManager)
    : oscServer_(nullptr)
    , router_(std::make_unique<RemoteCommandRouter>(app, layerManager))
    , userData_(nullptr)
    , port_(7000)
    , active_(false)
{
}

OSCRemoteControl::~OSCRemoteControl() {
    shutdown();
}

bool OSCRemoteControl::initialize(int port) {
    if (active_) {
        shutdown();
    }

    port_ = (port > 100 && port < 60000) ? port : 7000;

    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", port_);

    oscServer_ = lo_server_new(portStr, oscErrorHandler);
    if (!oscServer_) {
        return false;
    }

    // Register OSC message handler
    userData_ = new OSCUserData;
    userData_->instance = this;

    // Register catch-all handler
    lo_server_add_method(oscServer_, nullptr, nullptr, handleOSCMessage, userData_);

    // Register specific methods for compatibility with existing OSC interface
    lo_server_add_method(oscServer_, "/jadeo/quit", "", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/load", "s", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/seek", "i", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/fps", "f", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/offset", "i", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/offset", "s", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/midi/connect", "s", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/midi/disconnect", "", handleOSCMessage, userData_);

    // Layer-level commands
    lo_server_add_method(oscServer_, "/jadeo/layer/add", "s", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/remove", "i", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/duplicate", "i", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/reorder", "is", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/list", "", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/remove", "", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/seek", "i", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/play", "", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/pause", "", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/position", "ii", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/opacity", "f", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/visible", "i", handleOSCMessage, userData_);
    lo_server_add_method(oscServer_, "/jadeo/layer/*/zorder", "i", handleOSCMessage, userData_);

    active_ = true;
    return true;
}

int OSCRemoteControl::process() {
    if (!active_ || !oscServer_) {
        return 0;
    }

    int count = 0;
    while (lo_server_recv_noblock(oscServer_, 0) > 0) {
        count++;
    }
    return count;
}

void OSCRemoteControl::shutdown() {
    if (oscServer_) {
        lo_server_free(oscServer_);
        oscServer_ = nullptr;
    }
    if (userData_) {
        delete userData_;
        userData_ = nullptr;
    }
    active_ = false;
}

bool OSCRemoteControl::isActive() const {
    return active_ && oscServer_ != nullptr;
}

int OSCRemoteControl::handleOSCMessage(const char* path, const char* types,
                                       lo_arg** argv, int argc, lo_message msg,
                                       void* userData) {
    OSCUserData* data = static_cast<OSCUserData*>(userData);
    if (!data || !data->instance) {
        return 1;
    }

    // Convert OSC arguments to string vector
    std::vector<std::string> args = data->instance->convertOSCArgs(types, argv, argc);

    // Route command
    std::string pathStr(path);
    bool handled = data->instance->router_->routeCommand(pathStr, args);

    return handled ? 0 : 1;
}

std::vector<std::string> OSCRemoteControl::convertOSCArgs(const char* types, lo_arg** argv, int argc) {
    std::vector<std::string> args;
    args.reserve(argc);

    for (int i = 0; i < argc && types[i] != '\0'; ++i) {
        std::ostringstream oss;
        switch (types[i]) {
            case 'i':
                oss << argv[i]->i;
                break;
            case 'f':
                oss << argv[i]->f;
                break;
            case 's':
                oss << &argv[i]->s;
                break;
            case 'd':
                oss << argv[i]->d;
                break;
            case 'h':
                oss << argv[i]->h;
                break;
            case 't':
                oss << argv[i]->t.sec << "." << argv[i]->t.frac;
                break;
            default:
                // Unknown type, skip
                continue;
        }
        args.push_back(oss.str());
    }

    return args;
}

} // namespace xjadeo

