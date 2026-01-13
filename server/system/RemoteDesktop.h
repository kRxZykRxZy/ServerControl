#pragma once
#include <string>
#include <vector>
#include <functional>

// Mouse button constants
enum class MouseButton {
    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,
    SCROLL_UP = 4,
    SCROLL_DOWN = 5
};

// Input event types
struct MouseEvent {
    int x;
    int y;
    MouseButton button;
    bool pressed;  // true = press, false = release
};

struct KeyEvent {
    int keycode;
    bool pressed;  // true = press, false = release
};

// Screen frame data
struct ScreenFrame {
    std::vector<unsigned char> data;  // RGB24 or JPEG data
    int width;
    int height;
    int format;  // 0 = RGB24, 1 = JPEG
    long timestamp;
};

class RemoteDesktop {
public:
    RemoteDesktop();
    ~RemoteDesktop();
    
    // Screen capture control
    bool startCapture(int fps = 30, int quality = 75);
    void stopCapture();
    bool isCapturing() const;
    
    // Get the latest frame (non-blocking)
    bool getFrame(ScreenFrame& frame);
    
    // Input injection
    bool sendMouseEvent(const MouseEvent& event);
    bool sendKeyEvent(const KeyEvent& event);
    
    // Screen info
    int getScreenWidth() const;
    int getScreenHeight() const;
    
    // Callbacks for frame ready
    void setFrameCallback(std::function<void(const ScreenFrame&)> callback);
    
private:
    class Impl;
    Impl* pImpl;
};
