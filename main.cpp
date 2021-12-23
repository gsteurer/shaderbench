#define GLFW_INCLUDE_VULKAN  // need this so VK_VERSION_1_0 is defined before glfw is imported
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <fstream>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

/*

    --- validation layers
*/

#ifdef ENABLE_VALIDATION_LAYERS

const std::vector<const char*> getRequestedValidationLayers() {
    const std::vector<const char*> requestedLayers = {
        "VK_LAYER_KHRONOS_validation"};

    return requestedLayers;
}
bool checkValidationLayerSupport(const std::vector<const char*> requestedLayers) {
    std::vector<const char*> layers_not_found;
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (auto name : requestedLayers) {
        bool found = false;
        for (auto availableLayer : availableLayers) {
            if (strcmp(name, availableLayer.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "[WARN] requested validation layer " << name << " not found" << std::endl;
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     //VERBOSE, INFO, WARN, ERR loglevel
    VkDebugUtilsMessageTypeFlagsEXT messageType,                // general, validation, or perforamnce related message
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  //
    void* pUserData) {
    (void)messageType;
    (void)pUserData;
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[WARN] [VALIDATION]: " << pCallbackData->pMessage << std::endl;
    }
    /*
        The callback returns a boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted. 
        If the callback returns true, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error. 
        This is normally only used to test the validation layers themselves, so you should always return VK_FALSE.
    */
    return VK_FALSE;
}

VkResult createDebugMessenger(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;  // Optional
    // we need to look up the address of vkCreateDebugUtilsMessengerEXT because it is not automatically loaded
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(*instance, &createInfo, nullptr, debugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VkDebugUtilsMessengerCreateInfoEXT createDebugInfo() {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    return debugCreateInfo;
}
#endif

/*
    --- window
*/

struct Window {
    Window();
    ~Window();
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
};

Window::Window() {
    // init glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    // setup extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
#ifdef ENABLE_VALIDATION_LAYERS
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // use a message callback for validation layers
#endif

    // check extensions with available extensions
    uint32_t availableExtensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data());

    for (auto name : extensions) {
        bool found = false;
        for (auto availableExtension : availableExtensions) {
            if (strcmp(name, availableExtension.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("[FATAL] requested instance extension " + std::string(name) + " not found");
        }
    }

    // create instance and surface
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    instanceCreateInfo.enabledLayerCount = 0;

#ifdef ENABLE_VALIDATION_LAYERS
    auto requestedValidationLayers = getRequestedValidationLayers();
    if (checkValidationLayerSupport(requestedValidationLayers)) {
        auto debugCreateInfo = createDebugInfo();
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requestedValidationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = requestedValidationLayers.data();
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    } else {
        std::cerr << "[ERROR] could not enable requested validation layers." << std::endl;
    }
#else
    std::cout << "[DEBUG] validation layers not enabled" << std::endl;
#endif

    if (auto err = vkCreateInstance(&instanceCreateInfo, nullptr, &instance); err != VK_SUCCESS) {
        throw std::runtime_error("[FATAL] could not create vk instance with error \'" + std::to_string(err) + "\'");
    }
    if (auto err = glfwCreateWindowSurface(instance, window, nullptr, &surface); err != VK_SUCCESS) {
        throw std::runtime_error("[FATAL] could not create surface \'" + std::to_string(err) + "\'");
    }
}

Window::~Window() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

/*
    --- device helpers
*/
bool isDeviceSuitable(VkPhysicalDevice device, std::vector<const char*> requestedDeviceExtensions) {
    // name, type, supported vulkan versions
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // texture compression, 64bit floats, multiple viewport rendering
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    for (auto name : requestedDeviceExtensions) {
        bool found = false;
        for (auto availableExtension : availableExtensions) {
            if (strcmp(name, availableExtension.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "[WARN] requested device extension " << name << " not found for device" << std::endl;
            return false;
        }
    }

    return true;
}

int scoreDevice(VkPhysicalDevice physicalDeviceHandle) {
    if (physicalDeviceHandle == VK_NULL_HANDLE)
        return 0;
    //
    // name, type, supported vulkan versions
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDeviceHandle, &deviceProperties);
    // texture compression, 64bit floats, multiple viewport rendering
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDeviceHandle, &deviceFeatures);

    std::string deviceName(deviceProperties.deviceName);

    int score = 0;

    switch (deviceProperties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 10;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 100;
            break;
        default:
            break;
    }
    if (deviceFeatures.geometryShader)
        score++;
    if (deviceFeatures.tessellationShader)
        score++;

    std::cout << "[DEBUG] device: " << deviceName << " score: " << score << std::endl;

    return score;
}

/*
    --- queue helpers
*/
struct QueueFamilyInfo {
    int index;
    bool supportsGraphics;
    bool supportsCompute;
    bool supportsTransfer;
    bool supportsSparse;
    bool supportsProtected;
    bool supportsPresent;
};

// We need to check which queue families are supported by the device and which one of these supports the commands that we want to use.
std::vector<QueueFamilyInfo> findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::vector<QueueFamilyInfo> qfi;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        QueueFamilyInfo queueFamilyInfo = {};
        queueFamilyInfo.index = i;

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyInfo.supportsGraphics = true;
        }
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyInfo.supportsCompute = true;
        }
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            queueFamilyInfo.supportsTransfer = true;
        }
        if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
            queueFamilyInfo.supportsSparse = true;
        }
        if (queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT) {
            queueFamilyInfo.supportsProtected = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            queueFamilyInfo.supportsPresent = true;
        }

        qfi.push_back(queueFamilyInfo);
        i++;
    }

    // Assign index to queue families that could be found
    return qfi;
}

