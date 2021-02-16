#include "AssetManager.h"

#include <filesystem>

#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>

#include "MeshRenderer.h"

void AssetManager::sceneGui()
{
	static std::vector<std::string> scenePaths;
	static std::string scenes;
	static int currentScene = -1;

	if (scenePaths.size() == 0)
	{
		for (const std::filesystem::directory_entry& file : std::filesystem::directory_iterator("Assets/Scenes"))
		{
			std::string scenePath = file.path().string();
			std::replace(scenePath.begin(), scenePath.end(), '\\', '/');
			scenePaths.push_back(scenePath);
			std::string sceneName = file.path().filename().string();
			sceneName = sceneName.substr(0, sceneName.length() - 4); // remove ".ini"
			scenes += sceneName;
			scenes += '\0';
		}
	}
	if (currentScene < 0)
	{
		std::string lastLoadedScenePath = autoimgui::load_custom_param("lastLoadedScenePath");
		auto it = std::find(scenePaths.begin(), scenePaths.end(), lastLoadedScenePath);
		currentScene = it != scenePaths.end() ? int(it - scenePaths.begin()) : 0;
	}
	ImGui::Combo("Scene", &currentScene, scenes.c_str());
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		unloadCurrentScene();
		loadScene(scenePaths[currentScene]);
		autoimgui::save_custom_param("lastLoadedScenePath", scenePaths[currentScene]);
	}

	ImGui::Separator();

	if (ImGui::CollapsingHeader("Mesh renderers", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		for (MeshRenderer* mr : sceneMeshRenderers)
			if (ImGui::CollapsingHeader(mr->name.c_str()))
				mr->gui();
		ImGui::Unindent();
	}
}

REGISTER_IMGUI_WINDOW("Scene", []() { am->sceneGui(); });