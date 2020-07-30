#include "stdafx.h"
#include "Waves.h"

#include <ppl.h>
#include <algorithm>
#include <vector>
#include <cassert>

using namespace DirectX;

Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
    m_numRows = m;
    m_numCols = n;

    m_vertexCount = m * n;
    m_triangleCount = (m - 1) * (n - 1) * 2;

    m_timeStep = dt;
    m_spatialStep = dx;

    float d = damping * dt + 2.0f;
    float e = (speed * speed) * (dt * dt) / (dx * dx);
    m_k1 = (damping * dt) / d;
    m_k2 = (4.0f - 8.0f * e) / d;
    m_k3 = (2.0f * e) / d;

    m_prevSolution.resize((INT64)m * (INT64)n);
    m_currentSolution.resize((INT64)m * (INT64)n);
    m_normals.resize((INT64)m * (INT64)n);
    m_tangentX.resize((INT64)m * (INT64)n);

    // Generate grid vertices in system memory.
    float halfWidth = (n - 1) * dx * 0.5f;
    float halfDepth = (m - 1) * dx * 0.5f;

    for (int i = 0; i < m; ++i)
    {
        float z = halfDepth - i * dx;
        for (int j = 0; j < n; ++j)
        {
            float x = -halfWidth + j * dx;

            m_prevSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
            m_currentSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
            m_normals[i * n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
            m_tangentX[i * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
        }
    }
}

Waves::~Waves()
{

}

int Waves::GetRowCount()const
{
    return m_numRows;
}

int Waves::GetColumnCount()const
{
    return m_numCols;
}

float Waves::GetDepth()const
{
    return m_numRows * m_spatialStep;
}

void Waves::Disturb(int i, int j, float magnitude)
{

}

void Waves::Update(float dt)
{

}