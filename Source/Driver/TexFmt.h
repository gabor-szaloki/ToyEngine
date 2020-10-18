#pragma once

// Mirror of a part of DXGI_FORMAT enum from dxgiformat.h

enum class TexFmt
{
	UNKNOWN	                                = 0,
	R32G32B32A32_TYPELESS                   = 1,
	R32G32B32A32_FLOAT                      = 2,
	R32G32B32A32_UINT                       = 3,
	R32G32B32A32_SINT                       = 4,
	R32G32B32_TYPELESS                      = 5,
	R32G32B32_FLOAT                         = 6,
	R32G32B32_UINT                          = 7,
	R32G32B32_SINT                          = 8,
	R16G16B16A16_TYPELESS                   = 9,
	R16G16B16A16_FLOAT                      = 10,
	R16G16B16A16_UNORM                      = 11,
	R16G16B16A16_UINT                       = 12,
	R16G16B16A16_SNORM                      = 13,
	R16G16B16A16_SINT                       = 14,
	R32G32_TYPELESS                         = 15,
	R32G32_FLOAT                            = 16,
	R32G32_UINT                             = 17,
	R32G32_SINT                             = 18,
	R32G8X24_TYPELESS                       = 19,
	D32_FLOAT_S8X24_UINT                    = 20,
	R32_FLOAT_X8X24_TYPELESS                = 21,
	X32_TYPELESS_G8X24_UINT                 = 22,
	R10G10B10A2_TYPELESS                    = 23,
	R10G10B10A2_UNORM                       = 24,
	R10G10B10A2_UINT                        = 25,
	R11G11B10_FLOAT                         = 26,
	R8G8B8A8_TYPELESS                       = 27,
	R8G8B8A8_UNORM                          = 28,
	R8G8B8A8_UNORM_SRGB                     = 29,
	R8G8B8A8_UINT                           = 30,
	R8G8B8A8_SNORM                          = 31,
	R8G8B8A8_SINT                           = 32,
	R16G16_TYPELESS                         = 33,
	R16G16_FLOAT                            = 34,
	R16G16_UNORM                            = 35,
	R16G16_UINT                             = 36,
	R16G16_SNORM                            = 37,
	R16G16_SINT                             = 38,
	R32_TYPELESS                            = 39,
	D32_FLOAT                               = 40,
	R32_FLOAT                               = 41,
	R32_UINT                                = 42,
	R32_SINT                                = 43,
	R24G8_TYPELESS                          = 44,
	D24_UNORM_S8_UINT                       = 45,
	R24_UNORM_X8_TYPELESS                   = 46,
	X24_TYPELESS_G8_UINT                    = 47,
	R8G8_TYPELESS                           = 48,
	R8G8_UNORM                              = 49,
	R8G8_UINT                               = 50,
	R8G8_SNORM                              = 51,
	R8G8_SINT                               = 52,
	R16_TYPELESS                            = 53,
	R16_FLOAT                               = 54,
	D16_UNORM                               = 55,
	R16_UNORM                               = 56,
	R16_UINT                                = 57,
	R16_SNORM                               = 58,
	R16_SINT                                = 59,
	R8_TYPELESS                             = 60,
	R8_UNORM                                = 61,
	R8_UINT                                 = 62,
	R8_SNORM                                = 63,
	R8_SINT                                 = 64,
	A8_UNORM                                = 65,
};

inline unsigned int get_texel_byte_size_for_texfmt(TexFmt fmt)
{
	switch (fmt)
	{
	case TexFmt::R32G32B32A32_TYPELESS:
	case TexFmt::R32G32B32A32_FLOAT:
	case TexFmt::R32G32B32A32_UINT:
	case TexFmt::R32G32B32A32_SINT:
		return 16;

	case TexFmt::R32G32B32_TYPELESS:
	case TexFmt::R32G32B32_FLOAT:
	case TexFmt::R32G32B32_UINT:
	case TexFmt::R32G32B32_SINT:
		return 12;

	case TexFmt::R16G16B16A16_TYPELESS:
	case TexFmt::R16G16B16A16_FLOAT:
	case TexFmt::R16G16B16A16_UNORM:
	case TexFmt::R16G16B16A16_UINT:
	case TexFmt::R16G16B16A16_SNORM:
	case TexFmt::R16G16B16A16_SINT:
	case TexFmt::R32G32_TYPELESS:
	case TexFmt::R32G32_FLOAT:
	case TexFmt::R32G32_UINT:
	case TexFmt::R32G32_SINT:
	case TexFmt::R32G8X24_TYPELESS:
	case TexFmt::D32_FLOAT_S8X24_UINT:
	case TexFmt::R32_FLOAT_X8X24_TYPELESS:
	case TexFmt::X32_TYPELESS_G8X24_UINT:
		return 8;

	case TexFmt::R10G10B10A2_TYPELESS:
	case TexFmt::R10G10B10A2_UNORM:
	case TexFmt::R10G10B10A2_UINT:
	case TexFmt::R11G11B10_FLOAT:
	case TexFmt::R8G8B8A8_TYPELESS:
	case TexFmt::R8G8B8A8_UNORM:
	case TexFmt::R8G8B8A8_UNORM_SRGB:
	case TexFmt::R8G8B8A8_UINT:
	case TexFmt::R8G8B8A8_SNORM:
	case TexFmt::R8G8B8A8_SINT:
	case TexFmt::R16G16_TYPELESS:
	case TexFmt::R16G16_FLOAT:
	case TexFmt::R16G16_UNORM:
	case TexFmt::R16G16_UINT:
	case TexFmt::R16G16_SNORM:
	case TexFmt::R16G16_SINT:
	case TexFmt::R32_TYPELESS:
	case TexFmt::D32_FLOAT:
	case TexFmt::R32_FLOAT:
	case TexFmt::R32_UINT:
	case TexFmt::R32_SINT:
	case TexFmt::R24G8_TYPELESS:
	case TexFmt::D24_UNORM_S8_UINT:
	case TexFmt::R24_UNORM_X8_TYPELESS:
	case TexFmt::X24_TYPELESS_G8_UINT:
		return 4;

	case TexFmt::R8G8_TYPELESS:
	case TexFmt::R8G8_UNORM:
	case TexFmt::R8G8_UINT:
	case TexFmt::R8G8_SNORM:
	case TexFmt::R8G8_SINT:
	case TexFmt::R16_TYPELESS:
	case TexFmt::R16_FLOAT:
	case TexFmt::D16_UNORM:
	case TexFmt::R16_UNORM:
	case TexFmt::R16_UINT:
	case TexFmt::R16_SNORM:
	case TexFmt::R16_SINT:
		return 2;

	case TexFmt::R8_TYPELESS:
	case TexFmt::R8_UNORM:
	case TexFmt::R8_UINT:
	case TexFmt::R8_SNORM:
	case TexFmt::R8_SINT:
	case TexFmt::A8_UNORM:
		return 1;

	default:
		return 0;
	}
}