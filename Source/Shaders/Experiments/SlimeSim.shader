// Slime mold simulation based on Sebastian Lague's "Coding Adventure: Ant and Slime Simulations":
// https://youtu.be/X-iSQQgOd1A

#include "../Common.hlsl"

cbuffer SlimeSimConstantBuffer : register(b0)
{
	uint _Width;
	uint _Height;
	uint _NumAgents;
	float _DeltaTime;

	float _AgentMoveSpeed;
	float _AgentTurnSpeed;
	float _EvaporateSpeed;
	float _DiffuseSpeed;

	float _SensorOffsetAngle;
	float _SensorOffsetDistance;
	int _SensorRadius;
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

float sense(Agent agent, float angleOffset)
{
	float sensorAngle = agent.angle + angleOffset;
	float sinAngle, cosAngle;
	sincos(sensorAngle, sinAngle, cosAngle);
	float2 sensorDir = float2(cosAngle, sinAngle);
	int2 sensorCentre = round(agent.position + sensorDir * _SensorOffsetDistance);

	float sum = 0;
	for (int offsetX = -_SensorRadius; offsetX <= _SensorRadius; offsetX++)
	{
		for (int offsetY = -_SensorRadius; offsetY <= _SensorRadius; offsetY++)
		{
			int2 pos = sensorCentre + int2(offsetX, offsetY);
			if (all(pos >= 0) && all(pos < int2(_Width, _Height)))
				sum += _SlimeMap[pos].x;
		}
	}

	return sum;
}

[numthreads(64, 1, 1)]
void Update(uint id : SV_DispatchThreadID)
{
	if (id >= _NumAgents)
		return;

	Agent agent = _Agents[id];
	float random = hash01(agent.position.y * 4321 + agent.position.x * 6789 + hash(id));

	// Steer based on sensory data
	float weightForward = sense(agent, 0);
	float weightLeft = sense(agent, _SensorOffsetAngle);
	float weightRight = sense(agent, -_SensorOffsetAngle);

	// Continue in same direction, do nothing
	if (weightForward > weightLeft && weightForward > weightRight) {}
	// Turn randomly
	else if (weightForward < weightLeft && weightForward < weightRight)
		agent.angle += (random - 0.5) * 2 * _AgentTurnSpeed * _DeltaTime;
	// Turn right
	else if (weightRight > weightLeft)
		agent.angle -= random * _AgentTurnSpeed * _DeltaTime;
	// Turn left
	else if (weightLeft > weightRight)
		agent.angle += random * _AgentTurnSpeed * _DeltaTime;

	// Move agent based on direction and speed
	float sinAngle, cosAngle;
	sincos(agent.angle, sinAngle, cosAngle);
	float2 direction = float2(cosAngle, sinAngle);
	agent.position += direction * _AgentMoveSpeed * _DeltaTime;

	// Clamp position to map boundaries, and pick new random move angle if hit boundary
	if (any(agent.position < 0) || any(agent.position >= float2(_Width, _Height)))
	{
		agent.position.x = clamp(agent.position.x, 0.0, _Width - 0.01);
		agent.position.y = clamp(agent.position.y, 0.0, _Height - 0.01);
		agent.angle = random * 2 * PI;
	}

	// Set new position and draw trail
	_Agents[id] = agent;
	_SlimeMap[(int2)agent.position] = 1;
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