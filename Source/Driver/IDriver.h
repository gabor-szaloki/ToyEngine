#pragma once

#include "DriverCommon.h"

class ITexture;
class IBuffer;

class IDriver
{
public:
	virtual bool init(void* hwnd, int display_width, int display_height) = 0;
	virtual void shutdown() = 0;
	virtual void resize(int display_width, int display_height) = 0;
	virtual void getDisplaySize(int& display_width, int& display_height) = 0;

	virtual ITexture* getBackbufferTexture() = 0;
	virtual ITexture* createTexture(const TextureDesc& desc) = 0;
	virtual ITexture* createTextureStub() = 0;
	virtual IBuffer* createBuffer(const BufferDesc& desc) = 0;
	virtual ResId createSampler(const SamplerDesc& desc) = 0;
	virtual ResId createRenderState(const RenderStateDesc& desc) = 0;
	virtual ResId createShaderSet(const ShaderSetDesc& desc) = 0;
	virtual ResId createComputeShader(const ComputeShaderDesc& desc) = 0;
	virtual ResId createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId shader_set) = 0;
	virtual void destroyResource(ResId res_id) = 0;

	virtual void setInputLayout(ResId res_id) = 0;
	virtual void setIndexBuffer(ResId res_id) = 0;
	virtual void setVertexBuffer(ResId res_id) = 0;
	virtual void setConstantBuffer(ShaderStage stage, unsigned int slot, ResId res_id) = 0;
	virtual void setBuffer(ShaderStage stage, unsigned int slot, ResId res_id) = 0;
	virtual void setRwBuffer(unsigned int slot, ResId res_id) = 0;
	virtual void setTexture(ShaderStage stage, unsigned int slot, ResId res_id) = 0;
	virtual void setRwTexture(unsigned int slot, ResId res_id) = 0;
	virtual void setSampler(ShaderStage stage, unsigned int slot, ResId res_id) = 0;
	virtual void setRenderTarget(ResId target_id, ResId depth_id,
		unsigned int target_slice = 0, unsigned int depth_slice = 0, unsigned int target_mip = 0, unsigned int depth_mip = 0) = 0;
	virtual void setRenderTargets(unsigned int num_targets, ResId* target_ids, ResId depth_id,
		unsigned int* target_slices = nullptr, unsigned int depth_slice = 0, unsigned int* target_mips = nullptr, unsigned int depth_mip = 0) = 0;
	virtual void setRenderState(ResId res_id) = 0;
	virtual void setShader(ResId res_id, unsigned int variant_index) = 0;
	virtual void setView(float x, float y, float w, float h, float z_min, float z_max) = 0;
	virtual void setView(const ViewportParams& vp) = 0;
	virtual void getView(ViewportParams& vp) = 0;

	virtual void draw(unsigned int vertex_count, unsigned int start_vertex) = 0;
	virtual void drawIndexed(unsigned int index_count, unsigned int start_index, int base_vertex) = 0;
	virtual void dispatch(unsigned int num_threadgroups_x, unsigned int num_threadgroups_y, unsigned int num_threadgroups_z) = 0;

	virtual void clearRenderTargets(const RenderTargetClearParams clear_params) = 0;

	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;
	virtual void present() = 0;

	virtual void beginEvent(const char* label) = 0;
	virtual void endEvent() = 0;

	virtual unsigned int getShaderVariantIndexForKeywords(ResId shader_res_id, const char** keywords, unsigned int num_keywords) = 0;
	virtual TexFmt getIndexFormat() = 0;
	virtual const DriverSettings& getSettings() = 0;
	virtual void setSettings(const DriverSettings& new_settings) = 0;
	virtual void recompileShaders() = 0;
	virtual void setErrorShaderDesc(const ShaderSetDesc& desc) = 0;
	virtual ResId getErrorShader() = 0;
};