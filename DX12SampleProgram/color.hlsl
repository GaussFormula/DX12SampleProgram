cbuffer cbPerObject :register(b0)
{
    float4x4 gWorld;
};

cbuffer cbPass:register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float  gNearZ;
    float  gFarZ;
    float  gTotalTime;
    float  gDeltaTime;
};

struct VertexIn
{
    float3 PosL:POSITION;
    float4 Color:COLOR;
};

struct VertexOut
{
    float4 PosH:SV_POSITION;
    float4 Color:COLOR;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;

    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL.x,vin.PosL.y,(vin.PosL.z+0.1f)*1.1f, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);

    // Just pass vertex color into the pixel shader.
    vout.Color =float4(vout.PosH.x, vout.PosH.y, vout.PosH.z, 1.0f);

    return vout;
}

float4 PSMain(VertexOut pin) :SV_Target
{
    return pin.Color;
}