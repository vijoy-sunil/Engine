#ifndef VK_PIPELINE_H
#define VK_PIPELINE_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VKUtils.h"
#include "VKVertexData.h"
#include "VKRenderPass.h"
#include "VKDescriptor.h"
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKPipeline: protected VKUtils,
                      protected virtual VKVertexData,
                      protected virtual VKRenderPass,
                      protected VKDescriptor {
        private:
            /* Handle to pipeline layout object
            */
            VkPipelineLayout m_pipelineLayout;
            /* Handle to the pipeline
            */
            VkPipeline m_graphicsPipeline;
            /* Handle to the log object
            */
            static Log::Record* m_VKPipelineLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 4;
            /* Before we can pass the shader code to the pipeline, we have to wrap it in a VkShaderModule object. Shader 
             * modules are just a thin wrapper around the shader bytecode that we've previously loaded from a file and 
             * the functions defined in it
            */
            VkShaderModule createShaderModule (const std::vector <char>& code) {
                VkShaderModuleCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createInfo.codeSize = code.size();
                /* The size of the bytecode is specified in bytes, but the bytecode pointer is a uint32_t pointer rather 
                 * than a char pointer. Therefore we will need to cast the pointer with reinterpret_cast
                */
                createInfo.pCode = reinterpret_cast <const uint32_t*> (code.data());

                VkShaderModule shaderModule;
                VkResult result = vkCreateShaderModule (getLogicalDevice(), &createInfo, nullptr, &shaderModule);
                if (result != VK_SUCCESS) {
                    LOG_WARNING (m_VKPipelineLog) << "Failed to create shader module" << " " << result << std::endl;
                    return VK_NULL_HANDLE;          
                }
                return shaderModule;
            }

        public:
            VKPipeline (void) {
                m_VKPipelineLog = LOG_INIT (m_instanceId, 
                                            static_cast <Log::e_level> (TOGGLE_CORE_LOGGING & Log::VERBOSE),
                                            Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                            "./Build/Log/"); 
            }

            ~VKPipeline (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            /* An overview of the pipeline
             * Vertex/Index Buffer
             *      |
             * Input Assembler      [FIXED FUNCTION]
             * The input assembler collects the raw vertex data from the buffers you specify and may also use an index 
             * buffer to repeat certain elements without having to duplicate the vertex data itself
             *      |
             * Vertex Shader        [PROGRAMMABLE]
             * The vertex shader is run for every vertex and generally applies transformations to turn vertex positions 
             * from model space to screen space. It also passes per-vertex data (eg: color) down the pipeline.
             *      |
             * Tessellation         [PROGRAMMABLE]
             * The tessellation shaders allow you to subdivide geometry based on certain rules to increase the mesh quality
             *      |
             * Geometry Shader      [PROGRAMMABLE]
             * The geometry shader is run on every primitive (triangle, line, point) and can discard it or output more 
             * primitives than came in. This is similar to the tessellation shader, but much more flexible. However, it 
             * is not used much in today's applications because the performance is not that good on most graphics cards
             *      |
             * Rasterization        [FIXED FUNCTION]
             * The rasterization stage discretizes the primitives into fragments. These are the pixel elements that they 
             * fill on the framebuffer. Any fragments that fall outside the screen are discarded and the attributes 
             * outputted by the vertex shader are interpolated across the fragments. Usually the fragments that are 
             * behind other primitive fragments are also discarded here because of depth testing
             *      |
             * Fragement Shader     [PROGRAMMABLE]
             * The fragment shader is invoked for every fragment that survives and determines which framebuffer(s) the 
             * fragments are written to and with which color and depth values
             *      |     
             * Color Blending       [FIXED FUNCTION]
             * The color blending stage applies operations to mix different fragments that map to the same pixel in the 
             * framebuffer. Fragments can simply overwrite each other, add up or be mixed based upon transparency
             * 
             * Fixed function stages allow you to tweak their operations using parameters, but the way they work is 
             * predefined. Programmable stages are programmable, which means that you can upload your own code to the 
             * graphics card to apply exactly the operations you want
            */
            void createGraphicsPipeline (void) {
                /* Setup vertex input
                 * The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be 
                 * passed to the vertex shader. It describes this in roughly two ways
                 * 
                 * Bindings: spacing between data and whether the data is per-vertex or per-instance (instancing is the 
                 * practice of rendering multiple copies of the same mesh in a scene at once. This technique is primarily 
                 * used for objects such as trees, grass, or buildings which can be represented as repeated geometry 
                 * without appearing unduly repetitive)
                 * 
                 * Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them 
                 * from and at which offset
                */
                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                /* Specify the binding and attribute desctiprion 
                */
                auto bindingDescription = getBindingDescription();
                auto attributeDescriptions = getAttributeDescriptions();

                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
                vertexInputInfo.vertexAttributeDescriptionCount = static_cast <uint32_t> (attributeDescriptions.size());
                vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

                /* Setup input assembler
                 * The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be 
                 * drawn from the vertices and if primitive restart should be enabled
                 * 
                 * VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
                 * VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
                 * VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next 
                 * line
                 * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
                 * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first 
                 * two vertices of the next triangle
                */
                VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                /* If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and 
                 * triangles in the _STRIP topology modes
                */
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                /* Setup vertex shader and fragment shader pipeline stages
                */
                auto vertShaderCode = readFile (VERTEX_SHADER_BINARY);
                auto fragShaderCode = readFile (FRAGMENT_SHADER_BINARY);
                /* read file error
                */
                if (vertShaderCode.size() == 0 || fragShaderCode.size() == 0) {
                    LOG_ERROR (m_VKPipelineLog) << "Invalid file size for shader files" << std::endl;
                    throw std::runtime_error ("Invalid file size for shader files");
                }

                /* The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU doesn't 
                 * happen until the graphics pipeline is created. That means that we're allowed to destroy the shader 
                 * modules as soon as pipeline creation is finished, which is why we'll make them local variables in the 
                 * createGraphicsPipeline function instead of class members
                */
                VkShaderModule vertShaderModule = createShaderModule (vertShaderCode);
                VkShaderModule fragShaderModule = createShaderModule (fragShaderCode);
                if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
                    LOG_ERROR (m_VKPipelineLog) << "Invalid shader modules" << std::endl;
                    throw std::runtime_error ("Invalid shader modules");
                }

                /* To actually use the shaders we'll need to assign them to a specific pipeline stage through 
                 * VkPipelineShaderStageCreateInfo structures as part of the actual pipeline creation process
                */
                VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
                vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                /* There is an enum value for each of the programmable stages in the pipeline
                */
                vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                vertShaderStageInfo.module = vertShaderModule;
                /* The shader function to invoke (called as entrypoint) is specified here. That means that it's possible 
                 * to combine multiple fragment shaders into a single shader module and use different entry points to 
                 * differentiate between their behaviors 
                */
                vertShaderStageInfo.pName = "main";
                /* This field allows you to specify values for shader constants. You can use a single shader module where 
                 * its behavior can be configured at pipeline creation by specifying different values for the constants 
                 * used in it. This is more efficient than configuring the shader using variables at render time, because 
                 * the compiler can do optimizations like eliminating if statements that depend on these values. If you 
                 * don't have any constants like that, then you can set the member to nullptr
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
                 * combinations of states you want to use in your rendering operations. However, because all of the 
                 * operations you'll be doing in the pipeline are known in advance, the driver can optimize for it much 
                 * better
                 * 
                 * However, a limited amount of the state can actually be changed without recreating the pipeline at 
                 * draw time. Examples are the size of the viewport, line width and blend constants. If you want to use 
                 * dynamic state and keep these properties out, then you'll have to fill in a 
                 * VkPipelineDynamicStateCreateInfo structure
                 * 
                 * This will cause the configuration of these values to be ignored and you will be able (and required) 
                 * to specify the data at drawing time. This results in a more flexible setup and is very common for 
                 * things like viewport and scissor state.
                 * 
                 * Viewport
                 * A viewport basically describes the region of the framebuffer that the output will be rendered to. This 
                 * will almost always be (0, 0) to (width, height). Remember that the size of the swap chain and its 
                 * images may differ from the WIDTH and HEIGHT of the window. The swap chain images will be used as 
                 * framebuffers later on, so we should stick to their size
                 * viewport.width = static_cast <float> (getSwapChainExtent().width);
                 * viewport.height = static_cast <float> (getSwapChainExtent().height);
                 * 
                 * Scissor rectangle
                 * While viewports define the transformation from the image to the framebuffer, scissor rectangles define 
                 * in which regions pixels will actually be stored. Any pixels outside the scissor rectangles will be 
                 * discarded by the rasterizer. They function like a filter rather than a transformation. So if we wanted 
                 * to draw to the entire framebuffer, we would specify a scissor rectangle that covers it entirely
                 * 
                 * Dynamic state allows us set up the actual viewport(s) and scissor rectangle(s) up at drawing time.
                */
                std::vector <VkDynamicState> dynamicStates = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR
                };

                VkPipelineDynamicStateCreateInfo dynamicState{};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.dynamicStateCount = static_cast <uint32_t> (dynamicStates.size());
                dynamicState.pDynamicStates = dynamicStates.data();

                /* Without dynamic state, the viewport and scissor rectangle need to be set in the pipeline using the 
                 * VkPipelineViewportStateCreateInfo struct. This makes the viewport and scissor rectangle for this 
                 * pipeline immutable. Any changes required to these values would require a new pipeline to be created 
                 * with the new values
                 * 
                 * VkViewport viewport{};
                 * viewport.x = 0.0f;
                 * viewport.y = 0.0f;
                 * viewport.width = static_cast <float> (getSwapChainExtent().width);
                 * viewport.height = static_cast <float> (getSwapChainExtent().height);
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
                 * It's is possible to use multiple viewports and scissor rectangles on some graphics cards, so the 
                 * structure members reference an array of them. For now, it is just one which is specified below using 
                 * the count field
                */
                VkPipelineViewportStateCreateInfo viewportState{};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.scissorCount = 1;

                /* Setup rasterizer
                 * The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it 
                 * into fragments to be colored by the fragment shader. It also performs depth testing, face culling and 
                 * the scissor test, and it can be configured to output fragments that fill entire polygons or just the 
                 * edges (wireframe rendering). All this is configured using the VkPipelineRasterizationStateCreateInfo 
                 * structure
                 * 
                 * depth testing
                 * When an object is projected on the screen, the depth (z-value) of a generated fragment in the projected 
                 * screen image is compared to the value already stored in the buffer (depth test), and replaces it if 
                 * the new value is closer
                 * 
                 * face culling
                 * If we imagine any closed shape, each of its faces has two sides. Each side would either face the user 
                 * or show its back to the user. What if we could only render the faces that are facing the viewer? This 
                 * is exactly what face culling does.
                */
                VkPipelineRasterizationStateCreateInfo rasterizer{};
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                /* If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are 
                 * clamped to them as opposed to discarding them. This is useful in some special cases like shadow maps 
                 * (technique that generates fast approximate shadows.)
                */
                rasterizer.depthClampEnable = VK_FALSE;
                /* If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. 
                 * This basically disables any output to the framebuffer
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
                /* The cullMode variable determines the type of face culling to use. You can disable culling, cull the 
                 * front faces, cull the back faces or both. The frontFace variable specifies the vertex order for faces 
                 * to be considered front-facing and can be clockwise or counterclockwise
                */
                rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                /* The rasterizer can alter the depth values by adding a constant value or biasing them based on a 
                 * fragment's slope. This is sometimes used for shadow mapping
                */
                rasterizer.depthBiasEnable = VK_FALSE;
                rasterizer.depthBiasConstantFactor = 0.0f; 
                rasterizer.depthBiasClamp = 0.0f; 
                rasterizer.depthBiasSlopeFactor = 0.0f;      
                /* Depth and stencil testing
                 * Once the fragment shader has processed the fragment a so called stencil test is executed that, just 
                 * like the depth test, has the option to discard fragments using stencil
                */  

                /* Setup multisampling
                 * The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the ways to 
                 * perform anti-aliasing. It works by combining the fragment shader results of multiple polygons that 
                 * rasterize to the same pixel. This mainly occurs along edges, which is also where the most noticeable 
                 * aliasing artifacts occur. Because it doesn't need to run the fragment shader multiple times if only 
                 * one polygon maps to a pixel, it is significantly less expensive than simply rendering to a higher 
                 * resolution and then downscaling (known as super sampling)
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
                 * After a fragment shader has returned a color, it needs to be combined with the color that is already 
                 * in the framebuffer. This transformation is known as color blending and there are two ways to do it:
                 * (1) Mix the old and new value to produce a final color
                 * (2) Combine the old and new value using a bitwise operation
                 * 
                 * There are two types of structs to configure color blending. The first struct, 
                 * VkPipelineColorBlendAttachmentState contains the configuration per attached framebuffer and the second 
                 * struct, VkPipelineColorBlendStateCreateInfo contains the global color blending settings
                */
                VkPipelineColorBlendAttachmentState colorBlendAttachment{};
                /* This per-framebuffer struct allows you to configure the first way of color blending (if set to true) 
                 * using the formula configured using the struct members. If blendEnable is set to VK_FALSE, then the new 
                 * color from the fragment shader is passed through unmodified
                */
                colorBlendAttachment.blendEnable = VK_FALSE;
                /* The formula:
                 * finalColor.rgb = 
                 * (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb)
                 * 
                 * finalColor.a = 
                 * (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
                 * 
                 * The resulting color is AND'd with the colorWriteMask to determine which channels are actually passed 
                 * through
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
                /* Example: The most common way to use color blending is to implement alpha blending, where we want the 
                 * new color to be blended with the old color based on its opacity
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

                /* The second structure references the array of structures for all of the framebuffers and allows you to 
                 * set blend constants that you can use as blend factors in the aforementioned calculations
                */
                VkPipelineColorBlendStateCreateInfo colorBlending{};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                /* If you want to use the second method of blending (bitwise combination), then you should set 
                 * logicOpEnable to VK_TRUE. The bitwise operation can then be specified in the logicOp field. Note that 
                 * this will automatically disable the first method, as if you had set blendEnable to VK_FALSE for every 
                 * attached framebuffer. However, the colorWriteMask will also be used in this mode to determine which 
                 * channels in the framebuffer will actually be affected
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
                 * commonly used to pass the transformation matrix to the vertex shader, or to create texture samplers in 
                 * the fragment shader. Push constants are another way of passing dynamic values to shaders
                 * 
                 * These uniform values need to be specified during pipeline creation by creating a VkPipelineLayout 
                 * object
                */
                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                /* setLayoutCount is the number of descriptor sets included in the pipeline layout and pSetLayouts is a 
                 * pointer to an array of VkDescriptorSetLayout objects, meaning, it's possible to specify multiple 
                 * descriptor set layouts here. For example:
                 * 
                 * (1) Descriptor set layout A
                 * 'A' may contain layout info about an array of UBOs (binding 0), and, another UBO (binding 1)
                 * 
                 * (2) Descriptor set layout B
                 * 'B' may contain a differnt UBO (binding 0)
                 * 
                 * We then allocation descriptor set A and B from the pool (could be same/different pool), and finally
                 * bind them to the descriptors in the shader in the recordCommandBuffer function. The Shaders can then 
                 * reference specific descriptor sets like this
                 * 
                 * layout (set = 0, binding = 0) uniform UniformBufferObject_A0 { ... }
                 * layout (set = 0, binding = 1) uniform UniformBufferObject_A1 { ... }
                 * layout (set = 1, binding = 0) uniform UniformBufferObject_B0 { ... }
                 * 
                 * A use case would be, to put descriptors that vary per-object and descriptors that are shared into 
                 * separate descriptor sets. In that case you avoid rebinding most of the descriptors across draw calls
                 * (recordCommandBuffer) which is potentially more efficient
                */
                pipelineLayoutInfo.setLayoutCount = 1; 
                pipelineLayoutInfo.pSetLayouts = &getDescriptorSetLayout();
                pipelineLayoutInfo.pushConstantRangeCount = 0; 
                pipelineLayoutInfo.pPushConstantRanges = nullptr;

                VkResult result =  vkCreatePipelineLayout (getLogicalDevice(), 
                                                           &pipelineLayoutInfo, 
                                                           nullptr, 
                                                           &m_pipelineLayout);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKPipelineLog) << "Failed to create pipeline layout" << " " << result << std::endl;
                    throw std::runtime_error ("Failed to create pipeline layout");
                }

                /* What do we have until now?
                 * Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, 
                 * like input assembly, rasterizer, viewport and color blending
                 * 
                 * Shader stages: the shader modules that define the functionality of the programmable stages of the 
                 * graphics pipeline
                 * 
                 * Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
                 * 
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
                pipelineInfo.layout = m_pipelineLayout;

                pipelineInfo.renderPass = getRenderPass();
                /* index of the sub pass in the render pass
                */
                pipelineInfo.subpass = 0;
                /* Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. The idea of 
                 * pipeline derivatives is that it is less expensive to set up pipelines when they have much 
                 * functionality in common with an existing pipeline and switching between pipelines from the same parent 
                 * can also be done quicker
                */
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
                pipelineInfo.basePipelineIndex = -1;
                
                /* Create the pipeline
                 * The vkCreateGraphicsPipelines function actually has more parameters than the usual object creation 
                 * functions in Vulkan. It is designed to take multiple VkGraphicsPipelineCreateInfo objects and create 
                 * multiple VkPipeline objects in a single call.
                 * 
                 * The second parameter, for which we've passed the VK_NULL_HANDLE argument, references an optional 
                 * VkPipelineCache object. A pipeline cache can be used to store and reuse data relevant to pipeline 
                 * creation across multiple calls to vkCreateGraphicsPipelines and even across program executions if the 
                 * cache is stored to a file. This makes it possible to significantly speed up pipeline creation at a 
                 * later time
                */
                result = vkCreateGraphicsPipelines (getLogicalDevice(), 
                                                    VK_NULL_HANDLE,
                                                    1,
                                                    &pipelineInfo,
                                                    nullptr, 
                                                    &m_graphicsPipeline);
                if (result != VK_SUCCESS) {
                    LOG_ERROR (m_VKPipelineLog) << "Failed to create graphics pipeline" << " " << result << std::endl;
                    throw std::runtime_error ("Failed to create graphics pipeline");                
                }

                /* Destroy shader modules
                */
                vkDestroyShaderModule (getLogicalDevice(), vertShaderModule, nullptr);
                vkDestroyShaderModule (getLogicalDevice(), fragShaderModule, nullptr);
            }

            VkPipeline getPipeline (void) {
                return m_graphicsPipeline;
            }

            VkPipelineLayout getPipelineLayout (void) {
                return m_pipelineLayout;
            }

            void cleanUp (void) {
                /* Destroy pipeline
                */
                vkDestroyPipeline (getLogicalDevice(), m_graphicsPipeline, nullptr);
                /* Destroy pipeline layout
                */
                vkDestroyPipelineLayout (getLogicalDevice(), m_pipelineLayout, nullptr);
            }
    };

    Log::Record* VKPipeline::m_VKPipelineLog;
}   // namespace Renderer
#endif  // VK_PIPELINE_H
