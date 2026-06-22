#include "shared/VulkanInclude.h"

#include "renderer/GraphicsPipeline.h"

#include "resources/Vertex.h"
#include "shared/FileUtils.h"
#include "shared/VulkanUtils.h"
#include  "shared/EngineConfig.h"

#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"

#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

GraphicsPipeline::GraphicsPipeline(VulkanContext& context, SwapChainManager& swapChainManager, const PipelineConfig& config)
    : context(context), swapChainManager(swapChainManager), config(config) {
    createDescriptorSetLayouts();
    createGraphicsPipeline();
}

void GraphicsPipeline::createDescriptorSetLayouts() {
    descriptorSetLayouts.reserve(2);

    // Global (UBO) Layout

    std::array<vk::DescriptorSetLayoutBinding, 1> uboBindings{ {
        {.binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
    } };

    vk::DescriptorSetLayoutCreateInfo uboLayoutInfo{ 
        .bindingCount = static_cast<uint32_t>(uboBindings.size()), 
        .pBindings = uboBindings.data()
    };

    descriptorSetLayouts.push_back(vk::raii::DescriptorSetLayout(context.getDevice(), uboLayoutInfo));

    // IBL Info Layout

    std::array<vk::DescriptorSetLayoutBinding, 3> iblBindings{ {
        { .binding = 0, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment },
        { .binding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment },
        { .binding = 2, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment },
    }};

    vk::DescriptorSetLayoutCreateInfo iblLayoutInfo{
        .bindingCount = static_cast<uint32_t>(iblBindings.size()),
        .pBindings = iblBindings.data()
    };

    descriptorSetLayouts.push_back(vk::raii::DescriptorSetLayout(context.getDevice(), iblLayoutInfo));
}

void GraphicsPipeline::createGraphicsPipeline() {
    auto& device = context.getDevice();

    // Create shaderModule, which takes in vertex and frag shaders here. Basically specify and program the pipeline steps you want to
    vk::raii::ShaderModule shaderModule = VulkanUtils::createShaderModule(device, FileUtils::readFile(config.shaderPath));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    // Set dynamic states

    std::vector dynamicStates = { // Optionally specify settings to be defined at draw time, MUST BE SPECIFIED THEN IF YOU DO THIS
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    auto bindingDescriptions = config.bindingDescriptions;
    auto attributeDescriptions = config.attributeDescriptions;

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
        .pVertexBindingDescriptions = bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = config.topology
    };

    // Notice these are what we marked as dynamic state earlier, now specified in the viewport state
    vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // Format layouts how they want it

    std::vector<vk::DescriptorSetLayout> rawLayouts;
    rawLayouts.reserve(descriptorSetLayouts.size());

    for (const auto& layout : descriptorSetLayouts) {
        rawLayouts.push_back(*layout);
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(rawLayouts.size()),
        .pSetLayouts = rawLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size()),
        .pPushConstantRanges = config.pushConstantRanges.data()
    };

    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    auto format = swapChainManager.getSurfaceFormat().format;

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False
    };

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &format};

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = nullptr
    };

    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineRenderingCreateInfo.depthAttachmentFormat = swapChainManager.getDepthFormat();

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}