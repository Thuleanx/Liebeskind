#include "queue_family.h"

bool QueueFamilyIndices::isComplete() {
    return this->graphicsFamily.has_value() && this->presentFamily.has_value();
}

QueueFamilyIndices QueueFamilyIndices::findQueueFamilies(const vk::PhysicalDevice& device, const vk::SurfaceKHR &surface) {
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        const vk::QueueFamilyProperties& queueFamily = queueFamilies[i];
        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
            indices.graphicsFamily = i;
        if (device.getSurfaceSupportKHR(i, surface))
            indices.presentFamily = i;
    }

    return indices;
}