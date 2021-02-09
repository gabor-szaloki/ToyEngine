#pragma once

#include <d3d11.h>
#include <d3d11_1.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment( lib, "dxguid.lib")
#include <dxgidebug.h>

#include <memory>
#include <map>
#include <array>
#include <mutex>

#include <Driver/IDriver.h>

#define SAFE_RELEASE(resource) { if (resource != nullptr) { resource->Release(); resource = nullptr; } }
#define CONTEXT_LOCK_GUARD const std::scoped_lock<std::mutex> contextLock(Driver::get().getContextMutex());
#define RESOURCE_LOCK_GUARD const std::scoped_lock<std::mutex> resourceLock(Driver::get().getResourceMutex());

namespace drv_d3d11
{
	class Texture;
	class Buffer;
	class RenderState;
	class ShaderSet;
	class InputLayout;

	class Driver : public IDriver
	{
	public:
		static Driver& get();

		bool init(void* hwnd, int display_width, int display_height) override;
		void shutdown() override;
		void resize(int display_width, int display_height) override;
		void getDisplaySize(int& display_width, int& display_height) override;

		ITexture* getBackbufferTexture() override { return (ITexture*)backbuffer.get(); };
		ITexture* createTexture(const TextureDesc& desc) override;
		ITexture* createTextureStub() override;
		IBuffer* createBuffer(const BufferDesc& desc) override;
		ResId createRenderState(const RenderStateDesc& desc) override;
		ResId createShaderSet(const ShaderSetDesc& desc) override;
		ResId createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId shader_set) override;
		void destroyResource(ResId res_id) override;

		void setInputLayout(ResId res_id) override;
		void setIndexBuffer(ResId res_id) override;
		void setVertexBuffer(ResId res_id) override;
		void setConstantBuffer(ShaderStage stage, unsigned int slot, ResId res_id) override;
		void setBuffer(ShaderStage stage, unsigned int slot, ResId res_id) override;
		void setRwBuffer(unsigned int slot, ResId res_id) override;
		void setTexture(ShaderStage stage, unsigned int slot, ResId res_id, bool set_sampler_too) override;
		void setRwTexture(unsigned int slot, ResId res_id) override;
		void setRenderTarget(ResId target_id, ResId depth_id) override;
		void setRenderTargets(unsigned int num_targets, ResId* target_ids, ResId depth_id) override;
		void setRenderState(ResId res_id) override;
		void setShaderSet(ResId res_id) override;
		void setView(float x, float y, float w, float h, float z_min, float z_max) override;

		void draw(unsigned int vertex_count, unsigned int start_vertex) override;
		void drawIndexed(unsigned int index_count, unsigned int start_index, int base_vertex) override;

		void clearRenderTargets(const RenderTargetClearParams clear_params) override;

		void beginFrame() override;
		void endFrame() override;
		void present() override;

		void beginEvent(const char* label) override;
		void endEvent() override;

		TexFmt getIndexFormat() override { return TexFmt::R16_UINT; }
		const DriverSettings& getSettings() override { return settings; };
		void setSettings(const DriverSettings& new_settings) override;
		void recompileShaders() override;
		void setErrorShaderDesc(const ShaderSetDesc& desc) override;
		ResId getErrorShader() override;

		ID3D11Device& getDevice() { return *device; }
		ID3D11DeviceContext& getContext() { return *context; }
		std::mutex& getContextMutex() { return contextMutex; }
		std::mutex& getResourceMutex() { return resourceMutex; }

		ResId registerTexture(Texture* tex);
		void unregisterTexture(ResId id);
		ResId registerBuffer(Buffer* buf);
		void unregisterBuffer(ResId id);
		ResId registerRenderState(RenderState* rs);
		void unregisterRenderState(ResId id);
		ResId registerShaderSet(ShaderSet* shader_set);
		void unregisterShaderSet(ResId id);
		ResId registerInputLayout(InputLayout* input_layout);
		void unregisterInputLayout(ResId id);

	private:
		bool initResolutionDependentResources(int display_width, int display_height);
		void closeResolutionDependentResources();
		void releaseAllResources();

		int displayWidth = -1, displayHeight = -1;
		DriverSettings settings;

		HWND hWnd;

		ID3D11Device* device;
		ID3D11DeviceContext* context;
		IDXGISwapChain* swapchain;
		ID3DUserDefinedAnnotation* perf;
		ID3D11Debug* deviceDebug = nullptr;
		IDXGIInfoQueue* dxgiInfoQueue = nullptr;
		IDXGIDebug* dxgiDebug = nullptr;
		std::unique_ptr<Texture> backbuffer;

		std::mutex contextMutex;
		std::mutex resourceMutex;

		std::map<ResId, Texture*> textures;
		std::map<ResId, Buffer*> buffers;
		std::map<ResId, RenderState*> renderStates;
		std::map<ResId, ShaderSet*> shaders;
		std::map<ResId, InputLayout*> inputLayouts;

		std::unique_ptr<RenderState> defaultRenderState;
		std::unique_ptr<ShaderSet> errorShader;

		std::array<Texture*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> currentRenderTargets;
		Texture* currentDepthTarget = nullptr;
	};

	inline void set_debug_name(ID3D11DeviceChild* child, const std::string& name)
	{
		if (child != nullptr)
			child->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
	}
}