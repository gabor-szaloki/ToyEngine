#pragma once

#include <Driver/IDriver.h>

#include "DriverCommonD3D12.h"
#include "CommandQueue.h"

#include <3rdParty/DirectX-Headers/d3dx12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#include <string>
#include <memory>
#include <unordered_map>

#define ASSERT_NOT_IMPLEMENTED assert(false && "drv_d3d12: Not implemented.")

namespace drv_d3d12
{
	class DriverD3D12 : public IDriver
	{
	public:
		static constexpr int NUM_SWACHAIN_BUFFERS = 2;
		static constexpr DXGI_FORMAT SWAPCHAIN_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

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
		void update(float delta_time) override;
		void beginRender() override;
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
		void flush();
		void initDefaultPipelineStates();
		bool initResolutionDependentResources();
		void closeResolutionDependentResources();
		void enableDebugLayer();
		void updateBackbufferRTVs();
		void transitionResource(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
		void updateBufferResource(ID3D12GraphicsCommandList2* cmdList, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
		ID3D12PipelineState* getOrCreateCurrentGraphicsPipeline();

		//
		// Stuff for the cube demo
		// 
		// Vertex buffer for the cube.
		ComPtr<ID3D12Resource> m_VertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
		// Index buffer for the cube.
		ComPtr<ID3D12Resource> m_IndexBuffer;
		D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
		// Depth buffer.
		ComPtr<ID3D12Resource> m_DepthBuffer;
		// Root signature
		ComPtr<ID3D12RootSignature> m_RootSignature;
		// Shaders
		ComPtr<ID3DBlob> m_VertexShaderBlob;
		ComPtr<ID3DBlob> m_PixelShaderBlob;
		float m_FoV = 45.0f;
		XMMATRIX m_ModelMatrix;
		XMMATRIX m_ViewMatrix;
		XMMATRIX m_ProjectionMatrix;
		bool loadDemoContent();
		void destroyDemoContent();
		//

		int displayWidth = -1, displayHeight = -1;
		uint currentBackBufferIndex = 0;

		ViewportParams currentViewport;
		DriverSettings settings;

		HWND hWnd;

		ComPtr<ID3D12Device2> device;

		std::unique_ptr<CommandQueue> directCommandQueue;
		std::unique_ptr<CommandQueue> computeCommandQueue;
		std::unique_ptr<CommandQueue> copyCommandQueue;

		ComPtr<ID3D12GraphicsCommandList2> commandList;

		ComPtr<IDXGISwapChain4> swapchain;
		ComPtr<ID3D12Resource> backbuffers[NUM_SWACHAIN_BUFFERS]; // TODO: Make these our Texture type
		uint64 frameFenceValues[NUM_SWACHAIN_BUFFERS] = {};

		ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
		uint rtvDescriptorSize = 0;
		static constexpr uint NUM_RTV_DESCRIPTORS = 100;
		ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
		uint dsvDescriptorSize = 0;
		static constexpr uint NUM_DSV_DESCRIPTORS = 100;
		ComPtr<ID3D12DescriptorHeap> cbvSrvUavDescriptorHeap;
		uint cbvSrvUavDescriptorSize = 0;
		static constexpr uint NUM_CBV_SRV_UAV_DESCRIPTORS = 100;

		struct GraphicsPipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_GS gs;
			CD3DX12_PIPELINE_STATE_STREAM_HS hs;
			CD3DX12_PIPELINE_STATE_STREAM_DS ds;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blendState;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 depthStencilState;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT depthStencilFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizerState;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS renderTargetFormats;

			uint64 hash() const;
		};

		GraphicsPipelineStateStream currentGraphicsPipelineState;

		std::unordered_map<uint64, ID3D12PipelineState*> psoHashMap;

		ComPtr<IDXGIDebug> dxgiDebug = nullptr;
	};

	inline void set_debug_name(ID3D12DeviceChild* child, const std::string& name)
	{
		if (child != nullptr)
			child->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
	}
}