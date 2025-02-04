/*************************************************************************/
/*  skeleton_3d_editor_plugin.cpp                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "skeleton_3d_editor_plugin.h"

#include "core/io/resource_saver.h"
#include "editor/editor_file_dialog.h"
#include "editor/editor_properties.h"
#include "editor/editor_scale.h"
#include "editor/plugins/animation_player_editor_plugin.h"
#include "node_3d_editor_plugin.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/joint_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/resources/capsule_shape_3d.h"
#include "scene/resources/sphere_shape_3d.h"
#include "scene/resources/surface_tool.h"

void BoneTransformEditor::create_editors() {
	const Color section_color = get_theme_color(SNAME("prop_subsection"), SNAME("Editor"));

	section = memnew(EditorInspectorSection);
	section->setup("trf_properties", label, this, section_color, true);
	add_child(section);

	enabled_checkbox = memnew(CheckBox(TTR("Pose Enabled")));
	enabled_checkbox->set_flat(true);
	enabled_checkbox->set_visible(toggle_enabled);
	section->get_vbox()->add_child(enabled_checkbox);

	key_button = memnew(Button);
	key_button->set_text(TTR("Key Transform"));
	key_button->set_visible(keyable);
	key_button->set_icon(get_theme_icon(SNAME("Key"), SNAME("EditorIcons")));
	key_button->set_flat(true);
	section->get_vbox()->add_child(key_button);

	// Translation property.
	translation_property = memnew(EditorPropertyVector3());
	translation_property->setup(-10000, 10000, 0.001f, true);
	translation_property->set_label("Translation");
	translation_property->set_use_folding(true);
	translation_property->connect("property_changed", callable_mp(this, &BoneTransformEditor::_value_changed_vector3));
	section->get_vbox()->add_child(translation_property);

	// Rotation property.
	rotation_property = memnew(EditorPropertyVector3());
	rotation_property->setup(-10000, 10000, 0.001f, true);
	rotation_property->set_label("Rotation Degrees");
	rotation_property->set_use_folding(true);
	rotation_property->connect("property_changed", callable_mp(this, &BoneTransformEditor::_value_changed_vector3));
	section->get_vbox()->add_child(rotation_property);

	// Scale property.
	scale_property = memnew(EditorPropertyVector3());
	scale_property->setup(-10000, 10000, 0.001f, true);
	scale_property->set_label("Scale");
	scale_property->set_use_folding(true);
	scale_property->connect("property_changed", callable_mp(this, &BoneTransformEditor::_value_changed_vector3));
	section->get_vbox()->add_child(scale_property);

	// Transform/Matrix section.
	transform_section = memnew(EditorInspectorSection);
	transform_section->setup("trf_properties_transform", "Matrix", this, section_color, true);
	section->get_vbox()->add_child(transform_section);

	// Transform/Matrix property.
	transform_property = memnew(EditorPropertyTransform3D());
	transform_property->setup(-10000, 10000, 0.001f, true);
	transform_property->set_label("Transform");
	transform_property->set_use_folding(true);
	transform_property->connect("property_changed", callable_mp(this, &BoneTransformEditor::_value_changed_transform));
	transform_section->get_vbox()->add_child(transform_property);
}

void BoneTransformEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			create_editors();
			key_button->connect("pressed", callable_mp(this, &BoneTransformEditor::_key_button_pressed));
			enabled_checkbox->connect("pressed", callable_mp(this, &BoneTransformEditor::_checkbox_pressed));
			[[fallthrough]];
		}
		case NOTIFICATION_SORT_CHILDREN: {
			const Ref<Font> font = get_theme_font(SNAME("font"), SNAME("Tree"));
			int font_size = get_theme_font_size(SNAME("font_size"), SNAME("Tree"));

			Point2 buffer;
			buffer.x += get_theme_constant(SNAME("inspector_margin"), SNAME("Editor"));
			buffer.y += font->get_height(font_size);
			buffer.y += get_theme_constant(SNAME("vseparation"), SNAME("Tree"));

			const float vector_height = translation_property->get_size().y;
			const float transform_height = transform_property->get_size().y;
			const float button_height = key_button->get_size().y;

			const float width = get_size().x - get_theme_constant(SNAME("inspector_margin"), SNAME("Editor"));
			Vector<Rect2> input_rects;
			if (keyable && section->get_vbox()->is_visible()) {
				input_rects.push_back(Rect2(key_button->get_position() + buffer, Size2(width, button_height)));
			} else {
				input_rects.push_back(Rect2(0, 0, 0, 0));
			}

			if (section->get_vbox()->is_visible()) {
				input_rects.push_back(Rect2(translation_property->get_position() + buffer, Size2(width, vector_height)));
				input_rects.push_back(Rect2(rotation_property->get_position() + buffer, Size2(width, vector_height)));
				input_rects.push_back(Rect2(scale_property->get_position() + buffer, Size2(width, vector_height)));
				input_rects.push_back(Rect2(transform_property->get_position() + buffer, Size2(width, transform_height)));
			} else {
				const int32_t start = input_rects.size();
				const int32_t empty_input_rect_elements = 4;
				const int32_t end = start + empty_input_rect_elements;
				for (int i = start; i < end; ++i) {
					input_rects.push_back(Rect2(0, 0, 0, 0));
				}
			}

			for (int32_t i = 0; i < input_rects.size(); i++) {
				background_rects[i] = input_rects[i];
			}

			update();
			break;
		}
		case NOTIFICATION_DRAW: {
			const Color dark_color = get_theme_color(SNAME("dark_color_2"), SNAME("Editor"));

			for (int i = 0; i < 5; ++i) {
				draw_rect(background_rects[i], dark_color);
			}

			break;
		}
	}
}

void BoneTransformEditor::_value_changed(const double p_value) {
	if (updating) {
		return;
	}

	Transform3D tform = compute_transform_from_vector3s();
	_change_transform(tform);
}

void BoneTransformEditor::_value_changed_vector3(const String p_property_name, const Vector3 p_vector, const StringName p_edited_property_name, const bool p_boolean) {
	if (updating) {
		return;
	}
	Transform3D tform = compute_transform_from_vector3s();
	_change_transform(tform);
}

Transform3D BoneTransformEditor::compute_transform_from_vector3s() const {
	// Convert rotation from degrees to radians.
	Vector3 prop_rotation = rotation_property->get_vector();
	prop_rotation.x = Math::deg2rad(prop_rotation.x);
	prop_rotation.y = Math::deg2rad(prop_rotation.y);
	prop_rotation.z = Math::deg2rad(prop_rotation.z);

	return Transform3D(
			Basis(prop_rotation, scale_property->get_vector()),
			translation_property->get_vector());
}

void BoneTransformEditor::_value_changed_transform(const String p_property_name, const Transform3D p_transform, const StringName p_edited_property_name, const bool p_boolean) {
	if (updating) {
		return;
	}
	_change_transform(p_transform);
}

void BoneTransformEditor::_change_transform(Transform3D p_new_transform) {
	if (property.get_slicec('/', 0) == "bones" && property.get_slicec('/', 2) == "custom_pose") {
		undo_redo->create_action(TTR("Set Custom Bone Pose Transform"), UndoRedo::MERGE_ENDS);
		undo_redo->add_undo_method(skeleton, "set_bone_custom_pose", property.get_slicec('/', 1).to_int(), skeleton->get_bone_custom_pose(property.get_slicec('/', 1).to_int()));
		undo_redo->add_do_method(skeleton, "set_bone_custom_pose", property.get_slicec('/', 1).to_int(), p_new_transform);
		undo_redo->commit_action();
	} else if (property.get_slicec('/', 0) == "bones") {
		undo_redo->create_action(TTR("Set Bone Transform"), UndoRedo::MERGE_ENDS);
		undo_redo->add_undo_property(skeleton, property, skeleton->get(property));
		undo_redo->add_do_property(skeleton, property, p_new_transform);
		undo_redo->commit_action();
	}
}

void BoneTransformEditor::update_enabled_checkbox() {
	if (enabled_checkbox) {
		const String path = "bones/" + property.get_slicec('/', 1) + "/enabled";
		const bool is_enabled = skeleton->get(path);
		enabled_checkbox->set_pressed(is_enabled);
	}
}

void BoneTransformEditor::_update_properties() {
	if (updating) {
		return;
	}

	if (!skeleton) {
		return;
	}

	updating = true;

	Transform3D tform = skeleton->get(property);
	_update_transform_properties(tform);
}

void BoneTransformEditor::_update_custom_pose_properties() {
	if (updating) {
		return;
	}

	if (!skeleton) {
		return;
	}

	updating = true;

	Transform3D tform = skeleton->get_bone_custom_pose(property.to_int());
	_update_transform_properties(tform);
}

void BoneTransformEditor::_update_transform_properties(Transform3D tform) {
	Basis rotation_basis = tform.get_basis();
	Vector3 rotation_radians = rotation_basis.get_rotation_euler();
	Vector3 rotation_degrees = Vector3(Math::rad2deg(rotation_radians.x), Math::rad2deg(rotation_radians.y), Math::rad2deg(rotation_radians.z));
	Vector3 translation = tform.get_origin();
	Vector3 scale = tform.basis.get_scale();

	translation_property->update_using_vector(translation);
	rotation_property->update_using_vector(rotation_degrees);
	scale_property->update_using_vector(scale);
	transform_property->update_using_transform(tform);

	update_enabled_checkbox();
	updating = false;
}

BoneTransformEditor::BoneTransformEditor(Skeleton3D *p_skeleton) :
		skeleton(p_skeleton) {
	undo_redo = EditorNode::get_undo_redo();
}

void BoneTransformEditor::set_target(const String &p_prop) {
	property = p_prop;
}

void BoneTransformEditor::set_keyable(const bool p_keyable) {
	keyable = p_keyable;
}

void BoneTransformEditor::_update_key_button(const bool p_keyable) {
	bool is_keyable = keyable && p_keyable;
	if (key_button) {
		key_button->set_visible(is_keyable);
	}
}

void BoneTransformEditor::set_properties_read_only(const bool p_readonly) {
	enabled_checkbox->set_disabled(p_readonly);
	enabled_checkbox->update();
}

void BoneTransformEditor::set_transform_read_only(const bool p_readonly) {
	translation_property->set_read_only(p_readonly);
	rotation_property->set_read_only(p_readonly);
	scale_property->set_read_only(p_readonly);
	transform_property->set_read_only(p_readonly);
	translation_property->update();
	rotation_property->update();
	scale_property->update();
	transform_property->update();
	_update_key_button(!p_readonly);
}

void BoneTransformEditor::set_toggle_enabled(const bool p_enabled) {
	toggle_enabled = p_enabled;
	if (enabled_checkbox) {
		enabled_checkbox->set_visible(p_enabled);
	}
}

void BoneTransformEditor::_key_button_pressed() {
	if (!skeleton) {
		return;
	}

	const BoneId bone_id = property.get_slicec('/', 1).to_int();
	const String name = skeleton->get_bone_name(bone_id);

	if (name.is_empty()) {
		return;
	}

	Transform3D tform = compute_transform_from_vector3s();
	AnimationPlayerEditor::get_singleton()->get_track_editor()->insert_transform_key(skeleton, name, tform);
}

void BoneTransformEditor::_checkbox_pressed() {
	if (!skeleton) {
		return;
	}

	const BoneId bone_id = property.get_slicec('/', 1).to_int();
	if (enabled_checkbox) {
		undo_redo->create_action(TTR("Set Pose Enabled"));
		bool enabled = skeleton->is_bone_enabled(bone_id);
		undo_redo->add_do_method(skeleton, "set_bone_enabled", bone_id, !enabled);
		undo_redo->add_undo_method(skeleton, "set_bone_enabled", bone_id, enabled);
		undo_redo->commit_action();
	}
}

Skeleton3DEditor *Skeleton3DEditor::singleton = nullptr;

void Skeleton3DEditor::set_keyable(const bool p_keyable) {
	keyable = p_keyable;
	skeleton_options->get_popup()->set_item_disabled(SKELETON_OPTION_INSERT_KEYS, !p_keyable);
	skeleton_options->get_popup()->set_item_disabled(SKELETON_OPTION_INSERT_KEYS_EXISTED, !p_keyable);
};

void Skeleton3DEditor::set_rest_options_enabled(const bool p_rest_options_enabled) {
	rest_options->get_popup()->set_item_disabled(REST_OPTION_POSE_TO_REST, !p_rest_options_enabled);
};

void Skeleton3DEditor::_update_show_rest_only() {
	_update_pose_enabled(-1);
}

void Skeleton3DEditor::_update_pose_enabled(int p_bone) {
	if (!skeleton) {
		return;
	}
	if (pose_editor) {
		pose_editor->set_properties_read_only(skeleton->is_show_rest_only());

		if (selected_bone > 0) {
			pose_editor->set_transform_read_only(skeleton->is_show_rest_only() || !(skeleton->is_bone_enabled(selected_bone)));
		}
	}
	_update_gizmo_visible();
}

void Skeleton3DEditor::_on_click_skeleton_option(int p_skeleton_option) {
	if (!skeleton) {
		return;
	}

	switch (p_skeleton_option) {
		case SKELETON_OPTION_CREATE_PHYSICAL_SKELETON: {
			create_physical_skeleton();
			break;
		}
		case SKELETON_OPTION_INIT_POSE: {
			init_pose();
			break;
		}
		case SKELETON_OPTION_INSERT_KEYS: {
			insert_keys(true);
			break;
		}
		case SKELETON_OPTION_INSERT_KEYS_EXISTED: {
			insert_keys(false);
			break;
		}
	}
}

void Skeleton3DEditor::_on_click_rest_option(int p_rest_option) {
	if (!skeleton) {
		return;
	}

	switch (p_rest_option) {
		case REST_OPTION_POSE_TO_REST: {
			pose_to_rest();
			break;
		}
	}
}

void Skeleton3DEditor::init_pose() {
	const int bone_len = skeleton->get_bone_count();
	if (!bone_len) {
		return;
	}
	UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
	ur->create_action(TTR("Set Bone Transform"), UndoRedo::MERGE_ENDS);
	for (int i = 0; i < bone_len; i++) {
		ur->add_do_method(skeleton, "set_bone_pose", i, Transform3D());
		ur->add_undo_method(skeleton, "set_bone_pose", i, skeleton->get_bone_pose(i));
	}
	ur->commit_action();
}

void Skeleton3DEditor::insert_keys(bool p_all_bones) {
	if (!skeleton) {
		return;
	}

	int bone_len = skeleton->get_bone_count();
	Node *root = EditorNode::get_singleton()->get_tree()->get_root();
	String path = root->get_path_to(skeleton);

	AnimationTrackEditor *te = AnimationPlayerEditor::get_singleton()->get_track_editor();
	te->make_insert_queue();
	for (int i = 0; i < bone_len; i++) {
		const String name = skeleton->get_bone_name(i);

		if (name.is_empty()) {
			continue;
		}

		if (!p_all_bones && !te->has_transform_track(skeleton, name)) {
			continue;
		}

		Transform3D tform = skeleton->get_bone_pose(i);
		te->insert_transform_key(skeleton, name, tform);
	}
	te->commit_insert_queue();
}

void Skeleton3DEditor::pose_to_rest() {
	if (!skeleton) {
		return;
	}

	// Todo: Do method with multiple bone selection.
	UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
	ur->create_action(TTR("Set Bone Transform"), UndoRedo::MERGE_ENDS);

	ur->add_do_method(skeleton, "set_bone_pose", selected_bone, Transform3D());
	ur->add_undo_method(skeleton, "set_bone_pose", selected_bone, skeleton->get_bone_pose(selected_bone));
	ur->add_do_method(skeleton, "set_bone_custom_pose", selected_bone, Transform3D());
	ur->add_undo_method(skeleton, "set_bone_custom_pose", selected_bone, skeleton->get_bone_custom_pose(selected_bone));
	ur->add_do_method(skeleton, "set_bone_rest", selected_bone, skeleton->get_bone_rest(selected_bone) * skeleton->get_bone_custom_pose(selected_bone) * skeleton->get_bone_pose(selected_bone));
	ur->add_undo_method(skeleton, "set_bone_rest", selected_bone, skeleton->get_bone_rest(selected_bone));

	ur->commit_action();
}

void Skeleton3DEditor::create_physical_skeleton() {
	UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
	ERR_FAIL_COND(!get_tree());
	Node *owner = skeleton == get_tree()->get_edited_scene_root() ? skeleton : skeleton->get_owner();

	const int bc = skeleton->get_bone_count();

	if (!bc) {
		return;
	}

	Vector<BoneInfo> bones_infos;
	bones_infos.resize(bc);

	for (int bone_id = 0; bc > bone_id; ++bone_id) {
		const int parent = skeleton->get_bone_parent(bone_id);

		if (parent < 0) {
			bones_infos.write[bone_id].relative_rest = skeleton->get_bone_rest(bone_id);

		} else {
			const int parent_parent = skeleton->get_bone_parent(parent);

			bones_infos.write[bone_id].relative_rest = bones_infos[parent].relative_rest * skeleton->get_bone_rest(bone_id);

			// Create physical bone on parent.
			if (!bones_infos[parent].physical_bone) {
				bones_infos.write[parent].physical_bone = create_physical_bone(parent, bone_id, bones_infos);

				ur->create_action(TTR("Create physical bones"));
				ur->add_do_method(skeleton, "add_child", bones_infos[parent].physical_bone);
				ur->add_do_reference(bones_infos[parent].physical_bone);
				ur->add_undo_method(skeleton, "remove_child", bones_infos[parent].physical_bone);
				ur->commit_action();

				bones_infos[parent].physical_bone->set_bone_name(skeleton->get_bone_name(parent));
				bones_infos[parent].physical_bone->set_owner(owner);
				bones_infos[parent].physical_bone->get_child(0)->set_owner(owner); // set shape owner

				// Create joint between parent of parent.
				if (-1 != parent_parent) {
					bones_infos[parent].physical_bone->set_joint_type(PhysicalBone3D::JOINT_TYPE_PIN);
				}
			}
		}
	}
}

PhysicalBone3D *Skeleton3DEditor::create_physical_bone(int bone_id, int bone_child_id, const Vector<BoneInfo> &bones_infos) {
	const Transform3D child_rest = skeleton->get_bone_rest(bone_child_id);

	const real_t half_height(child_rest.origin.length() * 0.5);
	const real_t radius(half_height * 0.2);

	CapsuleShape3D *bone_shape_capsule = memnew(CapsuleShape3D);
	bone_shape_capsule->set_height(half_height * 2);
	bone_shape_capsule->set_radius(radius);

	CollisionShape3D *bone_shape = memnew(CollisionShape3D);
	bone_shape->set_shape(bone_shape_capsule);

	Transform3D capsule_transform;
	capsule_transform.basis = Basis(Vector3(1, 0, 0), Vector3(0, 0, 1), Vector3(0, -1, 0));
	bone_shape->set_transform(capsule_transform);

	Transform3D body_transform;
	body_transform.basis = Basis::looking_at(child_rest.origin);
	body_transform.origin = body_transform.basis.xform(Vector3(0, 0, -half_height));

	Transform3D joint_transform;
	joint_transform.origin = Vector3(0, 0, half_height);

	PhysicalBone3D *physical_bone = memnew(PhysicalBone3D);
	physical_bone->add_child(bone_shape);
	physical_bone->set_name("Physical Bone " + skeleton->get_bone_name(bone_id));
	physical_bone->set_body_offset(body_transform);
	physical_bone->set_joint_offset(joint_transform);
	return physical_bone;
}

Variant Skeleton3DEditor::get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	TreeItem *selected = joint_tree->get_selected();

	if (!selected) {
		return Variant();
	}

	Ref<Texture> icon = selected->get_icon(0);

	VBoxContainer *vb = memnew(VBoxContainer);
	HBoxContainer *hb = memnew(HBoxContainer);
	TextureRect *tf = memnew(TextureRect);
	tf->set_texture(icon);
	tf->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
	hb->add_child(tf);
	Label *label = memnew(Label(selected->get_text(0)));
	hb->add_child(label);
	vb->add_child(hb);
	hb->set_modulate(Color(1, 1, 1, 1));

	set_drag_preview(vb);
	Dictionary drag_data;
	drag_data["type"] = "nodes";
	drag_data["node"] = selected;

	return drag_data;
}

bool Skeleton3DEditor::can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	TreeItem *target = joint_tree->get_item_at_position(p_point);
	if (!target) {
		return false;
	}

	const String path = target->get_metadata(0);
	if (!path.begins_with("bones/")) {
		return false;
	}

	TreeItem *selected = Object::cast_to<TreeItem>(Dictionary(p_data)["node"]);
	if (target == selected) {
		return false;
	}

	const String path2 = target->get_metadata(0);
	if (!path2.begins_with("bones/")) {
		return false;
	}

	return true;
}

void Skeleton3DEditor::drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	if (!can_drop_data_fw(p_point, p_data, p_from)) {
		return;
	}

	TreeItem *target = joint_tree->get_item_at_position(p_point);
	TreeItem *selected = Object::cast_to<TreeItem>(Dictionary(p_data)["node"]);

	const BoneId target_boneidx = String(target->get_metadata(0)).get_slicec('/', 1).to_int();
	const BoneId selected_boneidx = String(selected->get_metadata(0)).get_slicec('/', 1).to_int();

	move_skeleton_bone(skeleton->get_path(), selected_boneidx, target_boneidx);
}

void Skeleton3DEditor::move_skeleton_bone(NodePath p_skeleton_path, int32_t p_selected_boneidx, int32_t p_target_boneidx) {
	Node *node = get_node_or_null(p_skeleton_path);
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
	ERR_FAIL_NULL(skeleton);
	UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
	ur->create_action(TTR("Set Bone Parentage"));
	// If the target is a child of ourselves, we move only *us* and not our children.
	if (skeleton->is_bone_parent_of(p_target_boneidx, p_selected_boneidx)) {
		const BoneId parent_idx = skeleton->get_bone_parent(p_selected_boneidx);
		const int bone_count = skeleton->get_bone_count();
		for (BoneId i = 0; i < bone_count; ++i) {
			if (skeleton->get_bone_parent(i) == p_selected_boneidx) {
				ur->add_undo_method(skeleton, "set_bone_parent", i, skeleton->get_bone_parent(i));
				ur->add_do_method(skeleton, "set_bone_parent", i, parent_idx);
				skeleton->set_bone_parent(i, parent_idx);
			}
		}
	}
	ur->add_undo_method(skeleton, "set_bone_parent", p_selected_boneidx, skeleton->get_bone_parent(p_selected_boneidx));
	ur->add_do_method(skeleton, "set_bone_parent", p_selected_boneidx, p_target_boneidx);
	skeleton->set_bone_parent(p_selected_boneidx, p_target_boneidx);

	update_joint_tree();
	ur->commit_action();
}

void Skeleton3DEditor::_joint_tree_selection_changed() {
	TreeItem *selected = joint_tree->get_selected();
	if (selected) {
		const String path = selected->get_metadata(0);

		if (path.begins_with("bones/")) {
			const int b_idx = path.get_slicec('/', 1).to_int();
			const String bone_path = "bones/" + itos(b_idx) + "/";

			pose_editor->set_target(bone_path + "pose");
			rest_editor->set_target(bone_path + "rest");
			custom_pose_editor->set_target(bone_path + "custom_pose");

			pose_editor->set_visible(true);
			rest_editor->set_visible(true);
			custom_pose_editor->set_visible(true);

			selected_bone = b_idx;
		}
	}
	set_rest_options_enabled(selected);
	_update_properties();
	_update_pose_enabled();
}

// May be not used with single select mode.
void Skeleton3DEditor::_joint_tree_rmb_select(const Vector2 &p_pos) {
}

void Skeleton3DEditor::_update_properties() {
	if (rest_editor) {
		rest_editor->_update_properties();
	}
	if (pose_editor) {
		pose_editor->_update_properties();
	}
	if (custom_pose_editor) {
		custom_pose_editor->_update_custom_pose_properties();
	}
	_update_gizmo_transform();
}

void Skeleton3DEditor::update_joint_tree() {
	joint_tree->clear();

	if (!skeleton) {
		return;
	}

	TreeItem *root = joint_tree->create_item();

	Map<int, TreeItem *> items;

	items.insert(-1, root);

	Ref<Texture> bone_icon = get_theme_icon(SNAME("BoneAttachment3D"), SNAME("EditorIcons"));

	Vector<int> bones_to_process = skeleton->get_parentless_bones();
	while (bones_to_process.size() > 0) {
		int current_bone_idx = bones_to_process[0];
		bones_to_process.erase(current_bone_idx);

		const int parent_idx = skeleton->get_bone_parent(current_bone_idx);
		TreeItem *parent_item = items.find(parent_idx)->get();

		TreeItem *joint_item = joint_tree->create_item(parent_item);
		items.insert(current_bone_idx, joint_item);

		joint_item->set_text(0, skeleton->get_bone_name(current_bone_idx));
		joint_item->set_icon(0, bone_icon);
		joint_item->set_selectable(0, true);
		joint_item->set_metadata(0, "bones/" + itos(current_bone_idx));

		// Add the bone's children to the list of bones to be processed.
		Vector<int> current_bone_child_bones = skeleton->get_bone_children(current_bone_idx);
		int child_bone_size = current_bone_child_bones.size();
		for (int i = 0; i < child_bone_size; i++) {
			bones_to_process.push_back(current_bone_child_bones[i]);
		}
	}
}

void Skeleton3DEditor::update_editors() {
}

void Skeleton3DEditor::create_editors() {
	set_h_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 0);

	set_focus_mode(FOCUS_ALL);

	Node3DEditor *ne = Node3DEditor::get_singleton();
	AnimationTrackEditor *te = AnimationPlayerEditor::get_singleton()->get_track_editor();

	// Create Top Menu Bar.
	separator = memnew(VSeparator);
	ne->add_control_to_menu_panel(separator);

	// Create Skeleton Option in Top Menu Bar.
	skeleton_options = memnew(MenuButton);
	ne->add_control_to_menu_panel(skeleton_options);

	skeleton_options->set_text(TTR("Skeleton3D"));
	skeleton_options->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon(SNAME("Skeleton3D"), SNAME("EditorIcons")));

	skeleton_options->get_popup()->add_item(TTR("Init pose"), SKELETON_OPTION_INIT_POSE);
	skeleton_options->get_popup()->add_item(TTR("Insert key of all bone poses"), SKELETON_OPTION_INSERT_KEYS);
	skeleton_options->get_popup()->add_item(TTR("Insert key of bone poses already exist track"), SKELETON_OPTION_INSERT_KEYS_EXISTED);
	skeleton_options->get_popup()->add_item(TTR("Create physical skeleton"), SKELETON_OPTION_CREATE_PHYSICAL_SKELETON);

	skeleton_options->get_popup()->connect("id_pressed", callable_mp(this, &Skeleton3DEditor::_on_click_skeleton_option));

	// Create Rest Option in Top Menu Bar.
	rest_options = memnew(MenuButton);
	ne->add_control_to_menu_panel(rest_options);

	rest_options->set_text(TTR("Edit Rest"));
	rest_options->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon(SNAME("BoneAttachment3D"), SNAME("EditorIcons")));

	rest_options->get_popup()->add_item(TTR("Apply current pose to rest"), REST_OPTION_POSE_TO_REST);
	rest_options->get_popup()->connect("id_pressed", callable_mp(this, &Skeleton3DEditor::_on_click_rest_option));
	set_rest_options_enabled(false);

	Vector<Variant> button_binds;
	button_binds.resize(1);

	edit_mode_button = memnew(Button);
	ne->add_control_to_menu_panel(edit_mode_button);
	edit_mode_button->set_tooltip(TTR("Edit Mode\nShow buttons on joints."));
	edit_mode_button->set_toggle_mode(true);
	edit_mode_button->set_flat(true);
	edit_mode_button->connect("toggled", callable_mp(this, &Skeleton3DEditor::edit_mode_toggled));

	edit_mode = false;

	set_keyable(te->has_keying());

	if (skeleton) {
		skeleton->add_child(handles_mesh_instance);
		handles_mesh_instance->set_skeleton_path(NodePath(""));
	}

	const Color section_color = get_theme_color(SNAME("prop_subsection"), SNAME("Editor"));

	EditorInspectorSection *bones_section = memnew(EditorInspectorSection);
	bones_section->setup("bones", "Bones", skeleton, section_color, true);
	add_child(bones_section);
	bones_section->unfold();

	ScrollContainer *s_con = memnew(ScrollContainer);
	s_con->set_h_size_flags(SIZE_EXPAND_FILL);
	s_con->set_custom_minimum_size(Size2(1, 350) * EDSCALE);
	bones_section->get_vbox()->add_child(s_con);

	joint_tree = memnew(Tree);
	joint_tree->set_columns(1);
	joint_tree->set_focus_mode(Control::FOCUS_NONE);
	joint_tree->set_select_mode(Tree::SELECT_SINGLE);
	joint_tree->set_hide_root(true);
	joint_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	joint_tree->set_h_size_flags(SIZE_EXPAND_FILL);
	joint_tree->set_allow_rmb_select(true);
	joint_tree->set_drag_forwarding(this);
	s_con->add_child(joint_tree);

	pose_editor = memnew(BoneTransformEditor(skeleton));
	pose_editor->set_label(TTR("Bone Pose"));
	pose_editor->set_toggle_enabled(true);
	pose_editor->set_keyable(te->has_keying());
	pose_editor->set_visible(false);
	add_child(pose_editor);

	rest_editor = memnew(BoneTransformEditor(skeleton));
	rest_editor->set_label(TTR("Bone Rest"));
	rest_editor->set_visible(false);
	add_child(rest_editor);
	rest_editor->set_transform_read_only(true);

	custom_pose_editor = memnew(BoneTransformEditor(skeleton));
	custom_pose_editor->set_label(TTR("Bone Custom Pose"));
	custom_pose_editor->set_visible(false);
	add_child(custom_pose_editor);
	custom_pose_editor->set_transform_read_only(true);
}

void Skeleton3DEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			edit_mode_button->set_icon(get_theme_icon("ToolBoneSelect", "EditorIcons"));
			get_tree()->connect("node_removed", callable_mp(this, &Skeleton3DEditor::_node_removed), Vector<Variant>(), Object::CONNECT_ONESHOT);
			break;
		}
		case NOTIFICATION_ENTER_TREE: {
			create_editors();
			update_joint_tree();
			update_editors();
			joint_tree->connect("item_selected", callable_mp(this, &Skeleton3DEditor::_joint_tree_selection_changed));
			joint_tree->connect("item_rmb_selected", callable_mp(this, &Skeleton3DEditor::_joint_tree_rmb_select));
#ifdef TOOLS_ENABLED
			skeleton->connect("pose_updated", callable_mp(this, &Skeleton3DEditor::_draw_gizmo));
			skeleton->connect("pose_updated", callable_mp(this, &Skeleton3DEditor::_update_properties));
			skeleton->connect("bone_enabled_changed", callable_mp(this, &Skeleton3DEditor::_update_pose_enabled));
			skeleton->connect("show_rest_only_changed", callable_mp(this, &Skeleton3DEditor::_update_show_rest_only));
#endif
			break;
		}
	}
}

void Skeleton3DEditor::_node_removed(Node *p_node) {
	if (skeleton && p_node == skeleton) {
		skeleton = nullptr;
		skeleton_options->hide();
		rest_options->hide();
	}

	_update_properties();
}

void Skeleton3DEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_node_removed"), &Skeleton3DEditor::_node_removed);
	ClassDB::bind_method(D_METHOD("_joint_tree_selection_changed"), &Skeleton3DEditor::_joint_tree_selection_changed);
	ClassDB::bind_method(D_METHOD("_joint_tree_rmb_select"), &Skeleton3DEditor::_joint_tree_rmb_select);
	ClassDB::bind_method(D_METHOD("_update_show_rest_only"), &Skeleton3DEditor::_update_show_rest_only);
	ClassDB::bind_method(D_METHOD("_update_pose_enabled"), &Skeleton3DEditor::_update_pose_enabled);
	ClassDB::bind_method(D_METHOD("_update_properties"), &Skeleton3DEditor::_update_properties);
	ClassDB::bind_method(D_METHOD("_on_click_skeleton_option"), &Skeleton3DEditor::_on_click_skeleton_option);
	ClassDB::bind_method(D_METHOD("_on_click_rest_option"), &Skeleton3DEditor::_on_click_rest_option);

	ClassDB::bind_method(D_METHOD("get_drag_data_fw"), &Skeleton3DEditor::get_drag_data_fw);
	ClassDB::bind_method(D_METHOD("can_drop_data_fw"), &Skeleton3DEditor::can_drop_data_fw);
	ClassDB::bind_method(D_METHOD("drop_data_fw"), &Skeleton3DEditor::drop_data_fw);

	ClassDB::bind_method(D_METHOD("move_skeleton_bone"), &Skeleton3DEditor::move_skeleton_bone);

	ClassDB::bind_method(D_METHOD("_draw_gizmo"), &Skeleton3DEditor::_draw_gizmo);
}

void Skeleton3DEditor::edit_mode_toggled(const bool pressed) {
	edit_mode = pressed;
	_update_gizmo_visible();
}

Skeleton3DEditor::Skeleton3DEditor(EditorInspectorPluginSkeleton *e_plugin, EditorNode *p_editor, Skeleton3D *p_skeleton) :
		editor(p_editor),
		editor_plugin(e_plugin),
		skeleton(p_skeleton) {
	singleton = this;

	// Handle.
	handle_material = Ref<ShaderMaterial>(memnew(ShaderMaterial));
	handle_shader = Ref<Shader>(memnew(Shader));
	handle_shader->set_code(R"(
// Skeleton 3D gizmo handle shader.

shader_type spatial;
render_mode unshaded, shadows_disabled, depth_draw_always;
uniform sampler2D texture_albedo : hint_albedo;
uniform float point_size : hint_range(0,128) = 32;
void vertex() {
	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix( pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)), COLOR.rgb* (1.0 / 12.92), lessThan(COLOR.rgb,vec3(0.04045)) );
	}
	VERTEX = VERTEX;
	POSITION=PROJECTION_MATRIX*INV_CAMERA_MATRIX*WORLD_MATRIX*vec4(VERTEX.xyz,1.0);
	POSITION.z = mix(POSITION.z, 0, 0.999);
	POINT_SIZE = point_size;
}
void fragment() {
	vec4 albedo_tex = texture(texture_albedo,POINT_COORD);
	vec3 col = albedo_tex.rgb + COLOR.rgb;
	col = vec3(min(col.r,1.0),min(col.g,1.0),min(col.b,1.0));
	ALBEDO = col;
	if (albedo_tex.a < 0.5) { discard; }
	ALPHA = albedo_tex.a;
}
)");
	handle_material->set_shader(handle_shader);
	Ref<Texture2D> handle = editor->get_gui_base()->get_theme_icon("EditorBoneHandle", "EditorIcons");
	handle_material->set_shader_param("point_size", handle->get_width());
	handle_material->set_shader_param("texture_albedo", handle);

	handles_mesh_instance = memnew(MeshInstance3D);
	handles_mesh_instance->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	handles_mesh.instantiate();
	handles_mesh_instance->set_mesh(handles_mesh);
}

void Skeleton3DEditor::update_bone_original() {
	if (!skeleton) {
		return;
	}
	if (skeleton->get_bone_count() == 0 || selected_bone == -1) {
		return;
	}
	bone_original = skeleton->get_bone_pose(selected_bone);
}

void Skeleton3DEditor::_hide_handles() {
	handles_mesh_instance->hide();
}

void Skeleton3DEditor::_draw_gizmo() {
	if (!skeleton) {
		return;
	}

	// If you call get_bone_global_pose() while drawing the surface, such as toggle rest mode,
	// the skeleton update will be done first and
	// the drawing surface will be interrupted once and an error will occur.
	skeleton->force_update_all_dirty_bones();

	// Handles.
	if (edit_mode) {
		_draw_handles();
	} else {
		_hide_handles();
	}
}

void Skeleton3DEditor::_draw_handles() {
	handles_mesh_instance->show();

	const int bone_len = skeleton->get_bone_count();
	handles_mesh->clear_surfaces();
	handles_mesh->surface_begin(Mesh::PRIMITIVE_POINTS);

	for (int i = 0; i < bone_len; i++) {
		Color c;
		if (i == selected_bone) {
			c = Color(1, 1, 0);
		} else {
			c = Color(0.1, 0.25, 0.8);
		}
		Vector3 point = skeleton->get_bone_global_pose(i).origin;
		handles_mesh->surface_set_color(c);
		handles_mesh->surface_add_vertex(point);
	}
	handles_mesh->surface_end();
	handles_mesh->surface_set_material(0, handle_material);
}

TreeItem *Skeleton3DEditor::_find(TreeItem *p_node, const NodePath &p_path) {
	if (!p_node) {
		return nullptr;
	}

	NodePath np = p_node->get_metadata(0);
	if (np == p_path) {
		return p_node;
	}

	TreeItem *children = p_node->get_first_child();
	while (children) {
		TreeItem *n = _find(children, p_path);
		if (n) {
			return n;
		}
		children = children->get_next();
	}

	return nullptr;
}

void Skeleton3DEditor::_subgizmo_selection_change() {
	if (!skeleton) {
		return;
	}

	// Once validated by subgizmos_intersect_ray, but required if through inspector's bones tree.
	if (!edit_mode) {
		skeleton->clear_subgizmo_selection();
		return;
	}

	int selected = -1;
	Skeleton3DEditor *se = Skeleton3DEditor::get_singleton();
	if (se) {
		selected = se->get_selected_bone();
	}

	if (selected >= 0) {
		Vector<Ref<Node3DGizmo>> gizmos = skeleton->get_gizmos();
		for (int i = 0; i < gizmos.size(); i++) {
			Ref<EditorNode3DGizmo> gizmo = gizmos[i];
			if (!gizmo.is_valid()) {
				continue;
			}
			Ref<Skeleton3DGizmoPlugin> plugin = gizmo->get_plugin();
			if (!plugin.is_valid()) {
				continue;
			}
			skeleton->set_subgizmo_selection(gizmo, selected, skeleton->get_bone_global_pose(selected));
			break;
		}
	} else {
		skeleton->clear_subgizmo_selection();
	}
}

void Skeleton3DEditor::select_bone(int p_idx) {
	if (p_idx >= 0) {
		TreeItem *ti = _find(joint_tree->get_root(), "bones/" + itos(p_idx));
		if (ti) {
			// Make visible when it's collapsed.
			TreeItem *node = ti->get_parent();
			while (node && node != joint_tree->get_root()) {
				node->set_collapsed(false);
				node = node->get_parent();
			}
			ti->select(0);
			joint_tree->scroll_to_item(ti);
		}
	} else {
		selected_bone = -1;
		joint_tree->deselect_all();
		_joint_tree_selection_changed();
	}
}

Skeleton3DEditor::~Skeleton3DEditor() {
	if (skeleton) {
#ifdef TOOLS_ENABLED
		skeleton->disconnect("show_rest_only_changed", callable_mp(this, &Skeleton3DEditor::_update_show_rest_only));
		skeleton->disconnect("bone_enabled_changed", callable_mp(this, &Skeleton3DEditor::_update_pose_enabled));
		skeleton->disconnect("pose_updated", callable_mp(this, &Skeleton3DEditor::_draw_gizmo));
		skeleton->disconnect("pose_updated", callable_mp(this, &Skeleton3DEditor::_update_properties));
		skeleton->set_transform_gizmo_visible(true);
#endif
		handles_mesh_instance->get_parent()->remove_child(handles_mesh_instance);
	}

	handles_mesh_instance->queue_delete();

	Node3DEditor *ne = Node3DEditor::get_singleton();

	if (separator) {
		ne->remove_control_from_menu_panel(separator);
		memdelete(separator);
	}

	if (skeleton_options) {
		ne->remove_control_from_menu_panel(skeleton_options);
		memdelete(skeleton_options);
	}

	if (rest_options) {
		ne->remove_control_from_menu_panel(rest_options);
		memdelete(rest_options);
	}

	if (edit_mode_button) {
		ne->remove_control_from_menu_panel(edit_mode_button);
		memdelete(edit_mode_button);
	}
}

bool EditorInspectorPluginSkeleton::can_handle(Object *p_object) {
	return Object::cast_to<Skeleton3D>(p_object) != nullptr;
}

void EditorInspectorPluginSkeleton::parse_begin(Object *p_object) {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(p_object);
	ERR_FAIL_COND(!skeleton);

	skel_editor = memnew(Skeleton3DEditor(this, editor, skeleton));
	add_custom_control(skel_editor);
}

Skeleton3DEditorPlugin::Skeleton3DEditorPlugin(EditorNode *p_node) {
	editor = p_node;

	skeleton_plugin = memnew(EditorInspectorPluginSkeleton);
	skeleton_plugin->editor = editor;

	EditorInspector::add_inspector_plugin(skeleton_plugin);

	Ref<Skeleton3DGizmoPlugin> gizmo_plugin = Ref<Skeleton3DGizmoPlugin>(memnew(Skeleton3DGizmoPlugin));
	Node3DEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);
}

EditorPlugin::AfterGUIInput Skeleton3DEditorPlugin::forward_spatial_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	Skeleton3DEditor *se = Skeleton3DEditor::get_singleton();
	Node3DEditor *ne = Node3DEditor::get_singleton();
	if (se->is_edit_mode()) {
		const Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (ne->get_tool_mode() != Node3DEditor::TOOL_MODE_SELECT) {
				if (!ne->is_gizmo_visible()) {
					return EditorPlugin::AFTER_GUI_INPUT_STOP;
				}
			}
			if (mb->is_pressed()) {
				se->update_bone_original();
			}
		}
		return EditorPlugin::AFTER_GUI_INPUT_DESELECT;
	}
	return EditorPlugin::AFTER_GUI_INPUT_PASS;
}

bool Skeleton3DEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("Skeleton3D");
}

void Skeleton3DEditor::_update_gizmo_transform() {
	Node3DEditor::get_singleton()->update_transform_gizmo();
};

void Skeleton3DEditor::_update_gizmo_visible() {
	_subgizmo_selection_change();
	if (edit_mode) {
		if (selected_bone == -1) {
#ifdef TOOLS_ENABLED
			skeleton->set_transform_gizmo_visible(false);
#endif
		} else {
#ifdef TOOLS_ENABLED
			if (skeleton->is_bone_enabled(selected_bone) && !skeleton->is_show_rest_only()) {
				skeleton->set_transform_gizmo_visible(true);
			} else {
				skeleton->set_transform_gizmo_visible(false);
			}
#endif
		}
	} else {
#ifdef TOOLS_ENABLED
		skeleton->set_transform_gizmo_visible(true);
#endif
	}
	_draw_gizmo();
}

int Skeleton3DEditor::get_selected_bone() const {
	return selected_bone;
}

Skeleton3DGizmoPlugin::Skeleton3DGizmoPlugin() {
	unselected_mat = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	unselected_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	unselected_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	unselected_mat->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	unselected_mat->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);

	selected_mat = Ref<ShaderMaterial>(memnew(ShaderMaterial));
	selected_sh = Ref<Shader>(memnew(Shader));
	selected_sh->set_code(R"(
// Skeleton 3D gizmo bones shader.

shader_type spatial;
render_mode unshaded, shadows_disabled;
void vertex() {
	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix( pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)), COLOR.rgb* (1.0 / 12.92), lessThan(COLOR.rgb,vec3(0.04045)) );
	}
	VERTEX = VERTEX;
	POSITION=PROJECTION_MATRIX*INV_CAMERA_MATRIX*WORLD_MATRIX*vec4(VERTEX.xyz,1.0);
	POSITION.z = mix(POSITION.z, 0, 0.998);
}
void fragment() {
	ALBEDO = COLOR.rgb;
	ALPHA = COLOR.a;
}
)");
	selected_mat->set_shader(selected_sh);

	// Regist properties in editor settings.
	EDITOR_DEF("editors/3d_gizmos/gizmo_colors/skeleton", Color(1, 0.8, 0.4));
	EDITOR_DEF("editors/3d_gizmos/gizmo_colors/selected_bone", Color(0.8, 0.3, 0.0));
	EDITOR_DEF("editors/3d_gizmos/gizmo_settings/bone_axis_length", (float)0.1);
	EDITOR_DEF("editors/3d_gizmos/gizmo_settings/bone_shape", 1);
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, "editors/3d_gizmos/gizmo_settings/bone_shape", PROPERTY_HINT_ENUM, "Wire,Octahedron"));
}

bool Skeleton3DGizmoPlugin::has_gizmo(Node3D *p_spatial) {
	return Object::cast_to<Skeleton3D>(p_spatial) != nullptr;
}

String Skeleton3DGizmoPlugin::get_gizmo_name() const {
	return "Skeleton3D";
}

int Skeleton3DGizmoPlugin::get_priority() const {
	return -1;
}

int Skeleton3DGizmoPlugin::subgizmos_intersect_ray(const EditorNode3DGizmo *p_gizmo, Camera3D *p_camera, const Vector2 &p_point) const {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(!skeleton, -1);

	Skeleton3DEditor *se = Skeleton3DEditor::get_singleton();

	if (!se->is_edit_mode()) {
		return -1;
	}

	if (Node3DEditor::get_singleton()->get_tool_mode() != Node3DEditor::TOOL_MODE_SELECT) {
		return -1;
	}

	// Select bone.
	real_t grab_threshold = 4 * EDSCALE;
	Vector3 ray_from = p_camera->get_global_transform().origin;
	Transform3D gt = skeleton->get_global_transform();
	int closest_idx = -1;
	real_t closest_dist = 1e10;
	const int bone_len = skeleton->get_bone_count();
	for (int i = 0; i < bone_len; i++) {
		Vector3 joint_pos_3d = gt.xform(skeleton->get_bone_global_pose(i).origin);
		Vector2 joint_pos_2d = p_camera->unproject_position(joint_pos_3d);
		real_t dist_3d = ray_from.distance_to(joint_pos_3d);
		real_t dist_2d = p_point.distance_to(joint_pos_2d);
		if (dist_2d < grab_threshold && dist_3d < closest_dist) {
			closest_dist = dist_3d;
			closest_idx = i;
		}
	}

	if (closest_idx >= 0) {
		WARN_PRINT("ray:");
		WARN_PRINT(itos(closest_idx));
		se->select_bone(closest_idx);
		return closest_idx;
	}

	se->select_bone(-1);
	return -1;
}

Transform3D Skeleton3DGizmoPlugin::get_subgizmo_transform(const EditorNode3DGizmo *p_gizmo, int p_id) const {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(!skeleton, Transform3D());

	return skeleton->get_bone_global_pose(p_id);
}

void Skeleton3DGizmoPlugin::set_subgizmo_transform(const EditorNode3DGizmo *p_gizmo, int p_id, Transform3D p_transform) {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(!skeleton);

	// Prepare for global to local.
	Transform3D original_to_local = Transform3D();
	int parent_idx = skeleton->get_bone_parent(p_id);
	if (parent_idx >= 0) {
		original_to_local = original_to_local * skeleton->get_bone_global_pose(parent_idx);
	}
	original_to_local = original_to_local * skeleton->get_bone_rest(p_id) * skeleton->get_bone_custom_pose(p_id);
	Basis to_local = original_to_local.get_basis().inverse();

	// Prepare transform.
	Transform3D t = Transform3D();

	// Basis.
	t.basis = to_local * p_transform.get_basis();

	// Origin.
	Vector3 orig = Vector3();
	orig = skeleton->get_bone_pose(p_id).origin;
	Vector3 sub = p_transform.origin - skeleton->get_bone_global_pose(p_id).origin;
	t.origin = orig + to_local.xform(sub);

	// Apply transform.
	skeleton->set_bone_pose(p_id, t);
}

void Skeleton3DGizmoPlugin::commit_subgizmos(const EditorNode3DGizmo *p_gizmo, const Vector<int> &p_ids, const Vector<Transform3D> &p_restore, bool p_cancel) {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(!skeleton);

	Skeleton3DEditor *se = Skeleton3DEditor::get_singleton();

	UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
	for (int i = 0; i < p_ids.size(); i++) {
		ur->create_action(TTR("Set Bone Transform"));
		ur->add_do_method(skeleton, "set_bone_pose", p_ids[i], skeleton->get_bone_pose(p_ids[i]));
		ur->add_undo_method(skeleton, "set_bone_pose", p_ids[i], se->get_bone_original());
	}
	ur->commit_action();
}

void Skeleton3DGizmoPlugin::redraw(EditorNode3DGizmo *p_gizmo) {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(p_gizmo->get_spatial_node());
	p_gizmo->clear();

	int selected = -1;
	Skeleton3DEditor *se = Skeleton3DEditor::get_singleton();
	if (se) {
		selected = se->get_selected_bone();
	}

	Color bone_color = EditorSettings::get_singleton()->get("editors/3d_gizmos/gizmo_colors/skeleton");
	Color selected_bone_color = EditorSettings::get_singleton()->get("editors/3d_gizmos/gizmo_colors/selected_bone");
	real_t bone_axis_length = EditorSettings::get_singleton()->get("editors/3d_gizmos/gizmo_settings/bone_axis_length");
	int bone_shape = EditorSettings::get_singleton()->get("editors/3d_gizmos/gizmo_settings/bone_shape");

	LocalVector<Color> axis_colors;
	axis_colors.push_back(Node3DEditor::get_singleton()->get_theme_color(SNAME("axis_x_color"), SNAME("Editor")));
	axis_colors.push_back(Node3DEditor::get_singleton()->get_theme_color(SNAME("axis_y_color"), SNAME("Editor")));
	axis_colors.push_back(Node3DEditor::get_singleton()->get_theme_color(SNAME("axis_z_color"), SNAME("Editor")));

	Ref<SurfaceTool> surface_tool(memnew(SurfaceTool));
	surface_tool->begin(Mesh::PRIMITIVE_LINES);

	if (p_gizmo->is_selected()) {
		surface_tool->set_material(selected_mat);
	} else {
		unselected_mat->set_albedo(bone_color);
		surface_tool->set_material(unselected_mat);
	}

	Vector<Transform3D> grests;
	grests.resize(skeleton->get_bone_count());

	LocalVector<int> bones;
	LocalVector<float> weights;
	bones.resize(4);
	weights.resize(4);
	for (int i = 0; i < 4; i++) {
		bones[i] = 0;
		weights[i] = 0;
	}
	weights[0] = 1;

	int current_bone_index = 0;
	Vector<int> bones_to_process = skeleton->get_parentless_bones();

	while (bones_to_process.size() > current_bone_index) {
		int current_bone_idx = bones_to_process[current_bone_index];
		current_bone_index++;

		Color current_bone_color = (current_bone_idx == selected) ? selected_bone_color : bone_color;

		Vector<int> child_bones_vector;
		child_bones_vector = skeleton->get_bone_children(current_bone_idx);
		int child_bones_size = child_bones_vector.size();

		// You have children but no parent, then you must be a root/parentless bone.
		if (skeleton->get_bone_parent(current_bone_idx) < 0) {
			grests.write[current_bone_idx] = skeleton->get_bone_rest(current_bone_idx);
		}

		for (int i = 0; i < child_bones_size; i++) {
			// Something wrong.
			if (child_bones_vector[i] < 0) {
				continue;
			}

			int child_bone_idx = child_bones_vector[i];

			grests.write[child_bone_idx] = grests[current_bone_idx] * skeleton->get_bone_rest(child_bone_idx);

			Vector3 v0 = grests[current_bone_idx].origin;
			Vector3 v1 = grests[child_bone_idx].origin;
			Vector3 d = (v1 - v0).normalized();
			real_t dist = v0.distance_to(v1);

			// Find closest axis.
			int closest = -1;
			real_t closest_d = 0.0;
			for (int j = 0; j < 3; j++) {
				real_t dp = Math::abs(grests[current_bone_idx].basis[j].normalized().dot(d));
				if (j == 0 || dp > closest_d) {
					closest = j;
				}
			}

			// Draw bone.
			switch (bone_shape) {
				case 0: { // Wire shape.
					surface_tool->set_color(current_bone_color);
					bones[0] = current_bone_idx;
					surface_tool->set_bones(bones);
					surface_tool->set_weights(weights);
					surface_tool->add_vertex(v0);
					bones[0] = child_bone_idx;
					surface_tool->set_bones(bones);
					surface_tool->set_weights(weights);
					surface_tool->add_vertex(v1);
				} break;

				case 1: { // Octahedron shape.
					Vector3 first;
					Vector3 points[6];
					int point_idx = 0;
					for (int j = 0; j < 3; j++) {
						Vector3 axis;
						if (first == Vector3()) {
							axis = d.cross(d.cross(grests[current_bone_idx].basis[j])).normalized();
							first = axis;
						} else {
							axis = d.cross(first).normalized();
						}

						surface_tool->set_color(current_bone_color);
						for (int k = 0; k < 2; k++) {
							if (k == 1) {
								axis = -axis;
							}
							Vector3 point = v0 + d * dist * 0.2;
							point += axis * dist * 0.1;

							bones[0] = current_bone_idx;
							surface_tool->set_bones(bones);
							surface_tool->set_weights(weights);
							surface_tool->add_vertex(v0);
							surface_tool->set_bones(bones);
							surface_tool->set_weights(weights);
							surface_tool->add_vertex(point);

							surface_tool->set_bones(bones);
							surface_tool->set_weights(weights);
							surface_tool->add_vertex(point);
							bones[0] = child_bone_idx;
							surface_tool->set_bones(bones);
							surface_tool->set_weights(weights);
							surface_tool->add_vertex(v1);
							points[point_idx++] = point;
						}
					}
					surface_tool->set_color(current_bone_color);
					SWAP(points[1], points[2]);
					bones[0] = current_bone_idx;
					for (int j = 0; j < 6; j++) {
						surface_tool->set_bones(bones);
						surface_tool->set_weights(weights);
						surface_tool->add_vertex(points[j]);
						surface_tool->set_bones(bones);
						surface_tool->set_weights(weights);
						surface_tool->add_vertex(points[(j + 1) % 6]);
					}
				} break;
			}

			// Axis as root of the bone.
			for (int j = 0; j < 3; j++) {
				bones[0] = current_bone_idx;
				surface_tool->set_color(axis_colors[j]);
				surface_tool->set_bones(bones);
				surface_tool->set_weights(weights);
				surface_tool->add_vertex(v0);
				surface_tool->set_bones(bones);
				surface_tool->set_weights(weights);
				surface_tool->add_vertex(v0 + (grests[current_bone_idx].basis.inverse())[j].normalized() * dist * bone_axis_length);

				if (j == closest) {
					continue;
				}
			}

			// Axis at the end of the bone children.
			if (i == child_bones_size - 1) {
				for (int j = 0; j < 3; j++) {
					bones[0] = child_bone_idx;
					surface_tool->set_color(axis_colors[j]);
					surface_tool->set_bones(bones);
					surface_tool->set_weights(weights);
					surface_tool->add_vertex(v1);
					surface_tool->set_bones(bones);
					surface_tool->set_weights(weights);
					surface_tool->add_vertex(v1 + (grests[child_bone_idx].basis.inverse())[j].normalized() * dist * bone_axis_length);

					if (j == closest) {
						continue;
					}
				}
			}

			// Add the bone's children to the list of bones to be processed.
			bones_to_process.push_back(child_bones_vector[i]);
		}
	}

	Ref<ArrayMesh> m = surface_tool->commit();
	p_gizmo->add_mesh(m, Ref<Material>(), Transform3D(), skeleton->register_skin(Ref<Skin>()));
}
