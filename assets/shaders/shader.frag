#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coord;
layout(location = 3) in vec3 color;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D texture_sampler;

void main()
{
	vec3 light_position = vec3(50.0f, 20.0f, 10.0f);
	vec3 view_position  = vec3(1.0f, 1.0f, 1.0f);
	vec3 light_color    = vec3(0.9f, 0.9f, 1.0f);
	vec3 object_color   = vec3(1.0f, 1.0f, 1.0f);

	// ambient
	float ambient_strength = 0.1f;
	vec3  ambient          = ambient_strength * light_color;

	// diffuse
	vec3  norm      = normalize(normal);
	vec3  light_dir = normalize(light_position - position);
	float diff      = max(dot(norm, light_dir), 0.0);
	vec3  diffuse   = diff * light_color;

	// specular
	float specular_strength = 0.5;
	vec3  view_dir          = normalize(view_position - position);
	vec3  reflect_dir       = reflect(-light_dir, norm);
	float spec              = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
	vec3  specular          = specular_strength * spec * light_color;
	vec3  result            = (ambient + diffuse + specular) * object_color;

	out_color = vec4(result, 1.0f) * texture(texture_sampler, texture_coord);

	// out_color = vec4(color, 1.0);
	// out_color = texture(texture_sampler, texture_coord);
}