std::optional<QueueFamilyInfo> selectQueueFamily(std::vector<QueueFamilyInfo> list) {
    for (const auto& item : list) {
        if (item.supportsGraphics && item.supportsPresent)
            return item;
    }
    return std::nullopt;
}

uint32_t selectQueueFamilyIndex(VkPhysicalDevice physicalDeviceHandle, VkSurfaceKHR surfaceHandle) {
    auto queueFamilies = findQueueFamilies(physicalDeviceHandle, surfaceHandle);
    auto queueFamily = selectQueueFamily(queueFamilies);
    if (!queueFamily.has_value())
        throw std::runtime_error("[FATAL] no suitable queue families");

    return queueFamily.value().index;
}

struct Device {
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceProperties deviceProperties;
    VkFence memoryTransferFence;
    VkCommandPool commandPoolHandle;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    int selectedQueue;
    uint32_t queueFamilyIndex;
    VkDevice handle;

    Device(VkInstance instance, VkSurfaceKHR surface);
    ~Device();
};

Device::Device(VkInstance instance, VkSurfaceKHR surface) {
    const std::vector<const char*> requestedDeviceExtensions = {

#ifdef __APPLE__
        "VK_KHR_portability_subset",
#endif
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    /*
        create physical device
    */
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("[FATAL] no devices found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    int currentScore = 0;
    for (const auto& device : devices) {
        if (isDeviceSuitable(device, requestedDeviceExtensions)) {
            auto newScore = scoreDevice(device);
            if (newScore > currentScore) {
                physicalDevice = device;
                currentScore = newScore;
            }
        }
    }

    /*
        report physical device properties
    */
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << "[DEBUG] selected device " << deviceProperties.deviceName << std::endl;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    if (deviceFeatures.geometryShader)
        std::cout << "[DEBUG] geometry shader supported" << std::endl;
    if (deviceFeatures.tessellationShader)
        std::cout << "[DEBUG] tessellation shader supported" << std::endl;

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("[FATAL] no suitable devices found");
    }

    /*
        create queue
    */
    // create a queue from a queue family that supports graphics capabilities
    auto queueFamilyIndex = selectQueueFamilyIndex(physicalDevice, surface);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;                         // BE CAREFUL REFACTORING, THIS GOES OUT OF SCOPE WHEN THE FUNCTION RETURNS
    queueCreateInfo.pQueuePriorities = &queuePriority;  // AND THIS REFERENCE IS NO LONGER VALID

    queueCreateInfos.push_back(queueCreateInfo);

    if (queueCreateInfos.size() < 1) {
        throw std::runtime_error("[FATAL] no queue infos created");
    }
    selectedQueue = 0;
    queueFamilyIndex = queueCreateInfos[selectedQueue].queueFamilyIndex;

    /*
        create virtual device
    */
    // enable specific features for the device
    VkPhysicalDeviceFeatures deviceFeatures{};
    // deviceFeatures.depthClamp = true;
    deviceFeatures.samplerAnisotropy = VK_TRUE;  // @@@ config parameter; is this a bug? do i need to query the device features?
    deviceFeatures.sampleRateShading = VK_TRUE;  // @@@ config parameter

    // specify extensions and validation layers
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requestedDeviceExtensions.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // these fields are ignored by up to date implementaitons of vulkan because validation layers are only set at the instance level
    // createInfo.enabledLayerCount
    // createInfo.ppEnabledExtensionNames
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &handle) != VK_SUCCESS) {
        std::runtime_error("[FATAL] could not create logical device");
    }

    /*
        create command pool
    */
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = 0;  // Optional

    if (vkCreateCommandPool(handle, &poolInfo, nullptr, &commandPoolHandle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkCreateFence(handle, &fenceInfo, nullptr, &memoryTransferFence) != VK_SUCCESS)
        throw std::runtime_error("failed to create fence for memory transfer!");

    vkGetDeviceQueue(handle, queueFamilyIndex, selectedQueue, &graphicsQueue);
    vkGetDeviceQueue(handle, queueFamilyIndex, selectedQueue, &presentQueue);
}

