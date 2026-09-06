#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragRounding;
layout(location = 2) in vec2 fragLocalPos;
layout(location = 3) in vec2 fragSize;

layout(location = 0) out vec4 outColor;

float roundedBoxSDF(vec2 centerPos, vec2 halfSize, float radius)
{
    vec2 q = abs(centerPos) - halfSize + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

void main()
{
    vec2 halfSize = fragSize * 0.5;
    vec2 centerPos = fragLocalPos - halfSize;

    float dist = roundedBoxSDF(centerPos, halfSize, fragRounding);

    float edgeSoftness = 0.5;
    float alpha = 1.0 - smoothstep(0.0, edgeSoftness, dist);

    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}