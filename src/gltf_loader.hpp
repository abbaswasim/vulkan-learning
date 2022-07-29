// VulkanEd Source Code
// Wasim Abbas
// http://www.waZim.com
// Copyright (c) 2021
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the 'Software'),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Version: 1.0.0

#include "utils.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <foundation/rortypes.hpp>
#include <math/rormatrix4.hpp>
#include <math/rorvector2.hpp>
#include <math/rorvector3.hpp>
#include <math/rorvector4.hpp>
#include <profiling/rorlog.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "ctpl_stl.h"

namespace ast
{
// struct Vertex
// {
//	ror::Vector3f m_position;
//	ror::Vector3f m_normal;
//	ror::Vector2f m_texture_coordinate;
//	ror::Vector3f m_color;
// };

struct Attribute
{
	size_t  m_offset{0};               // Start of the data
	size_t  m_count{0};                // How much of the data we need
	int32_t m_buffer_index{-1};        // Where the data is

	// TODO: Add types, float, int, vec2, vec4 etc
	// At the moment assuming position3, normal3, uv2, color3 all float
	// while index scalar of unsigned int 16 type
};

enum array_index
{
	position,
	normal,
	texture_coordinate,
	color,
	indices
};

struct Primitive
{
	std::array<Attribute, 5> m_arrays;

	int32_t m_material_index{-1};
	bool    m_has_indices{false};
};

struct Mesh
{
	std::vector<Primitive> m_parts;
};

struct Node
{
	int32_t               m_parent{-1};
	std::vector<uint32_t> m_children;
	int32_t               m_mesh_index{-1};
	ror::Matrix4f         m_transformation;
};

enum class AlphaMode
{
	opaque,
	mask,
	blend
};

enum class TextureFilter
{
	nearest                = 9728,
	linear                 = 9729,
	nearest_mipmap_nearest = 9984,
	linear_mipmap_nearest  = 9985,
	nearest_mipmap_linear  = 9986,
	linear_mipmap_linear   = 9987
};

enum class TextureWrap
{
	clamp_to_edge   = 33071,
	mirrored_repeat = 33648,
	repeat          = 10497
};

struct Material
{
	ror::Vector4f m_base_color_factor{1.0f};
	ror::Vector4f m_metalic_roughness_occlusion_factor{1.0f, 1.0f, 1.0f, 1.0f};
	ror::Vector4f m_emissive_factor{0.0f, 0.0f, 0.0f, 1.0f};
	int32_t       m_base_color_texture_index{-1};
	int32_t       m_metalic_roughness_occlusion_texture_index{-1};
	int32_t       m_normal_texture_index{-1};
	AlphaMode     m_alpha_mode{AlphaMode::opaque};
	float32_t     m_alpha_cutoff{0.5f};
	bool          m_double_sided{false};
};

struct Sampler
{
	TextureFilter m_mag_filter{TextureFilter::linear};
	TextureFilter m_min_filter{TextureFilter::linear_mipmap_linear};
	TextureWrap   m_wrap_s{TextureWrap::repeat};
	TextureWrap   m_wrap_t{TextureWrap::repeat};
};

struct Texture
{
	int32_t m_image_index{-1};
	int32_t m_sampler_index{-1};
};

// Inspired by https://github.com/SaschaWillems/Vulkan/blob/master/examples/gltfloading/gltfloading.cpp
class ROAR_ENGINE_ITEM GLTFModel
{
  public:
	FORCE_INLINE GLTFModel()                             = default;                   //! Default constructor
	FORCE_INLINE GLTFModel(const GLTFModel &a_other)     = default;                   //! Copy constructor
	FORCE_INLINE GLTFModel(GLTFModel &&a_other) noexcept = default;                   //! Move constructor
	FORCE_INLINE GLTFModel &operator=(const GLTFModel &a_other) = default;            //! Copy assignment operator
	FORCE_INLINE GLTFModel &operator=(GLTFModel &&a_other) noexcept = default;        //! Move assignment operator
	FORCE_INLINE virtual ~GLTFModel() noexcept                      = default;        //! Destructor

	void virtual temp();

