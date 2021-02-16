#pragma once

#include <memory>
#include <string>

#include <Common.h>
#include <Driver/IDriver.h>

#include "Transform.h"

struct StandardVertexData;
class IBuffer;
class Material;

struct SubmeshData
{
	std::string name;
	bool enabled;
	unsigned int startIndex;
	unsigned int numIndices;
	unsigned int startVertex;
	Material *material;
};

struct MeshData
{
	std::vector<StandardVertexData> vertexData;
	std::vector<unsigned int> indexData;
	std::vector<SubmeshData> submeshes;
};

class MeshRenderer
{
public:
	MeshRenderer(const std::string& name_, Material* material_, ResId input_layout_id = BAD_RESID);
	void setInputLayout(ResId res_id);
	void load(const MeshData& mesh_data);
	void render(RenderPass render_pass);
	void gui();

	const Transform& getTransform() const { return transform; }
	void setTransform(const Transform& t) { transform = t; transformMatrix = t.getMatrix(); }
	void setPosition(XMVECTOR position) { transform.position = position; transformMatrix = transform.getMatrix(); }
	void setPosition(XMFLOAT3 position) { setPosition(XMLoadFloat3(&position)); }
	void setPosition(float x, float y, float z) { setPosition(XMFLOAT3(x, y, z)); }
	void setRotation(XMVECTOR rotation) { transform.rotation = rotation; transformMatrix = transform.getMatrix(); }
	void setRotation(XMFLOAT3 rotation) { setRotation(XMLoadFloat3(&rotation)); }
	void setRotation(float pitch, float yaw, float roll) { setRotation(XMFLOAT3(pitch, yaw, roll)); }
	void setScale(float scale) { transform.scale = scale; transformMatrix = transform.getMatrix(); }
	const XMMATRIX& getTransformMatrix() const { return transformMatrix; }

	std::string name;

private:
	bool enabled = true;
	int firstSubmeshToRender = 0;
	int lastSubmeshToRender = 0;
	Transform transform;
	XMMATRIX transformMatrix = XMMatrixIdentity();

	Material* material = nullptr;
	ResId inputLayoutId = BAD_RESID;
	std::unique_ptr<IBuffer> cb;
	std::unique_ptr<IBuffer> vb;
	std::unique_ptr<IBuffer> ib;
	std::vector<SubmeshData> submeshes;
};
