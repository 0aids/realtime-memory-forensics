// Again AI was used to organize the example color node editor.
#include "gui_examples.hpp"
#include <SDL3/SDL.h>
#include <cmath>

namespace rmf::gui::examples
{

    static float current_time_seconds       = 0.f;
    static bool  emulate_three_button_mouse = false;

    ImU32 evaluate(const Graph<Node>& graph, const int root_node)
    {
        std::stack<int> postorder;
        dfs_traverse(graph, root_node, [&postorder](const int node_id)
                     { postorder.push(node_id); });

        std::stack<float> value_stack;
        while (!postorder.empty())
        {
            const int id = postorder.top();
            postorder.pop();
            const Node node = graph.node(id);

            switch (node.type)
            {
                case NodeType::add:
                {
                    float rhs = value_stack.top();
                    value_stack.pop();
                    float lhs = value_stack.top();
                    value_stack.pop();
                    value_stack.push(lhs + rhs);
                }
                break;
                case NodeType::multiply:
                {
                    float rhs = value_stack.top();
                    value_stack.pop();
                    float lhs = value_stack.top();
                    value_stack.pop();
                    value_stack.push(rhs * lhs);
                }
                break;
                case NodeType::sine:
                {
                    float x = value_stack.top();
                    value_stack.pop();
                    value_stack.push(std::abs(std::sin(x)));
                }
                break;
                case NodeType::time:
                    value_stack.push(current_time_seconds);
                    break;
                case NodeType::value:
                    if (graph.num_edges_from_node(id) == 0ull)
                        value_stack.push(node.value);
                    break;
                default: break;
            }
        }

        assert(value_stack.size() == 3ull);
        int b = static_cast<int>(
            255.f * clamp(value_stack.top(), 0.f, 1.f) + 0.5f);
        value_stack.pop();
        int g = static_cast<int>(
            255.f * clamp(value_stack.top(), 0.f, 1.f) + 0.5f);
        value_stack.pop();
        int r = static_cast<int>(
            255.f * clamp(value_stack.top(), 0.f, 1.f) + 0.5f);
        value_stack.pop();

        return IM_COL32(r, g, b, 255);
    }

    ColorNodeEditor::ColorNodeEditor() :
        graph_(), nodes_(), root_node_id_(-1),
        minimap_location_(ImNodesMiniMapLocation_BottomRight)
    {
    }

