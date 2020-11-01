#pragma once

#include "RendererCommon.h"
#include "ConstantBuffers.h"

namespace primitives
{
	constexpr unsigned int PLANE_VERTEX_COUNT = 4;
	constexpr unsigned int PLANE_INDEX_COUNT = 6;
	StandardVertexData plane_vertices[PLANE_VERTEX_COUNT] =
	{
		// Position						 Normal							Color		 UV
		{ XMFLOAT3(-1.0f,  0.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f,  0.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3( 1.0f,  0.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f,  0.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 0.0f) },
	};
	unsigned short plane_indices[PLANE_INDEX_COUNT] =
	{
		3,1,0,
		2,1,3,
	};

	constexpr unsigned int BOX_VERTEX_COUNT = 4*6;
	constexpr unsigned int BOX_INDEX_COUNT = 6*6;
	StandardVertexData box_vertices[BOX_VERTEX_COUNT] =
	{
		// Position						 Normal							Color		 UV
		{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 0.0f) },

		{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), 0xFFFFFFFFu, XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), 0xFFFFFFFFu, XMFLOAT2(1.0f, 0.0f) },
	};
	unsigned short box_indices[BOX_INDEX_COUNT] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};

	constexpr int SPHERE_LATITUDE_SEGMENTS = 20;
	constexpr int SPHERE_LONGITUDE_SEGMENTS = 40;
	constexpr unsigned int SPHERE_VERTEX_COUNT = (SPHERE_LONGITUDE_SEGMENTS + 1) * (SPHERE_LATITUDE_SEGMENTS + 1);
	constexpr unsigned int SPHERE_INDEX_COUNT = SPHERE_LONGITUDE_SEGMENTS * SPHERE_LATITUDE_SEGMENTS * 2 * 3;
	StandardVertexData sphere_vertices[SPHERE_VERTEX_COUNT];
	unsigned short sphere_indices[SPHERE_INDEX_COUNT];

	void init()
	{
		const float deltaLat = XM_PI / SPHERE_LATITUDE_SEGMENTS;
		const float deltaLong = (2.0f * XM_PI) / SPHERE_LONGITUDE_SEGMENTS;

		const unsigned int color = 0xFFFFFFFFu;

		int idx = 0;

		for (int i = 0; i <= SPHERE_LATITUDE_SEGMENTS; i++)
		{
			float lat = XM_PIDIV2 - i * deltaLat;

			float xz = cosf(lat);
			float y = sinf(lat);

			float v = (float)i / SPHERE_LATITUDE_SEGMENTS;

			for (int j = 0; j <= SPHERE_LONGITUDE_SEGMENTS; j++)
			{
				float longi = j * deltaLong;

				float x = xz * cosf(longi);
				float z = xz * sinf(longi);

				XMFLOAT3 pos = XMFLOAT3(x, y, z);
				XMFLOAT3 normal = pos;
				XMVECTOR normalV = XMLoadFloat3(&normal);
				XMVECTOR upV = XMVectorSet(0, 1, 0, 0);
				float u = (float)j / SPHERE_LONGITUDE_SEGMENTS;
				XMFLOAT2 uv = XMFLOAT2(u, v);

				sphere_vertices[idx++] = { pos, normal, color, uv };
			}
		}

		idx = 0;
		for (int i = 0; i < SPHERE_LATITUDE_SEGMENTS; ++i)
		{
			int k1 = i * (SPHERE_LONGITUDE_SEGMENTS + 1);     // beginning of current stack
			int k2 = k1 + SPHERE_LONGITUDE_SEGMENTS + 1;      // beginning of next stack

			for (int j = 0; j < SPHERE_LONGITUDE_SEGMENTS; ++j, ++k1, ++k2)
			{
				// 2 triangles per sector excluding first and last stacks
				// k1 => k2 => k1+1
				if (i != 0)
				{
					sphere_indices[idx++] = k1;
					sphere_indices[idx++] = k1 + 1;
					sphere_indices[idx++] = k2;
				}

				// k1+1 => k2 => k2+1
				if (i != SPHERE_LONGITUDE_SEGMENTS - 1)
				{
					sphere_indices[idx++] = k1 + 1;
					sphere_indices[idx++] = k2 + 1;
					sphere_indices[idx++] = k2;
				}
			}
		}
	}
}