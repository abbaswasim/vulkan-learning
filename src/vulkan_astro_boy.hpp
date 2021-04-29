// Copyright 2018
#pragma once

#include "assets/astroboy/astro_boy_geometry.hpp"
#include "assets/astroboy/astro_boy_animation.hpp"
#include "common.hpp"
#include "roar.hpp"
#include <array>
#include <foundation/rortypes.hpp>
#include <vulkan/vulkan_core.h>

/*
const unsigned int astro_boy_vertex_count = 3616;
const unsigned int astro_boy_positions_array_count = astro_boy_vertex_count * 3;
float astro_boy_positions[astro_boy_positions_array_count];
const unsigned int astro_boy_normals_array_count = astro_boy_vertex_count * 3;
float astro_boy_normals[astro_boy_normals_array_count];
const unsigned int astro_boy_uvs_array_count = astro_boy_vertex_count * 2;
float astro_boy_uvs[astro_boy_uvs_array_count];
const unsigned int astro_boy_weights_array_count = astro_boy_vertex_count * 3;
float astro_boy_weights[astro_boy_weights_array_count];
const unsigned int astro_boy_joints_array_count = astro_boy_vertex_count * 3;
int astro_boy_joints[astro_boy_joints_array_count];
const unsigned int astro_boy_triangles_count = 4881;
const unsigned int astro_boy_indices_array_count = 14643;
unsigned int astro_boy_indices[astro_boy_indices_array_count];
*/

namespace utl
{
static auto get_astro_boy_vertex_attributes()
{
	std::array<VkVertexInputAttributeDescription, 5> attributes;

	uint32_t off              = 0;

	uint32_t position_loc_bind = 0;
	uint32_t normal_loc_bind   = 1;
	uint32_t uv_loc_bind       = 2;
	uint32_t weight_loc_bind   = 3;
	uint32_t jointid_loc_bind  = 4;

	attributes[position_loc_bind].location = position_loc_bind;        // Position
	attributes[position_loc_bind].binding  = position_loc_bind + off;
	attributes[position_loc_bind].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[position_loc_bind].offset   = 0;

	attributes[normal_loc_bind].location = normal_loc_bind;        // Normal
	attributes[normal_loc_bind].binding  = normal_loc_bind + off;
	attributes[normal_loc_bind].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[normal_loc_bind].offset   = 0;

	attributes[uv_loc_bind].location = uv_loc_bind;        // UV
	attributes[uv_loc_bind].binding  = uv_loc_bind + off;
	attributes[uv_loc_bind].format   = VK_FORMAT_R32G32_SFLOAT;
	attributes[uv_loc_bind].offset   = 0;

	attributes[weight_loc_bind].location = weight_loc_bind;        // Weight
	attributes[weight_loc_bind].binding  = weight_loc_bind + off;
	attributes[weight_loc_bind].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[weight_loc_bind].offset   = 0;

	attributes[jointid_loc_bind].location = jointid_loc_bind;        // JointID
	attributes[jointid_loc_bind].binding  = jointid_loc_bind + off;
	attributes[jointid_loc_bind].format   = VK_FORMAT_R32G32B32_UINT;
	attributes[jointid_loc_bind].offset   = 0;

	return attributes;
}

static auto get_astro_boy_vertex_bindings()
{
	std::array<VkVertexInputBindingDescription, 5> bindings;

	uint32_t off              = 0; // Experimentation to be removed later, this just proves binding is in the index in bindings array but a number that needs to match the attribute binding

	uint32_t position_binding = 0;
	uint32_t normal_binding   = 1;
	uint32_t uv_binding       = 2;
	uint32_t weight_binding   = 3;
	uint32_t jointid_binding  = 4;

	bindings[position_binding].binding   = position_binding + off;
	bindings[position_binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindings[position_binding].stride    = sizeof(float32_t) * 3;

	bindings[normal_binding].binding   = normal_binding + off;
	bindings[normal_binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindings[normal_binding].stride    = sizeof(float32_t) * 3;

	bindings[uv_binding].binding   = uv_binding + off;
	bindings[uv_binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindings[uv_binding].stride    = sizeof(float32_t) * 2;

	bindings[weight_binding].binding   = weight_binding + off;
	bindings[weight_binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindings[weight_binding].stride    = sizeof(float32_t) * 3;

	bindings[jointid_binding].binding   = jointid_binding + off;
	bindings[jointid_binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindings[jointid_binding].stride    = sizeof(uint32_t) * 3;

	return bindings;
}

}        // namespace utl
