/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <fstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
/* Frames in flight
 * As of now, we are required to wait on the previous frame to finish before we can start rendering the next which 
 * results in unnecessary idling of the host. The way to fix this is to allow multiple frames to be in-flight at once, 
 * that is to say, allow the rendering of one frame to not interfere with the recording of the next. Any resource that 
 * is accessed and modified during rendering must be duplicated. Thus, we need multiple command buffers, semaphores, 
 * and fences. First, defines how many frames should be processed concurrently
 * 
 * We choose the number 2 because we don't want the CPU to get too far ahead of the GPU. With 2 frames in flight, the 
 * CPU and the GPU can be working on their own tasks at the same time. If the CPU finishes early, it will wait till the 
 * GPU finishes rendering before submitting more work. With 3 or more frames in flight, the CPU could get ahead of the 
 * GPU, adding frames of latency as shown in the scenario below:
 * 
 * What happens if frames in flight > swap chain size?
 * If they were, it could result in clashes over resource usage. In a case with 3 images and 6 frames, Frame 1 may be 
 * tied to Image 1, and Frame 4 could also be tied to Image 1. While Frame 1 is presenting, Frame 4 could begin drawing 
 * in theory. But in practise would cause delays in execution because no image can be acquired from the swap chain yet 
*/
const int MAX_FRAMES_IN_FLIGHT = 2;

#define RELEASE_BUILD   false
#define MACOS_BUILD     true
#define ZERO_IN_FLIGHT  false

/* You can simply enable validation layers for debug builds and completely disable them for release builds
*/
#if RELEASE_BUILD
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

bool checkLayerSupport (std::vector <const char*> requiredLayers) {
    bool layerFound;
    /* Query all available layers
    */
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties (&layerCount, nullptr);
    std::vector <VkLayerProperties> layers (layerCount);
    vkEnumerateInstanceLayerProperties (&layerCount, layers.data());

#if 0
    std::cout << "Vulkan available layers" << std::endl;
    for (const auto& layer: layers) 
        std::cout << layer.layerName << "," << layer.specVersion << std::endl;

    std::cout << std::endl << "Required layers" << std::endl;
    for (const auto& layer: requiredLayers)
        std::cout << layer << std::endl;
#endif

    for (uint32_t i = 0; i < requiredLayers.size(); i++) {
        layerFound = false;
        for (auto const& layer: layers) {
            if (!strcmp (requiredLayers[i], layer.layerName)) {
                layerFound = true;
                break;
            }
        }
    }

    return layerFound;    
}

bool checkExtensionSupport (std::vector <const char*> requiredExtensions) {
    bool extensionFound;
    /* Query all available extensions, to allocate an array to hold the extension details we first need to know how 
     * many there are. You can request just the number of extensions by leaving the latter parameter empty
    */
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, nullptr);
    std::vector <VkExtensionProperties> extensions  (extensionCount);
    vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, extensions.data());

#if 0
    std::cout << "Vulkan available extensions" << std::endl;
    for (const auto& extension: extensions) 
        std::cout << extension.extensionName << "," << extension.specVersion << std::endl;

    std::cout << std::endl << "Required extensions" << std::endl;
    for (const auto& extension: requiredExtensions)
        std::cout << extension << std::endl;
#endif

    for (uint32_t i = 0; i < requiredExtensions.size(); i++) {
        extensionFound = false;
        for (auto const& extension: extensions) {
            if (!strcmp (requiredExtensions[i], extension.extensionName)) {
                extensionFound = true;
                break;
            }
        }
    }

    return extensionFound;
}

/* List of device extensions
*/
const std::vector <const char*> deviceExtensions = {
#if MACOS_BUILD
    "VK_KHR_portability_subset",
#endif
    /* Extensions for enabling swap chain, since image presentation is heavily tied into the window system and the
     * surfaces associated with windows, it is not actually part of the Vulkan core
    */
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/* Just checking if a swap chain is available is not sufficient, because it may not actually be compatible with our 
 * window surface. Creating a swap chain also involves a lot more settings than instance and device creation, so we need 
 * to query for some more details before we're able to proceed. There are basically three kinds of properties we need to 
 * check: 
 * (1) Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
 * (2) Surface formats (pixel format, color space)
 * (3) Available presentation modes
 * 
 * Thi struct will be populated in querySwapChainSupport function
*/
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector <VkSurfaceFormatKHR> formats;
    std::vector <VkPresentModeKHR> presentModes;
};

bool checkDeviceExtensionSupport (VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, nullptr);
    std::vector <VkExtensionProperties> availableExtensions (extensionCount);
    vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, availableExtensions.data());

    /* Use a set of strings here to represent the unconfirmed required extensions. That way we can easily tick them off 
     * while enumerating the sequence of available extensions
    */
    std::set<std::string> requiredExtensions (deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions)
        requiredExtensions.erase (extension.extensionName);
    return requiredExtensions.empty();
}

