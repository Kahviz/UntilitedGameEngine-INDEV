#include "VulkanBuffer.h"

#include "BGE_ASSERTS.h"

#if VULKAN == 1
#include <Vulkan/VulkanHelpers.h>

bool VulkanBuffer::Create(VkDevice& p_device, VkPhysicalDevice p_physicalDevice, VkDeviceSize p_size, VkBufferUsageFlags p_usage, VkMemoryPropertyFlags p_properties) {
	m_device = p_device;
	m_physicalDevice = p_physicalDevice;
	m_usage = p_usage;
	m_properties = p_properties;
	m_deviceSize = p_size;

	VkBufferCreateInfo vkBufferCreateInfo{};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.size = p_size;
	vkBufferCreateInfo.usage = m_usage;
	vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	BGE_ASSERT_VKRESULT(vkCreateBuffer(m_device, &vkBufferCreateInfo, nullptr, &m_buffer), "Failed to create buffer");
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(m_device, m_buffer, &memReq);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, m_properties, m_physicalDevice);

	BGE_ASSERT_VKRESULT(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory), "Failed to allocate buffer memory!");

	BGE_ASSERT_VKRESULT(vkBindBufferMemory(m_device, m_buffer, m_memory, 0), "Failed to bind buffer memory!");

	m_created = true;

	return true;
}

void VulkanBuffer::Destroy() {
	if (m_mapped != nullptr) {
		vkUnmapMemory(m_device, m_memory);
		m_mapped = nullptr;
	}
	if (m_memory != VK_NULL_HANDLE) {
		vkFreeMemory(m_device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}

	if (m_buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(m_device, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}
}

bool VulkanBuffer::UploadData(const void* p_data, VkDeviceSize p_size) {
	void* data = nullptr;

	if (p_size > m_deviceSize)
		return false;

	BGE_ASSERT_VKRESULT(vkMapMemory(m_device, m_memory, 0, p_size, 0, &data), "Failed to map memory!");
	memcpy(data, p_data, p_size);

	vkUnmapMemory(m_device, m_memory);
	return true;
}

bool VulkanBuffer::Resize(VkDeviceSize p_newSize, VkCommandPool p_commandPool, VkQueue p_queue) {
	if (!m_created) {
		CreateError("VulkanBuffer not created before its resized!");
		return false;
	}
	if (m_mapped != nullptr) {
		Unmap();
	}

	VkBuffer oldBuffer = m_buffer;
	VkDeviceMemory oldMemory = m_memory;

	//New creating
	VkDeviceSize resizeSize = p_newSize;
	resizeSize = Min(p_newSize, m_deviceSize);

	VkBufferCreateInfo vkBufferCreateInfo{};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.size = resizeSize;
	vkBufferCreateInfo.usage = m_usage;
	vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	BGE_ASSERT_VKRESULT(vkCreateBuffer(m_device, &vkBufferCreateInfo, nullptr, &m_buffer), "Failed to create buffer");
	
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(m_device, m_buffer, &memReq);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, m_properties, m_physicalDevice);

	BGE_ASSERT_VKRESULT(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory), "Failed to allocate buffer memory!");

	BGE_ASSERT_VKRESULT(vkBindBufferMemory(m_device, m_buffer, m_memory, 0), "Failed to bind buffer memory!");

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(p_commandPool, m_device);

	VkBufferCopy copy{};
	copy.srcOffset = 0;
	copy.dstOffset = 0;
	copy.size = Min(m_deviceSize, p_newSize);

	vkCmdCopyBuffer(commandBuffer, oldBuffer, m_buffer, 1, &copy);

	EndSingleTimeCommands(commandBuffer, p_queue, m_device, p_commandPool);

	m_deviceSize = p_newSize;

	vkDestroyBuffer(m_device, oldBuffer, nullptr);
	vkFreeMemory(m_device, oldMemory, nullptr);

	Map();
	return true;
}

VkBuffer VulkanBuffer::GetBuffer() const {
	return m_buffer;
}

VkDeviceMemory VulkanBuffer::GetMemory() const {
	return m_memory;
}

void* VulkanBuffer::Map() {
	BGE_ASSERT_VKRESULT(vkMapMemory(m_device, m_memory, 0, m_deviceSize, 0, &m_mapped), "Failed to map memory!");
	
	return m_mapped;
}

void VulkanBuffer::Unmap() {
	if (m_mapped != nullptr) {
		vkUnmapMemory(m_device, m_memory);
		m_mapped = nullptr;
	}
}

void* VulkanBuffer::GetMappedMemory() const {
	return m_mapped;
}

bool VulkanBuffer::IsCreated() const {
	return m_created;
}
#endif