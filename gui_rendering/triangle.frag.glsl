#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) flat in uint fragIdx;

layout(std140, set = 0, binding = 0) writeonly buffer clickBuffer {
    uint idxs[];
};

layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;

layout(push_constant) uniform constants {
	uvec2 windowSize;
};

void main() {
    outColor = vec4(fragColor, 1.0);

    uvec2 pixelPos = uvec2(floor(gl_FragCoord.xy));
    idxs[pixelPos.x * windowSize.y + pixelPos.y] = fragIdx;
}