#pragma once

#include <array>

#include <Common.h>
#include <Util/CaseSensitiveIni.h>
#include <Util/ResIdHolder.h>
#include <Driver/IDriver.h>

#include "Material.h"

struct MeshData;
class MeshRenderer;

enum class LoadExecutionMode { ASYNC, SYNC };

class AssetManager
{
public:
	AssetManager();
	~AssetManager();

	ITexture* loadTextureFromPng(const std::string& path, bool srgb, LoadExecutionMode lem = LoadExecutionMode::ASYNC);
	bool loadTexturesToStandardMaterial(const MaterialTexturePaths& paths, Material* material, bool flip_normal_green, LoadExecutionMode lem = LoadExecutionMode::ASYNC);
	bool loadMesh(const std::string& name, MeshData& mesh_data);
	bool loadMesh2(const std::string& name, MeshData& mesh_data);
	bool loadMeshToMeshRenderer(const std::string& name, MeshRenderer& mesh_renderer, LoadExecutionMode lem = LoadExecutionMode::ASYNC);

	void loadScene(const std::string& scene_file);
	void unloadCurrentScene();

	std::vector<MeshRenderer*>& getSceneMeshRenderers() { return sceneMeshRenderers; }

	std::vector<std::string>& getGlobalShaderKeywords() { return globalShaderKeywords; }
	void setGlobalShaderKeyword(const std::string& keyword, bool enable);

	ITexture* getDefaultTexture(MaterialTexture::Purpose purpose) const { return defaultTextures[(int)purpose]; }
	IBuffer* getDefaultMeshIb() const { return defaultMeshIb.get(); }
	IBuffer* getDefaultMeshVb() const { return defaultMeshVb.get(); }
	ResId getDefaultInputLayout() const { return defaultInputLayout; }
	ResId getDefaultMaterialTextureSampler() const { return defaultMaterialTextureSampler; }

	void sceneGui();

private:
	void initInis();
	void initShaders();
	void initDefaultAssets();

	std::vector<ITexture*> engineTextures;
	std::vector<ITexture*> sceneTextures;
	std::vector<Material*> sceneMaterials;
	std::vector<MeshRenderer*> sceneMeshRenderers;

	std::vector<std::string> globalShaderKeywords;

	std::array<ResId, (int)RenderPass::_COUNT> standardShaders;
	ResIdHolder standardInputLayout = BAD_RESID;

	std::array<ITexture*, (int)MaterialTexture::Purpose::_COUNT> defaultTextures;
	std::unique_ptr<IBuffer> defaultMeshIb;
	std::unique_ptr<IBuffer> defaultMeshVb;
	ResId defaultInputLayout = BAD_RESID;
	ResIdHolder defaultMaterialTextureSampler;

	std::unique_ptr<mINI::INIFile> materialsIniFile;
	mINI::INIStructure materialsIni;
	std::unique_ptr<mINI::INIFile> modelsIniFile;
	mINI::INIStructure modelsIni;
	std::unique_ptr<mINI::INIFile> currentSceneIniFile;
	mINI::INIStructure currentSceneIni;
	std::string currentSceneIniFilePath;
};