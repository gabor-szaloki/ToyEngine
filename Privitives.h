#pragma once

#include "Drawable.h"

namespace Primitives
{
	class Plane : public Drawable
	{
	public:

		Plane() : Drawable() { }
		~Plane() { }

		void Init(ID3D11Device* device, ID3D11DeviceContext* context) override
		{
			StandardVertexData vertices[] =
			{
				// Position						 Normal							Tangent								 Color							   UV
				{ XMFLOAT3(-1.0f,  0.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3( 1.0f,  0.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3( 1.0f,  0.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
				{ XMFLOAT3(-1.0f,  0.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
			};

			WORD indices[] =
			{
				3,1,0,
				2,1,3,
			};

			InitBuffers(device, context, vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices));
		}
	};

	class Box : public Drawable
	{
	public:

		Box() : Drawable() { }
		~Box() { }

		void Init(ID3D11Device* device, ID3D11DeviceContext* context) override
		{
			StandardVertexData vertices[] =
			{
				// Position						 Normal							Tangent								 Color							   UV
				{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
				{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  1.0f,  0.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },

				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
				{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f, -1.0f,  0.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },

				{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
				{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(-1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },

				{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f,  1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f,  1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f,  1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
				{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3( 1.0f,  0.0f,  0.0f), XMFLOAT4( 0.0f,  0.0f,  1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },

				{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
				{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3( 0.0f,  0.0f, -1.0f), XMFLOAT4( 1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },

				{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
				{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
				{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
				{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3( 0.0f,  0.0f,  1.0f), XMFLOAT4(-1.0f,  0.0f,  0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
			};

			WORD indices[] =
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

			InitBuffers(device, context, vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices));
		}
	};

	class Sphere : public Drawable
	{
	public:

		Sphere() : Drawable() { }
		~Sphere() { }

		void Init(ID3D11Device* device, ID3D11DeviceContext* context) override
		{
			const int latitudeSegments = 20;
			const int longitudeSegments = 40;

			const float deltaLat = XM_PI / latitudeSegments;
			const float deltaLong = (2.0f * XM_PI) / longitudeSegments;

			StandardVertexData vertices[(longitudeSegments + 1) * (latitudeSegments + 1)];
			WORD indices[longitudeSegments * latitudeSegments * 2 * 3];

			XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

			int idx = 0;

			for (int i = 0; i <= latitudeSegments; i++)
			{
				float lat = XM_PIDIV2 - i * deltaLat;
				
				float xz = cosf(lat);
				float y = sinf(lat);

				float v = (float)i / latitudeSegments;

				for (int j = 0; j <= longitudeSegments; j++)
				{
					float longi = j * deltaLong;

					float x = xz * cosf(longi);
					float z = xz * sinf(longi);

					XMFLOAT3 pos = XMFLOAT3(x, y, z);
					XMFLOAT3 normal = pos;
					XMVECTOR normalV = XMLoadFloat3(&normal);
					XMVECTOR upV = XMVectorSet(0, 1, 0, 0);
					XMFLOAT4 tangent;
					XMStoreFloat4(&tangent, XMVector3Normalize(XMVector3Cross(normalV, upV)));
					tangent.w = 1;
					float u = (float)j / longitudeSegments;
					XMFLOAT2 uv = XMFLOAT2(u, v);

					vertices[idx++] = { pos, normal, tangent, color, uv };
				}
			}

			idx = 0;
			for (int i = 0; i < latitudeSegments; ++i)
			{
				int k1 = i * (longitudeSegments + 1);     // beginning of current stack
				int k2 = k1 + longitudeSegments + 1;      // beginning of next stack

				for (int j = 0; j < longitudeSegments; ++j, ++k1, ++k2)
				{
					// 2 triangles per sector excluding first and last stacks
					// k1 => k2 => k1+1
					if (i != 0)
					{
						indices[idx++] = k1;
						indices[idx++] = k1 + 1;
						indices[idx++] = k2;
					}

					// k1+1 => k2 => k2+1
					if (i != longitudeSegments - 1)
					{
						indices[idx++] = k1 + 1;
						indices[idx++] = k2 + 1;
						indices[idx++] = k2;
					}
				}
			}

			InitBuffers(device, context, vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices));
		}
	};
}