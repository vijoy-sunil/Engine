#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <GLFW/glfw3.h>
#include <map>

namespace Utils {
    typedef enum {
        CURSOR_POSITION = 0,
        SCROLL_OFFSET   = 1
    } e_mouseEventType;

    class UserInput {
        private:
            struct KeyEventInfo {
                struct Meta {
                    bool isPressed;
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
            static void keyCallBack (GLFWwindow* window, int key, int scanCode, int action, int mods) {
                static_cast <void>  (scanCode);
                auto thisPtr = reinterpret_cast <UserInput*> (glfwGetWindowUserPointer (window));
                /* Do not save event info if the key doesn't exist in pool
                */
                if (thisPtr->m_keyEventInfoPool.find (key) == thisPtr->m_keyEventInfoPool.end())
                    return;

                bool isPressed = thisPtr->m_keyEventInfoPool[key].meta.isPressed;

                if (action == GLFW_PRESS && !isPressed) {
                    thisPtr->m_keyEventInfoPool[key].meta.isPressed   = true;
                    thisPtr->m_keyEventInfoPool[key].meta.captureTime = std::chrono::high_resolution_clock::now();
                    thisPtr->m_keyEventInfoPool[key].params           = {action, mods};
                }
                if (action == GLFW_RELEASE) {
                    thisPtr->m_keyEventInfoPool[key].meta.isPressed   = false;
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
            static void cursorPosCallBack (GLFWwindow* window, double xPos, double yPos) {
                auto thisPtr = reinterpret_cast <UserInput*> (glfwGetWindowUserPointer (window));
                if (thisPtr->m_mouseEventInfoPool.find (CURSOR_POSITION) == thisPtr->m_mouseEventInfoPool.end())
                    return;

                thisPtr->m_mouseEventInfoPool[CURSOR_POSITION].meta.binding (xPos, yPos);
            }

            /* If you wish to be notified when the user scrolls, whether with a mouse wheel or touchpad gesture, set a 
             * scroll callback. The callback function receives two-dimensional scroll offsets. Note that, a normal mouse
             * wheel, being vertical, provides offsets along the Y-axis
            */
            static void scrollCallBack (GLFWwindow* window, double xOffset, double yOffset) {
                auto thisPtr = reinterpret_cast <UserInput*> (glfwGetWindowUserPointer (window));
                if (thisPtr->m_mouseEventInfoPool.find (SCROLL_OFFSET) == thisPtr->m_mouseEventInfoPool.end())
                    return;

                thisPtr->m_mouseEventInfoPool[SCROLL_OFFSET].meta.binding (xOffset, yOffset);
            }

        public:
            UserInput (void) {
            }

            ~UserInput (void) {
            }

        protected:
            void readyKeyCallBack             (GLFWwindow* window) {
                glfwSetWindowUserPointer      (window, this);
                glfwSetKeyCallback            (window, keyCallBack);
            }

            void deleteKeyCallBack            (GLFWwindow* window) {
                glfwSetKeyCallback            (window, NULL);
            }

            void readyCursorPositionCallBack  (GLFWwindow* window) {
                glfwSetWindowUserPointer      (window, this);
                glfwSetCursorPosCallback      (window, cursorPosCallBack);
                /* GLFW_CURSOR_DISABLED hides and grabs the cursor, providing virtual and unlimited cursor movement
                */
                glfwSetInputMode              (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            void deleteCursorPositionCallBack (GLFWwindow* window) {
                glfwSetCursorPosCallback      (window, NULL);
                glfwSetInputMode              (window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            void readyScrollOffsetCallBack    (GLFWwindow* window) {
                glfwSetWindowUserPointer      (window, this);
                glfwSetScrollCallback         (window, scrollCallBack);
                glfwSetInputMode              (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            void deleteScrollOffsetCallBack   (GLFWwindow* window) {
                glfwSetScrollCallback         (window, NULL);
                glfwSetInputMode              (window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            void createKeyEventBinding (int key, const std::function <void (float)>& binding) {
                m_keyEventInfoPool[key].meta.binding = binding;
            }

            void createMouseEventBinding (e_mouseEventType type, const std::function <void (double, double)>& binding) {
                m_mouseEventInfoPool[type].meta.binding = binding;
            }

            void handleKeyEvents (std::chrono::steady_clock::time_point currentTime) {
                for (auto const& [key, info]: m_keyEventInfoPool) {
                    if (info.meta.isPressed) {
                        float deltaTime = std::chrono::duration <float, std::chrono::seconds::period> 
                                          (currentTime - info.meta.captureTime).count();
                        info.meta.binding (deltaTime);
                    }
                }
            }

            void deleteKeyEventBinding (int key) {
                if (m_keyEventInfoPool.find  (key) != m_keyEventInfoPool.end())
                    m_keyEventInfoPool.erase (key);
            }

            void deleteMouseEventBinding (e_mouseEventType type) {
                if (m_mouseEventInfoPool.find  (type) != m_mouseEventInfoPool.end())
                    m_mouseEventInfoPool.erase (type);
            }

            void cleanUp (GLFWwindow* window) {
                m_keyEventInfoPool.  clear();
                m_mouseEventInfoPool.clear();

                deleteKeyCallBack            (window);
                deleteCursorPositionCallBack (window);
                deleteScrollOffsetCallBack   (window);
            }
    };
}   // namespace Utils
#endif  // USER_INPUT_H