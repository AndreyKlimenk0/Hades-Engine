
cbuffer Fill_Texture_Value : register(b0) {
    uint value;
};

RWTexture2D<uint> texture : register(t0);

[numthreads(16, 16, 1)]
void cs_main()
{
	
}