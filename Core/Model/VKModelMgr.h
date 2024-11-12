#ifndef VK_MODEL_MGR_H
#define VK_MODEL_MGR_H
/* We will use the tinyobjloader library to load model data from an OBJ file
*/
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#include "VKVertexData.h"
#include "../Scene/VKUniform.h"

namespace Core {
    class VKModelMgr: protected VKVertexData {
        private:
            struct InstanceData {
                glm::vec3 position;
                glm::vec3 rotateAxis;
                glm::vec3 scale;
                float rotateAngleDeg;
            };

            struct ModelInfo {
                struct Meta {
                    /* The attributes are combined into one array of vertices, this is known as interleaving vertex
                     * attributes
                    */
                    std::vector <Vertex> vertices;
                    /* Note that it is possible to use either uint16_t or uint32_t for your index buffer depending on the
                     * number of entries in vertices, you also have to specify the correct type when binding the index
                     * buffer
                    */
                    std::vector <uint32_t> indices;
                    std::vector <InstanceDataSSBO> instances;
                    std::vector <InstanceData>     instanceDatas;
                    uint32_t verticesCount;
                    uint32_t indicesCount;
                    uint32_t instancesCount;
                    uint32_t parsedDataLogInstanceId;
                } meta;

                struct Path {
                    const char* model;
                    const char* mtlFileDir;
                    std::vector <std::string> diffuseTextureImages;
                } path;

                struct Id {
                    std::vector <uint32_t> diffuseTextureImageInfos;
                    std::vector <uint32_t> vertexBufferInfos;
                    uint32_t indexBufferInfo;
                } id;
            };
            std::unordered_map <uint32_t, ModelInfo>   m_modelInfoPool;

            uint32_t m_textureImageInfoId;
            std::unordered_map <std::string, uint32_t> m_textureImagePool;

            Log::Record* m_VKModelMgrLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void deleteModelInfo (uint32_t modelInfoId) {
                if (m_modelInfoPool.find (modelInfoId) != m_modelInfoPool.end()) {
                    /* Delete parsed data log
                    */
                    LOG_CLOSE (m_modelInfoPool[modelInfoId].meta.parsedDataLogInstanceId);
                    m_modelInfoPool.erase (modelInfoId);
                    return;
                }

                LOG_ERROR (m_VKModelMgrLog) << "Failed to delete model info "
                                            << "[" << modelInfoId << "]"
                                            << std::endl;
                throw std::runtime_error ("Failed to delete model info");
            }

            void dumpParsedData (uint32_t modelInfoId) {
                auto modelInfo     = getModelInfo (modelInfoId);
                auto parsedDataLog = GET_LOG (modelInfo->meta.parsedDataLogInstanceId);

                LOG_INFO (parsedDataLog) << "Dumping parsed data "
                                         << "[" << modelInfoId << "]"
                                         << std::endl;

                LOG_INFO (parsedDataLog) << "Vertex data"
                                         << std::endl;
                for (auto const& vertex: modelInfo->meta.vertices) {
                LOG_INFO (parsedDataLog) << "[" << vertex.pos.x      << ", " << vertex.pos.y      << ", "
                                                << vertex.pos.z      << "]"
                                         << " "
                                         << "[" << vertex.texCoord.x << ", " << vertex.texCoord.y << "]"
                                         << " "
                                         << "[" << vertex.normal.x   << ", " << vertex.normal.y   << ", "
                                                << vertex.normal.z   << "]"
                                         << " "
                                         << "[" << vertex.texId      << "]"
                                         << std::endl;
                }

                LOG_INFO (parsedDataLog) << "Index data"
                                         << std::endl;
                uint32_t loopIdx = 0;
                while (loopIdx < modelInfo->meta.indicesCount) {
                LOG_INFO (parsedDataLog) << "[" << modelInfo->meta.indices[loopIdx++] << ", "
                                                << modelInfo->meta.indices[loopIdx++] << ", "
                                                << modelInfo->meta.indices[loopIdx++]
                                         << "]"
                                         << std::endl;
                }
            }

            void updateTextureImagePool (uint32_t modelInfoId, const std::string& texturePath) {
                auto modelInfo = getModelInfo (modelInfoId);

                if (m_textureImagePool.find (texturePath) == m_textureImagePool.end()) {
                    m_textureImagePool[texturePath] = m_textureImageInfoId;
                    m_textureImageInfoId++;
                }
                modelInfo->id.diffuseTextureImageInfos.push_back (m_textureImagePool[texturePath]);
            }

