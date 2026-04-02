#include "shared/VulkanInclude.h"

#include "renderer/Renderer.h"

#include  "shared/EngineConfig.h"
#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"
#include "renderer/GraphicsPipeline.h"
#include "renderer/ImageComputePipeline.h"
#include "renderer/ParticleComputePipeline.h"
#include "renderer/Vertex.h"

#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(VulkanContext& context, SwapChainManager& swapChainManager, GraphicsPipeline& graphicsPipeline, ImageComputePipeline& imageComputePipeline, GraphicsPipeline& particleGraphicsPipeline, ParticleComputePipeline& particleComputePipeline)
    : context(context), 
    swapChainManager(swapChainManager), 
    graphicsPipeline(graphicsPipeline), 
    imageComputePipeline(imageComputePipeline),
    particleGraphicsPipeline(particleGraphicsPipeline),
    particleComputePipeline(particleComputePipeline) {

    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    loadMeshes();

    startTime = std::chrono::steady_clock::now();
}

Renderer::~Renderer() {
    context.getDevice().waitIdle();

    for (auto& ubo : uniformBuffers) {
        vmaDestroyBuffer(context.getVmaAllocator(), ubo.buffer, ubo.allocation);
    }
}

void Renderer::drawFrame() {
    auto& device = context.getDevice();

    // Making sure we're not fenced here means we wait until we get a signal from graphicsQueue.submit for the corresponding frame
    auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);

    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }

    if (warmUpFrames >= 2) {
        std::chrono::time_point<std::chrono::steady_clock> curFrameTime = std::chrono::steady_clock::now();

        std::chrono::duration<float> totalTime = curFrameTime - startTime;
        std::chrono::duration<float> deltaSeconds = curFrameTime - prevFrameTime;
        prevFrameTime = curFrameTime;

        updateUniformBuffer(frameIndex, totalTime.count(), deltaSeconds.count());
        particleComputePipeline.updateParticleUBO(totalTime.count(), deltaSeconds.count());

        if (EngineConfig::PRINT_GPU_PROFILING) {
            // Note: With the following line uncommented, performance can be more consistent. Probably because it acts as a throttle? Fifo over mailbox should help this but doesn't entirely I think.
            // printf("[GPU Profiling] Draw time: %fms\n", context.getRenderPassTime(frameIndex));

            frameDeltas[frameDeltasI] = context.getRenderPassTime(frameIndex);
            frameDeltasI++;
            frameDeltasI = frameDeltasI % numDeltas;

            timeSinceLastPrint += deltaSeconds.count();

            if (timeSinceLastPrint > 1.0) {
                float average = 0.0;
                float max = 0.0;

                for (auto frameDelta : frameDeltas) {
                    average += frameDelta;

                    if (max < frameDelta) {
                        max = frameDelta;
                    }
                }

                average = average / numDeltas;

                printf("[GPU Profiling] Average: %fms, Max: %fms\n", average, max);

                timeSinceLastPrint -= 1.0;
            }
        }
    }
    else {
        warmUpFrames++;
        prevFrameTime = std::chrono::steady_clock::now();
    }

    context.resetQueryPool(frameIndex);

    // imageIndex is the index of the next image to be rendered to by the swapChain. acquireNextImage signals that presentation is complete for a frame 
    auto [result, imageIndex] = swapChainManager.getSwapChain().acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        swapChainManager.recreateSwapChain();
        return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Only reset the fence if we are submitting work
    device.resetFences(*inFlightFences[frameIndex]);

    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex] // Knowledge check: Why is this imageIndex and not frameIndex?
    };

    context.getGraphicsQueue().submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChainManager.getSwapChain(),
        .pImageIndices = &imageIndex
    };

    result = context.getPresentQueue().presentKHR(presentInfoKHR);

    if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized)
    {
        framebufferResized = false;
        swapChainManager.recreateSwapChain();
    }
    else
    {
        // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
        assert(result == vk::Result::eSuccess);
    }


    frameIndex = (frameIndex + 1) % EngineConfig::MAX_FRAMES_IN_FLIGHT;
}

