#ifndef XJADEO_OSDMANAGER_H
#define XJADEO_OSDMANAGER_H

#include <string>
#include <cstdint>

namespace xjadeo {

/**
 * OSDManager - Manages On-Screen Display state and configuration
 * 
 * Handles OSD modes, text, positions, and rendering settings.
 * This is a concrete class (not abstract) as OSD is a fixed component.
 */
class OSDManager {
public:
    // OSD mode flags
    enum Mode {
        NONE     = 0x0000,
        FRAME    = 0x0001,  // Display frame number
        SMPTE    = 0x0002,  // Display SMPTE timecode
        VTC      = 0x0200,  // Display video timecode
        TEXT     = 0x0040,  // Display custom text
        BOX      = 0x0100,  // Draw black box background
        MSG      = 0x0080,  // Display message
        NFO      = 0x0400,  // Display file info
        POS      = 0x1000,  // Display position
        GEO      = 0x2000   // Display geometry
    };

    OSDManager();
    ~OSDManager();

    // Mode control
    void setMode(int mode) { mode_ = mode; }
    int getMode() const { return mode_; }
    void enableMode(Mode flag) { mode_ |= flag; }
    void disableMode(Mode flag) { mode_ &= ~flag; }
    bool isModeEnabled(Mode flag) const { return (mode_ & flag) != 0; }

    // Frame number display
    void setFramePosition(int xAlign, int yPercent);
    int getFrameXAlign() const { return frameXAlign_; }
    int getFrameYPercent() const { return frameYPercent_; }
    void setFrameNumber(int64_t frame);
    std::string getFrameText() const { return frameText_; }

    // SMPTE timecode display
    void setSMPTEPosition(int xAlign, int yPercent);
    int getSMPTEXAlign() const { return smpteXAlign_; }
    int getSMPTEYPercent() const { return smpteYPercent_; }
    void setSMPTETimecode(const std::string& tc);
    std::string getSMPTETimecode() const { return smpteText_; }

    // Custom text display
    void setText(const std::string& text);
    std::string getText() const { return text_; }
    void setTextPosition(int xAlign, int yPercent);
    int getTextXAlign() const { return textXAlign_; }
    int getTextYPercent() const { return textYPercent_; }

    // Font configuration
    void setFontFile(const std::string& fontFile);
    std::string getFontFile() const { return fontFile_; }

    // Message display (temporary)
    void setMessage(const std::string& msg);
    std::string getMessage() const { return message_; }

    // Clear all OSD
    void clear();

private:
    int mode_;
    
    // Frame number display
    int frameXAlign_;  // 0=left, 1=center, 2=right
    int frameYPercent_; // 0-100
    std::string frameText_;
    
    // SMPTE timecode display
    int smpteXAlign_;
    int smpteYPercent_;
    std::string smpteText_;
    
    // Custom text display
    std::string text_;
    int textXAlign_;
    int textYPercent_;
    
    // Font
    std::string fontFile_;
    
    // Temporary message
    std::string message_;
    
    // Helper to format frame number
    void formatFrameNumber(int64_t frame);
    
    // Helper to format SMPTE timecode
    void formatSMPTE(int64_t frame, double framerate);
};

} // namespace xjadeo

#endif // XJADEO_OSDMANAGER_H

