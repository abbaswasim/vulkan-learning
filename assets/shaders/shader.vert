#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec3 weights;           // Basically colors for now
layout(location = 4) in uvec3 joint_ids;        // Basically weights used as colors for now

layout(location = 0) out vec3 position_out;
layout(location = 1) out vec3 normal_out;
layout(location = 2) out vec2 uv_out;
layout(location = 3) out vec3 color_out;

layout(binding = 0) uniform UBO
{
	mat4 model;
	mat4 view_projection;
	mat4 joints_matrices[64];
}ubo;

void main()
{
	mat4 keyframe_transform =
		ubo.joints_matrices[joint_ids.x] * weights.x +
		ubo.joints_matrices[joint_ids.y] * weights.y +
		ubo.joints_matrices[joint_ids.z] * weights.z;

	mat4 model_animated = ubo.model * keyframe_transform;

	normal_out          = mat3(transpose(inverse(ubo.model))) * normals;
	position_out        = vec3(model_animated * vec4(positions, 1.0));
	gl_Position         = ubo.view_projection * vec4(position_out, 1.0);

	uv_out              = uvs;
	uv_out.y            = 1.0 - uvs.y;

	color_out           = normals;
}
