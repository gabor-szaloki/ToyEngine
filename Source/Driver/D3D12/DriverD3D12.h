#pragma once

#define NOMINMAX
#include <d3d12.h>
#include <3rdParty/DirectX-Headers/d3dx12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <wrl.h>
//#pragma comment(lib, "runtimeobject.lib")
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#include <string>

#include <Driver/IDriver.h>

#define SAFE_RELEASE(resource) { if (resource != nullptr) { resource->Release(); resource = nullptr; } }
#define ASSERT_NOT_IMPLEMENTED assert(false && "drv_d3d12: Not implemented.")

namespace drv_d3d12
{
	class DriverD3D12 : public IDriver
	{
	public:
		static constexpr int NUM_SWACHAIN_BUFFERS = 2;

		static DriverD3D12& get();

		bool init(void* hwnd, int display_width, int display_height) override;
		void shutdown() override;
		void resize(int display_width, int display_height) override;
		void getDisplaySize(int& display_width, int& display_height) override;

		ITexture* getBackbufferTexture() override { return (ITexture*)nullptr; };
		ITexture* createTexture(const TextureDesc& desc) override;
		ITexture* createTextureStub() override;
		IBuffer* createBuffer(const BufferDesc& desc) override;
		ResId createSampler(const SamplerDesc& desc) override;
		ResId createRenderState(const RenderStateDesc& desc) override;
		ResId createShaderSet(const ShaderSetDesc& desc) override;
		ResId createComputeShader(const ComputeShaderDesc& desc) override;
		ResId createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId shader_set) override;
		void destroyResource(ResId res_id) override;

		void setInputLayout(ResId res_id) override;
		void setIndexBuffer(ResId res_id) override;
		void setVertexBuffer(ResId res_id) override;
		void setConstantBuffer(ShaderStage stage, unsigned int slot, ResId res_id) override;
		void setBuffer(ShaderStage stage, unsigned int slot, ResId res_id) override;
		void setRwBuffer(unsigned int slot, ResId res_id) override;
		void setTexture(ShaderStage stage, unsigned int slot, ResId res_id) override;
		void setRwTexture(unsigned int slot, ResId res_id) override;
		void setSampler(ShaderStage stage, unsigned int slot, ResId res_id) override;
		void setRenderTarget(ResId target_id, ResId depth_id,
			unsigned int target_slice = 0, unsigned int depth_slice = 0, unsigned int target_mip = 0, unsigned int depth_mip = 0) override;
		void setRenderTargets(unsigned int num_targets, ResId* target_ids, ResId depth_id,
			unsigned int* target_slices = nullptr, unsigned int depth_slice = 0, unsigned int* target_mips = nullptr, unsigned int depth_mip = 0) override;
		void setRenderState(ResId res_id) override;
		void setShader(ResId res_id, unsigned int variant_index) override;
		void setView(float x, float y, float w, float h, float z_min, float z_max) override;
		void setView(const ViewportParams& vp) override;
		void getView(ViewportParams& vp) override;

		void draw(unsigned int vertex_count, unsigned int start_vertex) override;
		void drawIndexed(unsigned int index_count, unsigned int start_index, int base_vertex) override;
		void dispatch(unsigned int num_threadgroups_x, unsigned int num_threadgroups_y, unsigned int num_threadgroups_z) override;

		void clearRenderTargets(const RenderTargetClearParams clear_params) override;

		void beginFrame() override;
		void endFrame() override;
		void present() override;

		void beginEvent(const char* label) override;
		void endEvent() override;

		unsigned int getShaderVariantIndexForKeywords(ResId shader_res_id, const char** keywords, unsigned int num_keywords) override;
		TexFmt getIndexFormat() override { return TexFmt::R32_UINT; }
		const DriverSettings& getSettings() override { return settings; };
		void setSettings(const DriverSettings& new_settings) override;
		void recompileShaders() override;
		void setErrorShaderDesc(const ShaderSetDesc& desc) override;
		ResId getErrorShader() override;

	private:
		bool initResolutionDependentResources();
		void closeResolutionDependentResources();
		void enableDebugLayer();
		void updateBackbufferRTVs();

		int displayWidth = -1, displayHeight = -1;
		uint currentBackBufferIndex = 0;

		ViewportParams currentViewport;
		DriverSettings settings;

		HWND hWnd;

		ComPtr<ID3D12Device2> device;
		ComPtr<ID3D12CommandQueue> commandQueue;
		ComPtr<IDXGISwapChain4> swapchain;
		ComPtr<ID3D12Resource> backbuffers[NUM_SWACHAIN_BUFFERS]; // TODO: Make these our Texture type
		ComPtr<ID3D12GraphicsCommandList> commandList;
		ComPtr<ID3D12CommandAllocator> commandAllocators[NUM_SWACHAIN_BUFFERS];
		ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
		uint rtvDescriptorSize = 0;

		// Synchronization objects
		ComPtr<ID3D12Fence> fence;
		uint64 fenceValue = 0;
		uint64 frameFenceValues[NUM_SWACHAIN_BUFFERS] = {};
		HANDLE fenceEvent = nullptr;

		ComPtr<IDXGIDebug> dxgiDebug = nullptr;
	};

	inline void set_debug_name(ID3D12DeviceChild* child, const std::string& name)
	{
		if (child != nullptr)
			child->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
	}

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw std::exception();
	}
}