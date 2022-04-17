#include "InputLayoutD3D12.h"

#include "DriverD3D12.h"

namespace drv_d3d12
{

static const char* get_semantic_string(VertexInputSemantic semantic)
{
	switch (semantic)
	{
	case VertexInputSemantic::POSITION:
		return "POSITION";
	case VertexInputSemantic::NORMAL:
		return "NORMAL";
	case VertexInputSemantic::TANGENT:
		return "TANGENT";
	case VertexInputSemantic::COLOR:
		return "COLOR";
	case VertexInputSemantic::TEXCOORD:
		return "TEXCOORD";
	default:
		assert(false);
		return nullptr;
	}
}

InputLayout::InputLayout(const InputLayoutElementDesc* descs, uint num_descs)
{
	inputElements.resize(num_descs);

	for (uint i = 0; i < num_descs; i++)
	{
		D3D12_INPUT_ELEMENT_DESC& elem = inputElements[i];

		elem.SemanticName = get_semantic_string(descs[i].semantic);
		elem.SemanticIndex = descs[i].semanticIndex;
		elem.Format = (DXGI_FORMAT)descs[i].format;
		elem.InputSlot = 0;
		elem.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elem.InstanceDataStepRate = 0;
	}

	id = DriverD3D12::get().registerInputLayout(this);
}

InputLayout::~InputLayout()
{
	DriverD3D12::get().unregisterInputLayout(id);
}

}