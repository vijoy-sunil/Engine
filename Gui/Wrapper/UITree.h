#ifndef UI_TREE_H
#define UI_TREE_H

#include "../UILogHelper.h"
#include "../UIConfig.h"
#include "../../Collection/Log/Log.h"

using namespace Collection;

namespace Gui {
    class UITree {
        private:
            struct NodeInfo {
                struct Meta {
                    std::string label;
                    e_nodeType type;
                    e_nodeActionType action;

                    std::vector <uint32_t> childInfoIds;
                    uint32_t parentInfoId;
                    /* Note that, some nodes may not have a core info id, in such cases it will be set to UINT32_MAX
                    */
                    uint32_t coreInfoId;
                } meta;

                struct State {
                    bool opened;
                    bool selected;
                    bool leaf;
                    bool locked;
                } state;

                struct Parameters {
                    ImGuiTreeNodeFlags treeNodeFlags;
                } params;
            };
            std::unordered_map <uint32_t, NodeInfo> m_nodeInfoPool;

            Log::Record* m_UITreeLog;
            const uint32_t m_instanceId = g_collectionSettings.instanceId++;

            void deleteNodeInfo (uint32_t nodeInfoId) {
                if (m_nodeInfoPool.find (nodeInfoId) != m_nodeInfoPool.end()) {
                    m_nodeInfoPool.erase (nodeInfoId);
                    return;
                }

                LOG_ERROR (m_UITreeLog) << "Failed to delete node info "
                                        << "[" << nodeInfoId << "]"
                                        << std::endl;
                throw std::runtime_error ("Failed to delete node info");
            }

            void createNode (uint32_t nodeInfoId, uint32_t& selectedNodeInfoId) {
                auto nodeInfo = getNodeInfo (nodeInfoId);

                if (nodeInfo->meta.action != UNDEFINED_ACTION) {
                    ImGui::SetNextItemOpen (nodeInfo->meta.action != CLOSE_ACTION);
                    nodeInfo->meta.action = UNDEFINED_ACTION;
                }

                if (nodeInfo->state.selected)
                    ImGui::PushStyleColor (ImGuiCol_Text, g_styleSettings.color.textActive);

                nodeInfo->state.opened = ImGui::TreeNodeEx ((void*)(intptr_t) nodeInfoId,
                                                            nodeInfo->params.treeNodeFlags,
                                                            nodeInfo->meta.label.c_str(),
                                                            nodeInfoId);
                if (nodeInfo->state.selected)
                    ImGui::PopStyleColor();

                if ((ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) ||
                     selectedNodeInfoId == nodeInfoId) {

                    selectedNodeInfoId              = nodeInfoId;
                    nodeInfo->state.selected        = true;
                    nodeInfo->params.treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
                }
                else {
                    nodeInfo->state.selected        = false;
                    nodeInfo->params.treeNodeFlags &= ~ImGuiTreeNodeFlags_Selected;
                }
            }

        public:
            UITree (void) {
                m_UITreeLog = LOG_INIT (m_instanceId, g_collectionSettings.logSaveDirPath);
                LOG_ADD_CONFIG (m_instanceId, Log::INFO,  Log::TO_FILE_IMMEDIATE);
                LOG_ADD_CONFIG (m_instanceId, Log::ERROR, Log::TO_FILE_IMMEDIATE | Log::TO_CONSOLE);
            }

            ~UITree (void) {
                LOG_CLOSE (m_instanceId);
            }

        protected:
            void readyNodeInfo (uint32_t nodeInfoId,
                                std::string label,
                                e_nodeType type,
                                e_nodeActionType action,
                                const std::vector <uint32_t>& childInfoIds,
                                uint32_t coreInfoId,
                                bool leaf,
                                ImGuiTreeNodeFlags treeNodeFlags) {

                if (m_nodeInfoPool.find (nodeInfoId) != m_nodeInfoPool.end()) {
                    LOG_ERROR (m_UITreeLog) << "Node info id already exists "
                                            << "[" << nodeInfoId << "]"
                                            << std::endl;
                    throw std::runtime_error ("Node info id already exists");
                }

                if (leaf)
                    treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                NodeInfo info{};
                info.meta.label            = label;
                info.meta.type             = type;
                info.meta.action           = action;

                info.meta.childInfoIds     = childInfoIds;
                info.meta.parentInfoId     = UINT32_MAX;
                info.meta.coreInfoId       = coreInfoId;

                info.state.opened          = false;
                info.state.selected        = false;
                info.state.leaf            = leaf;
                info.state.locked          = false;

                info.params.treeNodeFlags  = treeNodeFlags;
                m_nodeInfoPool[nodeInfoId] = info;
            }

