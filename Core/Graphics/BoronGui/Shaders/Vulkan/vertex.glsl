#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in float inBrightness;

layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;
    vec2 size;

    //GLOBALS
    vec2 viewportSize;

    //GuiProps
    float rounding;
} pc;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragRounding;
layout(location = 2) out vec2 fragLocalPos;
layout(location = 3) out vec2 fragSize;

void main()
{
    vec2 pixelPosition =
        pc.position +
        (inPosition + vec2(0.5)) * pc.size;

    vec2 ndc;
    ndc.x = (pixelPosition.x / pc.viewportSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (pixelPosition.y / pc.viewportSize.y) * 2.0;

    gl_Position = vec4(ndc, 0.0, 1.0);

    vec2 localPos = (inPosition + vec2(0.5)) * pc.size;

    fragLocalPos = localPos;
    fragRounding = pc.rounding;
    fragColor = pc.color; //pc.color;
    fragSize = pc.size;
}