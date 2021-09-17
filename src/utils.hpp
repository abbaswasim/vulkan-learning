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

#pragma once

#include "config.hpp"
#include "profiling/rorlog.hpp"
#include <CImg/CImg.h>
#include <foundation/rorutilities.hpp>

#include "transcoder/basisu_transcoder.h"

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace utl
{
struct TextureImage
{
	struct Mipmap
	{
		uint32_t m_width{0};
		uint32_t m_height{0};
		uint32_t m_depth{1};
		uint64_t m_offset{0};
	};

	void allocate(uint64_t a_size)
	{
		this->m_size = a_size;
		this->m_data.resize(this->m_size);
	}

	uint32_t get_width()
	{
		return this->m_mips[0].m_width;
	}

	uint32_t get_height()
	{
		return this->m_mips[0].m_height;
	}

	VkFormat get_format()
	{
		return this->m_format;
	}

	uint32_t get_mip_levels()
	{
		return ror::static_cast_safe<uint32_t>(this->m_mips.size());
	}

	std::vector<uint8_t> m_data;                                   // All mipmaps data, TODO: copying should be prevented
	uint64_t             m_size{0};                                // Size of all mipmaps combined
	VkFormat             m_format{VK_FORMAT_R8G8B8A8_SRGB};        // Texture format
	std::vector<Mipmap>  m_mips;                                   // Have at least one level
};

inline void read_texture_from_file_cimg(const char *a_file_name, TextureImage &a_texture)
{
	cimg_library::CImg<unsigned char> src(a_file_name);

	unsigned int width  = static_cast<uint32_t>(src.width());
	unsigned int height = static_cast<uint32_t>(src.height());
	unsigned int bpp    = static_cast<uint32_t>(src.spectrum());

	// Data is not stored like traditional RGBRGBRGBRGB triplets but rater RRRRGGGGBBBB
	// In other words R(0,0)R(1,0)R(0,1)R(1,1)G(0,0)G(1,0)G(0,1)G(1,1)B(0,0)B(1,0)B(0,1)B(1,1)
	src.mirror('y');
	unsigned char *ptr = src.data();

	unsigned int size = width * height;

	a_texture.allocate(size * 4);
	unsigned char *mixed = a_texture.m_data.data();

	for (unsigned int i = 0; i < size; i++)
	{
		for (unsigned int j = 0; j < bpp; j++)
		{
			mixed[(i * 4) + j] = ptr[i + (j * size)];
		}
		mixed[(i * 4) + 3] = 255;
	}

	TextureImage::Mipmap mip0;
	mip0.m_width       = width;
	mip0.m_height      = height;
	a_texture.m_format = VK_FORMAT_B8G8R8A8_SRGB;

	a_texture.m_mips.emplace_back(mip0);
}

// Not safe for unsigned to signed conversion but the compiler will complain about that
template <class _type_to,
		  class _type_from,
		  class _type_big = typename std::conditional<std::is_integral<_type_from>::value,
													  unsigned long long,
													  long double>::type>
FORCE_INLINE _type_to static_cast_safe(_type_from a_value)
{
	if (static_cast<_type_big>(a_value) > static_cast<_type_big>(std::numeric_limits<_type_to>::max()))
	{
		throw std::runtime_error("Loss of data doing casting.\n");        // TODO: Add some line number indication etc
	}

	return static_cast<_type_to>(a_value);
}

// Loads data at 8 bytes aligned address
using bytes_vector = std::vector<uint8_t>;
static_assert(alignof(bytes_vector) == 8, "Bytes vector not aligned to 8 bytes");

inline bool align_load_file(const std::string &a_file_path, bytes_vector &a_buffer)
{
	std::ifstream file(a_file_path, std::ios::in | std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Error! opening file " << a_file_path.c_str() << std::endl;
		return false;
	}

	auto file_size = file.tellg();
	file.seekg(0, std::ios_base::beg);

	if (file_size <= 0)
	{
		std::cout << "Error! reading file size " << a_file_path.c_str() << std::endl;
		return false;
	}

	a_buffer.resize(static_cast<size_t>(file_size));

	file.read(reinterpret_cast<char *>(a_buffer.data()), file_size);
	file.close();

	return true;
}

inline VkFormat basis_to_vk_format(basist::transcoder_texture_format a_fmt)
{
	switch (a_fmt)
	{
		case basist::transcoder_texture_format::cTFASTC_4x4:
			return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
		case basist::transcoder_texture_format::cTFBC7_RGBA:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		case basist::transcoder_texture_format::cTFRGBA32:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case basist::transcoder_texture_format::cTFETC1_RGB:
		case basist::transcoder_texture_format::cTFETC2_RGBA:
		case basist::transcoder_texture_format::cTFBC1_RGB:
		case basist::transcoder_texture_format::cTFBC3_RGBA:
		case basist::transcoder_texture_format::cTFBC4_R:
		case basist::transcoder_texture_format::cTFBC5_RG:
		case basist::transcoder_texture_format::cTFPVRTC1_4_RGB:
		case basist::transcoder_texture_format::cTFPVRTC1_4_RGBA:
		case basist::transcoder_texture_format::cTFATC_RGB:
		case basist::transcoder_texture_format::cTFATC_RGBA:
		case basist::transcoder_texture_format::cTFFXT1_RGB:
		case basist::transcoder_texture_format::cTFPVRTC2_4_RGB:
		case basist::transcoder_texture_format::cTFPVRTC2_4_RGBA:
		case basist::transcoder_texture_format::cTFETC2_EAC_R11:
		case basist::transcoder_texture_format::cTFETC2_EAC_RG11:
		case basist::transcoder_texture_format::cTFRGB565:
		case basist::transcoder_texture_format::cTFBGR565:
		case basist::transcoder_texture_format::cTFRGBA4444:
		case basist::transcoder_texture_format::cTFBC7_ALT:
		case basist::transcoder_texture_format::cTFTotalTextureFormats:
			ror::log_critical("Format not supported yet!, add support before continuing.");
			return VK_FORMAT_R8G8B8A8_SRGB;
	}

	return VK_FORMAT_B8G8R8A8_SRGB;
}

inline TextureImage read_texture_from_file(const char *a_file_name)
{
	std::filesystem::path file_name{a_file_name};

	TextureImage texture;

	// TODO: Cleanup

	if (file_name.extension() == ".ktx2")
	{
		// FIXME: Should only be called once per execution
		basist::basisu_transcoder_init();

		std::vector<uint8_t> ktx2_file_data;
		if (!utl::align_load_file(file_name, ktx2_file_data))
		{
			ror::log_critical("Failed to load texture file {}", file_name.c_str());
			return texture;
		}

		// Should be done for each transcode
		basist::etc1_global_selector_codebook sel_codebook(basist::g_global_selector_cb_size, basist::g_global_selector_cb);
		basist::ktx2_transcoder               dec(&sel_codebook);

		if (!dec.init(ktx2_file_data.data(), static_cast_safe<uint32_t>(ktx2_file_data.size())))
		{
			ror::log_critical("Basis transcode init failed.");
			return texture;
		}

		if (!dec.start_transcoding())
		{
			ror::log_critical("Basis start_transcoding failed.");
			return texture;
		}

		printf("Resolution: %ux%u\n", dec.get_width(), dec.get_height());
		printf("Mipmap Levels: %u\n", dec.get_levels());
		printf("Texture Array Size (layers): %u\n", dec.get_layers());
		printf("Total Faces: %u (%s)\n", dec.get_faces(), (dec.get_faces() == 6) ? "CUBEMAP" : "2D");
		printf("Is Texture Video: %u\n", dec.is_video());

		const bool is_etc1s = dec.get_format() == basist::basis_tex_format::cETC1S;
		printf("Supercompression Format: %s\n", is_etc1s ? "ETC1S" : "UASTC");

		printf("Supercompression Scheme: ");
		switch (dec.get_header().m_supercompression_scheme)
		{
			case basist::KTX2_SS_NONE:
				printf("NONE\n");
				break;
			case basist::KTX2_SS_BASISLZ:
				printf("BASISLZ\n");
				break;
			case basist::KTX2_SS_ZSTANDARD:
				printf("ZSTANDARD\n");
				break;
			default:
				printf("Invalid/unknown/unsupported\n");
				return texture;
		}

		printf("Has Alpha: %u\n", static_cast<uint32_t>(dec.get_has_alpha()));
		printf("Levels:\n");

		for (uint32_t i = 0; i < dec.get_levels(); i++)
		{
			printf("%u. Offset: %llu, Length: %llu, Uncompressed Length: %llu\n",
				   i, static_cast<long long unsigned int>(dec.get_level_index()[i].m_byte_offset),
				   static_cast<long long unsigned int>(dec.get_level_index()[i].m_byte_length),
				   static_cast<long long unsigned int>(dec.get_level_index()[i].m_uncompressed_byte_length));
		}

		basist::transcoder_texture_format tex_fmt = basist::transcoder_texture_format::cTFBC7_RGBA;
		// basist::transcoder_texture_format tex_fmt = basist::transcoder_texture_format::cTFRGBA32;

		// Use this for your format
		bool compressed = !basist::basis_transcoder_format_is_uncompressed(tex_fmt);

		if (!basis_is_format_supported(tex_fmt, dec.get_format()))
		{
			//Error not supported transcoder format
			ror::log_critical("Requested transcoded format not supported {}", basis_to_vk_format(tex_fmt));
			return texture;
		}

		const uint32_t total_layers       = std::max(1u, dec.get_layers());
		uint64_t       mips_size          = 0;
		uint64_t       decoded_block_size = basisu::get_qwords_per_block(basist::basis_get_basisu_texture_format(tex_fmt)) * sizeof(uint64_t);

		for (uint32_t level_index = 0; level_index < dec.get_levels(); level_index++)
		{
			for (uint32_t layer_index = 0; layer_index < total_layers; layer_index++)
			{
				for (uint32_t face_index = 0; face_index < dec.get_faces(); face_index++)
				{
					basist::ktx2_image_level_info level_info;

					if (!dec.get_image_level_info(level_info, level_index, layer_index, face_index))
					{
						printf("Failed retrieving image level information (%u %u %u)!\n", layer_index, level_index, face_index);
						return texture;
					}

					if (compressed)
						mips_size += decoded_block_size * level_info.m_total_blocks;
					else
						mips_size += level_info.m_orig_height * level_info.m_orig_width * 4;        // FIXME: Only works for RGBA32 uncompressed format
				}
			}
		}

		texture.allocate(mips_size);
		texture.m_format = basis_to_vk_format(tex_fmt);

		uint8_t *decoded_data = texture.m_data.data();
		uint64_t mip_offset   = 0;

		for (uint32_t level_index = 0; level_index < dec.get_levels(); level_index++)
		{
			for (uint32_t layer_index = 0; layer_index < total_layers; layer_index++)
			{
				for (uint32_t face_index = 0; face_index < dec.get_faces(); face_index++)
				{
					basist::ktx2_image_level_info level_info;

					if (!dec.get_image_level_info(level_info, level_index, layer_index, face_index))
					{
						ror::log_critical("Failed retrieving image level information {}, {}, {}", layer_index, level_index, face_index);
						return texture;
					}

					uint32_t decode_flags = 0;
					uint64_t decode_size  = level_info.m_orig_height * level_info.m_orig_width * 4;        // FIXME: Only works for RGBA

					if (compressed)
						decode_size = decoded_block_size * level_info.m_total_blocks;

					if (!dec.transcode_image_level(level_index, layer_index, face_index, decoded_data + mip_offset, (compressed ? level_info.m_total_blocks : static_cast<uint32_t>(decode_size)), tex_fmt, decode_flags))
					{
						ror::log_critical("Failed transcoding image level {}, {}, {}, {}", layer_index, level_index, face_index, tex_fmt);
						return texture;
					}

					TextureImage::Mipmap mip;
					mip.m_width  = level_info.m_orig_width;
					mip.m_height = level_info.m_orig_height;
					mip.m_offset = mip_offset;

					texture.m_mips.emplace_back(mip);

					if (cfg::get_visualise_mipmaps())
					{
						const std::vector<ror::Vector3f> colors{{1.0f, 0.0f, 0.0f},
																{0.0f, 1.0f, 0.0f},
																{0.0f, 0.0f, 1.0f},
																{1.0f, 1.0f, 0.0f},
																{1.0f, 0.0f, 1.0f},
																{0.0f, 1.0f, 1.0f},
																{0.0f, 0.0f, 0.0f},
																{1.0f, 1.0f, 1.0f},
																{1.0f, 0.0f, 0.0f},
																{0.0f, 0.0f, 1.0f}};

						for (size_t i = 0; i < decode_size; i += 4)        // FIXME: Only works for RGBA
						{
							uint8_t cs[3];

							cs[0] = static_cast_safe<uint8_t>(colors[level_index % 10].x * 255);
							cs[1] = static_cast_safe<uint8_t>(colors[level_index % 10].y * 255);
							cs[2] = static_cast_safe<uint8_t>(colors[level_index % 10].z * 255);

							decoded_data[mip_offset + i + 0] = cs[0];
							decoded_data[mip_offset + i + 1] = cs[1];
							decoded_data[mip_offset + i + 2] = cs[2];
						}
					}

					mip_offset += decode_size;
				}
			}
		}
	}
	else
	{
		read_texture_from_file_cimg(a_file_name, texture);
	}

	return texture;
}

}        // namespace utl
