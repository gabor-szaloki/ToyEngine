#include "MeshRenderer.h"

#include <Driver/IBuffer.h>

#include "Material.h"
#include "ConstantBuffers.h"
#include "WorldRenderer.h"

MeshRenderer::MeshRenderer(const char* name_, Material* material_, ResId input_layout_id)
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

void MeshRenderer::loadVertices(unsigned int num_vertices, unsigned int vertex_byte_size, void* data)
{
	BufferDesc vbDesc(name, vertex_byte_size, num_vertices, ResourceUsage::DEFAULT, BIND_VERTEX_BUFFER);
	vbDesc.initialData = data;
	vb.reset(drv->createBuffer(vbDesc));
}

void MeshRenderer::loadIndices(unsigned int num_indices, void* data)
{
	unsigned int indexByteSize = ::get_byte_size_for_texfmt(drv->getIndexFormat());
	BufferDesc ibDesc(name, indexByteSize, num_indices, ResourceUsage::DEFAULT, BIND_INDEX_BUFFER);
	ibDesc.initialData = data;
	ib.reset(drv->createBuffer(ibDesc));
}

void MeshRenderer::render(RenderPass render_pass)
{
	IBuffer* ibToUse;
	IBuffer* vbToUse;
	ResId ilToUse;
	XMMATRIX tmToUse;
	if (vb != nullptr && ib != nullptr)
	{
		ibToUse = ib.get();
		vbToUse = vb.get();
		ilToUse = inputLayoutId;
		tmToUse = transformMatrix;
	}
	else
	{
		ibToUse = wr->defaultMeshIb.get();
		vbToUse = wr->defaultMeshVb.get();
		ilToUse = wr->defaultInputLayout;
		tmToUse = XMMatrixScaling(.1f, .1f, .1f) * XMMatrixRotationY(wr->getTime() * 10.f) * XMMatrixTranslationFromVector(transformMatrix.r[3]);
	}

	PerObjectConstantBufferData perObjectCbData;
	perObjectCbData.world = tmToUse;
	cb->updateData(&perObjectCbData);

	material->set(render_pass);
	drv->setConstantBuffer(ShaderStage::VS, 2, cb->getId());

	drv->setInputLayout(ilToUse);
	drv->setIndexBuffer(ibToUse->getId());
	drv->setVertexBuffer(vbToUse->getId());
	drv->drawIndexed(ibToUse->getDesc().numElements, 0, 0);
}