        public:
            VKModelMgr (void) {
                m_VKModelMgrLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,    Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::WARNING, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR,   Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~VKModelMgr (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyModelInfo (uint32_t modelInfoId,
                                 const char* modelPath,
                                 const char* mtlFileDirPath) {

                if (m_modelInfoPool.find (modelInfoId) != m_modelInfoPool.end()) {
                    LOG_ERROR (m_VKModelMgrLog) << "Model info id already exists "
                                                << "[" << modelInfoId << "]"
                                                << std::endl;
                    throw std::runtime_error ("Model info id already exists");
                }

                ModelInfo info{};
                info.meta.parsedDataLogInstanceId = g_collectionSettings.instanceId++;
                info.path.model                   = modelPath;
                info.path.mtlFileDir              = mtlFileDirPath;
                /* Add default diffuse texture as the fist entry in the group of textures. This way, faces with no
                 * texture can sample from this default texture
                */
                info.path.diffuseTextureImages.push_back (g_coreSettings.defaultDiffuseTexturePath);

                info.id.indexBufferInfo           = UINT32_MAX;
                m_modelInfoPool[modelInfoId]      = info;
                m_textureImageInfoId              = 0;
                /* Config log for parsed data
                */
                std::string nameExtension = "_PD_" + std::to_string (modelInfoId);
                LOG_INIT       (info.meta.parsedDataLogInstanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (info.meta.parsedDataLogInstanceId,
                                Log::INFO,
                                Log::TO_FILE_IMMEDIATE,
                                nameExtension.c_str());
            }

            void createVertices (uint32_t modelInfoId, const std::vector <Vertex>& vertices) {
                auto modelInfo = getModelInfo (modelInfoId);
                modelInfo->meta.vertices      = vertices;
                modelInfo->meta.verticesCount = static_cast <uint32_t> (vertices.size());
            }

            void createIndices (uint32_t modelInfoId, const std::vector <uint32_t>& indices) {
                auto modelInfo = getModelInfo (modelInfoId);
                modelInfo->meta.indices      = indices;
                modelInfo->meta.indicesCount = static_cast <uint32_t> (indices.size());
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
                 * attrib.vertices, attrib.normals, attrib.texcoords vectors
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

                if (!tinyobj::LoadObj (&attrib, &shapes, &materials,
                                       &warn, &err,
                                       modelInfo->path.model,
                                       modelInfo->path.mtlFileDir)) {

                    LOG_ERROR (m_VKModelMgrLog) << "Failed to import model "
                                                << "[" << modelInfoId << "]"
                                                << " "
                                                << "[" << warn << "]"
                                                << " "
                                                << "[" << err  << "]"
                                                << std::endl;
                    throw std::runtime_error ("Failed to import model");
                }

                if (materials.size() == 0) {
                    LOG_WARNING (m_VKModelMgrLog) << "Failed to find .mtl file "
                                                  << "[" << modelInfoId << "]"
                                                  << " "
                                                  << "[" << modelInfo->path.mtlFileDir << "]"
                                                  << std::endl;
                }
                /* Extract texture image paths from .mtl file if any
                 * [ - ] Diffuse texure
                 * [ X ] Other textures like specular, emission etc.
                */
                else {
                    for (auto const& material: materials) {
                        if (!material.diffuse_texname.empty())
                            modelInfo->path.diffuseTextureImages.push_back (material.diffuse_texname);
                        else
                            LOG_WARNING (m_VKModelMgrLog) << "Failed to find diffuse textures "
                                                          << "[" << modelInfoId << "]"
                                                          << " "
                                                          << "[" << modelInfo->path.mtlFileDir << "]"
                                                          << std::endl;
                    }
                }
                /* Populate texture image pool, which contains all the textures used across models along with their
                 * respective texture image info ids
                */
                for (auto const& path: modelInfo->path.diffuseTextureImages)
                    updateTextureImagePool (modelInfoId, path);

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
                std::unordered_map <Vertex, uint32_t> uniqueVertices;
                std::vector <Vertex>   vertices;
                std::vector <uint32_t> indices;
                auto defaultTexCoords = std::vector <glm::vec2> {
                    {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f},
                    {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}
                };
                /* Iterate overall all faces (may belong to different objects in a scene) and populate the vertex and
                 * index vectors
                */
                for (auto const& shape: shapes) {
                    /* Note that, the triangulation feature has already made sure that there are three vertices per face,
                     * so we can now directly iterate over the vertices and dump them straight into our vertices vector
                    */
                    const uint32_t verticesPerFace = 3;
                    const uint32_t verticesPerQuad = 6;
                    uint32_t faceIndex             = 0;
                    uint32_t quadIndex             = 0;
                    uint32_t indexProcessedCount   = 0;

                    for (auto const& index: shape.mesh.indices) {
                        Vertex vertex;
                        /* The index variable is of type tinyobj::index_t, which contains the vertex_index, normal_index
                         * and texcoord_index members. We need to use these indices to look up the actual vertex
                         * attributes in the attrib arrays. Unfortunately the attrib.vertices array is an array of float
                         * values instead of something like glm::vec3, so you need to multiply the index by 3. Similarly,
                         * there are two texture coordinate components per entry. The offsets of 0, 1 and 2 are used to
                         * access the X, Y and Z components, or the U and V components in the case of texture coordinates
                        */
                        vertex.pos = {
                                        attrib.vertices[3 * index.vertex_index + 0],
                                        attrib.vertices[3 * index.vertex_index + 1],
                                        attrib.vertices[3 * index.vertex_index + 2]
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
                        if (!attrib.texcoords.empty())
                            vertex.texCoord = {
                                                       attrib.texcoords[2 * index.texcoord_index + 0],
                                                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                                              };

                        vertex.normal   = {
                                            attrib.normals[3 * index.normal_index + 0],
                                            attrib.normals[3 * index.normal_index + 1],
                                            attrib.normals[3 * index.normal_index + 2]
                                          };
                        /* We will handle missing texture faces (material_ids = -1) by adding +1 to all material_ids, this
                         * will allow us to use the default texture whose image info id is 0. Note that, the local texture
                         * id is an index into the current model's texture array embedded with in the model file. What we
                         * need is an image info id that can be used to index into the global texture pool, so that the
                         * shader can sample from the correct texture from the global pool of textures
                        */
                        uint32_t localTexId     = shape.mesh.material_ids[faceIndex] + 1;
                        std::string texturePath = modelInfo->path.diffuseTextureImages[localTexId];
                        vertex.texId            = m_textureImagePool[texturePath];
                        /* Manual uv mapping of default texture
                        */
                        if (vertex.texId == 0) {
                            vertex.texCoord = defaultTexCoords[quadIndex];
                            quadIndex       == verticesPerQuad - 1 ? quadIndex = 0: quadIndex++;
                        }
                        /* To take advantage of the index buffer, we should keep only the unique vertices and use the
                         * index buffer to reuse them whenever they come up. Every time we read a vertex from the OBJ
                         * file, we check if we've already seen a vertex with the exact same attributes before. If not,
                         * we add it to vertices array and store its index in the map container. After that we add the
                         * index of the new vertex to indices array
                         *
                         * If we've seen the exact same vertex before, then we look up its index in the map container and
                         * store that index in indices array
                        */
                        if (uniqueVertices.count (vertex) == 0) {
                            uniqueVertices[vertex] = static_cast <uint32_t> (vertices.size());
                            vertices.push_back (vertex);
                        }
                        indices.push_back (uniqueVertices[vertex]);
                        /* Increment face index after we process a face (3 vertices make up a face)
                        */
                        indexProcessedCount++;
                        if (indexProcessedCount == verticesPerFace) {
                            faceIndex++;
                            indexProcessedCount = 0;
                        }
                    }
                }
                createVertices (modelInfoId, vertices);
                createIndices  (modelInfoId, indices);
                dumpParsedData (modelInfoId);
            }

            std::unordered_map <std::string, uint32_t>& getTextureImagePool (void) {
                return m_textureImagePool;
            }

            uint32_t decodeTexIdLUTPacket (uint32_t modelInfoId,
                                           uint32_t modelInstanceId,
                                           uint32_t oldTexId) {

                auto modelInfo = getModelInfo (modelInfoId);

                if (modelInstanceId >= modelInfo->meta.instancesCount) {
                    LOG_ERROR (m_VKModelMgrLog) << "Invalid model instance id "
                                                << "[" << modelInstanceId << "]"
                                                << "->"
                                                << "[" << modelInfo->meta.instancesCount << "]"
                                                << std::endl;
                    throw std::runtime_error ("Invalid model instance id");
                }

                if (oldTexId > UINT8_MAX) {
                    LOG_ERROR (m_VKModelMgrLog) << "Failed to decode packet "
                                                << "[" << oldTexId << "]"
                                                << std::endl;
                    throw std::runtime_error ("Failed to decode packet");
                }

                uint32_t readIdx   = oldTexId / 4;
                uint32_t offsetIdx = oldTexId % 4;
                uint32_t mask      = UINT8_MAX << offsetIdx * 8;
                uint32_t packet    = modelInfo->meta.instances[modelInstanceId].texIdLUT[readIdx];
                uint32_t newTexId  = (packet & mask) >> offsetIdx * 8;

                return newTexId;
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

                    uint32_t modelInstanceId = 0;
                    for (auto const& instance: val.meta.instances) {
                        LOG_INFO (m_VKModelMgrLog) << "Model instance id "
                                                   << "[" << modelInstanceId << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKModelMgrLog) << "Model matrix"
                                                   << std::endl;
                        uint32_t rowIdx = 0;
                        while (rowIdx < 4) {
                            LOG_INFO (m_VKModelMgrLog) << "["
                                                       << instance.modelMatrix[rowIdx][0] << " "
                                                       << instance.modelMatrix[rowIdx][1] << " "
                                                       << instance.modelMatrix[rowIdx][2] << " "
                                                       << instance.modelMatrix[rowIdx][3]
                                                       << "]"
                                                       << std::endl;
                            rowIdx++;
                        }

                        LOG_INFO (m_VKModelMgrLog) << "Texture image info id look up table"
                                                   << std::endl;
                        uint32_t colIdx = 0;
                        rowIdx          = 0;
                        while (rowIdx < 16) {
                            LOG_INFO (m_VKModelMgrLog) << rowIdx << ": "
                                                       << "["
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++)
                                                       << " - "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++)
                                                       << " - "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++)
                                                       << " - "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++) << ", "
                                                       << decodeTexIdLUTPacket (key, modelInstanceId, colIdx++)
                                                       << "]"
                                                       << std::endl;
                            rowIdx++;
                        }

                        LOG_INFO (m_VKModelMgrLog) << "Position"
                                                   << std::endl;
                        LOG_INFO (m_VKModelMgrLog) << "[" << val.meta.instanceDatas[modelInstanceId].position.x << ", "
                                                          << val.meta.instanceDatas[modelInstanceId].position.y << ", "
                                                          << val.meta.instanceDatas[modelInstanceId].position.z
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKModelMgrLog) << "Rotate axis"
                                                   << std::endl;
                        LOG_INFO (m_VKModelMgrLog) << "[" << val.meta.instanceDatas[modelInstanceId].rotateAxis.x << ", "
                                                          << val.meta.instanceDatas[modelInstanceId].rotateAxis.y << ", "
                                                          << val.meta.instanceDatas[modelInstanceId].rotateAxis.z
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKModelMgrLog) << "Scale"
                                                   << std::endl;
                        LOG_INFO (m_VKModelMgrLog) << "[" << val.meta.instanceDatas[modelInstanceId].scale.x << ", "
                                                          << val.meta.instanceDatas[modelInstanceId].scale.y << ", "
                                                          << val.meta.instanceDatas[modelInstanceId].scale.z
                                                   << "]"
                                                   << std::endl;

