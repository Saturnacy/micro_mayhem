#version 330

uniform vec2 resolution;
uniform vec3 colorCenter;
uniform vec3 colorEdge;

out vec4 fragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uv, center);
    
    float t = smoothstep(0.0, 0.5, dist);

    vec3 color = mix(colorCenter, colorEdge, t);
    fragColor = vec4(color, 1.0);
}