// ------ Renderer functions -----------

void Renderer::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = context.getGraphicsIndex()
    };

    commandPool = vk::raii::CommandPool(context.getDevice(), poolInfo);
}

void Renderer::createCommandBuffers() {
    commandBuffers.clear();

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = EngineConfig::MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers(context.getDevice(), allocInfo);
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
    auto& commandBuffer = commandBuffers[frameIndex];

    uint32_t startIndex = frameIndex * EngineConfig::TIMESTAMPS_PER_FRAME;

    commandBuffer.begin({});

    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, context.getTimestampQueryPool(), startIndex);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, particleComputePipeline.getComputePipeline());
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        particleComputePipeline.getPipelineLayout(),
        0,
        { particleComputePipeline.getDescriptorSets()[0] },
        nullptr
    );
    commandBuffer.dispatch((EngineConfig::PARTICLE_COUNT + 255) / 256, 1, 1);

    vk::BufferMemoryBarrier2 barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eVertexAttributeInput,
        .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead,
        .buffer = particleComputePipeline.getParticleBuffer().buffer,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier
    };

    commandBuffer.pipelineBarrier2(dependencyInfo);

    // Before starting rendering, transition the compute target image to COLOR_ATTACHMENT_OPTIMAL
    transitionImageLayout(
        imageComputePipeline.getImage(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                         // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,                 // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,          // dstStage
        vk::ImageAspectFlagBits::eColor,
        commandBuffer
    );

    transitionImageLayout(
        swapChainManager.getDepthImage().image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth,
        commandBuffer
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = imageComputePipeline.getImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = swapChainManager.getDepthImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = { 0, 0 }, .extent = swapChainManager.getExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo); // WAHOOOOOOOOOOOOOOOOOOOOOOOOOOOO

    // Scene objects

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getGraphicsPipeline());

    // Wow, we set the dynamic settings we specified earlier!
    commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainManager.getExtent().width), static_cast<float>(swapChainManager.getExtent().height), 0.0f, 1.0f));
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainManager.getExtent()));

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getPipelineLayout(), 0, *descriptorSets[frameIndex], nullptr);

    for (auto& mesh : sceneObjects) {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &mesh->getVertexBuffer().buffer, &offset); // Raii doesn't play nicely with VMA so we make this call directly on the unwrapped command buffer
        vkCmdBindIndexBuffer(*commandBuffer, mesh->getIndexBuffer().buffer, offset, VK_INDEX_TYPE_UINT16);

        commandBuffer.drawIndexed(mesh->getIndexCount(), 1, 0, 0, 0);
    }

    // Snow

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, particleGraphicsPipeline.getGraphicsPipeline());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, particleGraphicsPipeline.getPipelineLayout(), 0, *descriptorSets[frameIndex], nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &particleComputePipeline.getParticleBuffer().buffer, &offset);
    commandBuffer.draw(EngineConfig::PARTICLE_COUNT, 1, 0, 0);

    commandBuffer.endRendering();

    // NOW THE COMPUTE STAGE

    transitionImageLayout( // Transition compute image to read only
        imageComputePipeline.getImage(),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::ImageAspectFlagBits::eColor,
        commandBuffer
    );

    // NOTE: We could modify transitionImage so that we can use a single barrier with these two transitionImageLayout calls
    transitionImageLayout( // Transition swapchain image so it can be written to
        swapChainManager.getImages()[imageIndex],              
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        vk::AccessFlagBits2::eNone,                     // srcAccessMask
        vk::AccessFlagBits2::eShaderWrite,              // dstAccessMask
        vk::PipelineStageFlagBits2::eTopOfPipe,                // srcStage
        vk::PipelineStageFlagBits2::eComputeShader,             // dstStage
        vk::ImageAspectFlagBits::eColor,
        commandBuffer
    );

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, imageComputePipeline.getComputePipeline());
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        imageComputePipeline.getPipelineLayout(),
        0,
        { imageComputePipeline.getDescriptorSets()[imageIndex] },
        nullptr
    );
    commandBuffer.dispatch(((swapChainManager.getExtent().width + 15) / 16.0), ((swapChainManager.getExtent().height+15)/16), 1);

    transitionImageLayout(
        swapChainManager.getImages()[imageIndex],              
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::ePresentSrcKHR,        
        vk::AccessFlagBits2::eShaderWrite,              // srcAccessMask
        {},                                             // dstAccessMask
        vk::PipelineStageFlagBits2::eComputeShader,     // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      // dstStage
        vk::ImageAspectFlagBits::eColor,
        commandBuffer
    );

    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, context.getTimestampQueryPool(), startIndex+1);

    commandBuffer.end();
}

