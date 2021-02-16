#include "MeshRenderer.h"

#include <3rdParty/imgui/imgui.h>
#include <Driver/IBuffer.h>
#include <Renderer/ConstantBuffers.h>
#include <Renderer/WorldRenderer.h>

#include "Material.h"

MeshRenderer::MeshRenderer(const std::string& name_, Material* material_, ResId input_layout_id)
	: name(name_), material(material_), inputLayoutId(input_layout_id)
{
	BufferDesc cbDesc;
	cbDesc.bindFlags = BIND_CONSTANT_BUFFER;
	cbDesc.numElements = 1;
	cbDesc.name = name_;
	cbDesc.elementByteSize = sizeof(PerObjectConstantBufferData);
	cb.reset(drv->createBuffer(cbDesc));
}

void MeshRenderer::setInputLayout(ResId res_id)
{
	inputLayoutId = res_id;
}

void MeshRenderer::load(const MeshData& mesh_data)
{
	assert(mesh_data.vertexData.size() > 0);
	assert(mesh_data.indexData.size() > 0);

	BufferDesc vbDesc(name, sizeof(mesh_data.vertexData[0]), (unsigned int)mesh_data.vertexData.size(), ResourceUsage::DEFAULT, BIND_VERTEX_BUFFER);
	vbDesc.initialData = (void*)mesh_data.vertexData.data();
	vb.reset(drv->createBuffer(vbDesc));

	unsigned int indexByteSize = ::get_byte_size_for_texfmt(drv->getIndexFormat());
	assert(sizeof(mesh_data.indexData[0]) == indexByteSize);
	BufferDesc ibDesc(name, indexByteSize, (unsigned int)mesh_data.indexData.size(), ResourceUsage::DEFAULT, BIND_INDEX_BUFFER);
	ibDesc.initialData = (void*)mesh_data.indexData.data();
	ib.reset(drv->createBuffer(ibDesc));

	submeshes.assign(mesh_data.submeshes.begin(), mesh_data.submeshes.end());

	firstSubmeshToRender = 0;
	lastSubmeshToRender = (int)submeshes.size() - 1;
}

void MeshRenderer::render(RenderPass render_pass)
{
	if (!enabled)
		return;

	IBuffer* ibToUse;
	IBuffer* vbToUse;
	ResId ilToUse;
	XMMATRIX tmToUse;
	bool useDefaultMesh = vb == nullptr || ib == nullptr;
	if (useDefaultMesh)
	{
		ibToUse = wr->defaultMeshIb.get();
		vbToUse = wr->defaultMeshVb.get();
		ilToUse = wr->defaultInputLayout;
		tmToUse = XMMatrixScaling(.1f, .1f, .1f) * XMMatrixRotationY(wr->getTime() * 10.f) * XMMatrixTranslationFromVector(transformMatrix.r[3]);
	}
	else
	{
		ibToUse = ib.get();
		vbToUse = vb.get();
		ilToUse = inputLayoutId;
		tmToUse = transformMatrix;
	}

	PerObjectConstantBufferData perObjectCbData;
	perObjectCbData.world = tmToUse;
	cb->updateData(&perObjectCbData);

	material->set(render_pass);
	drv->setConstantBuffer(ShaderStage::VS, 2, cb->getId());

	drv->setInputLayout(ilToUse);
	drv->setIndexBuffer(ibToUse->getId());
	drv->setVertexBuffer(vbToUse->getId());

	if (useDefaultMesh)
		drv->drawIndexed(ibToUse->getDesc().numElements, 0, 0);
	else
	{
		bool materialOverridden = false;
		for (int i = firstSubmeshToRender; i <= lastSubmeshToRender; i++)
		{
			const SubmeshData& submesh = submeshes[i];
			if (!submesh.enabled)
				continue;

			if (submesh.material != nullptr)
			{
				submesh.material->set(render_pass);
				materialOverridden = true;
			}
			else if (materialOverridden)
			{
				material->set(render_pass);
				materialOverridden = false;
			}
			drv->drawIndexed(submesh.numIndices, submesh.startIndex, submesh.startVertex);
		}
	}
}

namespace ImGui
{
	bool DragAngle3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.0f deg", ImGuiSliderFlags flags = 0)
	{
		float vDeg[3];
		for (int i = 0; i < 3; i++)
			vDeg[i] = to_deg(v[i]);
		bool changed = DragFloat3(label, vDeg, v_speed, v_min, v_max, format, flags);
		if (changed)
			for (int i = 0; i < 3; i++)
				v[i] = to_rad(vDeg[i]);
		return changed;
	}
}

void MeshRenderer::gui()
{
	static std::string buf;
	buf = "Enabled##" + name;
	ImGui::Checkbox(buf.c_str(), &enabled);

	if (submeshes.size() > 0)
	{
		buf = "Submesh range to render##" + name;
		ImGui::DragIntRange2(buf.c_str(), &firstSubmeshToRender, &lastSubmeshToRender, 1.0f, 0, submeshes.size() - 1);
		ImGui::Text("First submesh rendered: %s", submeshes[firstSubmeshToRender].name.c_str());
		ImGui::Text("Last submesh rendered: %s", submeshes[lastSubmeshToRender].name.c_str());
	}

	Transform tr = getTransform();
	bool changed = false;
	buf = "Position##" + name;
	changed |= ImGui::DragFloat3(buf.c_str(), tr.position.m128_f32, 0.1f);
	buf = "Rotation##" + name;
	changed |= ImGui::DragAngle3(buf.c_str(), tr.rotation.m128_f32);
	buf = "Scale##" + name;
	changed |= ImGui::DragFloat(buf.c_str(), &tr.scale, 0.1f);
	if (changed)
		setTransform(tr);

	buf = "Submeshes##" + name;
	if (ImGui::CollapsingHeader(buf.c_str()))
	{
		for (int i = 0; i < submeshes.size(); i++)
		{
			SubmeshData& submesh = submeshes[i];
			buf = submesh.name + "##" + name;
			if (ImGui::CollapsingHeader(buf.c_str()))
			{
				buf = "Enabled##" + name + submesh.name;
				ImGui::Checkbox(buf.c_str(), &submesh.enabled);
				ImGui::Text("material:    %s", submesh.material != nullptr ? submesh.material->name.c_str() : "-");
				ImGui::Text("startIndex:  %d", submesh.startIndex);
				ImGui::Text("numIndices:  %d", submesh.numIndices);
				ImGui::Text("startVertex: %d", submesh.startVertex);
			}
		}
	}
}
