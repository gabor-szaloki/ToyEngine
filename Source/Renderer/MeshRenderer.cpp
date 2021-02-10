#include "MeshRenderer.h"

#include <Driver/IBuffer.h>

#include "Material.h"
#include "ConstantBuffers.h"
#include "WorldRenderer.h"

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
}

void MeshRenderer::render(RenderPass render_pass)
{
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
		for (SubmeshData& submesh : submeshes)
			drv->drawIndexed(submesh.numIndices, submesh.startIndex, submesh.startVertex);
	}
}
