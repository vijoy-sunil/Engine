#ifndef VK_VERTEX_DATA_H
#define VK_VERTEX_DATA_H

/* GLM library provides us with linear algebra related types like vectors and matrices. The hash functions are defined in
 * the gtx folder, which means that it is technically still an experimental extension to GLM. Therefore you need to define 
 * GLM_ENABLE_EXPERIMENTAL to use it
*/
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "../VKConfig.h"
#include "../../Collections/Log/Log.h"

using namespace Collections;

namespace Core {
    struct Vertex {
        /* Define the attributes
        */
        glm::vec3 pos;
        glm::vec2 texCoord;
        glm::vec3 normal;
        uint32_t  texId;

        bool operator == (const Vertex& other) const {
            return pos      == other.pos      && 
                   texCoord == other.texCoord &&
                   normal   == other.normal   &&
                   texId    == other.texId;
        }
    };     
}   // namespace Core

namespace std {
    /* Hash function for Vertex struct
    */
    template <> 
    struct hash <Core::Vertex> {
        size_t operator() (const Core::Vertex& vertex) const {
            /* The difficulty with the hash function is that if your key type consists of several members, you will 
             * usually have the hash function calculate hash values for the individual members, and then somehow combine 
             * them into one hash value for the entire object. For good performance (i.e., few collisions) you should 
             * think carefully about how to combine the individual hash values to ensure you avoid getting the same output 
             * for different objects too often
            */
            size_t h1 = hash <glm::vec3>() (vertex.pos);
            size_t h2 = hash <glm::vec2>() (vertex.texCoord);
            size_t h3 = hash <glm::vec3>() (vertex.normal);
            size_t h4 = hash <uint32_t>()  (vertex.texId);
            /* https://stackoverflow.com/questions/1646807/quick-and-simple-hash-code-combinations/1646913#1646913
            */
            size_t hash = 17;
            hash = hash * 31 + h1;
            hash = hash * 31 + h2;
            hash = hash * 31 + h3;
            hash = hash * 31 + h4;
            return hash;
        }
    };
}   // namespace std

namespace Core {
    class VKVertexData {
        private:
            Log::Record* m_VKVertexDataLog;
            const uint32_t m_instanceId = g_collectionsSettings.instanceId++;

        public:
            VKVertexData (void) {
                m_VKVertexDataLog = LOG_INIT (m_instanceId, g_collectionsSettings.logSaveDirPath);
            }

            ~VKVertexData (void) {
                LOG_CLOSE (m_instanceId);
            }
        
        protected:
            /* We need to tell Vulkan how to pass the vertices array to the vertex shader once it's been uploaded into 
             * GPU memory. There are two types of structures needed to convey this information:
             * (1) VkVertexInputBindingDescription
             * (2) VkVertexInputAttributeDescription
             * 
             * A vertex binding describes at which rate to load data from memory throughout the vertices. It specifies 
             * the number of bytes between data entries and whether to move to the next data entry after each vertex or 
             * after each instance
            */
            VkVertexInputBindingDescription getBindingDescription (uint32_t bindingNumber,
                                                                   uint32_t stride,
                                                                   VkVertexInputRate inputRate) {
                VkVertexInputBindingDescription bindingDescription;
                /* If all of our per-vertex data is packed together in one array, then we're only going to have one 
                 * binding. The binding parameter specifies the index of the binding in the array of bindings
                */
                bindingDescription.binding = bindingNumber;
                /* The stride parameter specifies the number of bytes from one entry to the next
                */
                bindingDescription.stride = stride;
                /* The inputRate parameter can have one of the following values:
                 * VK_VERTEX_INPUT_RATE_VERTEX:   Move to the next data entry after each vertex
                 * VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
                */
                bindingDescription.inputRate = inputRate;
                return bindingDescription;
            }  

            /* An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data 
             * originating from a binding description
            */
            VkVertexInputAttributeDescription getAttributeDescription (uint32_t bindingNumber,
                                                                       uint32_t location,
                                                                       uint32_t offset,
                                                                       VkFormat format) {
                VkVertexInputAttributeDescription attributeDescription;
                /* The binding parameter tells Vulkan from which binding the per-vertex data comes
                */
                attributeDescription.binding = bindingNumber;
                /* The location parameter references the location directive of the input in the vertex shader
                */
                attributeDescription.location = location;
                /* The offset parameter specifies the number of bytes since the start of the per-vertex data to read 
                 * from. The binding is loading one Vertex at a time and the position attribute (pos) is at an offset of 
                 * 0 bytes from the beginning of this struct, for example
                */
                attributeDescription.offset = offset;
                /* The format parameter describes the type of data for the attribute. A bit confusingly, the formats are 
                 * specified using the same enumeration as color formats
                 *
                 * float: VK_FORMAT_R32_SFLOAT
                 * vec2:  VK_FORMAT_R32G32_SFLOAT
                 * vec3:  VK_FORMAT_R32G32B32_SFLOAT
                 * vec4:  VK_FORMAT_R32G32B32A32_SFLOAT
                 * 
                 * As you can see, you should use the format where the amount of color channels matches the number of 
                 * components in the shader data type. It is allowed to use more channels than the number of components 
                 * in the shader, but they will be silently discarded. If the number of channels is lower than the number 
                 * of components, then the BGA components will use default values of (0, 0, 1)
                 * 
                 * The color type (SFLOAT, UINT, SINT) and bit width should also match the type of the shader input. See 
                 * the following examples:
                 * ivec2:  VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
                 * uvec4:  VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
                 * double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
                */
                attributeDescription.format = format;
                return attributeDescription;
            }
    };
}   // namespace Core
#endif  // VK_VERTEX_DATA_H