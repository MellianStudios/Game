#include "common.inc.hlsl"

[[vk::binding(0, GLOBAL)]] SamplerState _sampler;
[[vk::binding(1, GLOBAL)]] Texture2D<float4> _texture;

struct Constants
{
    float4 colorMultiplier;
    float4 additiveColor;
    float4 uvOffsetAndExtent;
    uint channelRedirectors;
};

[[vk::push_constant]] Constants _constants;

struct VSOutput
{
    float2 uv : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target
{
    float2 dimensions;
    _texture.GetDimensions(dimensions.x, dimensions.y);

    float2 uv = float2(input.uv.x, 1.0 - input.uv.y);
    uv = uv * _constants.uvOffsetAndExtent.zw + _constants.uvOffsetAndExtent.xy;

    float4 color = _texture.SampleLevel(_sampler, uv, 0);

    int rRedirector = _constants.channelRedirectors & 0xFF;
    int gRedirector = (_constants.channelRedirectors >> 8) & 0xFF;
    int bRedirector = (_constants.channelRedirectors >> 16) & 0xFF;
    int aRedirector = (_constants.channelRedirectors >> 24) & 0xFF;

    color = color * _constants.colorMultiplier + _constants.additiveColor;

    float4 redirectedColor = float4(0, 0, 0, 0);
    redirectedColor.r = color[rRedirector];
    redirectedColor.g = color[gRedirector];
    redirectedColor.b = color[bRedirector];
    redirectedColor.a = color[aRedirector];

    return redirectedColor;
}