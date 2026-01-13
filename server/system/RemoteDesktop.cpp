#include "RemoteDesktop.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>

class RemoteDesktop::Impl {
public:
    Display* display;
    Window root;
    XImage* image;
    std::thread capture_thread;
    std::atomic<bool> capturing;
    std::mutex frame_mutex;
    ScreenFrame current_frame;
    std::function<void(const ScreenFrame&)> frame_callback;
    int fps;
    int quality;
    
    Impl() : display(nullptr), root(0), image(nullptr), capturing(false), fps(30), quality(75) {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Failed to open X display" << std::endl;
            return;
        }
        root = DefaultRootWindow(display);
    }
    
    ~Impl() {
        stopCapture();
        if (image) {
            XDestroyImage(image);
        }
        if (display) {
            XCloseDisplay(display);
        }
    }
    
    void captureLoop() {
        auto delay = std::chrono::milliseconds(1000 / fps);
        
        while (capturing) {
            auto start = std::chrono::steady_clock::now();
            
            captureFrame();
            
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto sleep_time = delay - elapsed;
            if (sleep_time.count() > 0) {
                std::this_thread::sleep_for(sleep_time);
            }
        }
    }
    
    void captureFrame() {
        if (!display) return;
        
        XWindowAttributes attrs;
        XGetWindowAttributes(display, root, &attrs);
        
        int width = attrs.width;
        int height = attrs.height;
        
        // Capture the screen
        XImage* img = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
        if (!img) return;
        
        // Convert to RGB24
        std::lock_guard<std::mutex> lock(frame_mutex);
        current_frame.width = width;
        current_frame.height = height;
        current_frame.format = 0;  // RGB24
        current_frame.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Allocate buffer for RGB24 data
        size_t buffer_size = width * height * 3;
        current_frame.data.resize(buffer_size);
        
        // Convert XImage to RGB24
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned long pixel = XGetPixel(img, x, y);
                int idx = (y * width + x) * 3;
                
                // Extract RGB components (assuming 24-bit or 32-bit color)
                current_frame.data[idx + 0] = (pixel >> 16) & 0xFF;  // R
                current_frame.data[idx + 1] = (pixel >> 8) & 0xFF;   // G
                current_frame.data[idx + 2] = pixel & 0xFF;           // B
            }
        }
        
        XDestroyImage(img);
        
        // Call callback if set
        if (frame_callback) {
            frame_callback(current_frame);
        }
    }
    
    void stopCapture() {
        if (capturing) {
            capturing = false;
            if (capture_thread.joinable()) {
                capture_thread.join();
            }
        }
    }
};

RemoteDesktop::RemoteDesktop() : pImpl(new Impl()) {}

RemoteDesktop::~RemoteDesktop() {
    delete pImpl;
}

bool RemoteDesktop::startCapture(int fps, int quality) {
    if (!pImpl->display) {
        std::cerr << "X11 display not available" << std::endl;
        return false;
    }
    
    if (pImpl->capturing) {
        return true;  // Already capturing
    }
    
    pImpl->fps = fps;
    pImpl->quality = quality;
    pImpl->capturing = true;
    pImpl->capture_thread = std::thread(&Impl::captureLoop, pImpl);
    
    return true;
}

void RemoteDesktop::stopCapture() {
    pImpl->stopCapture();
}

bool RemoteDesktop::isCapturing() const {
    return pImpl->capturing;
}

bool RemoteDesktop::getFrame(ScreenFrame& frame) {
    std::lock_guard<std::mutex> lock(pImpl->frame_mutex);
    if (pImpl->current_frame.data.empty()) {
        return false;
    }
    frame = pImpl->current_frame;
    return true;
}

bool RemoteDesktop::sendMouseEvent(const MouseEvent& event) {
    if (!pImpl->display) return false;
    
    // Move mouse
    XTestFakeMotionEvent(pImpl->display, -1, event.x, event.y, 0);
    
    // Button press/release
    if (event.pressed) {
        XTestFakeButtonEvent(pImpl->display, static_cast<int>(event.button), True, 0);
    } else {
        XTestFakeButtonEvent(pImpl->display, static_cast<int>(event.button), False, 0);
    }
    
    XFlush(pImpl->display);
    return true;
}

bool RemoteDesktop::sendKeyEvent(const KeyEvent& event) {
    if (!pImpl->display) return false;
    
    XTestFakeKeyEvent(pImpl->display, event.keycode, event.pressed ? True : False, 0);
    XFlush(pImpl->display);
    return true;
}

int RemoteDesktop::getScreenWidth() const {
    if (!pImpl->display) return 0;
    
    XWindowAttributes attrs;
    XGetWindowAttributes(pImpl->display, pImpl->root, &attrs);
    return attrs.width;
}

int RemoteDesktop::getScreenHeight() const {
    if (!pImpl->display) return 0;
    
    XWindowAttributes attrs;
    XGetWindowAttributes(pImpl->display, pImpl->root, &attrs);
    return attrs.height;
}

void RemoteDesktop::setFrameCallback(std::function<void(const ScreenFrame&)> callback) {
    pImpl->frame_callback = callback;
}