Device::~Device() {
    if (this->memoryTransferFence != VK_NULL_HANDLE)
        vkDestroyFence(this->handle, this->memoryTransferFence, nullptr);
    this->memoryTransferFence = VK_NULL_HANDLE;

    if (this->commandPoolHandle != VK_NULL_HANDLE)
        vkDestroyCommandPool(this->handle, this->commandPoolHandle, nullptr);
    this->commandPoolHandle = VK_NULL_HANDLE;

    if (this->handle != VK_NULL_HANDLE)
        vkDestroyDevice(this->handle, nullptr);
    this->handle = VK_NULL_HANDLE;
}

/*
    --- swap chain helpers
*/

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {
            width,
            height};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

// "For the color space we'll use SRGB if it is available because it results in more accurate perceived colors.
//    It is also pretty much the standard color space for images."
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// VK_PRESENT_MODE_FIFO_KHR is arguably the most important setting for the swap chain
//    because it represents the actual conditions for showing images to the screen.
//    Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available.
//    On mobile devices, where energy usage is more important,
//    you will probably want to use VK_PRESENT_MODE_FIFO_KHR instead
// VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a concern
//     It allows us to avoid tearing while still maintaining a fairly low latency by rendering new images
//     that are as up-to-date as possible right until the vertical blank.
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // use VK_PRESENT_MODE_MAILBOX_KHR if it is available
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    // otherwise use VK_PRESENT_MODE_FIFO_KHR if it is available
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
            return availablePresentMode;
        }
    }

    return availablePresentModes[0];
}

class SwapChain {
   public:
    VkSwapchainKHR handle;
    std::vector<VkImage> imageHandles;
    std::vector<VkImageView> imageViewHandles;

    VkExtent2D extent;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> imageFenceHandles;

    SwapChain(
        VkSurfaceKHR surface,
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t width,
        uint32_t height);

    ~SwapChain();

   private:
    VkDevice device;
};