                        LOG_INFO (m_VKModelMgrLog) << "Rotate angle deg "
                                                   << "[" << val.meta.instanceDatas[modelInstanceId].rotateAngleDeg
                                                   << "]"
                                                   << std::endl;
                        modelInstanceId++;
                    }

                    LOG_INFO (m_VKModelMgrLog) << "Vertices count "
                                               << "[" << val.meta.verticesCount << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Indices count "
                                               << "[" << val.meta.indicesCount << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Instances count "
                                               << "[" << val.meta.instancesCount << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Parsed data log instance id "
                                               << "[" << val.meta.parsedDataLogInstanceId << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Model path "
                                               << "[" << val.path.model << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Material file directory path "
                                               << "[" << val.path.mtlFileDir << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Diffuse texture image paths"
                                               << std::endl;
                    for (auto const& path: val.path.diffuseTextureImages)
                    LOG_INFO (m_VKModelMgrLog) << "[" << path << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Diffuse texture image info ids"
                                               << std::endl;
                    for (auto const& infoId: val.id.diffuseTextureImageInfos)
                    LOG_INFO (m_VKModelMgrLog) << "[" << infoId << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Vettex buffer info ids"
                                               << std::endl;
                    for (auto const& infoId: val.id.vertexBufferInfos)
                    LOG_INFO (m_VKModelMgrLog) << "[" << infoId << "]"
                                               << std::endl;

                    LOG_INFO (m_VKModelMgrLog) << "Index buffer info id "
                                               << "[" << val.id.indexBufferInfo << "]"
                                               << std::endl;
                }

                LOG_INFO (m_VKModelMgrLog) << "Dumping texture image pool"
                                           << std::endl;
                for (auto const& [path, infoId]: m_textureImagePool)
                LOG_INFO (m_VKModelMgrLog) << "[" << path << "]"
                                           << " "
                                           << "[" << infoId << "]"
                                           << std::endl;
            }

            void cleanUp (uint32_t modelInfoId) {
                deleteModelInfo (modelInfoId);
            }
    };
}   // namespace Core
#endif  // VK_MODEL_MGR_H
