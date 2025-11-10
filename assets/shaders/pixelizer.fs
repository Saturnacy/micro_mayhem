#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float pixelSize;
uniform vec2 renderSize;

void main() {
    vec2 pos = vec2(
        floor(fragTexCoord.x * renderSize.x / pixelSize) * pixelSize + (pixelSize / 2.0),
        floor(fragTexCoord.y * renderSize.y / pixelSize) * pixelSize + (pixelSize / 2.0)
    );
    
    finalColor = texture(texture0, pos / renderSize);
}