std::vector <const char*> getRequiredExtensions (void) {
    std::vector <const char*> requiredExtensions;
    /* Since Vulkan is a platform agnostic API, it can not interface directly with the window system on its own. To 
     * establish the connection between Vulkan and the window system to present results to the screen, we need to use 
     * the WSI (Window System Integration) extensions (ex: VK_KHR_surface) (included in glfw extensions)
    */
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        requiredExtensions.emplace_back (glfwExtensions[i]);

#if MACOS_BUILD
    /* If using MacOS with the latest MoltenVK sdk, you may get VK_ERROR_INCOMPATIBLE_DRIVER (-9) returned from 
     * vkCreateInstance. Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory.
     * To get over this error, first add the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR bit to 
     * VkInstanceCreateInfo struct's flags, then add VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME toinstance enabled 
     * extension list. 
     * 
     * Also, the "VK_KHR_get_physical_device_properties2" extension must be enabled for the Vulkan instance because it's 
     * listed as a dependency for the "VK_KHR_portability_subset" device extension (see create logic device function)
    */
    requiredExtensions.emplace_back (VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    requiredExtensions.emplace_back (VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    /* The validation layers will print debug messages to the standard output by default, but we can also handle them 
     * ourselves by providing an explicit callback in our program. Set up a debug messenger extension with a callback 
     * using the VK_EXT_debug_utils extension.
    */
    if (enableValidationLayers)
        requiredExtensions.emplace_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return requiredExtensions;
}

/* Create the handle to the debug messenger using instance and details about the messenger (createInfo struct). We have to 
 * look up its address ourselves using vkGetInstanceProcAddr since this is an extension function
*/
VkResult CreateDebugUtilsMessengerEXT (VkInstance instance, 
                                       const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                                       const VkAllocationCallbacks* pAllocator, 
                                       VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

/* Destroy the handle to the debug messenger, similarly to vkCreateDebugUtilsMessengerEXT the function needs to be 
 * explicitly loaded.
*/
void DestroyDebugUtilsMessengerEXT (VkInstance instance, 
                                    VkDebugUtilsMessengerEXT debugMessenger, 
                                    const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

/* Read all of the bytes from the specified file and return them in a byte array managed by std::vector. This function is
 * used to read shader binary files
*/
std::vector <char> readFile (const std::string& filename) {
    /* ate: Start reading at the end of the file
     * binary: Read the file as binary file (avoid text transformations)
     *
     * The advantage of starting to read at the end of the file is that we can use the read position to determine the 
     * size of the file and allocate a buffer
    */
    std::ifstream file (filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std:: cout << "Failed to open file" << filename << std::endl;
        return {};
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector <char> buffer(fileSize);
    /* seek back to the beginning of the file and read all of the bytes at once
    */
    file.seekg(0);
    file.read (buffer.data(), fileSize);

    file.close();
    return buffer;
}

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    /* Reference to the window
    */
    GLFWwindow* window;
    /* Handle to the instance
    */
    VkInstance instance;
    /* Handle to the debug callback
    */
    VkDebugUtilsMessengerEXT debugMessenger;
    /* VK_KHR_surface (instance level extension) exposes a VkSurfaceKHR object that represents an abstract type of surface
     * to present rendered images to
    */
    VkSurfaceKHR surface;
    /* The graphics card that we'll end up selecting will be stored in a VkPhysicalDevice handle. This object will be 
     * implicitly destroyed when the VkInstance is destroyed, so we won't need to do anything new in the cleanup function
    */
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    /* Handle to the logical device
    */
    VkDevice device;
    /* Handle to graphics queue, device queues are implicitly cleaned up when the device is destroyed, so we don't need
     * to do anything in the cleanup function
    */
    VkQueue graphicsQueue;
    /* Handle to present queue
    */
    VkQueue presentQueue;
    /* Handle to the swap chain
    */
    VkSwapchainKHR swapChain;
    /* Handle to images in the swap chain
    */
    std::vector <VkImage> swapChainImages;
    /* Vector to store imageViews for images in swap chain
    */
    std::vector <VkImageView> swapChainImageViews;
    /* Hanlde to swap chain 'format' member from 'VkSurfaceFormatKHR' surface format, and extent
    */
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    /* Handle to render pass object
    */
    VkRenderPass renderPass;
    /* Handle to pipeline layout object
    */
    VkPipelineLayout pipelineLayout;
    /* Handle to the pipeline
    */
    VkPipeline graphicsPipeline;
    /* A framebuffer object references all of the VkImageView objects that represent the attachments (example: the color 
     * attachment). However, the image that we have to use for the attachment depends on which image the swap chain 
     * returns when we retrieve one for presentation. That means that we have to create a framebuffer for all of the 
     * images in the swap chain and use the one that corresponds to the retrieved image at drawing time
    */
    std::vector <VkFramebuffer> swapChainFramebuffers;
    /* Handle to command pool
    */
    VkCommandPool commandPool;

    /* When we are setting 0 images in flight
    */
#if ZERO_IN_FLIGHT
    /* Handle to command buffer, command buffers will be automatically freed when their command pool is destroyed, so we 
     * don't need explicit cleanup
    */
    VkCommandBuffer commandBuffer;
    /* We'll need one semaphore to signal that an image has been acquired from the swapchain and is ready for rendering, 
     * another one to signal that rendering has finished and presentation can happen, and a fence to make sure only one 
     * frame is rendering at a time
    */
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
#else
    /* Each frame should have its own command buffer, set of semaphores, and fence
    */
    std::vector <VkCommandBuffer> commandBuffers;
    std::vector <VkSemaphore> imageAvailableSemaphores;
    std::vector <VkSemaphore> renderFinishedSemaphores;
    std::vector <VkFence> inFlightFences;

    /* To use the right objects (command buffers and sync objects) every frame, keep track of the current frame
    */
    uint32_t currentFrame = 0;
#endif

    /* Although many drivers and platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize, it is 
     * not guaranteed to happen. That's why we'll add some extra code to also handle resizes explicitly
    */
    bool framebufferResized = false;

    struct QueueFamilyIndices {
        /* It's not really possible to use a magic value to indicate the nonexistence of a queue family, since any value 
         * of uint32_t could in theory be a valid queue family index including 0. std::optional is a wrapper that contains 
         * no value until you assign something to it.
        */
        std::optional <uint32_t> graphicsFamily;
        /* The presentation is a queue-specific feature, we need to find a queue family that supports presenting to the 
         * surface we created. It's actually possible that the queue families supporting drawing (graphic) commands and
         * the ones supporting presentation do not overlap
        */
        std::optional <uint32_t> presentFamily;
        /* Easy method to quickly check if a family index has been found
        */
        bool isComplete (void) {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    /* Vulkan does not come with any validation layers built-in, but the LunarG Vulkan SDK provides a nice set of layers 
     * that check for common errors. Just like extensions, validation layers need to be enabled by specifying their name. 
     * All of the useful standard validation is bundled into a layer included in the SDK that is known as
     * VK_LAYER_KHRONOS_validation
    */
    const std::vector <const char*> requiredLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    /* Setup the debug callback function (for validation layer functionality), the VKAPI_ATTR and VKAPI_CALL ensure that 
     * the function has the right signature for Vulkan to call it. The pCallbackData parameter refers to a 
     * VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message itself
    */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cout << "Validation layer/msg: " << pCallbackData-> pMessage << std::endl;
        std::cout << "Validation layer/msg severity: " << messageSeverity << std::endl;
        std::cout << "Validation layer/msg type: " << messageType << std::endl;

        /* The callback returns a boolean that indicates if the Vulkan call that triggered the validation layer message 
         * should be aborted. If the callback returns true, then the call is aborted with the 
         * VK_ERROR_VALIDATION_FAILED_EXT error.
        */
        return VK_FALSE;
    }

    /* Fill up the struct that will be used to provide details about the debug messenger and its callback:
     * messageSeverity field allows you to specify all the types of severities you would like your callback to be 
     * called for.
     * messageType field lets you filter which types of messages your callback is notified about.
     * pfnUserCallback field specifies the pointer to the callback function.
     * You can optionally pass a pointer to the pUserData field which will be passed along to the callback function 
     * via the pUserData parameter. (You could use this to pass a pointer to the HelloTriangleApplication class, for 
     * example)
     * 
     * NOTE: We need this as a separate function rather than being used inside the setup debug messenger function
    */
    void populateDebugMessengerCreateInfo (VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback  = debugCallback;
        createInfo.pUserData        = nullptr;
    }

    void setupDebugMessenger (void) {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo (createInfo);

        /* Next, we need to pass this struct to vkCreateDebugUtilsMessengerEXT function to create the handle to the debug
         * messenger object (VkDebugUtilsMessengerEXT object) and associate it with our instance
        */
        if (CreateDebugUtilsMessengerEXT (instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
            std::cout << "Failed to set up debug messenger!" << std::endl;
    }

    void initWindow () {
        /* First initializes the GLFW library. Because GLFW was originally designed to create an OpenGL context, we need 
         * to tell it to not create an OpenGL context with a subsequent call
        */
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        /* Disable window resizing if we are not handling it
         * glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);
        */

        /* Create window, note that the fourth parameter allows you to optionally specify a monitor to open the window on
        * and the last parameter is only relevant to OpenGL
        */
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        /* Set user pointer of 'window', this pointer is used in the callback function
        */
        glfwSetWindowUserPointer (window, this);
        /* To detect window resizes we can use the glfwSetFramebufferSizeCallback function in the GLFW framework to set 
         * up a callback (this is done to handle resizes explicitly)
        */
        glfwSetFramebufferSizeCallback (window, framebufferResizeCallback);
    }

    /* The reason that we're creating a static function as a callback is because GLFW does not know how to properly call 
     * a member function with the right 'this' pointer to our application class instance. However, we do get a reference 
     * to the GLFWwindow in the callback and glfwSetWindowUserPointer function allows you to store an arbitrary 
     * pointer inside of it. The 'this' pointer can then be used to properly set the boolean framebufferResized
    */
    static void framebufferResizeCallback (GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast <HelloTriangleApplication*> (glfwGetWindowUserPointer (window));
        app->framebufferResized = true;
    }

    void initVulkan () {
        /* Initialize the Vulkan library by creating an instance. The instance is the connection between your application
         * and the Vulkan library and creating it involves specifying some details about your application to the driver
        */
        createInstance();
        /* A valid instance to have been created before setting up debug messenger
        */
        setupDebugMessenger();
        /* The window surface needs to be created right after the instance creation
        */
        createSurface();
        /* Next, we need to look for and select a graphics card in the system that supports the features we need
        */
        pickPhysicalDevice();
        /* After selecting a physical device to use we need to set up a logical device to interface with it
        */
        createLogicalDevice();
        /* Create swap chain
        */
        createSwapChain();
        /* Create basic image view for every image in the swap chain so that we can use them as color targets later on
        */
        createImageViews();
        /* Before we can finish creating the pipeline, we need to tell Vulkan about the framebuffer attachments that 
         * will be used while rendering. We need to specify how many color and depth buffers there will be, how many 
         * samples to use for each of them and how their contents should be handled throughout the rendering operations. 
         * All of this information is wrapped in a render pass object, for which we'll create a new createRenderPass 
         * function
        */
        createRenderPass();
        /* Graphics pipeline is the sequence of operations that take the vertices and textures of your meshes all the way 
         * to the pixels in the render targets (ex: window)
        */
        createGraphicsPipeline();
        /* The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object
         * which is created in createFrameBuffers function
        */
        createFrameBuffers();
        /* Create command pool
        */
        createCommandPool();
        /* Create command buffer
        */
#if ZERO_IN_FLIGHT
        createCommandBuffer();
#else
        createCommandBuffers();
#endif
        /* Create synchronization primitives (semaphores and fences)
        */
        createSyncObjects();
    }

    bool createInstance (void) {
        /* This data is technically optional when creating an instance, but it may provide some useful information to the
         * driver in order to optimize our specific application
        */
        VkApplicationInfo appInfo{};
        /* Many structures in Vulkan require you to explicitly specify the type of structure in the sType member
        */
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        /* This next struct is not optional and tells the Vulkan driver which global extensions and validation layers we 
         * want to use
        */
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        /* Why do we need a separated debug messenger struct? 
         * The vkCreateDebugUtilsMessengerEXT call requires a valid instance to have been created and 
         * vkDestroyDebugUtilsMessengerEXT must be called before the instance is destroyed. This currently leaves us 
         * unable to debug any issues in the vkCreateInstance and vkDestroyInstance calls. However, you'll see that there 
         * is a way to create a separate debug utils messenger specifically for those two function calls. It requires you 
         * to simply pass a pointer to a VkDebugUtilsMessengerCreateInfoEXT struct in the pNext extension field of 
         * VkInstanceCreateInfo
        */
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        /* Setup validation layers
         * Vulkan allows you to enable extensive checks through a feature known as validation layers. Validation layers 
         * are pieces of code that can be inserted between the API and the graphics driver to do things like running extra
         * checks on function parameters and tracking memory management problems. The nice thing is that you can enable
         * them during development and then completely disable them when releasing your application for zero overhead.
        */
        if (enableValidationLayers && !checkLayerSupport(requiredLayers)) {
            std::cout << "Required layers not available" << std::endl;
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
        else if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast <uint32_t> (requiredLayers.size());
            createInfo.ppEnabledLayerNames = requiredLayers.data();

            /* By creating an additional debug messenger this way it will automatically be used during vkCreateInstance 
             * and vkDestroyInstance and cleaned up after that.
            */
            populateDebugMessengerCreateInfo (debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }

        /* Setup extensions
        */
        std::vector <const char*> requiredExtensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast <uint32_t> (requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

#if MACOS_BUILD
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        /* We have already queried glfw required instance extensions, we can verify if they are already in the supported
         * vulkan extensions listed below
        */
        if (!checkExtensionSupport (requiredExtensions)) {
            std::cout << "Required extensions not available" << std::endl;
            return false;
        }

        /* We are ready to create an instance, nearly all Vulkan functions return a value of type VkResult that is either 
         * VK_SUCCESS or an error code
        */
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            std::cout << "Failed to create instance " << result << std::endl; 
            return false;
        }

        return true;
    }

    void createSurface (void) {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            std::cout << "Failed to create window surface!" << std::endl;
    }

    /* Almost every operation in Vulkan, anything from drawing to uploading textures, requires commands to be submitted 
     * to a queue. There are different types of queues that originate from different queue families and each family of 
     * queues allows only a subset of commands.
    */
    QueueFamilyIndices checkQueueFamilySupport (VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        /* Query list of available queue families
        */
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties (device, &queueFamilyCount, nullptr);
        std::vector <VkQueueFamilyProperties> queueFamilies (queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties (device, &queueFamilyCount, queueFamilies.data());

        /* keep track of queue family index
        */
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            /* find a queue family that supports graphics commnands
            */
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;
            /* find a queue family that has the capability of presenting to our window surface
            */
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR (device, i, surface, &presentSupport);
            if (presentSupport)
                indices.presentFamily = i;
            /* loop break
            */
            if (indices.isComplete())
                break;

            i++;
        }
        return indices;
    }

    SwapChainSupportDetails checkSwapChainSupport (VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        /* There are basically three kinds of properties we need to query: 
         * (1) Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
        */
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR (device, surface, &details.capabilities);
        /* (2) Surface formats (pixel format, color space)
        */
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize (formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
        /* (3) Available presentation modes
        */
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize (presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &presentModeCount, details.presentModes.data());
        }
        return details;
    }

    bool checkPhysicalDeviceSupport (VkPhysicalDevice device) {
        /* list of gpu devices have already been queried and is passed into this function one by one, which is then 
         * checked for support
        */
        QueueFamilyIndices indices = checkQueueFamilySupport(device);
        /* check device extension support
        */
        bool extensionsSupported = checkDeviceExtensionSupport (device);
        /* It should be noted that the availability of a presentation queue, as we checked in the previous chapter, 
         * implies that the swap chain extension must be supported. However, it's still good to be explicit about things, 
         * and the extension does have to be explicitly enabled
        */

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = checkSwapChainSupport (device);
            /* Swap chain support is sufficient (!) for now if there is at least one supported image format and one 
             * supported presentation mode given the window surface we have
            */
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    /* If the swapChainAdequate conditions were met then the support is definitely sufficient, but there may still be many 
     * different modes of varying optimality. We'll need to find the right settings when creating the best possible 
     * swap chain. There are three types of settings to determine:
     * 
     * (1) Surface format (color depth)
     * (2) Presentation mode (conditions for "swapping" images to the screen)
     * (3) Swap extent (resolution of images in swap chain)
    */

    /* (1) Surface format
     * Note that we'll pass the formats member of the SwapChainSupportDetails struct as argument to this function
     *
     * Each VkSurfaceFormatKHR entry contains a format and a colorSpace member.
     *  
     * format: The format member specifies the color channels and types. For example, VK_FORMAT_B8G8R8A8_SRGB means that 
     * we store the B, G, R and alpha channels in that order with an 8 bit unsigned integer for a total of 32 bits per 
     * pixel. 
     * 
     * colorSpace: The colorSpace member indicates if the SRGB color space is supported or not using the 
     * VK_COLOR_SPACE_SRGB_NONLINEAR_KHR flag. 
     * 
     * For the color space we'll use SRGB if it is available, because it results in more accurate perceived colors. It is 
     * also pretty much the standard color space for images, like the textures we'll use later on. Because of that we 
     * should also use an SRGB color format, of which one of the most common ones is VK_FORMAT_B8G8R8A8_SRGB
    */
    VkSurfaceFormatKHR pickSwapSurfaceFormat (const std::vector <VkSurfaceFormatKHR>& availableFormats) {
        /* Choose the format and colorSpace from available formats (we have already populated this list)
        */
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        /* If not it's okay to just settle with the first format that is specified
        */
        return availableFormats [0];
    }

    /* (2) Presenation mode
     * This represents the actual conditions for showing images to the screen
     * 
     * There are four possible modes available in Vulkan:
     * 
     * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, 
     * which may result in tearing.
     * 
     * VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the queue 
     * when the display is refreshed and the program inserts rendered images at the back of the queue. If the queue is 
     * full then the program has to wait. This is most similar to vertical sync as found in modern games. The moment that 
     * the display is refreshed is known as "vertical blank".
     * 
     * VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and the 
     * queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is 
     * transferred right away when it finally arrives. This may result in visible tearing.
     * 
     * VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application 
     * when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode can 
     * be used to render frames as fast as possible while still avoiding tearing, resulting in fewer latency issues than 
     * standard vertical sync. This is commonly known as "triple buffering".
    */
    VkPresentModeKHR pickSwapPresentMode (const std::vector <VkPresentModeKHR>& availablePresentModes) {
        /* VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a concern. It allows us to avoid 
         * tearing while still maintaining a fairly low latency by rendering new images that are as up-to-date as 
         * possible right until the vertical blank. On mobile devices, where energy usage is more important, you will 
         * probably want to use VK_PRESENT_MODE_FIFO_KHR instead
        */
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;
        }
        /* Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available, so we return this otherwise
        */
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /* (3) Swap extent
     * The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution 
     * of the window that we're drawing to in pixels. The range of the possible resolutions is defined in the 
     * VkSurfaceCapabilitiesKHR structure (which we have already queried)
     * 
     * Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent member. 
     * However, some window managers do allow us to differ here and this is indicated by setting the width and height in 
     * currentExtent to a special value: the maximum value of uint32_t. In that case we'll pick the resolution that best 
     * matches the window within the minImageExtent and maxImageExtent bounds.
     * 
     * GLFW uses two units when measuring sizes: pixels and screen coordinates. For example, the resolution {WIDTH, HEIGHT} 
     * that we specified earlier when creating the window is measured in screen coordinates. But Vulkan works with pixels, 
     * so the swap chain extent must be specified in pixels as well. Unfortunately, if you are using a high DPI display 
     * (like Apple's Retina display), screen coordinates don't correspond to pixels. Instead, due to the higher pixel 
     * density, the resolution of the window in pixel will be larger than the resolution in screen coordinates. So if 
     * Vulkan doesn't fix the swap extent for us, we can't just use the original {WIDTH, HEIGHT}. Instead, we must use 
     * glfwGetFramebufferSize to query the resolution of the window in pixel before matching it against the minimum and 
     * maximum image extent.
    */
    VkExtent2D pickSwapExtent (const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits <uint32_t> ::max())
            return capabilities.currentExtent;
            
        else {
            int width, height;
            glfwGetFramebufferSize (window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast <uint32_t> (width),
                static_cast <uint32_t> (height)
            };

            actualExtent.width = std::clamp (actualExtent.width, 
                                             capabilities.minImageExtent.width, 
                                             capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp (actualExtent.height, 
                                              capabilities.minImageExtent.height, 
                                              capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    bool pickPhysicalDevice (void) {
        /* Query all available graphic cards with vulkan support, very similar to listing extensions
        */
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices (instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            std::cout << "Failed to find GPUs with Vulkan support" << std::endl;
            return false;
        }
        std::vector <VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices (instance, &deviceCount, devices.data());

        for (const auto& device: devices) {
            if (checkPhysicalDeviceSupport (device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            std::cout << "Failed to find GPUs with Vulkan support" << std::endl;
            return false;
        }
        return true;
    }

    bool createLogicalDevice (void) {
        QueueFamilyIndices indices = checkQueueFamilySupport (physicalDevice);
        /* The creation of a logical device involves specifying a bunch of details in structs again, of which the first 
         * one will be VkDeviceQueueCreateInfo. This structure describes the number of queues we want for a single queue 
         * family. We need to have multiple VkDeviceQueueCreateInfo structs to create a queue from different families
        */
        std::vector <VkDeviceQueueCreateInfo> queueCreateInfos;
        /* It's very likely that these end up being the same queue family after all, but we will treat them as if they
         * were separate queues for a uniform approach
        */
        std::set <uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(), 
            indices.presentFamily.value()
        };
        /* Assign priorities to queues to influence the scheduling of command buffer execution using floating point
         * numbers between 0.0 and 1.0. This is required even if there is only a single queue
        */
        float queuePriority = 1.0f;
        /* Populate the structs
        */
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }
        /* The next information to specify is the set of device features that we'll be using. These are the features that
         * we can query for with vkGetPhysicalDeviceFeatures
        */
        VkPhysicalDeviceFeatures deviceFeatures{};
        /* .
         * .
         * . Right now we don't need anything special, so we can simply define it and leave everything to VK_FALSE
         * . (we did not have any feature availability checked in the current physical device)
         * .
        */

        /* With the previous two structures in place, we can start filling in the main VkDeviceCreateInfo structure
        */
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast <uint32_t> (queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

        /* The remainder of the information bears a resemblance to the VkInstanceCreateInfo struct and requires you to 
         * specify extensions and validation layers. The difference is that these are device specific this time.
         *
         * Previous implementations of Vulkan made a distinction between instance and device specific validation layers, 
         * but this is no longer the case. That means that the enabledLayerCount and ppEnabledLayerNames fields of 
         * VkDeviceCreateInfo are ignored by up-to-date implementations. However, it is still a good idea to set them 
         * anyway to be compatible with older implementations
        */
        if (enableValidationLayers && !checkLayerSupport(requiredLayers)) {
            std::cout << "Required layers not available" << std::endl;
            createInfo.enabledLayerCount = 0;
        }
        else if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast <uint32_t> (requiredLayers.size());
            createInfo.ppEnabledLayerNames = requiredLayers.data();
        }

        /* Setup device extensions
        */
        createInfo.enabledExtensionCount = static_cast <uint32_t> (deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        /* We're now ready to instantiate the logical device
         * NOTE: Logical devices don't interact directly with instances, which is why it's not included as a parameter
         * while creating or destroying it
        */
        VkResult result = vkCreateDevice (physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            std::cout << "Failed to create logic device" << std::endl;
            return false;
        }

        /* Retrieve queue handles for each queue family, The parameters are the logical device, queue family, queue index 
         * and a pointer to the variable to store the queue handle in. Because we're only creating a single queue from 
         * this family, we'll simply use index 0.
        */
        vkGetDeviceQueue (device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue (device, indices.presentFamily.value(), 0, &presentQueue);

        return true;
    }

    /* Vulkan does not have the concept of a "default framebuffer", hence it requires an infrastructure that will own 
     * the buffers we will render to before we visualize them on the screen. This infrastructure is known as the swap 
     * chain and must be created explicitly in Vulkan. The swap chain is essentially a queue of images that are waiting 
     * to be presented to the screen. 
     * 
     * Our application will acquire such an image to draw to it, and then return it to the queue. How exactly the queue 
     * works and the conditions for presenting an image from the queue depend on how the swap chain is set up, but the
     * general purpose of the swap chain is to synchronize the presentation of images with the refresh rate of the screen
    */
    bool createSwapChain (void) {
        SwapChainSupportDetails swapChainSupport = checkSwapChainSupport (physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = pickSwapSurfaceFormat (swapChainSupport.formats);
        VkPresentModeKHR presentMode = pickSwapPresentMode (swapChainSupport.presentModes);
        VkExtent2D extent = pickSwapExtent (swapChainSupport.capabilities); 

        /* Aside from the above properties we also have to decide how many images we would like to have in the swap chain. 
         * The implementation specifies the minimum number that it requires to function.
         *
         * However, simply sticking to this minimum means that we (the application) may sometimes have to wait on the 
         * driver to complete internal operations before we can acquire another image to render to. Therefore it is 
         * recommended to request at least one more image than the minimum
         * 
         * Remember that we only specified a minimum number of images in the swap chain, so the implementation is allowed 
         * to create a swap chain with more
        */
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        /* Make sure the imageCount is within bounds
         * if the queried maxImageCount was '0', this means that there is no maximum
        */
        if (swapChainSupport.capabilities.maxImageCount > 0 && 
            imageCount > swapChainSupport.capabilities.maxImageCount) 
            imageCount = swapChainSupport.capabilities.maxImageCount;

        /* We are now ready to create the swap chain
        */
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        /* Specify which surface the swap chain should be tied to
        */
        createInfo.surface = surface;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.presentMode = presentMode;
        createInfo.imageExtent = extent;
        createInfo.minImageCount = imageCount;
        /* imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are 
         * developing a stereoscopic 3D application
        */
        createInfo.imageArrayLayers = 1;
        /* The imageUsage bit field specifies what kind of operations we'll use the images in the swap chain for.
         * Here, we're going to render directly to them, which means that they're used as color attachment. It is also 
         * possible that you'll render images to a separate image first to perform operations like post-processing. In 
         * that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to 
         * transfer the rendered image to a swap chain image
        */
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        /* Next, we need to specify how to handle swap chain images that will be used across multiple queue families. 
         * That will be the case in our application if the graphics queue family is different from the presentation queue. 
         * We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the 
         * presentation queue
        */
        QueueFamilyIndices indices = checkQueueFamilySupport (physicalDevice);
        uint32_t queueFamilyIndices[] = { 
            indices.graphicsFamily.value(), 
            indices.presentFamily.value()
        };

        /* If the queue families differ, then we'll be using the concurrent mode (Images can be used across multiple queue 
         * families without explicit ownership transfers.) Concurrent mode requires you to specify in advance between 
         * which queue families ownership will be shared using the queueFamilyIndexCount and pQueueFamilyIndices 
         * parameters
        */
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        /* If the graphics queue family and presentation queue family are the same, which will be the case on most 
         * hardware, then we should stick to exclusive mode (An image is owned by one queue family at a time and ownership 
         * must be explicitly transferred before using it in another queue family. This option offers the best 
         * performance.)
        */
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        /* We can specify that a certain transform should be applied to images in the swap chain if it is supported, 
         * like a 90 degree clockwise rotation or horizontal flip. To specify that you do not want any transformation, 
         * simply specify the current transformation
        */
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        /* The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the 
         * window system. You'll almost always want to simply ignore the alpha channel, hence 
         * VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        */
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        /* If the clipped member is set to VK_TRUE then that means that we don't care about the color of pixels that are 
         * obscured, for example because another window is in front of them. Unless you really need to be able to read 
         * these pixels back and get predictable results, you'll get the best performance by enabling clipping
        */
        createInfo.clipped = VK_TRUE;
        /* With Vulkan it's possible that your swap chain becomes invalid or unoptimized while your application is 
         * running, for example because the window was resized. In that case the swap chain actually needs to be recreated 
         * from scratch and a reference to the old one must be specified in this field.
         * 
         * For now, we'll only ever create one swap chain.
        */
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR (device, &createInfo, nullptr, &swapChain);
        if (result != VK_SUCCESS) {
            std::cout << "Failed to create swap chain" << std::endl;
            return false;
        }

        /* Retrieve image handles from swap chain. Again, remember that we only specified a minimum number of images in 
         * the swap chain, so the implementation is allowed to create a swap chain with more. That's why we'll first query 
         * the final number of images with vkGetSwapchainImagesKHR
        */
        vkGetSwapchainImagesKHR (device, swapChain, &imageCount, nullptr);
        swapChainImages.resize (imageCount);
        vkGetSwapchainImagesKHR (device, swapChain, &imageCount, swapChainImages.data());

        /* Save format and extent
        */
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
        return true;
    }

    /* To use any VkImage, including those in the swap chain, in the render pipeline we have to create a VkImageView 
     * object. An image view is quite literally a view into an image. It describes how to access the image and which part 
     * of the image to access
    */
    bool createImageViews (void) {
        /* resize the list to fit all of the image views we'll be creating
        */
        swapChainImageViews.resize (swapChainImages.size());
        /* iterate over all of the swap chain images and populate the struct VkImageViewCreateInfo
        */
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            /* The viewType and format fields specify how the image data should be interpreted (ex: 1D/2D/3D textures)
            */
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            /* The components field allows you to swizzle (mix) the color channels around. ex: you can map all of the 
             * channels to the red channel for a monochrome texture by setting all channels to VK_COMPONENT_SWIZZLE_R. For
             * now we will set it to default mapping
            */
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            /* The subresourceRange field describes what the image's purpose is and which part of the image should be 
             * accessed. Here, our images will be used as color targets without any mipmapping levels or multiple layers
            */
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView (device, &createInfo, nullptr, &swapChainImageViews[i]);
            if (result != VK_SUCCESS) {
                std::cout << "Failed to create image views" << std::endl;
                return false;
            }
        }
        return true;
    }

    void createRenderPass (void) {
        /* Frame buffer (swap chain) attachments
         * FBOs (frame buffer objects) are "offscreen" rendering targets. All this means is that instead of making your 
         * picture appear on your display, you render it to some other place -- an FBO. Before you can do this, you have 
         * to create and configure the FBO. Part of that configuration is adding a color attachment -- a buffer to hold 
         * the per-pixel color information of the rendered picture. Maybe you stop there, or maybe you also add a depth 
         * attachment. If you are rendering 3D geometry, and you want it to look correct, you'll likely have to add this 
         * depth attachment
         * 
         * In our case we'll have just a single color buffer attachment with the same format as the swap chain images
        */
        VkAttachmentDescription colorAttachment{};
        /* The format of the color attachment should match the format of the swap chain images, and we're not doing 
         * anything with multisampling yet, so we'll stick to 1 sample
        */
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        /* The loadOp and storeOp determine what to do with the data in the attachment before rendering and after 
         * rendering. 
         
         * We have the following choices for loadOp:
         * VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
         * VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
         * VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them
         * 
         * In our case we're going to use the clear operation to clear the framebuffer to black before drawing a new 
         * frame. 
         * 
         * There are only two possibilities for the storeOp:
         * VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
         * VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
        */
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        /* The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil 
         * data. Our application won't do anything with the stencil buffer, so the results of loading and storing are 
         * irrelevant
        */
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        /* Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however 
         * the layout of the pixels in memory can change based on what you're trying to do with an image. In other words,
         * images need to be transitioned to specific layouts that are suitable for the operation that they're going to 
         * be involved in next
         *
         * Some of the most common layouts are:
         * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
         * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
         * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
         * 
         * VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image was in. 
         * The caveat of this special value is that the contents of the image are not guaranteed to be preserved, but 
         * that doesn't matter since we're going to clear it anyway. We want the image to be ready for presentation 
         * using the swap chain after rendering, which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout
        */
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        /* Setup subpasses and attachment references
         * The idea of sub passes is that you have multiple operations in a sequence that operate on the same pixels in 
         * the frame buffer, which is mainly useful for things like deferred rendering
         * 
         * A graphics pipeline describes a set of operations that usually take vertices from an input buffer and 
         * ultimately write pixels to an output framebuffer. This task of writing pixels to one or more framebuffers 
         * constitutes a single sub pass. The sub pass describes which framebuffers will be accessed (read/written) by 
         * the graphics pipeline and in which state they should be at various stages in the pipeline (e.g. they should 
         * be writable right before the fragment shader starts running). It is possible that this is all of your 
         * rendering and then you can wrap this single sub pass into a render pass and call it a day.
         * 
         * However, let's say you want to render various post-processing effects like bloom, depth-of-field and motion 
         * blur one after another to composite the final shot. Let's assume you already have your scene rendered to a 
         * framebuffer. Then you could apply the post-processing effects by having:
         * 
         * render pass 1
         * - sub pass: render scene with added bloom to a new framebuffer
         * render pass 2
         * - sub pass: add blur to bloom framebuffer and output it to a new framebuffer
         * render pass 3
         * - sub pass: add motion blur to depth-of-field framebuffer and output to the final framebuffer
         * 
         * This approach works, but the problem is that we have to write the pixels to memory every time, only to read 
         * them back right away in the next operation. We can do this more efficiently by having a single render pass 
         * and multiple sub passes:
         * 
         * render pass
         * - sub pass 1: apply bloom to scene and output
         * - sub pass 2: apply blur to previous output
         * - sub pass 3: apply depth-of-field to previous output
         * 
         * Each sub pass may run a different graphics pipeline, but sub passes describe that they're reading from 
         * attachments that have been written by the sub pass right before. This allows the graphics driver to optimize 
         * the memory operations to much more efficiently execute all these operations in a row because it can chain 
         * them together.
         * 
         * There is a catch however: you may only use sub passes like this if the fragment shader at each pixel only 
         * reads from the exact same pixel in the previous operation's output. That's why it is best used for 
         * post-processing effects and deferred rendering and less useful for chaining other operations. If you need to 
         * read other pixels, then you will have to use multiple render passes. 
         * 
         * In other words, sub passes control the state and usage of your framebuffers at the point that they start 
         * being used by the graphics pipeline and at the point when they stop being used. They don't affect the 
         * passing of variables between shaders and pipeline stages, that is controlled by the pipeline itself. They are 
         * really designed to allow you to efficiently pass images between graphics pipelines and not within them.
        */

        /* Every subpass references one or more of the attachments that we've described earlier. These references are 
         * themselves VkAttachmentReference structs.
        */
        VkAttachmentReference colorAttachmentRef{};
        /* The VkAttachmentReference does not reference the attachment object directly, it references the index in the 
         * attachments array specified in VkRenderPassCreateInfo. This allows subpasses to reference the same attachment
        */
        colorAttachmentRef.attachment = 0;
        /* The layout specifies which layout we would like the attachment to have during a subpass that uses this 
         * reference. Vulkan will automatically transition the attachment to this layout when the subpass is started. 
         * We intend to use the attachment to function as a color buffer and the 
         * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout will give us the best performance, as its name implies
        */
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        /* Setup subpass
         * Vulkan may also support compute subpasses in the future, so we have to be explicit about this being a 
         * graphics subpass
        */
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        /* Specify the reference to the attachment
        */
        subpass.colorAttachmentCount = 1;
        /* The index of the attachment in this array is directly referenced from the fragment shader with the 
         * layout(location = 0) out vec4 outColor directive.
         *
         * [?] Does this mean that by specifying (location = 0) in the fragment shader we effectively output the shading 
         * result to the first color attachment in the subpass.
         * 
         * The following other types of attachments can be referenced by a subpass:
         * pInputAttachments: Attachments that are read from a shader
         * pResolveAttachments: Attachments used for multisampling color attachments
         * pDepthStencilAttachment: Attachment for depth and stencil data
         * pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
        */
        subpass.pColorAttachments = &colorAttachmentRef;

        /* Setup subpass dependencies
         * Remember that the subpasses in a render pass automatically take care of image layout transitions. These 
         * transitions are controlled by subpass dependencies, which specify memory and execution dependencies between 
         * subpasses. We have only a single subpass right now, but the operations right before and right after this 
         * subpass also count as implicit "subpasses"
         * 
         * There are two built-in dependencies that take care of the transition at the start of the render pass and at 
         * the end of the render pass, but the former does not occur at the right time. It assumes that the transition 
         * occurs at the start of the pipeline, but we haven't acquired the image yet at that point (see drawFrame)
         * 
         * Solution: (We choose option #2)
         * (1) We could change the waitStages for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT to 
         * ensure that the render passes don't begin until the image is available, OR
         * (2) We can make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage (note that
         * this wait is not the same wait as in the draw frame function)
         * 
         * Image layout transition
         * Before the render pass the layout of the image will be transitioned to the layout you specify 
         * (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL). However, by default this happens at the beginning of the pipeline 
         * at which point we haven't acquired the image yet (we acquire it in the 
         * VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage per drawFrame). That means that we need to change the 
         * behaviour of the render pass to also only change the layout once we've come to that stage
         * 
         * The stage masks in the subpass dependency allow the subpass to already begin before the image is available up 
         * until the point where it needs to write to it
        */
        VkSubpassDependency dependency{};
        /* The first two fields specify the indices of the dependency and the dependent subpass. The special value 
         * VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on whether it is 
         * specified in srcSubpass or dstSubpass. The index 0 refers to our subpass, which is the first and only one. 
         * The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of 
         * the subpasses is VK_SUBPASS_EXTERNAL)
        */
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        /* The next two fields specify the operations to wait on and the stages in which these operations occur. We need 
         * to wait for the swap chain to finish reading from the image before we can access it. This can be accomplished 
         * by waiting on the color attachment output stage itself
         * 
         * The 'source' is the implicit subpass and the 'destination' is our main subpass
        */
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        /* The operations that should wait on this are in the color attachment stage and involve the writing of the 
         * color attachment. These settings will prevent the transition from happening until it's actually necessary 
         * (and allowed): when we want to start writing colors to it
        */
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        /* Create render pass
        */
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass (device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
    }

    /* An overview of the pipeline
     * Vertex/Index Buffer
     *      |
     * Input Assembler      [FIXED FUNCTION]
     * The input assembler collects the raw vertex data from the buffers you specify and may also use an index buffer to 
     * repeat certain elements without having to duplicate the vertex data itself
     *      |
     * Vertex Shader        [PROGRAMMABLE]
     * The vertex shader is run for every vertex and generally applies transformations to turn vertex positions from 
     * model space to screen space. It also passes per-vertex data (eg: color) down the pipeline.
     *      |
     * Tessellation         [PROGRAMMABLE]
     * The tessellation shaders allow you to subdivide geometry based on certain rules to increase the mesh quality
     *      |
     * Geometry Shader      [PROGRAMMABLE]
     * The geometry shader is run on every primitive (triangle, line, point) and can discard it or output more primitives 
     * than came in. This is similar to the tessellation shader, but much more flexible. However, it is not used much in 
     * today's applications because the performance is not that good on most graphics cards
     *      |
     * Rasterization        [FIXED FUNCTION]
     * The rasterization stage discretizes the primitives into fragments. These are the pixel elements that they fill on 
     * the framebuffer. Any fragments that fall outside the screen are discarded and the attributes outputted by the 
     * vertex shader are interpolated across the fragments. Usually the fragments that are behind other primitive 
     * fragments are also discarded here because of depth testing
     *      |
     * Fragement Shader     [PROGRAMMABLE]
     * The fragment shader is invoked for every fragment that survives and determines which framebuffer(s) the fragments 
     * are written to and with which color and depth values
     *      |     
     * Color Blending       [FIXED FUNCTION]
     * The color blending stage applies operations to mix different fragments that map to the same pixel in the 
     * framebuffer. Fragments can simply overwrite each other, add up or be mixed based upon transparency
     * 
     * Fixed function stages allow you to tweak their operations using parameters, but the way they work is predefined.
     * Programmable stages are programmable, which means that you can upload your own code to the graphics card to apply 
     * exactly the operations you want
    */
    bool createGraphicsPipeline (void) {
        /* Setup vertex input
         * The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be passed
         * to the vertex shader. It describes this in roughly two ways
         * 
         * Bindings: spacing between data and whether the data is per-vertex or per-instance ( instancing is the practice 
         * of rendering multiple copies of the same mesh in a scene at once. This technique is primarily used for objects
         * such as trees, grass, or buildings which can be represented as repeated geometry without appearing unduly 
         * repetitive)
         * Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from 
         * and at which offset
        */
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        /* Because we're hard coding the vertex data directly in the vertex shader, we'll fill in this structure to 
         * specify that there is no vertex data to load for now
        */
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        /* Setup input assembler
         * The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn 
         * from the vertices and if primitive restart should be enabled
         * 
         * VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
         * VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
         * VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
         * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
         * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two 
         * vertices of the next triangle
        */
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        /* If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles in 
         * the _STRIP topology modes
        */
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        /* Setup vertex shader and fragment shader pipeline stages
        */
        auto vertShaderCode = readFile("Build/Bin/vert.spv");
        auto fragShaderCode = readFile("Build/Bin/frag.spv");
        /* read file error
        */
        if (vertShaderCode.size() == 0 || fragShaderCode.size() == 0)
            return false;

        /* The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU doesn't happen 
         * until the graphics pipeline is created. That means that we're allowed to destroy the shader modules as soon
         * as pipeline creation is finished, which is why we'll make them local variables in the createGraphicsPipeline 
         * function instead of class members
        */
        VkShaderModule vertShaderModule = createShaderModule (vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule (fragShaderCode);
        if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE)
            return false;

        /* To actually use the shaders we'll need to assign them to a specific pipeline stage through 
         * VkPipelineShaderStageCreateInfo structures as part of the actual pipeline creation process
        */
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        /* There is an enum value for each of the programmable stages in the pipeline
        */
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        /* The shader function to invoke (called as entrypoint) is specified here. That means that it's possible to 
         * combine multiple fragment shaders into a single shader module and use different entry points to differentiate 
         * between their behaviors 
        */
        vertShaderStageInfo.pName = "main";
        /* This field allows you to specify values for shader constants. You can use a single shader module where its 
         * behavior can be configured at pipeline creation by specifying different values for the constants used in it. 
         * This is more efficient than configuring the shader using variables at render time, because the compiler can 
         * do optimizations like eliminating if statements that depend on these values. If you don't have any constants 
         * like that, then you can set the member to nullptr
        */
        vertShaderStageInfo.pSpecializationInfo = nullptr;

        /* Populate struct for frag shader
        */
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.pSpecializationInfo = nullptr;

        /* We will reference these later in the pipeline creation process
        */
        VkPipelineShaderStageCreateInfo shaderStages[] = {
            vertShaderStageInfo, 
            fragShaderStageInfo
        };

        /* Setup dynamic state
         * The graphics pipeline in Vulkan is almost completely immutable, so you must recreate the pipeline from 
         * scratch if you want to change shaders, bind different framebuffers or change the blend function. The 
         * disadvantage is that you'll have to create a number of pipelines that represent all of the different 
         * combinations of states you want to use in your rendering operations. However, because all of the operations 
         * you'll be doing in the pipeline are known in advance, the driver can optimize for it much better
         * 
         * However, a limited amount of the state can actually be changed without recreating the pipeline at draw time. 
         * Examples are the size of the viewport, line width and blend constants. If you want to use dynamic state and 
         * keep these properties out, then you'll have to fill in a VkPipelineDynamicStateCreateInfo structure
         * 
         * This will cause the configuration of these values to be ignored and you will be able (and required) to specify 
         * the data at drawing time. This results in a more flexible setup and is very common for things like viewport 
         * and scissor state.
         * 
         * Viewport
         * A viewport basically describes the region of the framebuffer that the output will be rendered to. This will 
         * almost always be (0, 0) to (width, height). Remember that the size of the swap chain and its images may differ 
         * from the WIDTH and HEIGHT of the window. The swap chain images will be used as framebuffers later on, so we 
         * should stick to their size
         * viewport.width = (float) swapChainExtent.width
         * viewport.height = (float) swapChainExtent.height
         * 
         * Scissor rectangle
         * While viewports define the transformation from the image to the framebuffer, scissor rectangles define in 
         * which regions pixels will actually be stored. Any pixels outside the scissor rectangles will be discarded by 
         * the rasterizer. They function like a filter rather than a transformation. So if we wanted to draw to the 
         * entire framebuffer, we would specify a scissor rectangle that covers it entirely
         * 
         * Dynamic state allows us set up the actual viewport(s) and scissor rectangle(s) up at drawing time.
        */
        std::vector <VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        /* Without dynamic state, the viewport and scissor rectangle need to be set in the pipeline using the 
         * VkPipelineViewportStateCreateInfo struct. This makes the viewport and scissor rectangle for this pipeline 
         * immutable. Any changes required to these values would require a new pipeline to be created with the new values
         * 
         * VkViewport viewport{};
         * viewport.x = 0.0f;
         * viewport.y = 0.0f;
         * viewport.width = (float) swapChainExtent.width;
         * viewport.height = (float) swapChainExtent.height;
         * viewport.minDepth = 0.0f;
         * viewport.maxDepth = 1.0f;
         * 
         * VkRect2D scissor{};
         * scissor.offset = {0, 0};
         * scissor.extent = swapChainExtent;
         * 
         * VkPipelineViewportStateCreateInfo viewportState{};
         * viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
         * viewportState.viewportCount = 1;
         * viewportState.pViewports = &viewport;
         * viewportState.scissorCount = 1;
         * viewportState.pScissors = &scissor;
         * 
         * It's is possible to use multiple viewports and scissor rectangles on some graphics cards, so the structure 
         * members reference an array of them. For now, it is just one which is specified below using the count field
        */
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        /* Setup rasterizer
         * The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into 
         * fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor 
         * test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe 
         * rendering). All this is configured using the VkPipelineRasterizationStateCreateInfo structure
         * 
         * depth testing
         * When an object is projected on the screen, the depth (z-value) of a generated fragment in the projected screen 
         * image is compared to the value already stored in the buffer (depth test), and replaces it if the new value is 
         * closer
         * 
         * face culling
         * If we imagine any closed shape, each of its faces has two sides. Each side would either face the user or show 
         * its back to the user. What if we could only render the faces that are facing the viewer? This is exactly what 
         * face culling does.
        */
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        /*  If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to 
         * them as opposed to discarding them. This is useful in some special cases like shadow maps (technique that 
         * generates fast approximate shadows.)
        */
        rasterizer.depthClampEnable = VK_FALSE;
        /* If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. This 
         * basically disables any output to the framebuffer
        */
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        /* The polygonMode determines how fragments are generated for geometry
         * VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
         * VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
         * VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
        */
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        /* The lineWidth describes the thickness of lines in terms of number of fragments
        */
        rasterizer.lineWidth = 1.0f;
        /* The cullMode variable determines the type of face culling to use. You can disable culling, cull the front 
         * faces, cull the back faces or both. The frontFace variable specifies the vertex order for faces to be 
         * considered front-facing and can be clockwise or counterclockwise
        */
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        /* The rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's 
         * slope. This is sometimes used for shadow mapping, but we won't be using it
        */
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; 
        rasterizer.depthBiasClamp = 0.0f; 
        rasterizer.depthBiasSlopeFactor = 0.0f;      
        /* Depth and stencil testing
         * Once the fragment shader has processed the fragment a so called stencil test is executed that, just like the 
         * depth test, has the option to discard fragments using stencil
         * 
         * TBD
         * 
        */  

        /* Setup multisampling
         * The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the ways to perform 
         * anti-aliasing. It works by combining the fragment shader results of multiple polygons that rasterize to the 
         * same pixel. This mainly occurs along edges, which is also where the most noticeable aliasing artifacts occur. 
         * Because it doesn't need to run the fragment shader multiple times if only one polygon maps to a pixel, it is 
         * significantly less expensive than simply rendering to a higher resolution and then downscaling (known as 
         * super sampling)
        */
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr; 
        multisampling.alphaToCoverageEnable = VK_FALSE; 
        multisampling.alphaToOneEnable = VK_FALSE; 

        /* Color blending
         * After a fragment shader has returned a color, it needs to be combined with the color that is already in the 
         * framebuffer. This transformation is known as color blending and there are two ways to do it:
         * (1) Mix the old and new value to produce a final color
         * (2) Combine the old and new value using a bitwise operation
         * 
         * There are two types of structs to configure color blending. The first struct, 
         * VkPipelineColorBlendAttachmentState contains the configuration per attached framebuffer and the second struct,
         * VkPipelineColorBlendStateCreateInfo contains the global color blending settings
        */
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        /* This per-framebuffer struct allows you to configure the first way of color blending (if set to true) using
         * the formula configured using the struct members. If blendEnable is set to VK_FALSE, then the new color from 
         * the fragment shader is passed through unmodified
        */
        colorBlendAttachment.blendEnable = VK_FALSE;
        /* The formula:
         * finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb)
         * finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
         * 
         * The resulting color is AND'd with the colorWriteMask to determine which channels are actually passed through
         * finalColor = finalColor & colorWriteMask;
        */
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; 
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; 
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;   
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                              VK_COLOR_COMPONENT_G_BIT | 
                                              VK_COLOR_COMPONENT_B_BIT | 
                                              VK_COLOR_COMPONENT_A_BIT;   
        /* Example: The most common way to use color blending is to implement alpha blending, where we want the new color
         * to be blended with the old color based on its opacity
         * finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor
         * finalColor.a = newAlpha.a
         * 
         * This can be configured like below
         * colorBlendAttachment.blendEnable = VK_TRUE;
         * colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
         * colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
         * colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
         * colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
         * colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
         * colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        */   

        /* The second structure references the array of structures for all of the framebuffers and allows you to set 
         * blend constants that you can use as blend factors in the aforementioned calculations
        */
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        /* If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to 
         * VK_TRUE. The bitwise operation can then be specified in the logicOp field. Note that this will automatically
         * disable the first method, as if you had set blendEnable to VK_FALSE for every attached framebuffer. However,
         * the colorWriteMask will also be used in this mode to determine which channels in the framebuffer will actually
         * be affected
        */
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; 
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; 
        colorBlending.blendConstants[1] = 0.0f; 
        colorBlending.blendConstants[2] = 0.0f; 
        colorBlending.blendConstants[3] = 0.0f; 

        /* Setup pipeline layout for uniforms/push constants
         * You can use uniform values in shaders, which are globals similar to dynamic state variables that can be 
         * changed at drawing time to alter the behavior of your shaders without having to recreate them. They are 
         * commonly used to pass the transformation matrix to the vertex shader, or to create texture samplers in the 
         * fragment shader. Push constants are another way of passing dynamic values to shaders
         * 
         * These uniform values need to be specified during pipeline creation by creating a VkPipelineLayout object
        */
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; 
        pipelineLayoutInfo.pSetLayouts = nullptr; 
        pipelineLayoutInfo.pushConstantRangeCount = 0; 
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout (device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error ("Failed to create pipeline layout");

        /* What do we have until now?
         * Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, like input 
         * assembly, rasterizer, viewport and color blending
         * Shader stages: the shader modules that define the functionality of the programmable stages of the 
         * graphics pipeline
         * Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
         * Render pass: the attachments referenced by the pipeline stages and their usage
         *
         * All of these combined fully define the functionality of the graphics pipeline
        */
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;

        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;

        pipelineInfo.renderPass = renderPass;
        /* index of the sub pass in the render pass
        */
        pipelineInfo.subpass = 0;
        /* Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. The idea of 
         * pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality in 
         * common with an existing pipeline and switching between pipelines from the same parent can also be done 
         * quicker
        */
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        
        /* Create the pipeline
         * The vkCreateGraphicsPipelines function actually has more parameters than the usual object creation functions 
         * in Vulkan. It is designed to take multiple VkGraphicsPipelineCreateInfo objects and create multiple 
         * VkPipeline objects in a single call.
         * 
         * The second parameter, for which we've passed the VK_NULL_HANDLE argument, references an optional 
         * VkPipelineCache object. A pipeline cache can be used to store and reuse data relevant to pipeline creation 
         * across multiple calls to vkCreateGraphicsPipelines and even across program executions if the cache is stored 
         * to a file. This makes it possible to significantly speed up pipeline creation at a later time
        */
        if (vkCreateGraphicsPipelines (device, 
                                       VK_NULL_HANDLE, 
                                       1, 
                                       &pipelineInfo, 
                                       nullptr, 
                                       &graphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline");

        /* Destroy shader modules
        */
        vkDestroyShaderModule (device, vertShaderModule, nullptr);
        vkDestroyShaderModule (device, fragShaderModule, nullptr);
        return true;
    }

    /* Before we can pass the shader code to the pipeline, we have to wrap it in a VkShaderModule object. Shader modules 
     * are just a thin wrapper around the shader bytecode that we've previously loaded from a file and the functions 
     * defined in it
    */
    VkShaderModule createShaderModule (const std::vector <char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        /* The size of the bytecode is specified in bytes, but the bytecode pointer is a uint32_t pointer rather than a 
         * char pointer. Therefore we will need to cast the pointer with reinterpret_cast
        */
        createInfo.pCode = reinterpret_cast <const uint32_t*> (code.data());

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule (device, &createInfo, nullptr, &shaderModule);
        if (result != VK_SUCCESS) {
            std::cout << "Failed to create shader module" << std::endl;
            return VK_NULL_HANDLE;          
        }
        return shaderModule;
    }

    void createFrameBuffers (void) {
        /* Resize the container to hold all of the framebuffers
        */
        swapChainFramebuffers.resize (swapChainImageViews.size());
        /* Iterate through the image views and create framebuffers from them
        */
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            /* specify with which renderPass the framebuffer needs to be compatible. You can only use a framebuffer with
             * the render passes that it is compatible with, which roughly means that they use the same number and type
             * of attachments
            */
            framebufferInfo.renderPass = renderPass;
            /* The attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to
             * the respective attachment descriptions in the render pass pAttachment array
            */
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            /* layers refers to the number of layers in image arrays. Our swap chain images are single images, so the
             * number of layers is 1
            */
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer (device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer!");
        }
    }

    /* It is possible for the window surface to change such that the swap chain is no longer compatible with it. One of 
     * the reasons that could cause this to happen is the size of the window changing. We have to catch these events and 
     * recreate the swap chain and all of the creation functions for the objects that depend on the swap chain or the 
     * window size. The image views need to be recreated because they are based directly on the swap chain images. And, 
     * the framebuffers directly depend on the swap chain images, and thus must be recreated as well
     * 
     * Note that we don't recreate the renderpass here for simplicity. In theory it can be possible for the swap chain 
     * image format to change during an applications' lifetime, e.g. when moving a window from an standard range to an 
     * high dynamic range monitor. This may require the application to recreate the renderpass to make sure the change 
     * between dynamic ranges is properly reflected
    */
    void recreateSwapChain (void) {
        /* There is another case where a swap chain may become out of date and that is a special kind of window 
         * resizing: window minimization. This case is special because it will result in a frame buffer size of 0. We 
         * will handle that by pausing until the window is in the foreground again
        */
        int width = 0, height = 0;
        glfwGetFramebufferSize (window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize (window, &width, &height);
            /* This function puts the calling thread to sleep until at least one event is available in the event queue
            */
            glfwWaitEvents();
        }

        /* We first call vkDeviceWaitIdle, because we shouldn't touch resources that may still be in use
        */
        vkDeviceWaitIdle (device);
        /* Make sure that the old versions of these objects are cleaned up before recreating them
        */
        cleanupSwapChain();
        /* Note that in chooseSwapExtent we already query the new window resolution to make sure that the swap chain 
         * images have the (new) right size, so there's no need to modify chooseSwapExtent (remember that we already 
         * had to use glfwGetFramebufferSize get the resolution of the surface in pixels when creating the swap chain)
        */
        createSwapChain();
        createImageViews();
        createFrameBuffers();
        /* That's all it takes to recreate the swap chain! However, the disadvantage of this approach is that we need to 
         * stop all rendering before creating the new swap chain. It is possible to create a new swap chain while 
         * drawing commands on an image from the old swap chain are still in-flight. You need to pass the previous swap 
         * chain to the oldSwapChain field in the VkSwapchainCreateInfoKHR struct and destroy the old swap chain as soon 
         * as you've finished using it
        */

        /* How do we figure out when swap chain recreation is necessary and call our new recreateSwapChain function?
         * Luckily, Vulkan will usually just tell us that the swap chain is no longer adequate during presentation. The 
         * vkAcquireNextImageKHR and vkQueuePresentKHR functions can return the following special values to indicate 
         * this:
         * VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used 
         * for rendering. Usually happens after a window resize.
         * VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface 
         * properties are no longer matched exactly
        */
    }

    /* Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function calls. 
     * You have to record all of the operations you want to perform in command buffer objects. The advantage of this is 
     * that when we are ready to tell the Vulkan what we want to do, all of the commands are submitted together and 
     * Vulkan can more efficiently process the commands since all of them are available together
     * 
     * We have to create a command pool before we can create command buffers. Command pools manage the memory that is 
     * used to store the buffers and command buffers are allocated from them
    */
    void createCommandPool (void) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        /* There are two possible flags for command pools
         * VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often 
         * (may change memory allocation behavior)
         * VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without 
         * this flag they all have to be reset together
         * 
         * We will be recording a command buffer every frame, so we want to be able to reset and rerecord over it. Thus, 
         * we need to set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag bit for our command pool
        */
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        /* Command buffers are executed by submitting them on one of the device queues, like the graphics and 
         * presentation queues we retrieved. Each command pool can only allocate command buffers that are submitted on a 
         * single type of queue
         * 
         * We're going to record commands for drawing, which is why we've chosen the graphics queue family
        */
        QueueFamilyIndices queueFamilyIndices = checkQueueFamilySupport (physicalDevice);
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool (device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool");
    }

#if ZERO_IN_FLIGHT
    void createCommandBuffer (void) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        /* Specify the command pool and number of buffers to allocate
        */
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        /* The level parameter specifies if the allocated command buffers are primary or secondary command buffers
         * VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other 
         * command buffers
         * VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers
         * We won't make use of the secondary command buffer functionality here, but you can imagine that it's helpful to
         * reuse common operations from primary command buffers
        */
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        
        if (vkAllocateCommandBuffers (device, &allocInfo, &commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffer");
    }
#else
    /* Create multiple command buffers
    */
    void createCommandBuffers (void) {
        commandBuffers.resize (MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        /* Specify the command pool and number of buffers to allocate
        */
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
        /* The level parameter specifies if the allocated command buffers are primary or secondary command buffers
         * VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other 
         * command buffers
         * VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers
         * We won't make use of the secondary command buffer functionality here, but you can imagine that it's helpful to
         * reuse common operations from primary command buffers
        */
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        
        if (vkAllocateCommandBuffers (device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers");
    }
#endif

    /* Command buffer recording writes the commands we want to execute into a command buffer. The VkCommandBuffer used 
     * will be passed in as a parameter, as well as the index of the current swapchain image we want to write to
    */
    void recordCommandBuffer (VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        /* We always begin recording a command buffer by calling vkBeginCommandBuffer with a small 
         * VkCommandBufferBeginInfo structure as argument that specifies some details about the usage of this specific 
         * command buffer
        */
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        /* The flags parameter specifies how we're going to use the command buffer
         * VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it 
         * once.
         * VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely 
         * within a single render pass.
         * VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already 
         * pending execution.
         * None of these flags are applicable for us right now.
        */
        beginInfo.flags = 0;
        /* The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which state to 
         * inherit from the calling primary command buffers
        */
        beginInfo.pInheritanceInfo = nullptr;   

        /* If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset 
         * it. It's not possible to append commands to a buffer at a later time.
        */
        if (vkBeginCommandBuffer (commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer");

        /* (1) Begin render pass cmd
         * 
         * Drawing starts by beginning the render pass with vkCmdBeginRenderPass. The render pass is configured using 
         * some parameters in a VkRenderPassBeginInfo struct
        */
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        /* The first parameters are the render pass itself and the attachments to bind. We created a framebuffer for 
         * each swap chain image where it is specified as a color attachment. Thus we need to bind the framebuffer for 
         * the swapchain image we want to draw to. Using the imageIndex parameter which was passed in, we can pick the 
         * right framebuffer for the current swapchain image
        */
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        /* The next two parameters define the size of the render area. The render area defines where shader loads and 
         * stores will take place. The pixels outside this region will have undefined values. It should match the size 
         * of the attachments for best performance
        */
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        /* The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as 
         * load operation for the color attachment. I've defined the clear color to simply be black with 100% opacity
        */
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        /* The render pass can now begin
         * All of the functions that record commands can be recognized by their vkCmd prefix. They all return void, so 
         * there will be no error handling until we've finished recording
         * 
         * The final parameter controls how the drawing commands within the render pass will be provided.
         * VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself 
         * and no secondary command buffers will be executed.
         * VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary 
         * command buffers.
         * We will not be using secondary command buffers, so we'll go with the first option.
        */
        vkCmdBeginRenderPass (commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* (2) Bind graphics pipeline cmd
         * 
         * The second parameter specifies if the pipeline object is a graphics or compute pipeline
        */
        vkCmdBindPipeline (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        /* (3) Configure dynamic state cmds
         * 
         * Up until now, we've told Vulkan which operations to execute in the graphics pipeline and which attachment to 
         * use in the fragment shader. Also, we did specify viewport and scissor state for this pipeline to be dynamic. 
         * So we need to set them in the command buffer before issuing our draw command
        */
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast <float> (swapChainExtent.width);
        viewport.height = static_cast <float> (swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport (commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor (commandBuffer, 0, 1, &scissor);

        /* (4) Draw cmd
         *
         * The actual vkCmdDraw function is a bit anticlimactic, but it's so simple because of all the information we 
         * specified in advance
         * vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
         * instanceCount: Used for instanced rendering, use 1 if you're not doing that.
         * firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
         * firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex
        */
        vkCmdDraw (commandBuffer, 3, 1, 0, 0);

        /* (5) End render pass cmd
        */
        vkCmdEndRenderPass (commandBuffer);

        /* Finish recording command
        */
        if (vkEndCommandBuffer (commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");
    }

    /* A core design philosophy in Vulkan is that synchronization of execution on the GPU is explicit. The order of 
     * operations is up to us to define using various synchronization primitives which tell the driver the order we want 
     * things to run in. This means that many Vulkan API calls which start executing work on the GPU are asynchronous, 
     * the functions will return before the operation has finished and there are a number of events that we need to order
     * explicitly
    */
    void createSyncObjects (void) {
        /* A semaphore is used to add order between queue operations. Queue operations refer to the work we submit to a 
         * queue, either in a command buffer or from within a function. Examples of queues are the graphics queue and 
         * the presentation queue. Semaphores are used both to order work inside the same queue and between different 
         * queues
         * 
         * The way we use a semaphore to order queue operations is by providing the same semaphore as a 'signal' 
         * semaphore in one queue operation and as a 'wait' semaphore in another queue operation. For example, lets say 
         * we have semaphore S and queue operations A and B that we want to execute in order. What we tell Vulkan is 
         * that operation A will 'signal' semaphore S when it finishes executing, and operation B will 'wait' on 
         * semaphore S before it begins executing. When operation A finishes, semaphore S will be signaled, while 
         * operation B wont start until S is signaled. After operation B begins executing, semaphore S is automatically 
         * reset back to being unsignaled, allowing it to be used again
         * 
         * Note that, the waiting only happens on the GPU. The CPU continues running without blocking
        */
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        /* A fence has a similar purpose, in that it is used to synchronize execution, but it is for ordering the 
         * execution on the CPU, otherwise known as the host. Simply put, if the host needs to know when the GPU has 
         * finished something, we use a fence.
         * 
         * Whenever we submit work to execute, we can attach a fence to that work. When the work is finished, the fence 
         * will be signaled. Then we can make the host wait for the fence to be signaled, guaranteeing that the work has 
         * finished before the host continues
         * 
         * Fences must be reset manually to put them back into the unsignaled state. This is because fences are used to 
         * control the execution of the host, and so the host gets to decide when to reset the fence. Contrast this to 
         * semaphores which are used to order work on the GPU without the host being involved
        */
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        /* On the first frame we call drawFrame(), which immediately waits on inFlightFence to be signaled. 
         * inFlightFence is only signaled after a frame has finished rendering, yet since this is the first frame, 
         * there are no previous frames in which to signal the fence! Thus vkWaitForFences() blocks indefinitely, 
         * waiting on something which will never happen. To combat this, create the fence in the signaled state, so 
         * that the first call to vkWaitForFences() returns immediately since the fence is already signaled
        */
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        /* Create required semaphores and fences
        */
#if ZERO_IN_FLIGHT
        if (vkCreateSemaphore (device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore (device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence (device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
            throw std::runtime_error ("Failed to create semaphores and/or fences");
#else
        imageAvailableSemaphores.resize (MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize (MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize (MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore (device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore (device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence (device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
                throw std::runtime_error ("Failed to create semaphores and/or fences");
        }
#endif
    }

    /* At a high level, rendering a frame in Vulkan consists of a common set of steps:
     * (1) Wait for the previous frame to finish
     * (2) Acquire an image from the swap chain
     * (3) Record a command buffer which draws the scene onto that image
     * (4) Submit the recorded command buffer into the queue
     * (5) Present the swap chain image
    */
    void drawFrame (void) {
        /* (1)
         * At the start of the frame, we want to wait until the previous frame has finished, so that the command buffer 
         * and semaphores are available to use. The vkWaitForFences function takes an array of fences and waits on the 
         * host for either any or all of the fences to be signaled before returning. The VK_TRUE we pass here indicates 
         * that we want to wait for all fences, but in the case of a single one it doesn't matter. This function also 
         * has a timeout parameter that we set to the maximum value of a 64 bit unsigned integer, UINT64_MAX, which 
         * effectively disables the timeout
         * 
         * We need to make sure only one frame is being drawn/rendered at a time, why?
         * We use a fence for waiting on the previous frame to finish, this is so that we don't draw more than one frame 
         * at a time. Because we re-record the command buffer every frame, we cannot record the next frame's work to the 
         * command buffer until the current frame has finished executing, as we don't want to overwrite the current 
         * contents of the command buffer while the GPU is using it
        */
#if ZERO_IN_FLIGHT
        vkWaitForFences (device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
#else
        vkWaitForFences (device, 1, &inFlightFences [currentFrame], VK_TRUE, UINT64_MAX);
#endif

        /* (2)
         * The first two parameters of vkAcquireNextImageKHR are the logical device and the swap chain from which we 
         * wish to acquire an image. The third parameter specifies a timeout in nanoseconds for an image to become 
         * available. Using the maximum value of a 64 bit unsigned integer means we effectively disable the timeout.
         * 
         * The next two parameters specify synchronization objects that are to be signaled when the presentation engine 
         * is finished using the image. That's the point in time where we can start drawing to it
         * 
         * The index refers to the VkImage in our swapChainImages array. We're going to use that index to pick the 
         * VkFrameBuffer. It just returns the index of the next image that will be available at some point notified by
         * the semaphore
        */
        uint32_t imageIndex;
#if ZERO_IN_FLIGHT
        VkResult result = vkAcquireNextImageKHR (device, 
                                                 swapChain, 
                                                 UINT64_MAX, 
                                                 imageAvailableSemaphore, 
                                                 VK_NULL_HANDLE, 
                                                 &imageIndex);
#else
        VkResult result = vkAcquireNextImageKHR (device, 
                                                 swapChain, 
                                                 UINT64_MAX, 
                                                 imageAvailableSemaphores [currentFrame], 
                                                 VK_NULL_HANDLE, 
                                                 &imageIndex);
#endif

        /* If the swap chain turns out to be out of date when attempting to acquire an image, then it is no longer 
         * possible to present to it. Therefore we should immediately recreate the swap chain and try again in the next 
         * drawFrame call
        */
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } 
        /* You could also decide to recreate and return if the swap chain is suboptimal, but we've chosen to proceed 
         * anyway in that case because we've already acquired an image. Both VK_SUCCESS and VK_SUBOPTIMAL_KHR are 
         * considered "success" return codes
        */
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error ("Failed to acquire swap chain image");

        /* After waiting for fence, we need to manually reset the fence to the unsignaled state immediately after. But
         * we delay it to upto this point to avoid deadlock on inFlightFence
         *
         * When vkAcquireNextImageKHR returns VK_ERROR_OUT_OF_DATE_KHR, we recreate the swapchain and then return from 
         * drawFrame. But before that happens, the current frame's fence was waited upon and reset. Since we return 
         * immediately, no work is submitted for execution and the fence will never be signaled, causing vkWaitForFences 
         * to halt forever.
         * 
         * To overcome this, delay resetting the fence until after we know for sure we will be submitting work with it. 
         * Thus, if we return early, the fence is still signaled and vkWaitForFences wont deadlock the next time we use 
         * the same fence object
        */
#if ZERO_IN_FLIGHT
        vkResetFences (device, 1, &inFlightFence);
#else
        vkResetFences (device, 1, &inFlightFences [currentFrame]);
#endif

        /* (3)
         * First, we call vkResetCommandBuffer on the command buffer to make sure it is able to be recorded. Then, we 
         * use the recordCommandBuffer function to record the commands we want
        */
#if ZERO_IN_FLIGHT
        vkResetCommandBuffer (commandBuffer, 0);
        recordCommandBuffer (commandBuffer, imageIndex);
#else
        vkResetCommandBuffer (commandBuffers [currentFrame], 0);
        recordCommandBuffer (commandBuffers [currentFrame], imageIndex);
#endif

        /* (4)
         * Queue submission and synchronization is configured through parameters in the VkSubmitInfo structure
        */
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        /* The first three parameters specify which semaphores to wait on before execution begins and in which stage(s) 
         * of the pipeline to wait. We want to wait with writing colors to the image until it's available, so we're 
         * specifying the stage of the graphics pipeline that writes to the color attachment. That means that 
         * theoretically the implementation can already start executing our vertex shader and such while the image is 
         * not yet available
         * 
         * Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores
        */
#if ZERO_IN_FLIGHT
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
#else
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores [currentFrame]};
#endif
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        /* The next two parameters specify which command buffers to actually submit for execution
        */
        submitInfo.commandBufferCount = 1;
#if ZERO_IN_FLIGHT
        submitInfo.pCommandBuffers = &commandBuffer;
#else
        submitInfo.pCommandBuffers = &commandBuffers [currentFrame];
#endif
        /* The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the 
         * command buffer(s) have finished execution
        */
#if ZERO_IN_FLIGHT
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
#else
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores [currentFrame]};
#endif
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        /* The last parameter references an optional fence that will be signaled when the command buffers finish 
         * execution. This allows us to know when it is safe for the command buffer to be reused, thus we want to give 
         * it inFlightFence. Now on the next frame, the CPU will wait for this command buffer to finish executing 
         * before it records new commands into it
        */
#if ZERO_IN_FLIGHT
        if (vkQueueSubmit (graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
            throw std::runtime_error ("Failed to submit draw command buffer");
#else
        if (vkQueueSubmit (graphicsQueue, 1, &submitInfo, inFlightFences [currentFrame]) != VK_SUCCESS)
            throw std::runtime_error ("Failed to submit draw command buffer");
#endif

        /* (5)
         * After queueing all rendering commands and transitioning the image to the correct layout, it is time to queue 
         * an image for presentation
        */
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        /* The first two parameters specify which semaphores to wait on before presentation can happen, just like 
         * VkSubmitInfo. Since we want to wait on the command buffer to finish execution, we take the semaphores which 
         * will be signalled and wait on them, thus we use signalSemaphores
        */
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        /* The next two parameters specify the swap chains to present images to and the index of the image for each 
         * swap chain
        */
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        /* Applications that do not need per-swapchain results can use NULL for pResults. If non-NULL, each entry in 
         * pResults will be set to the VkResult for presenting the swapchain corresponding to the same index in 
         * pSwapchains
         * 
         * It's not necessary if you're only using a single swap chain, because you can simply use the return value of 
         * the present function
        */
        presentInfo.pResults = nullptr;

        /* The vkQueuePresentKHR function returns the same values with the same meaning as vkAcquireNextImageKHR. In 
         * this case we will also recreate the swap chain if it is suboptimal, because we want the best possible result
        */
        result = vkQueuePresentKHR (presentQueue, &presentInfo);
        /* Why didn't we check 'framebufferResized' boolean after vkAcquireNextImageKHR?
         * It is important to note that a signalled semaphore can only be destroyed by vkDeviceWaitIdle if it is being
         * waited on by a vkQueueSubmit. Since we are handling the resize explicitly using the boolean, returning after
         * vkAcquireNextImageKHR (thus calling vkDeviceWaitIdle) will make the semaphore signalled but have nothing
         * waiting on it
        */
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS)
            throw std::runtime_error ("Failed to present swap chain image");

        /* Update frame index to loop around MAX_FRAMES_IN_FLIGHT
        */
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void mainLoop (void) {
        /* Add an event loop to keep the application running until either an error occurs or the window is closed
        */
        while (!glfwWindowShouldClose (window)) {
            glfwPollEvents();
            drawFrame();
        }
        /* Remember that all of the operations in drawFrame are asynchronous. That means that when we exit the loop in 
         * mainLoop, drawing and presentation operations may still be going on. Cleaning up resources while that is 
         * happening is a bad idea. To fix that problem, we should wait for the logical device to finish operations 
         * before exiting mainLoop and destroying the window
        */
        vkDeviceWaitIdle (device);
    }

    /* We'll move the cleanup code of all objects that are recreated as part of a swap chain refresh to this function
    */
    void cleanupSwapChain (void) {
        /* Destroy the framebuffers before the image views and render pass that they are based on
        */
        for (auto framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer (device, framebuffer, nullptr);
        /* Unlike images, the image views were explicitly created by us, so we need to destroy them
        */
        for (auto imageView : swapChainImageViews)
            vkDestroyImageView (device, imageView, nullptr);
        /* Destroy swap chain
        */
        vkDestroySwapchainKHR (device, swapChain, nullptr);
    }

    void cleanup (void) {
        /* Destroy swap chain and its dependents
        */
        cleanupSwapChain();
        /* Destroy synchronization primitives
        */
#if ZERO_IN_FLIGHT
        vkDestroySemaphore (device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore (device, renderFinishedSemaphore, nullptr);
        vkDestroyFence (device, inFlightFence, nullptr);
#else
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore (device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore (device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence (device, inFlightFences[i], nullptr);
        }
#endif
        /* Destroy command pool
        */
        vkDestroyCommandPool (device, commandPool, nullptr);
        /* Destroy pipeline
        */
        vkDestroyPipeline (device, graphicsPipeline, nullptr);
        /* Destroy pipeline layout
        */
        vkDestroyPipelineLayout (device, pipelineLayout, nullptr);
        /* Destroy render pas
        */
        vkDestroyRenderPass (device, renderPass, nullptr);
        /* Destroy logical device handle
        */
        vkDestroyDevice (device, nullptr);
        /* Destroy debug messenger handle
        */
        if (enableValidationLayers)
            DestroyDebugUtilsMessengerEXT (instance, debugMessenger, nullptr);
        /* Destroy surface
        */
        vkDestroySurfaceKHR(instance, surface, nullptr);
        /* The VkInstance should be only destroyed right before the program exits, all of the other Vulkan resources that 
         * we create should be cleaned up before the instance is destroyed
        */
        vkDestroyInstance (instance, nullptr);
        /* Once the window is closed, we need to clean up resources by destroying it and terminating GLFW itself
        */
        glfwDestroyWindow (window);
        glfwTerminate();
    }
};

int main (void) {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}