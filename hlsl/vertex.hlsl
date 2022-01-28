#ifndef __VERTEX__
#define __VERTEX__

struct Vertex_XC_In {
    float2 position : POSITION;
    float4 color    : COLOR;
};

struct Vertex_XC_Out {
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

struct Vertex_XUV_In {
    float3 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct Vertex_XUV_Out {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

struct Vertex_XNUV_In {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct Vertex_XNUV_Out {
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

#endif