            void createTree (uint32_t nodeInfoId, uint32_t& selectedNodeInfoId) {
                auto nodeInfo = getNodeInfo (nodeInfoId);

                createNode (nodeInfoId, selectedNodeInfoId);
                if (nodeInfo->state.opened && !nodeInfo->state.leaf) {
                    for (auto const& infoId: nodeInfo->meta.childInfoIds)
                        createTree (infoId, selectedNodeInfoId);

                    ImGui::TreePop();
                }
            }

            void openRootToNode (uint32_t nodeInfoId) {
                auto nodeInfo         = getNodeInfo (nodeInfoId);
                uint32_t parentInfoId = nodeInfo->meta.parentInfoId;

                nodeInfo->meta.action = OPEN_ACTION;
                if (parentInfoId != UINT32_MAX)
                    openRootToNode (parentInfoId);
            }

            void openAllNodes (void) {
                for (auto& [key, val]: m_nodeInfoPool)
                    val.meta.action = OPEN_ACTION;
            }

            void closeAllNodes (void) {
                for (auto& [key, val]: m_nodeInfoPool)
                    val.meta.action = CLOSE_ACTION;
            }

            NodeInfo* getNodeInfo (uint32_t nodeInfoId) {
                if (m_nodeInfoPool.find (nodeInfoId) != m_nodeInfoPool.end())
                    return &m_nodeInfoPool[nodeInfoId];

                LOG_ERROR (m_UITreeLog) << "Failed to find node info "
                                        << "[" << nodeInfoId << "]"
                                        << std::endl;
                throw std::runtime_error ("Failed to find node info");
            }

            void dumpNodeInfoPool (void) {
                LOG_INFO (m_UITreeLog) << "Dumping node info pool"
                                       << std::endl;

                for (auto const& [key, val]: m_nodeInfoPool) {
                    LOG_INFO (m_UITreeLog) << "Node info id "
                                           << "[" << key << "]"
                                           << std::endl;

                    LOG_INFO (m_UITreeLog) << "Label "
                                           << "[" << val.meta.label << "]"
                                           << std::endl;

                    LOG_INFO (m_UITreeLog) << "Type "
                                           << "[" << getNodeTypeString (val.meta.type) << "]"
                                           << std::endl;

                    LOG_INFO (m_UITreeLog) << "Action "
                                           << "[" << getNodeActionTypeString (val.meta.action) << "]"
                                           << std::endl;

                    LOG_INFO (m_UITreeLog) << "Child info ids"
                                           << std::endl;
                    for (auto const& infoId: val.meta.childInfoIds)
                    LOG_INFO (m_UITreeLog) << "[" << infoId << "]"
                                           << std::endl;

                    LOG_INFO (m_UITreeLog) << "Parent info id "
                                           << "[" << val.meta.parentInfoId << "]"
                                           << std::endl;

                    LOG_INFO (m_UITreeLog) << "Core info id "
                                           << "[" << val.meta.coreInfoId << "]"
                                           << std::endl;

                    std::string boolString = val.state.opened   == true ? "TRUE": "FALSE";
                    LOG_INFO (m_UITreeLog) << "Opened state "
                                           << "[" << boolString << "]"
                                           << std::endl;

                    boolString             = val.state.selected == true ? "TRUE": "FALSE";
                    LOG_INFO (m_UITreeLog) << "Selected state "
                                           << "[" << boolString << "]"
                                           << std::endl;

                    boolString             = val.state.leaf     == true ? "TRUE": "FALSE";
                    LOG_INFO (m_UITreeLog) << "Leaf state "
                                           << "[" << boolString << "]"
                                           << std::endl;

                    boolString             = val.state.locked   == true ? "TRUE": "FALSE";
                    LOG_INFO (m_UITreeLog) << "Locked state "
                                           << "[" << boolString << "]"
                                           << std::endl;

                    LOG_INFO (m_UITreeLog) << "Tree node flags "
                                           << "[" << val.params.treeNodeFlags << "]"
                                           << std::endl;
                }
            }

            void cleanUp (uint32_t nodeInfoId) {
                if (nodeInfoId == UINT32_MAX)   m_nodeInfoPool.clear();
                else                            deleteNodeInfo (nodeInfoId);
            }
    };
}   // namespace Gui
#endif  // UI_TREE_H