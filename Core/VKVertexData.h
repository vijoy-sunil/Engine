#ifndef VK_VERTEX_DATA_H
#define VK_VERTEX_DATA_H
/* GLFW will include its own definitions and automatically load the Vulkan header vulkan/vulkan.h with it
*/
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
/* GLM library provides us with linear algebra related types like vectors and matrices
*/
#include <glm/glm.hpp>
#include "../Collections/Log/include/Log.h"
#include <vector>

using namespace Collections;

namespace Renderer {
    class VKVertexData {
        private:
            struct Vertex {
                /* Define the attributes that we're going to use in the vertex shader
                */
                glm::vec2 pos;
                glm::vec3 color;
            }; 
            /* The position and color values are combined into one array of vertices. This is known as interleaving 
             * vertex attributes
            */
            const std::vector <Vertex> m_vertices = {
                {   {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}  },  // top left
                {   {0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}  },  // top right
                {   {0.5f, 0.5f},   {0.0f, 0.0f, 1.0f}  },  // bottom right
                {   {-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}  }   // bottom left
            };
            /* Contents of index buffer
             * Note that it is possible to use either uint16_t or uint32_t for your index buffer depending on the number 
             * of entries in vertices, you also have to specify the correct type when binding the index buffer
            */
            const std::vector <uint32_t> m_indices = {
                0, 1, 2, 2, 3, 0
            };
            /* Handle to the log object
            */
            static Log::Record* m_VKVertexDataLog;
            /* instance id for logger
            */
            const size_t m_instanceId = 21;          

        public:
            VKVertexData (void) {
                m_VKVertexDataLog = LOG_INIT (m_instanceId, 
                                              Log::VERBOSE, 
                                              Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                                              "./Build/Log/");
                LOG_INFO (m_VKVertexDataLog) << "Constructor called" << std::endl; 
            }

            ~VKVertexData (void) {
                LOG_INFO (m_VKVertexDataLog) << "Destructor called" << std::endl;
                LOG_CLOSE (m_instanceId);
            }
        
        protected:
            std::vector <Vertex> getVertices (void) {
                return m_vertices;
            }

            std::vector <uint32_t> getIndices (void) {
                return m_indices;
            }

            /* We need to tell Vulkan how to pass this data format 'm_vertices' to the vertex shader once it's been 
             * uploaded into GPU memory. There are two types of structures needed to convey this information:
             * (1) VkVertexInputBindingDescription
             * (2) VkVertexInputAttributeDescription
             * 
             * A vertex binding describes at which rate to load data from memory throughout the vertices. It specifies 
             * the number of bytes between data entries and whether to move to the next data entry after each vertex or 
             * after each instance
            */
            static VkVertexInputBindingDescription getBindingDescription (void) {
                VkVertexInputBindingDescription bindingDescription{};
                /* All of our per-vertex data is packed together in one array, so we're only going to have one binding. 
                 * The binding parameter specifies the index of the binding in the array of bindings
                */
                bindingDescription.binding = 0;
                /* The stride parameter specifies the number of bytes from one entry to the next
                */
                bindingDescription.stride = sizeof (Vertex);
                /* The inputRate parameter can have one of the following values:
                 * VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
                 * VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
                 * 
                 * We're not going to use instanced rendering, so we'll stick to per-vertex data
                */
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                return bindingDescription;
            }  

            /* An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
             * originating from a binding description. We have two attributes, position and color, so we need two 
             * attribute description structs
            */
            static std::array <VkVertexInputAttributeDescription, 2> getAttributeDescriptions (void) {
                std::array <VkVertexInputAttributeDescription, 2> attributeDescriptions{};
                /* The binding parameter tells Vulkan from which binding the per-vertex data comes
                */
                attributeDescriptions[0].binding = 0;
                /* The location parameter references the location directive of the input in the vertex shader (the 
                 * input in the vertex shader with location 0 is the position, which has two 32-bit float components)
                */
                attributeDescriptions[0].location = 0;
                /* The format parameter describes the type of data for the attribute. A bit confusingly, the formats are 
                 * specified using the same enumeration as color formats
                 *
                 * float: VK_FORMAT_R32_SFLOAT
                 * vec2: VK_FORMAT_R32G32_SFLOAT
                 * vec3: VK_FORMAT_R32G32B32_SFLOAT
                 * vec4: VK_FORMAT_R32G32B32A32_SFLOAT
                 * 
                 * As you can see, you should use the format where the amount of color channels matches the number of 
                 * components in the shader data type. It is allowed to use more channels than the number of components 
                 * in the shader, but they will be silently discarded. If the number of channels is lower than the number 
                 * of components, then the BGA components will use default values of (0, 0, 1). 
                 * 
                 * The color type (SFLOAT, UINT, SINT) and bit width should also match the type of the shader input. See 
                 * the following examples:
                 * ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
                 * uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
                 * double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
                */
                attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
                /* The offset parameter specifies the number of bytes since the start of the per-vertex data to read 
                 * from. The binding is loading one Vertex at a time and the position attribute (pos) is at an offset of 
                 * 0 bytes from the beginning of this struct
                */
                attributeDescriptions[0].offset = offsetof(Vertex, pos);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(Vertex, color);
                return attributeDescriptions;
            }
    };

    Log::Record* VKVertexData::m_VKVertexDataLog;
}   // namespace Renderer
#endif  // VK_VERTEX_DATA_H