void Renderer::transitionImageLayout(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask,
    vk::ImageAspectFlags imageAspectFlags,
    vk::raii::CommandBuffer& curCommandBuffer
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = imageAspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    curCommandBuffer.pipelineBarrier2(dependencyInfo);
}

void Renderer::createSyncObjects() {
    // Semaphore: Order execution on the GPU
    // Fence: Block CPU (host) until signaled
    // Knowledge check: Why the three sync objects specified in the tutorial?
    // Knowledge check: Why are there two loops here?

    auto& device = context.getDevice();

    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

    for (size_t i = 0; i < EngineConfig::MAX_FRAMES_IN_FLIGHT; i++) {
        presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo())); // Notice device is specified
        inFlightFences.emplace_back(vk::raii::Fence(device, { .flags = vk::FenceCreateFlagBits::eSignaled }));
    }

    for (size_t i = 0; i < swapChainManager.getImages().size(); i++) {
        renderFinishedSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
    }
}


void Renderer::createUniformBuffers() {
    uniformBuffers.reserve(EngineConfig::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < EngineConfig::MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers.push_back(
            context.createVmaBuffer(
                sizeof(MVP),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                VMA_MEMORY_USAGE_AUTO
            )
        );
    }
}

void Renderer::updateUniformBuffer(uint32_t frameIndex, float totalTime, float deltaTime) {
    MVP ubo{};

    ubo.model = glm::mat4(1.0f);
    
    // Orbit camera
    float radius = 3.5f;
    glm::vec3 center = glm::vec3(0, 0, 0.2f);

    float orbitSpeed = 0.2f;
    float cameraX = sin(totalTime * orbitSpeed) * radius;
    float cameraY = cos(totalTime * orbitSpeed) * radius;

    ubo.view = lookAt(glm::vec3(cameraX, cameraY, 0.0f), center, glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainManager.getExtent().width) / static_cast<float>(swapChainManager.getExtent().height), 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffers[frameIndex].info.pMappedData, &ubo, sizeof(ubo));
}

void Renderer::createDescriptorPool() {
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, EngineConfig::MAX_FRAMES_IN_FLIGHT);

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = EngineConfig::MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    descriptorPool = vk::raii::DescriptorPool(context.getDevice(), poolInfo);
}

void Renderer::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts;
    layouts.reserve(EngineConfig::MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < EngineConfig::MAX_FRAMES_IN_FLIGHT; i++) {
        layouts.push_back(*graphicsPipeline.getDescriptorSetLayout());
    }

    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descriptorSets = vk::raii::DescriptorSets(context.getDevice(), allocInfo);

    for (uint32_t i = 0; i < EngineConfig::MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i].buffer,
            .offset = 0,
            .range = sizeof(MVP),
        };

        vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo
        };

        context.getDevice().updateDescriptorSets(descriptorWrite, nullptr);
    }
}

void Renderer::loadMeshes() {
    sceneObjects.push_back(std::make_unique<Mesh>("assets/models/lowpolytree.obj", context));
}