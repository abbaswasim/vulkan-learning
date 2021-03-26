#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec3 weights;          // Basically colors for now
layout(location = 4) in vec3 joint_ids;        // Basically weights used as colors for now

layout(location = 0) out vec3 fragColor;

void main()
{
	mat4 m = mat4(1.62699234, 0.170699045, 0.331812143, 0.328510523,
				  -0.594050169, 0.467512727, 0.908771395, 0.899728834,
				  -0.00000003, 1.65900362, -0.290235788, -0.287347853,
				  0.0, -4.9770112, 10.4109173, 11.3023491);

	mat4 roty = mat4(
		-1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		-0.0, 0.0, -1.0, 0.0,
		0.0, 0.0, 0.0, 1.0);

	mat4 vulkan_clip_correction = mat4(1.0, 0.0, 0.0, 0.0,
									   0.0, -1.0, 0.0, 0.0,
									   0.0, 0.0, 0.5, 0.0,
									   0.0, 0.0, 0.5, 1.0f);
	// gl_Position = vec4(positions, 1.0) * 0.01;
	gl_Position = vulkan_clip_correction * roty * m * vec4(positions, 1.0);
	// gl_Position = vulkan_clip_correction * m * vec4(positions, 1.0);
	// fragColor = (vulkan_clip_correction * roty * m * vec4(normals, 1.0)).xyz;
	fragColor = normals;
}
