// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
#pragma once
#include "stdafx.h"

class GeometryGenerator
{
public:
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

    struct Vertex
    {
        Vertex():
            Position(0.0f,0.0f,0.0f),
            Normal(0.0f, 0.0f, 0.0f),
            TangentU(0.0f, 0.0f, 0.0f),
            TexC(0.0f,0.0f)
        {}
        Vertex(
            const DirectX::XMFLOAT3& p,
            const DirectX::XMFLOAT3& n,
            const DirectX::XMFLOAT3& t,
            const DirectX::XMFLOAT2& uv
        ):
            Position(p),
            Normal(n),
            TangentU(t),
            TexC(uv)
        {}

        Vertex(
            float px, float py, float pz,
            float nx, float ny, float nz,
            float tx, float ty, float tz,
            float u, float v
        ):
            Position(px,py,pz),
            Normal(nx,ny,nz),
            TangentU(tx,ty,tz),
            TexC(u,v)
        {}

        DirectX::XMFLOAT3 Position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        DirectX::XMFLOAT3 Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        DirectX::XMFLOAT3 TangentU = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        DirectX::XMFLOAT2 TexC = DirectX::XMFLOAT2(0.0f, 0.0f);
    };

    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<uint32> Indices32;
        
        std::vector<uint16>& GetIndices16()
        {
            if (m_indices16.empty())
            {
                m_indices16.resize(Indices32.size());
                for (size_t i = 0; i < Indices32.size(); i++)
                {
                    m_indices16[i] = static_cast<uint16>(Indices32[i]);
                }
            }
        }

    private:
        std::vector<uint16> m_indices16;
    };



};