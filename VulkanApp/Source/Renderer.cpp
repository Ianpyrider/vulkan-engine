#include "VulkanInclude.h"

#include "Renderer.h"

#include "EngineConfig.h"
#include "VulkanContext.h"
#include "SwapChainManager.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

#include "Vertex.h"

const std::vector<Vertex> triangleInfo = {
    { glm::vec2(0.0, -0.5), glm::vec3(1.0, 0.0, 0.0) },
    { glm::vec2(0.5, 0.5), glm::vec3(0.0, 1.0, 0.0) },
    { glm::vec2(-0.5, 0.5), glm::vec3(0.0, 0.0, 1.0) }
};

Renderer::Renderer(VulkanContext& context, SwapChainManager& swapChainManager, GraphicsPipeline& graphicsPipeline, ComputePipeline& computePipeline)
    : context(context), swapChainManager(swapChainManager), graphicsPipeline(graphicsPipeline), computePipeline(computePipeline) {
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    createVertexBuffer();
}

Renderer::~Renderer() {
    vmaDestroyBuffer(context.getVmaAllocator(), vertexBuffer.buffer, vertexBuffer.allocation);
}

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
        .commandBufferCount = engineConfig::MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers(context.getDevice(), allocInfo);
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
    auto& commandBuffer = commandBuffers[frameIndex];

    uint32_t startIndex = frameIndex * engineConfig::TIMESTAMPS_PER_FRAME;

    commandBuffer.begin({});

    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, context.getTimestampQueryPool(), startIndex);

    // Before starting rendering, transition the compute target image to COLOR_ATTACHMENT_OPTIMAL
    transitionImageLayout(
        computePipeline.getImage(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                         // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,                 // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,          // dstStage
        commandBuffer
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        //.imageView = swapChainManager.getImageViews()[imageIndex],
        .imageView = computePipeline.getImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = { 0, 0 }, .extent = swapChainManager.getExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo); // WAHOOOOOOOOOOOOOOOOOOOOOOOOOOOO

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getGraphicsPipeline());

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &vertexBuffer.buffer, &offset); // Raii doesn't play nicely with VMA so we make this call directly on the unwrapped command buffer
    
    // Wow, we set the dynamic settings we specified earlier!
    commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainManager.getExtent().width), static_cast<float>(swapChainManager.getExtent().height), 0.0f, 1.0f));
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainManager.getExtent()));

    commandBuffer.draw(triangleInfo.size(), 1, 0, 0);

    commandBuffer.endRendering();

    // NOW THE COMPUTE STAGE

    transitionImageLayout( // Transition compute image to read only
        computePipeline.getImage(),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eComputeShader,
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
        commandBuffer
    );

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.getComputePipeline());
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        computePipeline.getPipelineLayout(),
        0,
        { computePipeline.getDescriptorSets()[imageIndex] },
        nullptr
    );
    commandBuffer.dispatch(swapChainManager.getExtent().width, swapChainManager.getExtent().height, 1);

    transitionImageLayout(
        swapChainManager.getImages()[imageIndex],              
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::ePresentSrcKHR,        
        vk::AccessFlagBits2::eShaderWrite,              // srcAccessMask
        {},                                             // dstAccessMask
        vk::PipelineStageFlagBits2::eComputeShader,     // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      // dstStage
        commandBuffer
    );

    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, context.getTimestampQueryPool(), startIndex+1);

    commandBuffer.end();
}

void Renderer::transitionImageLayout(
    vk::Image& image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask,
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
            .aspectMask = vk::ImageAspectFlagBits::eColor,
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

    for (size_t i = 0; i < engineConfig::MAX_FRAMES_IN_FLIGHT; i++) {
        presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo())); // Notice device is specified
        inFlightFences.emplace_back(vk::raii::Fence(device, { .flags = vk::FenceCreateFlagBits::eSignaled }));
    }

    for (size_t i = 0; i < swapChainManager.getImages().size(); i++) {
        renderFinishedSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
    }
}

void Renderer::drawFrame() {
    auto& device = context.getDevice();

    // Making sure we're not fenced here means we wait until we get a signal from graphicsQueue.submit for the corresponding frame
    auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);

    if (fenceResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to wait for fence!");
    }

    if (engineConfig::PRINT_GPU_PROFILING && warmUpFrames >= 2) {
        printf("Render pass time (ms): %f\n", context.getRenderPassTime(frameIndex));
    }
    else {
        warmUpFrames++;
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


    frameIndex = (frameIndex + 1) % engineConfig::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::createVertexBuffer() { 
    // Apparently using this buffer that's accessible to both the GPU and CPU is inefficient, can update for performance later
    vertexBuffer = context.createVmaBuffer(sizeof(Vertex) * triangleInfo.size());
    memcpy(vertexBuffer.info.pMappedData, triangleInfo.data(), sizeof(Vertex) * triangleInfo.size());
}