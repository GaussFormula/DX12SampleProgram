#pragma once

#include "stdafx.h"

class Waves
{
public:
    Waves(int m, int n, float dx, float dt, float speed, float damping);
    Waves(const Waves& rhs) = delete;
    Waves& operator=(const Waves& rhs) = delete;
    ~Waves();

    int GetRowCount()const;
    int GetColumnCount()const;
    int GetVertexCount()const;
    int GetTriangleCount()const;
    float GetWidth()const;
    float GetDepth()const;

    // Return the solution at the ith grid point.
    const DirectX::XMFLOAT3& Position(int i)const { return m_currentSolution[i]; }

    // Return the solution normal at the ith grid point.
    const DirectX::XMFLOAT3& Normal(int i) const { return m_normals[i]; }

    // Return the solution tangent vendor at the ith grid point in the local x-axis
    // direction.
    const DirectX::XMFLOAT3& TangentX(int i)const { return m_tangentX[i]; }

    void Update(float dt);
    void Disturb(int i, int j, float magnitude);

private:
    int m_numRows = 0;
    int m_numCols = 0;

    int m_vertexCount = 0;
    int m_triangleCount = 0;

    float m_timeStep = 0.0f;
    float m_spatialStep = 0.0f;

    // Simulation constants we can precompute.
    float m_k1 = 0.0f;
    float m_k2 = 0.0f;
    float m_k3 = 0.0f;

    std::vector<DirectX::XMFLOAT3> m_currentSolution;
    std::vector<DirectX::XMFLOAT3> m_prevSolution;
    std::vector<DirectX::XMFLOAT3> m_normals;
    std::vector<DirectX::XMFLOAT3> m_tangentX;
};