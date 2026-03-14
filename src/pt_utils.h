#ifndef PT_UTILS_H
#define PT_UTILS_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <stack>

namespace godot {

    namespace PTUtils {

        /**
         * Gets the project-relative path for a given addon-relative path
         */
        godot::String get_project_relative_path(
            const godot::String& addon_relative_path);

        /**
         * Gathers all nodes of type T from the provided all_nodes array.
         * Uses a depth-first search to traverse the entire node hierarchy.
         * Note that this can be slow for large scenes, so use with caution.
         */
        template <typename T>
        inline TypedArray<T> gather_nodes_of_type(Node* root) {
            std::stack<Node*> pending;
            TypedArray<T> nodes;
            pending.push(root);

            while (!pending.empty()) {
                Node* current_node = pending.top();
                pending.pop();
                // If cast is successful, add to result
                T* casted = Object::cast_to<T>(current_node);
                if (casted != nullptr) {
                    nodes.append(casted);
                }

                for (uint32_t i = 0; i < current_node->get_child_count(); ++i) {
                    Node* child = current_node->get_child(i);
                    pending.push(child);
                }
            }

            return nodes;
        }

        PackedFloat32Array transform3d_to_float32_array(
            const Transform3D& transform);

        PackedByteArray get_camera_bytes(Camera3D* camera, uint32_t width,
                                         uint32_t height);

    }  // namespace PTUtils

}  // namespace godot
#endif