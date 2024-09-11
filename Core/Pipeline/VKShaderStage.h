#ifndef VK_SHADER_STAGE_H
#define VK_SHADER_STAGE_H

#include "VKPipelineMgr.h"

namespace Core {
    class VKShaderStage: protected virtual VKPipelineMgr {
        private:
            Log::Record* m_VKShaderStageLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

            /* Read all of the bytes from the specified file and return them in a byte array managed by std::vector. This 
             * function is used to read shader binary files
            */
            std::vector <char> getByteCode (const char* filePath) {
                /* ate: Start reading at the end of the file
                 * binary: Read the file as binary file (avoid text transformations)
                 *
                 * The advantage of starting to read at the end of the file is that we can use the read position to 
                 * determine the size of the file and allocate a buffer
                */
                std::ifstream file (filePath, std::ios::ate | std::ios::binary);
                if (!file.is_open()) {
                    LOG_WARNING (m_VKShaderStageLog) << "Failed to open file " 
                                                     << "[" << filePath << "]" 
                                                     << std::endl;
                    return {};
                }

                size_t fileSize = static_cast <size_t> (file.tellg());
                std::vector <char> buffer (fileSize);
                /* Seek back to the beginning of the file and read all of the bytes at once
                */
                file.seekg (0);
                file.read (buffer.data(), fileSize);

                file.close();
                return buffer;
            }

            /* Before we can pass the shader code to the pipeline, we have to wrap it in a VkShaderModule object. Shader 
             * modules are just a thin wrapper around the shader byte code that we've previously loaded from a file
            */
            VkShaderModule getShaderModule (uint32_t deviceInfoId, const std::vector <char>& shaderCode) {
                auto deviceInfo = getDeviceInfo (deviceInfoId);

                VkShaderModuleCreateInfo createInfo;
                createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createInfo.pNext    = VK_NULL_HANDLE;
                createInfo.flags    = 0;
                createInfo.codeSize = shaderCode.size();
                /* The size of the bytecode is specified in bytes, but the bytecode pointer is a uint32_t pointer rather 
                 * than a char pointer. Therefore we will need to cast the pointer with reinterpret_cast
                */
                createInfo.pCode = reinterpret_cast <const uint32_t*> (shaderCode.data());

                VkShaderModule shaderModule;
                VkResult result = vkCreateShaderModule (deviceInfo->resource.logDevice, 
                                                        &createInfo, 
                                                        VK_NULL_HANDLE, 
                                                        &shaderModule);
                if (result != VK_SUCCESS) {
                    LOG_WARNING (m_VKShaderStageLog) << "Failed to create shader module " 
                                                     << "[" << string_VkResult (result) << "]"
                                                     << std::endl;
                    return VK_NULL_HANDLE;          
                }
                return shaderModule;
            }

        public:
            VKShaderStage (void) {
                m_VKShaderStageLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE); 
            }

            ~VKShaderStage (void) { 
                LOG_CLOSE (m_instanceId);
            }

        protected:  
            VkShaderModule createShaderStage (uint32_t deviceInfoId,
                                              uint32_t pipelineInfoId,
                                              VkShaderStageFlagBits stage,
                                              const char* shaderBinaryPath,
                                              const char* entryPoint) {

                auto pipelineInfo = getPipelineInfo (pipelineInfoId);
                auto shaderCode   = getByteCode     (shaderBinaryPath);

                if (shaderCode.size() == 0) {
                    LOG_ERROR (m_VKShaderStageLog) << "Invalid file size for shader file " 
                                                   << "[" << pipelineInfoId << "]"
                                                   << " "
                                                   << "[" << shaderBinaryPath << "]"
                                                   << std::endl;
                    throw std::runtime_error ("Invalid file size for shader file");
                }

                auto module = getShaderModule (deviceInfoId, shaderCode);
                if (module == VK_NULL_HANDLE) {
                    LOG_ERROR (m_VKShaderStageLog) << "Invalid shader module "
                                                   << "[" << pipelineInfoId << "]"
                                                   << " "
                                                   << "[" << shaderBinaryPath << "]"
                                                   << std::endl; 
                    throw std::runtime_error ("Invalid shader module");
                }

                VkPipelineShaderStageCreateInfo createInfo;
                createInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                createInfo.pNext  = VK_NULL_HANDLE;
                createInfo.flags  = 0; 
                createInfo.stage  = stage;
                createInfo.module = module;
                /* The shader function to invoke (called as entry point) is specified here. That means that it's possible 
                 * to combine multiple fragment shaders into a single shader module and use different entry points to 
                 * differentiate between their behaviors 
                */
                createInfo.pName  = entryPoint;
                /* This field allows you to specify values for shader constants. You can use a single shader module where 
                 * its behavior can be configured at pipeline creation by specifying different values for the constants 
                 * used in it. This is more efficient than configuring the shader using variables at render time, because 
                 * the compiler can do optimizations like eliminating if statements that depend on these values. If you 
                 * don't have any constants like that, then you can set the member to VK_NULL_HANDLE
                */
                createInfo.pSpecializationInfo = VK_NULL_HANDLE;

                pipelineInfo->state.stages.push_back (createInfo);
                return module;
            }
    };
}   // namespace Core
#endif  // VK_SHADER_STAGE_H