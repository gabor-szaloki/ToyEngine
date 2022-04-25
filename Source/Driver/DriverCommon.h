#pragma once

#include <float.h>
#include <string>
#include <cmath>
#include <Common.h>
#include "TexFmt.h"
#include "DriverConsts.h"
#include "DriverSettings.h"

typedef int ResId;
constexpr ResId BAD_RESID = -1;

inline unsigned int calc_mip_levels(unsigned int tex_width, unsigned int tex_height = 0)
{
	unsigned int maxDim = tex_width < tex_height ? tex_height : tex_width;
	return static_cast<unsigned int>(std::floor(std::log2(maxDim))) + 1;
}

inline unsigned int calc_subresource(unsigned int mip_slice, unsigned int array_slice, unsigned int mip_levels)
{
	return mip_slice + (array_slice * mip_levels);
}

struct IntBox // Todo: move outside of Driver
{
	int left;
	int top;
	int front;
	int right;
	int bottom;
	int back;
};

enum class ShaderStage
{
	VS = 0,
	PS = 1,
	GS = 2,
	HS = 3,
	DS = 4,
	CS = 5,
	GRAPHICS_STAGE_COUNT = 5,
};

struct SamplerDesc
{
	FilterMode filter = FILTER_DEFAULT;
	TexAddr addressU = TexAddr::WRAP;
	TexAddr addressV = TexAddr::WRAP;
	TexAddr addressW = TexAddr::WRAP;
	float mipBias = 0.0f;
	unsigned int maxAnisotropy = 16;
	ComparisonFunc comparisonFunc = ComparisonFunc::ALWAYS;
	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float minLOD = 0;
	float maxLOD = FLT_MAX;

	SamplerDesc(
		FilterMode filter_ = FILTER_DEFAULT, TexAddr address_mode = TexAddr::WRAP,
		ComparisonFunc comparison_func = ComparisonFunc::ALWAYS) :
		filter(filter_), addressU(address_mode), addressV(address_mode), addressW(address_mode),
		comparisonFunc(comparison_func) {}
	void setAddressMode(TexAddr mode) { addressU = addressV = addressW = mode; }
};

struct TextureDesc
{
	std::string name = "unnamedtex";
	unsigned int width = 0;
	unsigned int height = 0;
	TexFmt format = TexFmt::R8G8B8A8_UNORM;
	TexFmt srvFormatOverride = TexFmt::INVALID;
	TexFmt uavFormatOverride = TexFmt::INVALID;
	TexFmt rtvFormatOverride = TexFmt::INVALID;
	TexFmt dsvFormatOverride = TexFmt::INVALID;
	unsigned int mips = 1;
	ResourceUsage usageFlags = ResourceUsage::DEFAULT;
	unsigned int bindFlags = 0;
	unsigned int cpuAccessFlags = 0;
	unsigned int miscFlags = 0;

	TextureDesc() {};
	TextureDesc(
		const std::string& name_, unsigned int width_, unsigned int height_,
		TexFmt format_ = TexFmt::R8G8B8A8_UNORM, unsigned int mips_ = 1,
		ResourceUsage usage_flags = ResourceUsage::DEFAULT,
		unsigned int bind_flags = 0, unsigned int cpu_access_flags = 0, unsigned int misc_flags = 0) :
		name(name_), width(width_), height(height_), format(format_), mips(mips_), usageFlags(usage_flags),
		bindFlags(bind_flags), cpuAccessFlags(cpu_access_flags), miscFlags(misc_flags) {}

	unsigned int calcMipLevels() const { return mips > 0 ? mips : calc_mip_levels(width, height); }
	TexFmt getSrvFormat() const { return srvFormatOverride != TexFmt::INVALID ? srvFormatOverride : format; }
	TexFmt getUavFormat() const { return uavFormatOverride != TexFmt::INVALID ? uavFormatOverride : format; }
	TexFmt getRtvFormat() const { return rtvFormatOverride != TexFmt::INVALID ? rtvFormatOverride : format; }
	TexFmt getDsvFormat() const { return dsvFormatOverride != TexFmt::INVALID ? dsvFormatOverride : format; }
};

