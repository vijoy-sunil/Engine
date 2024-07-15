#ifndef VK_MODEL_MGR_H
#define VK_MODEL_MGR_H
/* We will use the tinyobjloader library to load vertices and faces from an OBJ file
*/
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "VKVertexData.h"

using namespace Collections;

namespace Renderer {
    class VKModelMgr: protected VKVertexData {
        private:
            struct ModelInfo {
                struct Meta {
                    uint32_t uniqueVerticesCount;
                    uint32_t indicesCount;
                    glm::mat4 modelMatrix;
                    const char* modelPath;
                    const char* textureImagePath; 
                    const char* vertexShaderBinaryPath;
                    const char* fragmentShaderBinaryPath;
                } meta;

                struct Id {
                    uint32_t vertexBufferInfo;
                    uint32_t indexBufferInfo;
                    std::vector <uint32_t> uniformBufferInfos;
                    uint32_t swapChainImageInfoBase;
                    uint32_t textureImageInfo;
                    uint32_t depthImageInfo;
                    uint32_t multiSampleImageInfo;
                } id;

                struct Resource {
                    /* The attributes are combined into one array of vertices, this is known as interleaving vertex 
                     * attributes
                    */
                    std::vector <Vertex> vertices;
                    /* Note that it is possible to use either uint16_t or uint32_t for your index buffer depending on the
                     * number of entries in vertices, you also have to specify the correct type when binding the index 
                     * buffer
                    */
                    std::vector <uint32_t> indices;
                    VkDescriptorPool descriptorPool;
                    VkSampler textureSampler;
                    std::vector <VkDescriptorSet> descriptorSets;
                } resource;
            };
            std::map <uint32_t, ModelInfo> m_modelInfoPool{};
            
            static Log::Record* m_VKModelMgrLog;
            const size_t m_instanceId = g_collectionsId++; 

            void deleteModelInfo (uint32_t modelInfoId) {
                if (m_modelInfoPool.find (modelInfoId) != m_modelInfoPool.end()) {
                    m_modelInfoPool.erase (modelInfoId);
                    return;
                }

                LOG_ERROR (m_VKModelMgrLog) << "Failed to delete model info "
                                            << "[" << modelInfoId << "]"          
                                            << std::endl;
                throw std::runtime_error ("Failed to delete model info");   
            }

