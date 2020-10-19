#pragma once

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <memory>
#include <map>
#include <wrl/client.h>

#include <Driver/IDriver.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class D3D11Texture;
class D3D11Buffer;
class D3D11RenderState;
class D3D11ShaderSet;
class D3D11InputLayout;

class D3D11Driver : public IDriver
{
public:
	bool init(void* hwnd, int display_width, int display_height) override;
	void shutdown() override;
	void resize(int display_width, int display_height) override;

	ITexture* createTexture(const TextureDesc& desc) override;
	IBuffer* createBuffer(const BufferDesc& desc) override;
	ResId createRenderState(const RenderStateDesc& desc) override;
	ResId createShaderSet(const ShaderSetDesc& desc) override;
	ResId createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId shader_set) override;

	void setIndexBuffer(ResId* res_id) override;
	void setVertexBuffer(ResId* res_id) override;
	void setConstantBuffer(ShaderStage stage, unsigned int slot, ResId* res_id) override;
	void setBuffer(ShaderStage stage, unsigned int slot, ResId* res_id) override;
	void setRwBuffer(unsigned int slot, ResId* res_id) override;
	void setTexture(ShaderStage stage, unsigned int slot, ResId* res_id) override;
	void setRwTexture(unsigned int slot, ResId* res_id) override;
	void setRenderTarget(ResId* target_id, ResId* depth_id) override;
	void setRenderTargets(unsigned int num_targets, ResId** target_ids, ResId* depth_id) override;
	void setRenderState(ResId* res_id) override;

	const DriverSettings& getSettings() override { return settings; };
	void setSettings(const DriverSettings& new_settings) override;

	ID3D11Device* getDevice() { return device.Get(); }
	ID3D11DeviceContext* getContext() { return context.Get(); }

	ResId registerTexture(D3D11Texture* tex);
	void unregisterTexture(ResId id);
	ResId registerBuffer(D3D11Buffer* buf);
	void unregisterBuffer(ResId id);
	ResId registerRenderState(D3D11RenderState* rs);
	void unregisterRenderState(ResId id);
	ResId registerShaderSet(D3D11ShaderSet* shader_set);
	void unregisterShaderSet(ResId id);
	ResId registerInputLayout(D3D11InputLayout* input_layout);
	void unregisterInputLayout(ResId id);

private:
	bool initResolutionDependentResources(int display_width, int display_height);
	void closeResolutionDependentResources();

	DriverSettings settings;

	HWND hWnd;

	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ComPtr<IDXGISwapChain> swapchain;
	std::unique_ptr<D3D11Texture> backbuffer;

	std::map<ResId, D3D11Texture*> textures;
	std::map<ResId, D3D11Buffer*> buffers;
	std::map<ResId, D3D11RenderState*> renderStates;
	std::map<ResId, D3D11ShaderSet*> shaders;
	std::map<ResId, D3D11InputLayout*> inputLayouts;

	ResId defaultRenderState;
};

D3D11Driver* get_drv();