SwapChain::SwapChain(
    VkSurfaceKHR surface,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t width,
    uint32_t height) {
    //
    this->device = device;
    /*
        ---
    */
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    uint32_t formatCount;
    if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr); result != VK_SUCCESS) {
        throw std::runtime_error("[FATAL] could not vkGetPhysicalDeviceSurfaceFormatsKHR");
    }

    if (formatCount != 0) {
        surfaceFormats.resize(formatCount);
        auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[FATAL] could not vkGetPhysicalDeviceSurfaceFormatsKHR");
        }
    } else {
        throw std::runtime_error("[FATAL] physical device does not support present any format");
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[FATAL] could not vkGetPhysicalDeviceSurfacePresentModesKHR");
        }
    } else {
        throw std::runtime_error("[FATAL] physical device does not support present any mode");
    }

    /*
        --- create swap chain
    */
    extent = chooseSwapExtent(capabilities, width, height);
    surfaceFormat = chooseSwapSurfaceFormat(surfaceFormats);
    presentMode = chooseSwapPresentMode(presentModes);

    // number of images in the swap chain
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo;
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;  // number of images in the swap chain
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;                              // This is always 1 unless you are developing a stereoscopic 3D application.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  //  If rendering images to a separate image first to perform operations like post-processing use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead then use a memory operation to transfer the rendered image to a swap chain image.

    // handle swap chain images that will be used across multiple queue families
    // if the graphics queue family is different from the presentation queue.
    // We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;  // An image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family.
    //  specify in advance between which queue families ownership will be shared using the queueFamilyIndexCount and pQueueFamilyIndices
    createInfo.queueFamilyIndexCount = 0;  // Optional
    createInfo.pQueueFamilyIndices = nullptr;

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // specifies if the alpha channel should be used for blending with other windows in the window system.
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;  // we don't care about the color of pixels that are obscured, for example because another window is in front of them.
    // @@@ todo: handle oldSwapchain?
    createInfo.oldSwapchain = VK_NULL_HANDLE;  // he window was resized. In that case the swap chain actually needs to be recreated from scratch and a reference to the old one must be specified in this field.

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &handle) != VK_SUCCESS) {
        throw std::runtime_error("[FATAL] failed to create swap chain");
    }

    /*
        --- create swap chain images
    */
    uint32_t numImages;

    vkGetSwapchainImagesKHR(device, handle, &numImages, nullptr);
    this->imageHandles.resize(numImages);
    vkGetSwapchainImagesKHR(device, handle, &numImages, imageHandles.data());

    this->imageViewHandles.resize(numImages);
    this->imageAvailableSemaphores.resize(numImages);
    this->renderFinishedSemaphores.resize(numImages);
    this->imageFenceHandles.resize(numImages);

    for (size_t idx = 0; idx < this->imageHandles.size(); idx++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = this->imageHandles[idx];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = surfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &createInfo, nullptr, &this->imageViewHandles[idx]) != VK_SUCCESS)
            throw std::runtime_error("failed to create image views!");

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[idx]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image available semaphore!");
        }

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[idx]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render finished semaphore!");
        }

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateFence(device, &fenceInfo, nullptr, &this->imageFenceHandles[idx]) != VK_SUCCESS)
            throw std::runtime_error("failed to create submit fence!");
    }
}

SwapChain::~SwapChain() {
    for (auto imageAvailableSemaphore : imageAvailableSemaphores) {
        if (imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        }
    }
    imageAvailableSemaphores.clear();

    for (auto renderFinishedSemaphore : renderFinishedSemaphores) {
        if (renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        }
    }
    renderFinishedSemaphores.clear();

    for (auto submitFence : imageFenceHandles) {
        if (submitFence != VK_NULL_HANDLE) {
            vkDestroyFence(device, submitFence, nullptr);
        }
    }
    imageFenceHandles.clear();

    for (auto imageViewHandle : this->imageViewHandles) {
        if (imageViewHandle != VK_NULL_HANDLE)
            vkDestroyImageView(device, imageViewHandle, nullptr);
    }
    this->imageViewHandles.clear();

    // @@@ need to vkDestroyImage here?

    if (this->handle != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device, this->handle, nullptr);
    this->handle = VK_NULL_HANDLE;
}

std::vector<char> loadSpirvFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);  // read at end of file and read as binary.

    if (!file.is_open()) {
        throw std::runtime_error("could not open shader file \'" + filename + "\'!");
    }
    // we read at end to determine file size
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