struct BufferDesc
{
	std::string name = "unnamedbuf";
	unsigned int elementByteSize = 0;
	unsigned int numElements = 0;
	ResourceUsage usageFlags = ResourceUsage::DEFAULT;
	unsigned int bindFlags = 0;
	unsigned int cpuAccessFlags = 0;
	unsigned int miscFlags = 0;
	void* initialData = nullptr;

	BufferDesc() {};
	BufferDesc(
		const std::string& name_, unsigned int element_byte_size, unsigned int num_elements,
		ResourceUsage usage_flags = ResourceUsage::DEFAULT,
		unsigned int bind_flags = 0, unsigned int cpu_access_flags = 0, unsigned int misc_flags = 0) :
		name(name_), elementByteSize(element_byte_size), numElements(num_elements), usageFlags(usage_flags),
		bindFlags(bind_flags), cpuAccessFlags(cpu_access_flags), miscFlags(misc_flags) {};
};

struct RasterizerDesc
{
	bool wireframe = false;
	CullMode cullMode = CullMode::BACK;
	int depthBias = 0;
	float slopeScaledDepthBias = 0;
	bool depthClipEnable = true;
};

struct DepthStencilDesc
{
	bool depthTest = true;
	bool depthWrite = true;
	ComparisonFunc DepthFunc = ComparisonFunc::LESS_EQUAL;
};

struct RenderStateDesc
{
	RasterizerDesc rasterizerDesc;
	DepthStencilDesc depthStencilDesc;
};

struct ShaderSetDesc
{
	std::string name;
	std::string sourceFilePath;
	std::string shaderFuncNames[(int)ShaderStage::GRAPHICS_STAGE_COUNT] = {};

	ShaderSetDesc() {}
	ShaderSetDesc(const std::string& name_, const std::string& source_file_path) : name(name_), sourceFilePath(source_file_path) {}
};

struct ComputeShaderDesc
{
	std::string name;
	std::string sourceFilePath;
	std::string shaderFuncName;

	ComputeShaderDesc() {}
	ComputeShaderDesc(const std::string& name_, const std::string& source_file_path, const std::string& shader_func_name) :
		name(name_), sourceFilePath(source_file_path), shaderFuncName(shader_func_name) {}
};

struct InputLayoutElementDesc
{
	VertexInputSemantic semantic;
	unsigned int semanticIndex;
	TexFmt format;
};

struct ViewportParams
{
	float x, y, w, h, z_min, z_max;
};

struct RenderTargetClearParams
{
	unsigned int clearFlags = CLEAR_FLAG_ALL;
	unsigned char colorTargetMask = 0xFF;
	float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float depth = 1.0f;
	unsigned char stencil = 0;

	RenderTargetClearParams(
		unsigned int clear_flags = CLEAR_FLAG_ALL, unsigned char color_target_mask = 0xFF,
		float depth_ = 1.0f, unsigned char stencil_ = 0) :
		clearFlags(clear_flags), colorTargetMask(color_target_mask), depth(depth_), stencil(stencil_) {}

	static inline RenderTargetClearParams clear_color(float color_r, float color_g, float color_b, float color_a)
	{
		RenderTargetClearParams p(CLEAR_FLAG_COLOR);
		p.color[0] = color_r;
		p.color[1] = color_g;
		p.color[2] = color_b;
		p.color[3] = color_a;
		return p;
	}

	static inline RenderTargetClearParams clear_color(XMFLOAT4 color)
	{
		return clear_color(color.x, color.y, color.z, color.w);
	}

	static inline RenderTargetClearParams clear_depth(float depth)
	{
		RenderTargetClearParams p(CLEAR_FLAG_DEPTH);
		p.depth = depth;
		return p;
	}

	static inline RenderTargetClearParams clear_all(float color_r, float color_g, float color_b, float color_a, float depth)
	{
		RenderTargetClearParams p(CLEAR_FLAG_ALL);
		p.color[0] = color_r;
		p.color[1] = color_g;
		p.color[2] = color_b;
		p.color[3] = color_a;
		p.depth = depth;
		return p;
	}
};