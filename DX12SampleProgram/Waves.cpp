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
    m_k1 = ((double)damping * dt - 2.0) / d;
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

            m_prevSolution[(INT64)i * n + j] = XMFLOAT3(x, 0.0f, z);
            m_currentSolution[(INT64)i * n + j] = XMFLOAT3(x, 0.0f, z);
            m_normals[(INT64)i * n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
            m_tangentX[(INT64)i * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
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

int Waves::GetVertexCount()const
{
    return m_vertexCount;
}

int Waves::GetTriangleCount()const
{
    return m_triangleCount;
}

float Waves::GetDepth()const
{
    return m_numRows * m_spatialStep;
}

float Waves::GetWidth()const
{
    return m_numCols * m_spatialStep;
}

void Waves::Disturb(int i, int j, float magnitude)
{
    // Don't disturb boundaries.
    assert(i > 1 && i < m_numRows - 2);
    assert(j > 1 && j < m_numCols - 2);

    float halfMag = 0.5f * magnitude;

    // Disturb the ijth vertex height and its neighbors.
    m_currentSolution[(INT64)i * m_numCols + j].y += magnitude;
    m_currentSolution[(INT64)i * m_numCols + j + 1].y += halfMag;
    m_currentSolution[(INT64)i * m_numCols + j - 1].y += halfMag;
    m_currentSolution[((INT64)i + 1) * m_numCols + j].y += halfMag;
    m_currentSolution[((INT64)i - 1) * m_numCols + j].y += halfMag;
}

void Waves::Update(float dt)
{
    static float t = 0;

    // Accumulate time;
    t += dt;

    // Only update the simulation at the specified time step.
    if (t >= m_timeStep)
    {
        // Only update interior points. We use zero boundry conditions.
        concurrency::parallel_for(1, m_numRows - 1, [this](INT64 i)
            // for(int i=1;i<m_numRows-1;++i)
            {
                for (int j = 1; j < m_numCols - 1; ++j)
                {
                    // After this update we will be discarding the old previous
                    // buffer, so overwrite that buffer with the new update.
                    // Note how we can do this inplace (read/write to same element)
                    // because we won't need prev_ij again and the assignment happens
                    // last.

                    // Note j indexes x and i indexes z: h(x_j,z_i,t_k)
                    // Moreover, our +z axis goes "down"; this is just to keep
                    // consistent with our row indices going down.
                    m_prevSolution[i * m_numCols + j].y =
                        m_k1 * m_prevSolution[i * m_numCols + j].y +
                        m_k2 * m_currentSolution[i * m_numCols + j].y +
                        m_k3 * (m_currentSolution[(i + 1) * m_numCols + j].y +
                            m_currentSolution[(i - 1) * m_numCols + j].y +
                            m_currentSolution[i * m_numCols + j + 1].y +
                            m_currentSolution[i * m_numCols + j - 1].y
                            );
                }
            }
        );

        // We just overwrote the previous buffer with the new data, so
        // this data needs to become the current solution and the old 
        // current solution becomes the new previous solution.
        std::swap(m_prevSolution, m_currentSolution);

        t = 0.0f; // Reset time.

        // Compute normals using finite difference scheme.
        concurrency::parallel_for(1, m_numRows - 1, [this](INT64 i)
            {
                for (int j = 1; j < m_numCols; ++j)
                {
                    float l = m_currentSolution[i * m_numCols + j - 1].y;
                    float r = m_currentSolution[i * m_numCols + j + 1].y;
                    float t = m_currentSolution[(i - 1) * m_numCols + j].y;
                    float b = m_currentSolution[(i + 1) * m_numCols + j].y;

                    m_normals[i * m_numCols + j].x = -r + l;
                    m_normals[i * m_numCols + j].y = 2.0f * m_spatialStep;
                    m_normals[i * m_numCols + j].z = b - t;

                    XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&m_normals[i * m_numCols + j]));
                    XMStoreFloat3(&m_normals[i * m_numCols + j], n);

                    m_tangentX[i * m_numCols + j] = XMFLOAT3(2.0f * m_spatialStep, r - l, 0.0f);
                    XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&m_tangentX[i * m_numCols + j]));
                    XMStoreFloat3(&m_tangentX[i * m_numCols + j], T);
                }
            }
        );
    }
}