	void load_from_file(std::filesystem::path a_filename)
	{
		ctpl::thread_pool tp(static_cast<int32_t>(std::thread::hardware_concurrency() - 1));

		cgltf_options options{};        // Default setting
		cgltf_data *  data{nullptr};
		cgltf_result  result         = cgltf_parse_file(&options, a_filename.c_str(), &data);
		cgltf_result  buffers_result = cgltf_load_buffers(&options, data, a_filename.c_str());

		if (buffers_result != cgltf_result_success)
			ror::log_critical("Can't load gltf binary data");

		std::filesystem::path root_dir = a_filename.parent_path();

		if (result == cgltf_result_success)
		{
			std::unordered_map<cgltf_image *, int32_t>    image_to_index{};
			std::unordered_map<cgltf_sampler *, int32_t>  sampler_to_index{};
			std::unordered_map<cgltf_texture *, int32_t>  texture_to_index{};
			std::unordered_map<cgltf_material *, int32_t> material_to_index{};
			std::unordered_map<cgltf_buffer *, int32_t>   buffer_to_index{};
			std::unordered_map<cgltf_mesh *, int32_t>     mesh_to_index{};
			std::unordered_map<cgltf_node *, uint32_t>    node_to_index{};

			std::vector<std::future<utl::TextureImage>> future_texures{data->images_count};

			auto lambda = [&](int a_thread_id,const std::filesystem::path& a_texture_path) -> utl::TextureImage {
				(void) a_thread_id;

				// ror::log_critical("Going to load texture {}", a_texture_path.c_str());

				return utl::read_texture_from_file(a_texture_path.c_str());
			};

			// Read all the images
			for (size_t i = 0; i < data->images_count; ++i)
			{
				assert(data->images[i].uri && "Image URI is null, only support URI based images");
				auto texture_path = root_dir / data->images[i].uri;

				future_texures[i] = tp.push(lambda, texture_path);
				image_to_index.emplace(&data->images[i], i);
			}

			for (size_t i = 0; i < data->images_count; ++i)
			{
				this->m_images.emplace_back(future_texures[i].get());
			}

			// Lets have a default sampler at index 0
			this->m_samplers.emplace_back();

			// Read all the samplers
			for (size_t i = 0; i < data->samplers_count; ++i)
			{
				Sampler sampler;

				sampler.m_mag_filter = static_cast<TextureFilter>(data->samplers[i].mag_filter);
				sampler.m_min_filter = static_cast<TextureFilter>(data->samplers[i].min_filter);
				sampler.m_wrap_s     = static_cast<TextureWrap>(data->samplers[i].wrap_s);
				sampler.m_wrap_t     = static_cast<TextureWrap>(data->samplers[i].wrap_t);

				this->m_samplers.emplace_back(sampler);
				sampler_to_index.emplace(&data->samplers[i], i + 1);        // Index 0 is default hence the +1
			}

			for (size_t i = 0; i < data->textures_count; ++i)
			{
				assert(data->textures[i].image && "Texture must have an image");

				Texture t;
				t.m_image_index = (image_to_index.find(data->textures[i].image) != image_to_index.end() ? image_to_index[data->textures[i].image] : -1);
				assert(t.m_image_index != -1 && "No image loaded for the texture");

				if (data->textures[i].sampler != nullptr)
				{
					t.m_sampler_index = (sampler_to_index.find(data->textures[i].sampler) != sampler_to_index.end() ? sampler_to_index[data->textures[i].sampler] : -1);
				}

				// If no samplers provided use default at index 0
				if (t.m_sampler_index == -1)
					t.m_sampler_index = 0;

				this->m_textures.emplace_back(t);
				texture_to_index.emplace(&data->textures[i], i);
			}

			// Read all the materials
			for (size_t i = 0; i < data->materials_count; ++i)
			{
				auto convert_to_vec4 = [](float input[4]) { return ror::Vector4f{input[0], input[1], input[2], input[3]}; };

				cgltf_material &mat = data->materials[i];

				Material material;
				material.m_double_sided = mat.double_sided;
				material.m_alpha_cutoff = mat.alpha_cutoff;

				if (mat.has_pbr_metallic_roughness)
				{
					material.m_base_color_factor                         = convert_to_vec4(mat.pbr_metallic_roughness.base_color_factor);
					material.m_metalic_roughness_occlusion_factor.x      = mat.pbr_metallic_roughness.metallic_factor;
					material.m_metalic_roughness_occlusion_factor.y      = mat.pbr_metallic_roughness.roughness_factor;
					material.m_metalic_roughness_occlusion_texture_index = texture_to_index.find(mat.pbr_metallic_roughness.metallic_roughness_texture.texture) != texture_to_index.end() ? texture_to_index[mat.pbr_metallic_roughness.metallic_roughness_texture.texture] : -1;
					material.m_base_color_texture_index                  = texture_to_index.find(mat.pbr_metallic_roughness.base_color_texture.texture) != texture_to_index.end() ? texture_to_index[mat.pbr_metallic_roughness.base_color_texture.texture] : -1;
				}

				material.m_normal_texture_index = texture_to_index.find(mat.normal_texture.texture) != texture_to_index.end() ? texture_to_index[mat.normal_texture.texture] : -1;

				switch (mat.alpha_mode)
				{
					case cgltf_alpha_mode_opaque:
						material.m_alpha_mode = AlphaMode::opaque;
						break;
					case cgltf_alpha_mode_mask:
						material.m_alpha_mode = AlphaMode::mask;
						break;
					case cgltf_alpha_mode_blend:
						material.m_alpha_mode = AlphaMode::blend;
						break;
				}

				material_to_index.emplace(&mat, i);
			}

			assert(data->buffers_count > 0 && "No buffers loaded");
			for (size_t i = 0; i < data->buffers_count; ++i)
			{
				std::vector<uint8_t> buffer(static_cast<uint8_t *>(data->buffers[i].data), static_cast<uint8_t *>(data->buffers[i].data) + data->buffers[i].size);
				this->m_buffers.emplace_back(buffer);
				buffer_to_index.emplace(&data->buffers[i], i);
			}

			// Read all the meshes
			for (size_t i = 0; i < data->meshes_count; ++i)
			{
				Mesh        mesh;
				cgltf_mesh &cmesh = data->meshes[i];

				for (size_t j = 0; j < cmesh.primitives_count; ++j)
				{
					cgltf_primitive &cprim = cmesh.primitives[j];
					Primitive        prim{};

					if (cprim.has_draco_mesh_compression)
						ror::log_critical("Mesh has draco mesh compression but its not supported");

					if (cprim.material)
						prim.m_material_index = material_to_index.find(cprim.material) != material_to_index.end() ? material_to_index[cprim.material] : -1;

					assert(cprim.type == cgltf_primitive_type_triangles && "Mesh primitive type is not triangles which is the only supported type at the moment");

					if (cprim.indices)
					{
						assert(cprim.indices->type == cgltf_type_scalar && "Indices are not the right type, only SCALAR indices supported");
						assert(cprim.indices->component_type == cgltf_component_type_r_16u && "Indices are not in the right component right, only uint16_t supported");
						assert(cprim.indices->buffer_view && "Indices doesn't have a valid buffer view");
						// assert(cprim.indices->buffer_view->type == cgltf_buffer_view_type_indices && "Indices buffer view type is wrong"); type is alway invalid, because no such thing in bufferView in glTF

						if (cprim.indices->buffer_view->has_meshopt_compression)
							ror::log_critical("Mesh has meshopt_compression but its not supported");

						prim.m_has_indices                                 = true;
						prim.m_arrays[array_index::indices].m_count        = cprim.indices->count;
						prim.m_arrays[array_index::indices].m_buffer_index = buffer_to_index.find(cprim.indices->buffer_view->buffer) != buffer_to_index.end() ? buffer_to_index[cprim.indices->buffer_view->buffer] : -1;
						prim.m_arrays[array_index::indices].m_offset       = cprim.indices->buffer_view->offset;
					}

					for (size_t k = 0; k < cmesh.primitives[j].attributes_count; ++k)
					{
						cgltf_attribute &attrib = cmesh.primitives[j].attributes[k];

						assert(attrib.data->buffer_view && "Attribute doesn't have a valid buffer view");
						array_index current_index = array_index::position;

						switch (attrib.type)
						{
							case cgltf_attribute_type_position:
								assert(attrib.data->component_type == cgltf_component_type_r_32f && attrib.data->type == cgltf_type_vec3 && "Position not in the right format, float3 required");
								current_index = array_index::position;
								break;
							case cgltf_attribute_type_normal:
								current_index = array_index::normal;
								assert(attrib.data->component_type == cgltf_component_type_r_32f && attrib.data->type == cgltf_type_vec3 && "Normal not in the right format, float3 required");
								break;
							case cgltf_attribute_type_texcoord:
								current_index = array_index::texture_coordinate;
								assert(attrib.data->component_type == cgltf_component_type_r_32f && attrib.data->type == cgltf_type_vec2 && "Texture coordinate not in the right format, float2 required");
								break;
							case cgltf_attribute_type_color:
								current_index = array_index::color;
								assert(attrib.data->component_type == cgltf_component_type_r_32f && attrib.data->type == cgltf_type_vec3 && "Color not in the right format, float3 required");
								break;
							case cgltf_attribute_type_tangent:
							case cgltf_attribute_type_joints:
							case cgltf_attribute_type_weights:
							case cgltf_attribute_type_invalid:
								assert(0 && "Attribute not supported yet");
								break;
						}

						assert(attrib.data->buffer_view != nullptr && "No buffer view present in attribute accessor");
						assert(!attrib.data->is_sparse && "Don't support sparse attribute accessors");

						prim.m_arrays[current_index].m_buffer_index = buffer_to_index.find(attrib.data->buffer_view->buffer) != buffer_to_index.end() ? buffer_to_index[attrib.data->buffer_view->buffer] : -1;
						prim.m_arrays[current_index].m_count        = attrib.data->count;
						prim.m_arrays[current_index].m_offset       = attrib.data->buffer_view->offset;
					}

					mesh.m_parts.emplace_back(prim);
				}

				m_meshes.emplace_back(mesh);
				mesh_to_index.emplace(&cmesh, i);
			}

			// Read all the nodes
			for (size_t i = 0; i < data->nodes_count; ++i)
			{
				cgltf_node &cnode = data->nodes[i];

				Node node;
				node.m_mesh_index = utl::static_cast_safe<int32_t>(i);

				this->m_nodes.emplace_back(node);        // Will fill the rest later
				node_to_index.emplace(&cnode, i);
			}

			// Read the scene
			// Flattens all scenes into a node tree
			for (size_t i = 0; i < data->scenes_count; ++i)
			{
				Node node;

				cgltf_scene &scene = data->scenes[i];

				for (size_t j = 0; j < scene.nodes_count; ++j)
				{
					cgltf_node *cnode = scene.nodes[j];

					node.m_mesh_index = mesh_to_index.find(cnode->mesh) != mesh_to_index.end() ? mesh_to_index[cnode->mesh] : -1;
					cgltf_node_transform_local(cnode, node.m_transformation.m_values);

					for (size_t k = 0; k < cnode->children_count; ++k)
					{
						assert(node_to_index.find(cnode->children[k]) != node_to_index.end() && "Children not found in the inserted nodes");

						uint32_t child_index = node_to_index[cnode->children[k]];

						if (cnode->children[k]->mesh)
							this->m_nodes[child_index].m_mesh_index = mesh_to_index.find(cnode->children[k]->mesh) != mesh_to_index.end() ? mesh_to_index[cnode->children[k]->mesh] : -1;

						cgltf_node_transform_local(cnode->children[k], this->m_nodes[child_index].m_transformation.m_values);

						this->m_nodes[child_index].m_parent = utl::static_cast_safe<int32_t>(node_to_index[cnode]);

						node.m_children.push_back(child_index);
					}
				}
			}

			cgltf_free(data);
		}
		else
		{
			ror::log_critical("Can't load gltf file {}", a_filename.c_str());
		}
	}

  protected:
  private:
	std::vector<utl::TextureImage>    m_images;
	std::vector<Texture>              m_textures;
	std::vector<Sampler>              m_samplers;
	std::vector<Material>             m_materials;
	std::vector<Mesh>                 m_meshes;
	std::vector<Node>                 m_nodes;
	std::vector<std::vector<uint8_t>> m_buffers;        // Single set of buffers for all data
};

void GLTFModel::temp()
{}

}        // namespace ast
