// Slime mold simulation based on Sebastian Lague's "Coding Adventure: Ant and Slime Simulations":
// https://youtu.be/X-iSQQgOd1A

#include "../Common.hlsl"

cbuffer SlimeSimConstantBuffer : register(b0)
{
	uint _Width;
	uint _Height;
	uint _NumAgents;
	float _AgentMoveSpeed;

	float _DeltaTime;
	float _EvaporateSpeed;
	float _DiffuseSpeed;
	float pad;
}

struct Agent
{
	float2 position;
	float angle;
};

RWStructuredBuffer<Agent> _Agents : register(u0);
RWTexture2D<unorm float4> _SlimeMap : register(u1);
RWTexture2D<unorm float4> _PostProcessedSlimeMap : register(u2);

// Hash function www.cs.ubc.ca/~rbridson/docs/schechter-sca08-turbulence.pdf
uint hash(uint state)
{
	state ^= 2747636419u;
	state *= 2654435769u;
	state ^= state >> 16;
	state *= 2654435769u;
	state ^= state >> 16;
	state *= 2654435769u;
	return state;
}

float hash01(uint state)
{
	uint random = hash(state);
	return random / (float)0xffffffff;
}

[numthreads(16, 16, 1)]
void Clear(uint2 id : SV_DispatchThreadID)
{
	if (any(id >= uint2(_Width, _Height)))
		return;
	_SlimeMap[id] = 0;
	_PostProcessedSlimeMap[id] = 0;
}

[numthreads(64, 1, 1)]
void Update(uint id : SV_DispatchThreadID)
{
	if (id >= _NumAgents)
		return;

	Agent agent = _Agents[id];
	float random = hash01(agent.position.y * 4321 + agent.position.x * 6789 + hash(id));

	// Move agent based on direction and speed
	float2 direction = float2(cos(agent.angle), sin(agent.angle));
	float2 newPos = agent.position + direction * _AgentMoveSpeed * _DeltaTime;

	// Clamp position to map boundaries, and pick new random move angle if hit boundary
	if (any(newPos < 0) || any(newPos >= float2(_Width, _Height)))
	{
		newPos.x = clamp(newPos.x, 0.0, _Width - 0.01);
		newPos.y = clamp(newPos.y, 0.0, _Height - 0.01);
		_Agents[id].angle = random * 2 * PI;
	}

	// Set new position and draw trail
	_Agents[id].position = newPos;
	_SlimeMap[(int2)newPos] = 1;
}

[numthreads(16, 16, 1)]
void PostProcess(uint2 id : SV_DispatchThreadID)
{
	if (any(id >= uint2(_Width, _Height)))
		return;

	float3 originalValue = _SlimeMap[id].rgb;

	// Simulate diffusion with a simple 3x3 blur
	float3 sum = 0;
	for (int offsetX = -1; offsetX <= 1; offsetX++)
	{
		for (int offsetY = -1; offsetY <= 1; offsetY++)
		{
			int2 coords = id + int2(offsetX, offsetY);
			coords = clamp(coords, 0, int2(_Width - 1, _Height - 1));
			sum += _SlimeMap[coords].rgb;
		}
	}
	float3 blurResult = sum / 9;

	// Blend between the original value and the blurred value based on diffusion speed
	float3 diffusedValue = lerp(originalValue, blurResult, _DiffuseSpeed * _DeltaTime);

	// Subtract from the value to simulate evaporation
	float3 evaporatedValue = max(0, diffusedValue - _EvaporateSpeed * _DeltaTime);

	_PostProcessedSlimeMap[id] = float4(evaporatedValue, 1);
}