        public:
            VKModelMgr (void) {
                m_VKModelMgrLog = LOG_INIT (m_instanceId, g_pathSettings.logSaveDir);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKModelMgr (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyModelInfo (uint32_t modelInfoId,
                                 const char* modelPath,
                                 const char* textureImagePath,
                                 const char* vertexShaderBinaryPath,
                                 const char* fragmentShaderBinaryPath,
                                 const std::vector <uint32_t>& infoIds, 
                                 const std::vector <std::vector <uint32_t>>& infoIdGroups) {
                
                if (m_modelInfoPool.find (modelInfoId) != m_modelInfoPool.end()) {
                    LOG_ERROR (m_VKModelMgrLog) << "Model info id already exists "
                                                << "[" << modelInfoId << "]"
                                                << std::endl;
                    throw std::runtime_error ("Model info id already exists");
                }

                ModelInfo info{};
                info.meta.modelPath                = modelPath;
                info.meta.textureImagePath         = textureImagePath;
                info.meta.vertexShaderBinaryPath   = vertexShaderBinaryPath;
                info.meta.fragmentShaderBinaryPath = fragmentShaderBinaryPath;
                info.id.vertexBufferInfo           = infoIds[0];
                info.id.indexBufferInfo            = infoIds[1];
                info.id.swapChainImageInfoBase     = infoIds[2];
                info.id.textureImageInfo           = infoIds[3];
                info.id.depthImageInfo             = infoIds[4];
                info.id.multiSampleImageInfo       = infoIds[5];

                info.id.uniformBufferInfos         = infoIdGroups[0];
                m_modelInfoPool[modelInfoId]       = info;
            }

            void createVertices (uint32_t modelInfoId, 
                                 const std::vector <Vertex>& vertices) {

                auto modelInfo = getModelInfo (modelInfoId);

                modelInfo->meta.uniqueVerticesCount = static_cast <uint32_t> (vertices.size());
                modelInfo->resource.vertices        = vertices;
            }

            void createIndices (uint32_t modelInfoId,
                                const std::vector <uint32_t>& indices) {

                auto modelInfo = getModelInfo (modelInfoId);

                modelInfo->meta.indicesCount = static_cast <uint32_t> (indices.size());
                modelInfo->resource.indices  = indices;
            }

            /* OBJ file format
             * The first character of each line specifies the type of command. If the first character is a pound sign, #, 
             * the line is a comment and the rest of the line is ignored. Any blank lines are also ignored. The file is 
             * read in by a tool and parsed from top to bottom just like you would read it. In the descriptions that 
             * follow, the first character is a command, followed by any arguments. Anything shown in square brackets is 
             * optional
             * 
             * # a comment line
             * These are always ignored. Usually the first line of every OBJ file will be a comment that says what program
             * wrote the file out. Also, its quite common for comments to contain the number of verticies and/or faces an 
             * object used
             * 
             * v x y z
             * The vertex command, this specifies a vertex by its three coordinates. The vertex is implicitly named by 
             * the order it is found in the file. For example, the first vertex in the file is referenced as '1', the 
             * second as '2' and so on. None of the vertex commands actually specify any geometry, they are just points in 
             * space
             * 
             * vt u v [w]
             * The vertex texture command specifies the UV (and optionally W) mapping. These will be floating point values 
             * between 0 and 1 which say how to map the texture. They really don't tell you anything by themselves, they 
             * must be grouped with a vertex in a 'f' face command
             * 
             * vn x y z
             * The vertex normal command specifies a normal vector. A lot of times these aren't used, because the 'f' 
             * face command will use the order the 'v' commands are given to determine the normal instead. Like the 'vt' 
             * commands, they don't mean anything until grouped with a vertex in the 'f' face command
             * 
             * f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
             * The face command specifies a polygon made from the verticies listed. You may have as many verticies as you 
             * like. To reference a vertex you just give its index in the file, for example 'f 54 55 56 57' means a face 
             * built from vertecies 54 - 57. For each vertex, there may also be an associated vt, which says how to map 
             * the texture at this point, and/or a vn, which specifies a normal at this point. If you specify a vt or vn 
             * for one vertex, you must specify one for all. If you want to have a vertex and a vertex normal, but no 
             * vertex texture, it will look like: 'f v1//vt1'. The normal is what tells it which way the polygon faces. 
             * If you don't give one, it is determined by the order the verticies are given. They are assumed to be in 
             * counter-clockwise direction
             * 
             * Faces consist of an arbitrary amount of vertices, where each vertex refers to a position, normal and/or 
             * texture coordinate by index. This makes it possible to not just reuse entire vertices, but also individual 
             * attributes
             * 
             * usemtl name
             * The use material command lets you name a material to use. All 'f' face commands that follow will use the 
             * same material, until another usemtl command is encountered
            */

            /* Note that, you should run your program with optimization enabled (with the -O3 compiler flag). This is 
             * necessary, because otherwise loading the model will be very slow
            */
            void importOBJModel (uint32_t modelInfoId) {
                auto modelInfo = getModelInfo (modelInfoId);
                /* The attrib container holds all of the positions, normals and texture coordinates in its 
                 * attrib.vertices, attrib.normals, attrib.texcoords etc. vectors
                */
                tinyobj::attrib_t attrib;
                /* The shapes container contains all of the separate objects and their faces. Each face consists of an 
                 * array of vertices, and each vertex contains the indices of the position, normal and texture coordinate 
                 * attributes
                */
                std::vector <tinyobj::shape_t> shapes;
                std::vector <tinyobj::material_t> materials;
                /* The err string contains errors and the warn string contains warnings that occurred while loading the 
                 * file, like a missing material definition. Loading only really failed if the LoadObj function returns 
                 * false.
                 * 
                 * As mentioned before, faces in OBJ files can actually contain an arbitrary number of vertices, whereas 
                 * our application can only render triangles. Luckily the LoadObj has an optional parameter to 
                 * automatically triangulate such faces, which is enabled by default
                */
                std::string warn, err;

                if (!tinyobj::LoadObj (&attrib, &shapes, &materials, &warn, &err, modelInfo->meta.modelPath)) {
                    LOG_ERROR (m_VKModelMgrLog) << "Failed to import model "
                                                << "[" << modelInfoId << "]"
                                                << " "
                                                << "[" << warn << "]"
                                                << " "
                                                << "[" << err  << "]"
                                                << std::endl;
                    throw std::runtime_error ("Failed to import model");
                }

                /* Map to take advantage of indices vector (index buffer). Note that, to be able to use std::unordered_map
                 * with a user-defined key-type, you need to define two thing:
                 * (1) A hash function; this must be a class that overrides operator() and calculates the hash value given
                 * an object of the key-type. One particularly straight-forward way of doing this is to specialize the 
                 * std::hash template for your key-type
                 * 
                 * (2) A comparison function for equality; this is required because the hash cannot rely on the fact that 
                 * the hash function will always provide a unique hash value for every distinct key (i.e., it needs to be 
                 * able to deal with collisions), so it needs a way to compare two given keys for an exact match. You can 
                 * implement this by overloading operator==() for your key type
                */
                std::unordered_map <Vertex, uint32_t> uniqueVertices{};
                std::vector <Vertex>   vertices;
                std::vector <uint32_t> indices;
                /* Iterate overall all faces (may belong to different objects in a scene) and populate the vertex and 
                 * index vectors
                */
                for (auto const& shape: shapes) {
                    /* Note that, the triangulation feature has already made sure that there are three vertices per face, 
                     * so we can now directly iterate over the vertices and dump them straight into our vertices vector
                    */
                    for (auto const& index: shape.mesh.indices) {
                        Vertex vertex{};
                        /* The index variable is of type tinyobj::index_t, which contains the vertex_index, normal_index 
                         * and texcoord_index members. We need to use these indices to look up the actual vertex 
                         * attributes in the attrib arrays. Unfortunately the attrib.vertices array is an array of float 
                         * values instead of something like glm::vec3, so you need to multiply the index by 3. Similarly, 
                         * there are two texture coordinate components per entry. The offsets of 0, 1 and 2 are used to 
                         * access the X, Y and Z components, or the U and V components in the case of texture coordinates
                        */
                        vertex.pos   = {
                                            attrib.vertices[3 * index.vertex_index + 0],
                                            attrib.vertices[3 * index.vertex_index + 1],
                                            attrib.vertices[3 * index.vertex_index + 2]
                                       };

                        vertex.color = {
                                            attrib.colors[3 * index.vertex_index + 0],
                                            attrib.colors[3 * index.vertex_index + 1],
                                            attrib.colors[3 * index.vertex_index + 2]
                                       }; 
                        /* The OBJ format assumes a coordinate system where a vertical coordinate of 0 means the bottom 
                         * of the image, however we've uploaded our image into Vulkan in a top to bottom orientation where
                         * 0 means the top of the image. Solve this by flipping the vertical component of the texture 
                         * coordinates
                         * 
                         * (0, 0)-----------(1, 0)  top ^
                         * |                |
                         * |     (u, v)     |
                         * |                |
                         * (0, 1)-----------(1, 1)  bottom v
                         * 
                         * In Vulkan, 
                         * the u coordinate goes from 0.0 to 1.0, left to right
                         * the v coordinate goes from 0.0 to 1.0, top to bottom
                        */
                        vertex.texCoord = {
                                                   attrib.texcoords[2 * index.texcoord_index + 0],
                                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                                          };

                        vertex.normal   = {
                                            attrib.normals[3 * index.normal_index + 0],
                                            attrib.normals[3 * index.normal_index + 1],
                                            attrib.normals[3 * index.normal_index + 2]
                                          }; 
                        
                        /* To take advantage of the index buffer, we should keep only the unique vertices and use the 
                         * index buffer to reuse them whenever they come up. Every time we read a vertex from the OBJ 
                         * file, we check if we've already seen a vertex with the exact same position and texture 
                         * coordinates before. If not, we add it to m_vertices and store its index in the uniqueVertices 
                         * container. After that we add the index of the new vertex to m_indices. 
                         * 
                         * If we've seen the exact same vertex before, then we look up its index in uniqueVertices and 
                         * store that index in m_indices
                        */
                        if (uniqueVertices.count (vertex) == 0) {
                            uniqueVertices[vertex] = static_cast <uint32_t> (vertices.size());
                            vertices.push_back (vertex);
                        }
                        indices.push_back (uniqueVertices[vertex]);
                    }
                }
                createVertices (modelInfoId, vertices);
                createIndices  (modelInfoId, indices);
            }

            ModelInfo* getModelInfo (uint32_t modelInfoId) {
                if (m_modelInfoPool.find (modelInfoId) != m_modelInfoPool.end())
                    return &m_modelInfoPool[modelInfoId];
                
                LOG_ERROR (m_VKModelMgrLog) << "Failed to find model info "
                                            << "[" << modelInfoId << "]"
                                            << std::endl;
                throw std::runtime_error ("Failed to find model info");
            }

            void dumpModelInfoPool (void) {
                LOG_INFO (m_VKModelMgrLog) << "Dumping model info pool"
                                           << std::endl;

                for (auto const& [key, val]: m_modelInfoPool) {
                    LOG_INFO (m_VKModelMgrLog) << "Model info id " 
                                               << "[" << key << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Unique vertices count " 
                                               << "[" << val.meta.uniqueVerticesCount << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Indices count " 
                                               << "[" << val.meta.indicesCount << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Model matrix" 
                                               << std::endl;
                    uint32_t rowIdx = 0;
                    while (rowIdx < 4) {
                        LOG_INFO (m_VKModelMgrLog) << "["
                                                   << val.meta.modelMatrix[rowIdx][0] << " "
                                                   << val.meta.modelMatrix[rowIdx][1] << " "
                                                   << val.meta.modelMatrix[rowIdx][2] << " "
                                                   << val.meta.modelMatrix[rowIdx][3]
                                                   << "]"
                                                   << std::endl;
                        rowIdx++;
                    }

                    LOG_INFO (m_VKModelMgrLog) << "Model path " 
                                               << "[" << val.meta.modelPath << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Texture image path " 
                                               << "[" << val.meta.textureImagePath << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Vertex shader binary path " 
                                               << "[" << val.meta.vertexShaderBinaryPath << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Fragment shader binary path " 
                                               << "[" << val.meta.fragmentShaderBinaryPath << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Vettex buffer info id " 
                                               << "[" << val.id.vertexBufferInfo << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Index buffer info id " 
                                               << "[" << val.id.indexBufferInfo << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Uniform buffer info ids" 
                                               << std::endl;
                    for (auto const& infoId: val.id.uniformBufferInfos)
                    LOG_INFO (m_VKModelMgrLog) << "[" << infoId << "]"
                                               << std::endl;                    

                    LOG_INFO (m_VKModelMgrLog) << "Swap chain image info id base " 
                                               << "[" << val.id.swapChainImageInfoBase << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Texture image info id " 
                                               << "[" << val.id.textureImageInfo << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Depth image info id " 
                                               << "[" << val.id.depthImageInfo << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Multi sample image info id " 
                                               << "[" << val.id.multiSampleImageInfo << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Descriptor sets count " 
                                               << "[" << val.resource.descriptorSets.size() << "]"
                                               << std::endl;
                }
            }

            void cleanUp (uint32_t modelInfoId) {
                deleteModelInfo (modelInfoId);
            }
    };

    Log::Record* VKModelMgr::m_VKModelMgrLog;
}   // namespace Renderer
#endif  // VK_MODEL_MGR_H