    void ColorNodeEditor::show(bool* is_open)
    {
        current_time_seconds = 0.001f * SDL_GetTicks();
        ImGui::Begin("color node editor", is_open,
                     ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Mini-map"))
            {
                const char* names[]     = {"Top Left", "Top Right",
                                           "Bottom Left", "Bottom Right"};
                int         locations[] = {
                    ImNodesMiniMapLocation_TopLeft,
                    ImNodesMiniMapLocation_TopRight,
                    ImNodesMiniMapLocation_BottomLeft,
                    ImNodesMiniMapLocation_BottomRight};
                for (int i = 0; i < 4; i++)
                {
                    bool selected = minimap_location_ == locations[i];
                    if (ImGui::MenuItem(names[i], NULL, &selected))
                        minimap_location_ =
                            (ImNodesMiniMapLocation)locations[i];
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Style"))
            {
                if (ImGui::MenuItem("Classic"))
                {
                    ImGui::StyleColorsClassic();
                    ImNodes::StyleColorsClassic();
                }
                if (ImGui::MenuItem("Dark"))
                {
                    ImGui::StyleColorsDark();
                    ImNodes::StyleColorsDark();
                }
                if (ImGui::MenuItem("Light"))
                {
                    ImGui::StyleColorsLight();
                    ImNodes::StyleColorsLight();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::TextUnformatted(
            "Edit the color of the output color window using nodes.");
        ImGui::Columns(2);
        ImGui::TextUnformatted(
            "A -- add node\nX -- delete selected node or link");
        ImGui::NextColumn();
        if (ImGui::Checkbox("emulate_three_button_mouse",
                            &emulate_three_button_mouse))
        {
            ImNodes::GetIO().EmulateThreeButtonMouse.Modifier =
                emulate_three_button_mouse ? &ImGui::GetIO().KeyAlt :
                                             NULL;
        }
        ImGui::Columns(1);

        ImNodes::BeginNodeEditor();

        // Popup for adding nodes
        if (ImGui::IsWindowFocused(
                ImGuiFocusedFlags_RootAndChildWindows) &&
            ImNodes::IsEditorHovered() &&
            ImGui::IsKeyReleased(ImGuiKey_A))
        {
            if (!ImGui::IsAnyItemHovered())
                ImGui::OpenPopup("add node");
        }

        if (ImGui::BeginPopup("add node"))
        {
            const ImVec2 click_pos =
                ImGui::GetMousePosOnOpeningCurrentPopup();
            auto add_ui_node =
                [&](UiNodeType type, NodeType graphType)
            {
                UiNode ui_node;
                ui_node.type = type;
                if (type == UiNodeType::add ||
                    type == UiNodeType::multiply)
                {
                    int valL = graph_.insert_node(
                        Node(NodeType::value, 0.f));
                    int valR = graph_.insert_node(
                        Node(NodeType::value, 0.f));
                    ui_node.id = graph_.insert_node(Node(graphType));
                    if (type == UiNodeType::add)
                    {
                        ui_node.ui.add.lhs = valL;
                        ui_node.ui.add.rhs = valR;
                    }
                    else
                    {
                        ui_node.ui.multiply.lhs = valL;
                        ui_node.ui.multiply.rhs = valR;
                    }
                    graph_.insert_edge(ui_node.id, valL);
                    graph_.insert_edge(ui_node.id, valR);
                }
                else if (type == UiNodeType::sine)
                {
                    int val = graph_.insert_node(
                        Node(NodeType::value, 0.f));
                    ui_node.id =
                        graph_.insert_node(Node(NodeType::sine));
                    ui_node.ui.sine.input = val;
                    graph_.insert_edge(ui_node.id, val);
                }
                else if (type == UiNodeType::output)
                {
                    int r = graph_.insert_node(
                        Node(NodeType::value, 0.f));
                    int g = graph_.insert_node(
                        Node(NodeType::value, 0.f));
                    int b = graph_.insert_node(
                        Node(NodeType::value, 0.f));
                    ui_node.id =
                        graph_.insert_node(Node(NodeType::output));
                    ui_node.ui.output.r = r;
                    ui_node.ui.output.g = g;
                    ui_node.ui.output.b = b;
                    graph_.insert_edge(ui_node.id, r);
                    graph_.insert_edge(ui_node.id, g);
                    graph_.insert_edge(ui_node.id, b);
                    root_node_id_ = ui_node.id;
                }
                else
                {
                    ui_node.id =
                        graph_.insert_node(Node(NodeType::time));
                }
                nodes_.push_back(ui_node);
                ImNodes::SetNodeScreenSpacePos(ui_node.id, click_pos);
            };

            if (ImGui::MenuItem("add"))
                add_ui_node(UiNodeType::add, NodeType::add);
            if (ImGui::MenuItem("multiply"))
                add_ui_node(UiNodeType::multiply, NodeType::multiply);
            if (ImGui::MenuItem("output") && root_node_id_ == -1)
                add_ui_node(UiNodeType::output, NodeType::output);
            if (ImGui::MenuItem("sine"))
                add_ui_node(UiNodeType::sine, NodeType::sine);
            if (ImGui::MenuItem("time"))
                add_ui_node(UiNodeType::time, NodeType::time);
            ImGui::EndPopup();
        }

        // Render nodes
        for (const auto& node : nodes_)
        {
            ImNodes::BeginNode(node.id);
            switch (node.type)
            {
                case UiNodeType::add:
                case UiNodeType::multiply:
                {
                    bool isAdd = node.type == UiNodeType::add;
                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted(isAdd ? "add" :
                                                   "multiply");
                    ImNodes::EndNodeTitleBar();
                    auto drawInput = [&](int attr, const char* label)
                    {
                        ImNodes::BeginInputAttribute(attr);
                        ImGui::TextUnformatted(label);
                        if (graph_.num_edges_from_node(attr) == 0)
                        {
                            ImGui::SameLine();
                            ImGui::PushItemWidth(
                                100.f - ImGui::CalcTextSize(label).x);
                            ImGui::DragFloat("##h",
                                             &graph_.node(attr).value,
                                             0.01f);
                            ImGui::PopItemWidth();
                        }
                        ImNodes::EndInputAttribute();
                    };
                    drawInput(isAdd ? node.ui.add.lhs :
                                      node.ui.multiply.lhs,
                              "left");
                    drawInput(isAdd ? node.ui.add.rhs :
                                      node.ui.multiply.rhs,
                              "right");
                    ImNodes::BeginOutputAttribute(node.id);
                    ImGui::Indent(100.f -
                                  ImGui::CalcTextSize("result").x);
                    ImGui::TextUnformatted("result");
                    ImNodes::EndOutputAttribute();
                }
                break;
                case UiNodeType::output:
                {
                    ImNodes::PushColorStyle(
                        ImNodesCol_TitleBar,
                        IM_COL32(11, 109, 191, 255));
                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted("output");
                    ImNodes::EndNodeTitleBar();
                    auto drawOut = [&](int attr, const char* l)
                    {
                        ImNodes::BeginInputAttribute(attr);
                        ImGui::TextUnformatted(l);
                        if (graph_.num_edges_from_node(attr) == 0)
                        {
                            ImGui::SameLine();
                            ImGui::PushItemWidth(
                                100.f - ImGui::CalcTextSize(l).x);
                            ImGui::DragFloat("##h",
                                             &graph_.node(attr).value,
                                             0.01f, 0.f, 1.f);
                            ImGui::PopItemWidth();
                        }
                        ImNodes::EndInputAttribute();
                    };
                    drawOut(node.ui.output.r, "r");
                    drawOut(node.ui.output.g, "g");
                    drawOut(node.ui.output.b, "b");
                    ImNodes::PopColorStyle();
                }
                break;
                case UiNodeType::sine:
                {
                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted("sine");
                    ImNodes::EndNodeTitleBar();
                    ImNodes::BeginInputAttribute(node.ui.sine.input);
                    ImGui::TextUnformatted("number");
                    if (graph_.num_edges_from_node(
                            node.ui.sine.input) == 0)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(
                            100.f - ImGui::CalcTextSize("number").x);
                        ImGui::DragFloat(
                            "##h",
                            &graph_.node(node.ui.sine.input).value,
                            0.01f, 0.f, 1.f);
                        ImGui::PopItemWidth();
                    }
                    ImNodes::EndInputAttribute();
                    ImNodes::BeginOutputAttribute(node.id);
                    ImGui::Indent(100.f -
                                  ImGui::CalcTextSize("output").x);
                    ImGui::TextUnformatted("output");
                    ImNodes::EndOutputAttribute();
                }
                break;
                case UiNodeType::time:
                {
                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted("time");
                    ImNodes::EndNodeTitleBar();
                    ImNodes::BeginOutputAttribute(node.id);
                    ImGui::TextUnformatted("output");
                    ImNodes::EndOutputAttribute();
                }
                break;
            }
            ImNodes::EndNode();
        }

        for (const auto& edge : graph_.edges())
        {
            if (graph_.node(edge.from).type == NodeType::value)
                ImNodes::Link(edge.id, edge.from, edge.to);
        }

        ImNodes::MiniMap(0.2f, minimap_location_);
        ImNodes::EndNodeEditor();

        // Link logic
        int start_attr, end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
        {
            NodeType sT = graph_.node(start_attr).type;
            NodeType eT = graph_.node(end_attr).type;
            if (sT != eT)
            {
                if (sT != NodeType::value)
                    std::swap(start_attr, end_attr);
                graph_.insert_edge(start_attr, end_attr);
            }
        }

        int link_id;
        if (ImNodes::IsLinkDestroyed(&link_id))
            graph_.erase_edge(link_id);

        // Deletion logic
        if (ImGui::IsKeyReleased(ImGuiKey_X))
        {
            int nL = ImNodes::NumSelectedLinks();
            if (nL > 0)
            {
                std::vector<int> sel(nL);
                ImNodes::GetSelectedLinks(sel.data());
                for (int id : sel)
                    graph_.erase_edge(id);
            }
            int nN = ImNodes::NumSelectedNodes();
            if (nN > 0)
            {
                std::vector<int> sel(nN);
                ImNodes::GetSelectedNodes(sel.data());
                for (int id : sel)
                {
                    auto it = std::find_if(
                        nodes_.begin(), nodes_.end(),
                        [id](const UiNode& n) { return n.id == id; });
                    if (it != nodes_.end())
                    {
                        if (it->type == UiNodeType::add)
                        {
                            graph_.erase_node(it->ui.add.lhs);
                            graph_.erase_node(it->ui.add.rhs);
                        }
                        else if (it->type == UiNodeType::multiply)
                        {
                            graph_.erase_node(it->ui.multiply.lhs);
                            graph_.erase_node(it->ui.multiply.rhs);
                        }
                        else if (it->type == UiNodeType::output)
                        {
                            graph_.erase_node(it->ui.output.r);
                            graph_.erase_node(it->ui.output.g);
                            graph_.erase_node(it->ui.output.b);
                            root_node_id_ = -1;
                        }
                        else if (it->type == UiNodeType::sine)
                        {
                            graph_.erase_node(it->ui.sine.input);
                        }
                        graph_.erase_node(id);
                        nodes_.erase(it);
                    }
                }
            }
        }

        ImGui::End();

        const ImU32 color = root_node_id_ != -1 ?
            evaluate(graph_, root_node_id_) :
            IM_COL32(255, 20, 147, 255);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
        ImGui::Begin("output color");
        ImGui::End();
        ImGui::PopStyleColor();
    }

} // namespace rmf::gui::examples
