#ifndef __COLOR_CONVERSION__
#define __COLOR_CONVERSION__

float3 SRGB_to_linear(float3 sRGBCol)
{
    float3 linearRGBLo = sRGBCol / 12.92;
    float3 linearRGBHi = pow((sRGBCol + 0.055) / 1.055, 2.4);
    float3 linearRGB = select(sRGBCol <= 0.04045, linearRGBLo, linearRGBHi);
    return linearRGB;
}

float3 linear_to_SRGB(float3 linearCol)
{
    float3 sRGBLo = linearCol * 12.92;
    float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
    float3 sRGB = select(linearCol <= 0.0031308, sRGBLo, sRGBHi);
    return sRGB;
}
#endif