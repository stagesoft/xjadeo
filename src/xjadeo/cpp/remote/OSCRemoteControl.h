#ifndef XJADEO_OSCREMOTECONTROL_H
#define XJADEO_OSCREMOTECONTROL_H

#include "RemoteControl.h"
#include "RemoteCommandRouter.h"
#include <memory>
#include <string>
#include <vector>

// Forward declaration for liblo types
struct lo_server_struct;
typedef struct lo_server_struct* lo_server;
struct lo_message;
typedef struct lo_message lo_message;

namespace xjadeo {

// Forward declarations
class XjadeoApplication;
class LayerManager;

/**
 * OSCRemoteControl - OSC (Open Sound Control) remote control implementation
 * 
 * Implements RemoteControl interface using liblo for OSC protocol.
 * This is the only remote control implementation for now, but the architecture
 * is ready for future implementations (MessageQueue, IPC, etc.).
 */
class OSCRemoteControl : public RemoteControl {
public:
    OSCRemoteControl(XjadeoApplication* app, LayerManager* layerManager);
    virtual ~OSCRemoteControl();

    // RemoteControl interface
    bool initialize(int port) override;
    int process() override;
    void shutdown() override;
    bool isActive() const override;
    const char* getProtocolName() const override { return "OSC"; }

private:
    // OSC server
    lo_server oscServer_;
    std::unique_ptr<RemoteCommandRouter> router_;

    // OSC user data (for callbacks)
    struct OSCUserData {
        OSCRemoteControl* instance;
    };
    OSCUserData* userData_;

    // OSC message handlers (static callbacks that forward to instance)
    static int handleOSCMessage(const char* path, const char* types, 
                                 lo_arg** argv, int argc, lo_message msg, 
                                 void* userData);
    
    // Convert OSC arguments to string vector
    std::vector<std::string> convertOSCArgs(const char* types, lo_arg** argv, int argc);

    // Port
    int port_;
    bool active_;
};

} // namespace xjadeo

#endif // XJADEO_OSCREMOTECONTROL_H

