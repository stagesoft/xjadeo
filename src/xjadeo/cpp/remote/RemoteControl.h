#ifndef XJADEO_REMOTECONTROL_H
#define XJADEO_REMOTECONTROL_H

#include <string>

namespace xjadeo {

/**
 * RemoteControl - Abstract base class for remote control protocols
 * 
 * Interface for all remote control protocols (OSC, MessageQueue, IPC, etc.)
 * Only OSCRemoteControl is implemented for now, but architecture is ready
 * for future implementations.
 */
class RemoteControl {
public:
    virtual ~RemoteControl() = default;

    /**
     * Initialize remote control
     * @param port Port number or identifier (protocol-specific)
     * @return true on success, false on failure
     */
    virtual bool initialize(int port) = 0;

    /**
     * Process incoming messages
     * Should be called regularly from main loop
     * @return Number of messages processed
     */
    virtual int process() = 0;

    /**
     * Shutdown remote control
     */
    virtual void shutdown() = 0;

    /**
     * Check if remote control is active
     * @return true if active, false otherwise
     */
    virtual bool isActive() const = 0;

    /**
     * Get protocol name
     * @return String identifier (e.g., "OSC", "MQ", "IPC")
     */
    virtual const char* getProtocolName() const = 0;
};

} // namespace xjadeo

#endif // XJADEO_REMOTECONTROL_H

