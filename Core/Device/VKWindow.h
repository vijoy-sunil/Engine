#ifndef VK_WINDOW_H
#define VK_WINDOW_H

#include "VKDeviceMgr.h"

namespace Core {
    typedef enum {
        CURSOR_POSITION = 0,
        SCROLL_OFFSET   = 1
    } e_mouseEventType;

    class VKWindow: protected virtual VKDeviceMgr {
        private:
            struct KeyEventInfo {
                struct Meta {
                    bool pressed;
                    /* std::function is an example of a broader concept called type erasure. For example,
                     * std::function <void (void)> represents any callable that can be invoked with no arguments. It
                     * could be a function pointer or a function object that has a concrete type, or a closure built
                     * from a lambda. It doesn't matter what the source type is, as long as it fits the contract - it
                     * just works. Instead of using the concrete source type, we "erase" it - and we just deal with
                     * std::function
                    */
                    std::function <void (float)> binding;
                    std::chrono::steady_clock::time_point captureTime;
                } meta;

                struct Parameters {
                    int action;
                    int mods;
                } params;
            };
            std::unordered_map <int, KeyEventInfo> m_keyEventInfoPool;

            struct MouseEventInfo {
                struct Meta {
                    std::function <void (double, double)> binding;
                } meta;
            };
            std::unordered_map <e_mouseEventType, MouseEventInfo> m_mouseEventInfoPool;

            /* Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize,
             * it is not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly
             * using this boolean
            */
            bool m_frameBufferResized;
            bool m_windowIconified;

            Log::Record* m_VKWindowLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            /* The reason that we're creating a static function as a callback is because GLFW does not know how to
             * properly call a member function with the right 'this' pointer to our VKWindow class instance. However,
             * we do get a reference to the GLFWwindow in the callback and glfwSetWindowUserPointer function allows you
             * to store an arbitrary pointer inside of it. The 'this' pointer can then be used to properly set the
             * boolean to indicate that a resize has happened
            */
            static void frameBufferResizeCallback (GLFWwindow* window, int width, int height) {
                /* Suppress unused parameter warning
                */
                static_cast <void> (width);
                static_cast <void> (height);
                auto thisPtr = reinterpret_cast <VKWindow*> (glfwGetWindowUserPointer (window));
                thisPtr->setFrameBufferResized (true);
            }

            static void windowIconifyCallback (GLFWwindow* window, int iconified) {
                auto thisPtr =  reinterpret_cast <VKWindow*> (glfwGetWindowUserPointer (window));
                if (iconified)  thisPtr->m_windowIconified = true;
                else            thisPtr->m_windowIconified = false;
            }

        public:
            VKWindow (void) {
                m_frameBufferResized = false;
                m_windowIconified    = false;
                m_VKWindowLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
            }