class RenderPass {
   public:
    RenderPass(VkDevice device, VkFormat format);
    ~RenderPass();
    VkRenderPass handle;

   private:
    VkDevice device;
};

RenderPass::RenderPass(VkDevice device, VkFormat format) {
    this->device = device;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

RenderPass::~RenderPass() {
    if (handle != VK_NULL_HANDLE)
        vkDestroyRenderPass(device, handle, nullptr);
    handle = VK_NULL_HANDLE;
}

class Pipeline {
   public:
    Pipeline(VkDevice device, VkExtent2D extent, VkRenderPass renderPass, VkDescriptorSetLayout setLayout);
    ~Pipeline();
    VkPipelineLayout layout;
    VkPipeline handle;

   private:
    VkDevice device;
};

Pipeline::Pipeline(VkDevice device, VkExtent2D extent, VkRenderPass renderPass, VkDescriptorSetLayout setLayout) {
    this->device = device;
    /*
        --- set up shaders
    */
    auto vertCode = loadSpirvFile("build/shaders/shader.vert.spv");

    VkShaderModule vertShaderModule;
    VkShaderModuleCreateInfo vertModuleCreateInfo{};
    vertModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleCreateInfo.codeSize = vertCode.size();
    vertModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
    if (vkCreateShaderModule(device, &vertModuleCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    VkPipelineShaderStageCreateInfo vertCreateInfo{};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertCreateInfo.module = vertShaderModule;
    vertCreateInfo.pName = "main";

    VkShaderModule fragShaderModule;
    auto fragCode = loadSpirvFile("build/shaders/shader.frag.spv");
    VkShaderModuleCreateInfo fragModuleCreateInfo{};
    fragModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleCreateInfo.codeSize = fragCode.size();
    fragModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
    if (vkCreateShaderModule(device, &fragModuleCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    VkPipelineShaderStageCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragCreateInfo.module = fragShaderModule;
    fragCreateInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
        vertCreateInfo,
        fragCreateInfo};

    /*
        --- create pipeline
    */

    // specify vertex data
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    // specify the kind of geometry that is drawn
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    inputAssembly.pNext = nullptr;

    // viewport defines the tranformation from the image to the framebuffer
    // the region of the framebuffer the output will be rendered to
    // the size o fthe swap chain and its images may differ from the window

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // scissor rectangles define in which regions pixels are stored
    // pixels outside the scissor are discarded by the rasterizer
    // functions like a filter instead of a transformation
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    // combine scissor and rect into a state
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // rasterizer takes the geometry shaped by the verticies from the vertex shader and turns it into fragments
    // performs depth testing, face culling, and the scissor test
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;         // fragments beyond the near and far planes are clamped instead of being discarded
    rasterizer.rasterizerDiscardEnable = VK_FALSE;  // disables output to the framebuffer
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // fill area of polygon with fragments
    rasterizer.lineWidth = 1.0f;                    // thickness of lines in terms of number of fragments; wideLines required for > 1.0
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;  // @@@ LANDMINE VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // alter depth values by adding a constant or bias them based on a fragment's slow
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp = 0.0f;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::array<VkDescriptorSetLayout, 1> setLayouts = {setLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());  // Optional
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;  // Optional
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex = -1;               // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    /*
        --- clean up shaders
    */
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

Pipeline::~Pipeline() {
    if (handle != VK_NULL_HANDLE)
        vkDestroyPipeline(device, handle, nullptr);
    handle = VK_NULL_HANDLE;

    if (layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, layout, nullptr);
    layout = VK_NULL_HANDLE;
}

class Framebuffer {
   public:
    Framebuffer(VkDevice device, std::vector<VkImageView> imageViews, VkExtent2D extent, VkRenderPass renderPass);
    ~Framebuffer();
    std::vector<VkFramebuffer> handles;

   private:
    VkDevice device;
};

Framebuffer::Framebuffer(VkDevice device, std::vector<VkImageView> imageViews, VkExtent2D extent, VkRenderPass renderPass) {
    this->device = device;
    handles.resize(imageViews.size());

    for (size_t idx = 0; idx < imageViews.size(); idx++) {
        VkImageView attachments[] = {
            imageViews[idx]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &handles[idx]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

Framebuffer::~Framebuffer() {
    for (auto framebuffer : handles) {
        if (framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    handles.clear();
}

class CommandBuffer {
   public:
    CommandBuffer(VkDevice device, VkCommandPool commandPool, std::vector<VkFramebuffer> framebuffers);
    ~CommandBuffer();
    std::vector<VkCommandBuffer> handles;

   private:
    VkDevice device;
    VkCommandPool commandPool;
};

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, std::vector<VkFramebuffer> framebuffers) {
    this->device = device;
    this->commandPool = commandPool;
    handles.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)handles.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, handles.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(device,
                         commandPool,
                         static_cast<uint32_t>(handles.size()),
                         handles.data());

    handles.clear();
}

/*
Vulkan expects the data in your structure to be aligned in memory in a specific way, for example:

    Scalars have to be aligned by N (= 4 bytes given 32 bit floats).
    A vec2 must be aligned by 2N (= 8 bytes)
    A vec3 or vec4 must be aligned by 4N (= 16 bytes)
    A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
    A mat4 matrix must have the same alignment as a vec4.

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap14.html#interfaces-resources-layout
*/
struct UniformBufferObject {
    alignas(16) glm::vec3 resolution;
    alignas(4) float time;
    alignas(16) glm::vec4 mouse;
};

class Uniform {
   public:
    Uniform(VkPhysicalDevice physicalDevice, VkDevice device, size_t numSwapChainImages);
    ~Uniform();
    void Update(void* data, uint32_t currentImage);

    std::vector<VkBuffer> bufferHandles;
    std::vector<VkDeviceMemory> memoryHandles;

   private:
    VkDevice device;
};

Uniform::Uniform(VkPhysicalDevice physicalDevice, VkDevice device, size_t numSwapChainImages) {
    this->device = device;
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    for (size_t i = 0; i < numSwapChainImages; i++) {
        //

        /*
            create ubo buffer
        */
        VkBuffer uniformBufferHandle;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBufferHandle) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        /*
            create ubo memory
        */
        uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, uniformBufferHandle, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        bool found = false;
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flags) == flags) {
                allocInfo.memoryTypeIndex = i;
                found = true;
            }
        }
        if (!found)
            throw std::runtime_error("failed to find suitable memory type!");

        VkDeviceMemory uniformMemoryHandle;
        if (vkAllocateMemory(device, &allocInfo, nullptr, &uniformMemoryHandle) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }
        vkBindBufferMemory(device, uniformBufferHandle, uniformMemoryHandle, 0);

        bufferHandles.push_back(uniformBufferHandle);
        memoryHandles.push_back(uniformMemoryHandle);
    }
}
Uniform::~Uniform() {
    for (auto uniformMemoryHandle : memoryHandles) {
        if (uniformMemoryHandle != VK_NULL_HANDLE)
            vkFreeMemory(device, uniformMemoryHandle, nullptr);
    }
    this->memoryHandles.clear();

    for (auto uniformBufferHandle : bufferHandles) {
        if (uniformBufferHandle != VK_NULL_HANDLE)
            vkDestroyBuffer(device, uniformBufferHandle, nullptr);
    }
    this->bufferHandles.clear();
}

void Uniform::Update(void* data, uint32_t currentImage) {
    void* dst;
    vkMapMemory(device, memoryHandles[currentImage], 0, sizeof(UniformBufferObject), 0, &dst);
    memcpy(dst, data, sizeof(UniformBufferObject));
    vkUnmapMemory(device, this->memoryHandles[currentImage]);
}

class DescriptorSet {
   public:
    DescriptorSet(VkDevice device, size_t numSwapChainImages, std::vector<VkBuffer> uniformBuffers);
    ~DescriptorSet();

    VkDescriptorPool pool;
    VkDescriptorSetLayout layout;
    std::vector<VkDescriptorSet> handles;

   private:
    VkDevice device;
};

DescriptorSet::DescriptorSet(VkDevice device, size_t numSwapChainImages, std::vector<VkBuffer> uniformBuffers) {
    this->device = device;
    /*
    --- create descriptor set layout
    */

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    /*
    --- create descriptor pool
    */
    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(numSwapChainImages);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(numSwapChainImages);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    /*
    --- create descriptor sets
    */
    std::vector<VkDescriptorSetLayout> layouts(numSwapChainImages, layout);

    /*
    specify the descriptor pool to allocate from, the number of descriptor sets to allocate, and the descriptor layout to base them on
    we will create one descriptor set for each swap chain image, all with the same layout
    */
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(numSwapChainImages);
    allocInfo.pSetLayouts = layouts.data();

    handles.resize(numSwapChainImages);
    if (vkAllocateDescriptorSets(device, &allocInfo, handles.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    /*
    --- update descriptor sets
    */
    for (size_t idx = 0; idx < numSwapChainImages; idx++) {
        //
        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[idx];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = handles[idx];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // used to set which resources are used by a descriptor set
        vkUpdateDescriptorSets(
            device,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0,
            nullptr);
    }
}
DescriptorSet::~DescriptorSet() {
    if (pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    pool = VK_NULL_HANDLE;

    if (layout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    layout = VK_NULL_HANDLE;
}

void RecordCommand(VkCommandBuffer commandBuffer,
                   VkRenderPass renderPass,
                   VkFramebuffer framebuffer,
                   VkExtent2D extent,
                   VkPipeline pipeline,
                   VkPipelineLayout layout,
                   std::vector<VkDescriptorSet> descriptorSets) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            descriptorSets.data(),
                            0,
                            nullptr);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

class Application {
   public:
    void Init() {
        nextSemaphoreIdx = 0;
        window = new Window();

#ifdef ENABLE_VALIDATION_LAYERS
        // enable custom debug messenger
        std::cout << "[DEBUG] validation layers enabled" << std::endl;
        if (auto err = createDebugMessenger(&window->instance, &debugMessenger); err != VK_SUCCESS) {
            throw std::runtime_error("[FATAL] could not create debug messenger with error \'" + std::to_string(err) + "\'");
        }
#endif

        device = new Device(window->instance, window->surface);
        swapChain = nullptr;
        renderPass = nullptr;
        pipeline = nullptr;
        framebuffer = nullptr;
        commandBuffer = nullptr;
        descriptorSet = nullptr;
        uniform = nullptr;

        this->Resize();
    }
    void Run() {
        while (!glfwWindowShouldClose(window->window)) {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window->window, &width, &height);
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(window->window, &width, &height);
                glfwWaitEvents();
            }
            glfwPollEvents();

            auto nextImageFence = swapChain->imageFenceHandles[nextSemaphoreIdx];
            vkWaitForFences(device->handle, 1, &nextImageFence, VK_TRUE, UINT64_MAX);

            auto imageAvailableSemaphore = swapChain->imageAvailableSemaphores[nextSemaphoreIdx];
            auto renderFinishedSemaphore = swapChain->renderFinishedSemaphores[nextSemaphoreIdx];
            this->nextSemaphoreIdx = (this->nextSemaphoreIdx++) % swapChain->imageViewHandles.size();

            uint32_t imageIdx;
            auto status = vkAcquireNextImageKHR(
                device->handle,
                swapChain->handle,
                UINT64_MAX,
                imageAvailableSemaphore,
                VK_NULL_HANDLE,
                &imageIdx);

            /*
            update uniform
            */
            double xpos, ypos;
            glfwGetCursorPos(window->window, &xpos, &ypos);
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            ubo.time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            ubo.mouse = glm::vec4(xpos, ypos, 0.0, 0.0);
            ubo.resolution = glm::vec3(width, height, 0.0);
            uniform->Update(reinterpret_cast<void*>(&ubo), imageIdx);

            bool mustResize = (status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR);
            if (mustResize) {
                this->Resize();
            } else {
                std::vector<VkCommandBuffer> commandBuffers;
                commandBuffers.push_back(commandBuffer->handles[imageIdx]);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                // @@@ imageAvailableSemaphores should belong to swapchain?
                VkSemaphore submitWaitSemaphores[] = {imageAvailableSemaphore};
                VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = submitWaitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
                submitInfo.pCommandBuffers = commandBuffers.data();

                VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                // "it's best to call vkResetFences right before using the fence"
                vkResetFences(device->handle, 1, &nextImageFence);
                if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, nextImageFence) != VK_SUCCESS)
                    throw std::runtime_error("failed to submit draw command buffer!");

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                VkSemaphore presentWaitSemaphores[] = {renderFinishedSemaphore};
                presentInfo.pWaitSemaphores = presentWaitSemaphores;

                VkSwapchainKHR swapChains[] = {swapChain->handle};
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIdx;
                presentInfo.pResults = nullptr;  // Optional

                if (vkQueuePresentKHR(device->presentQueue, &presentInfo) != VK_SUCCESS)
                    throw std::runtime_error("failed to present command buffer!");
            }
        }

        vkDeviceWaitIdle(device->handle);  //drain queues after exiting event loop
    }
    void Resize() {
        this->CleanupExtent();
        int width, height;
        glfwGetFramebufferSize(window->window, &width, &height);
        swapChain = new SwapChain(window->surface, device->physicalDevice, device->handle, width, height);
        renderPass = new RenderPass(device->handle, swapChain->surfaceFormat.format);

        uniform = new Uniform(device->physicalDevice, device->handle, swapChain->imageViewHandles.size());
        descriptorSet = new DescriptorSet(device->handle, swapChain->imageViewHandles.size(), uniform->bufferHandles);

        pipeline = new Pipeline(device->handle, swapChain->extent, renderPass->handle, descriptorSet->layout);
        framebuffer = new Framebuffer(device->handle, swapChain->imageViewHandles, swapChain->extent, renderPass->handle);
        commandBuffer = new CommandBuffer(device->handle, device->commandPoolHandle, framebuffer->handles);

        for (size_t idx = 0; idx < framebuffer->handles.size(); idx++) {
            std::vector<VkDescriptorSet> descriptorSets = {
                descriptorSet->handles[idx],
            };

            RecordCommand(
                commandBuffer->handles[idx],
                renderPass->handle,
                framebuffer->handles[idx],
                swapChain->extent,
                pipeline->handle,
                pipeline->layout,
                descriptorSets);
        }
    }

    void Cleanup() {
#ifdef ENABLE_VALIDATION_LAYERS
        // clean up custom debug messenger
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(window->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(window->instance, debugMessenger, nullptr);
        }
#endif
        this->CleanupExtent();

        if (device != nullptr)
            delete device;
        if (window != nullptr)
            delete window;
    }

   private:
    void CleanupExtent() {
        if (commandBuffer != nullptr)
            delete commandBuffer;
        if (framebuffer != nullptr)
            delete framebuffer;
        if (pipeline != nullptr)
            delete pipeline;
        if (descriptorSet != nullptr)
            delete descriptorSet;
        if (uniform != nullptr)
            delete uniform;
        if (renderPass != nullptr)
            delete renderPass;
        if (swapChain != nullptr)
            delete swapChain;
    }
    Window* window;
    Device* device;
    SwapChain* swapChain;
    Pipeline* pipeline;
    Framebuffer* framebuffer;
    RenderPass* renderPass;
    CommandBuffer* commandBuffer;
    DescriptorSet* descriptorSet;
    Uniform* uniform;
    UniformBufferObject ubo;
    size_t nextSemaphoreIdx;

#ifdef ENABLE_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
};

int main() {
    Application* app = new Application;
    try {
        app->Init();
        app->Run();
        app->Cleanup();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        delete app;
        return -1;
    }

    delete app;
    return 0;
}