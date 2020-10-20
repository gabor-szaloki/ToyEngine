#pragma once

#include <float.h>
#include "TexFmt.h"
#include "DriverConsts.h"
#include "DriverSettings.h"

typedef int ResId;
#define BAD_RESID -1;

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
	const char* name = "unnamedtex";
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
	bool hasSampler = true;
	SamplerDesc samplerDesc;

	TextureDesc() {};
	TextureDesc(
		const char* name_, unsigned int width_, unsigned int height_,
		TexFmt format_ = TexFmt::R8G8B8A8_UNORM, unsigned int mips_ = 1,
		ResourceUsage usage_flags = ResourceUsage::DEFAULT,
		unsigned int bind_flags = 0, unsigned int cpu_access_flags = 0, unsigned int misc_flags = 0, bool has_sampler = true) :
		name(name_), width(width_), height(height_), format(format_), usageFlags(usage_flags),
		bindFlags(bind_flags), cpuAccessFlags(cpu_access_flags), miscFlags(misc_flags), hasSampler(has_sampler) {}
};

struct BufferDesc
{
	const char* name = "unnamedbuf";
	unsigned int elementByteSize = 0;
	unsigned int numElements = 0;
	ResourceUsage usageFlags = ResourceUsage::DEFAULT;
	unsigned int bindFlags = 0;
	unsigned int cpuAccessFlags = 0;
	unsigned int miscFlags = 0;

	BufferDesc() {};
	BufferDesc(
		const char* name_, unsigned int element_byte_size, unsigned int num_elements,
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
	bool scissorEnable = false;
};

struct DepthStencilDesc
{
	bool depthTest = true;
	bool depthWrite = true;
	ComparisonFunc DepthFunc = ComparisonFunc::LESS;
};

struct RenderStateDesc
{
	RasterizerDesc rasterizerDesc;
	DepthStencilDesc depthStencilDesc;
};

struct ShaderSetDesc
{
	const char* sourceFilePath = nullptr;
	const char* shaderFuncNames[(int)ShaderStage::GRAPHICS_STAGE_COUNT] = {};
	
	ShaderSetDesc(const char* source_file_path = nullptr) : sourceFilePath(source_file_path) {}
};

struct ComputeShaderDesc
{
	const char* sourceFilePath = nullptr;
	const char* shaderFuncName = nullptr;
};

struct InputLayoutElementDesc
{
	VertexInputSemantic semantic;
	unsigned int semanticIndex;
	TexFmt format;
};