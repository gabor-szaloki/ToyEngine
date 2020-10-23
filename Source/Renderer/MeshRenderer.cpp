#include "MeshRenderer.h"

#include <Driver/IBuffer.h>

#include "Material.h"
#include "ConstantBuffers.h"

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
	PerObjectConstantBufferData perObjectCbData;
	perObjectCbData.world = worldTransform;
	cb->updateData(&perObjectCbData);

	material->set(render_pass);
	drv->setInputLayout(inputLayoutId);
	drv->setIndexBuffer(ib->getId());
	drv->setVertexBuffer(vb->getId());
	drv->setConstantBuffer(ShaderStage::VS, 2, cb->getId());
	drv->drawIndexed(ib->getDesc().numElements, 0, 0);
}