            ~VKWindow (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* GLFW provides many kinds of input. While some can only be polled, like time, or only received via
             * callbacks, like scrolling, many provide both callbacks and polling. Callbacks are more work to use than
             * polling but is less CPU intensive and guarantees that you do not miss state changes
             *
             * If you wish to be notified when a physical key is pressed or released or when it repeats, set a key
             * callback. The callback function receives the keyboard key, platform-specific scancode, key action and
             * modifier bits
             *
             * The action is one of GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE. Events with GLFW_PRESS and GLFW_RELEASE
             * actions are emitted for every key press. Most keys will also emit events with GLFW_REPEAT actions while a
             * key is held down
             *
             * The scancode is unique for every key, regardless of whether it has a key token. Scancodes are platform-
             * specific but consistent over time, so keys will have different scancodes depending on the platform but
             * they are safe to save to disk. You can query the scancode for any key token supported on the current
             * platform with glfwGetKeyScancode. For example,
             *
             *      const int scancode = glfwGetKeyScancode (GLFW_KEY_X);
             *      set_key_mapping (scancode, swap_weapons);
             *
             * If you wish to know what the state of the Caps Lock and Num Lock keys was when input events were generated,
             * set the GLFW_LOCK_KEY_MODS input mode using
             *
             *      glfwSetInputMode (window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
             *
             * When this input mode is enabled, any callback that receives modifier bits will have the GLFW_MOD_CAPS_LOCK
             * bit set if Caps Lock was on when the event occurred and the GLFW_MOD_NUM_LOCK bit set if Num Lock was on
            */
            static void keyCallback (GLFWwindow* window, int key, int scanCode, int action, int mods) {
                static_cast <void>  (scanCode);
                auto thisPtr = reinterpret_cast <VKWindow*> (glfwGetWindowUserPointer (window));
                /* Do not save event info if the key doesn't exist in pool
                */
                if (thisPtr->m_keyEventInfoPool.find (key) == thisPtr->m_keyEventInfoPool.end())
                    return;

                bool pressed = thisPtr->m_keyEventInfoPool[key].meta.pressed;

                if (action == GLFW_PRESS && !pressed) {
                    thisPtr->m_keyEventInfoPool[key].meta.pressed     = true;
                    thisPtr->m_keyEventInfoPool[key].meta.captureTime = std::chrono::high_resolution_clock::now();
                    thisPtr->m_keyEventInfoPool[key].params           = {action, mods};
                }
                if (action == GLFW_RELEASE) {
                    thisPtr->m_keyEventInfoPool[key].meta.pressed     = false;
                    thisPtr->m_keyEventInfoPool[key].meta.captureTime = std::chrono::high_resolution_clock::now();
                    thisPtr->m_keyEventInfoPool[key].params           = {action, mods};
                }
            }

            /* If you wish to be notified when the cursor moves over the window, set a cursor position callback. The
             * callback functions receives the cursor position, measured in screen coordinates but relative to the
             * top-left corner of the window content area. On platforms that provide it, the full sub-pixel cursor
             * position is passed on
             *
             * Note that, as of now on Windows the callback performs as expected where once the mouse leaves the window's
             * area the callback stops firing. For OSX the window never loses focus and therefore the cursor callback is
             * always being called
            */
            static void cursorPositionCallback (GLFWwindow* window, double xPos, double yPos) {
                auto thisPtr = reinterpret_cast <VKWindow*> (glfwGetWindowUserPointer (window));
                if (thisPtr->m_mouseEventInfoPool.find (CURSOR_POSITION) == thisPtr->m_mouseEventInfoPool.end())
                    return;

                thisPtr->m_mouseEventInfoPool[CURSOR_POSITION].meta.binding (xPos, yPos);
            }

            /* If you wish to be notified when the user scrolls, whether with a mouse wheel or touchpad gesture, set a
             * scroll callback. The callback function receives two-dimensional scroll offsets. Note that, a normal mouse
             * wheel, being vertical, provides offsets along the Y-axis
            */
            static void scrollOffsetCallback (GLFWwindow* window, double xOffset, double yOffset) {
                auto thisPtr = reinterpret_cast <VKWindow*> (glfwGetWindowUserPointer (window));
                if (thisPtr->m_mouseEventInfoPool.find (SCROLL_OFFSET) == thisPtr->m_mouseEventInfoPool.end())
                    return;

                thisPtr->m_mouseEventInfoPool[SCROLL_OFFSET].meta.binding (xOffset, yOffset);
            }

            std::unordered_map <int, KeyEventInfo>& getKeyEventInfoPool (void) {
                return m_keyEventInfoPool;
            }

            std::unordered_map <e_mouseEventType, MouseEventInfo>& getMouseEventInfoPool (void) {
                return m_mouseEventInfoPool;
            }

            void setFrameBufferResized (bool val) {
                m_frameBufferResized = val;
            }

            bool isFrameBufferResized (void) {
                return m_frameBufferResized;
            }

            bool isWindowIconified (void) {
                return m_windowIconified;
            }

            void createWindow (uint32_t deviceInfoId, int width, int height, bool enResizing = true) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);
                /* First, initialize the GLFW library. Because GLFW was originally designed to create an OpenGL context,
                 * we need to tell it to not create an OpenGL context with a subsequent call
                */
                glfwInit();
                glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
                if (!enResizing)
                    glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

                /* Create window, note that the fourth parameter allows you to optionally specify a monitor to open the
                 * window on and the last parameter is only relevant to OpenGL
                */
                std::string windowTitle = g_windowSettings.titlePrefix + std::to_string (deviceInfoId);
                GLFWwindow *window = glfwCreateWindow (width,
                                                       height,
                                                       windowTitle.c_str(),
                                                       NULL,
                                                       NULL);
                /* Set user pointer of window, this pointer is used in the callback function
                */
                glfwSetWindowUserPointer       (window, this);
                glfwSetFramebufferSizeCallback (window, frameBufferResizeCallback);
                glfwSetWindowIconifyCallback   (window, windowIconifyCallback);
                deviceInfo->resource.window = window;
            }

            void cleanUp (uint32_t deviceInfoId) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);

                getKeyEventInfoPool().  clear();
                getMouseEventInfoPool().clear();
                /* Once the window is closed, we need to clean up resources by destroying it and terminating GLFW itself
                */
                glfwDestroyWindow (deviceInfo->resource.window);
                glfwTerminate();
            }
    };
}   // namespace Core
#endif  // VK_WINDOW_H