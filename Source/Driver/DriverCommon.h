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
	FilterMode filter = FilterMode::FILTER_DEFAULT;
	TexAddr addressU = TexAddr::WRAP;
	TexAddr addressV = TexAddr::WRAP;
	TexAddr addressW = TexAddr::WRAP;
	float mipBias = 0.0f;
	unsigned int maxAnisotropy = 16;
	ComparisonFunc comparisonFunc = ComparisonFunc::ALWAYS;
	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float minLOD = 0;
	float maxLOD = FLT_MAX;
};

struct TextureDesc
{
	const char* name = "unnamedtex";
	unsigned int width = 0;
	unsigned int height = 0;
	TexFmt format = TexFmt::R8G8B8A8_UNORM;
	unsigned int mips = 1;
	ResourceUsage usageFlags = ResourceUsage::DEFAULT;
	unsigned int bindFlags = 0;
	unsigned int cpuAccessFlags = 0;
	unsigned int miscFlags = 0;
	SamplerDesc samplerDesc;
	bool hasSampler = true;
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