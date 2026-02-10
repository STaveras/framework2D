#version 450

// Sampler for the sprite texture
layout(binding = 0) uniform sampler2D spriteTexture;

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord; // Texture coordinate
layout(location = 1) in vec4 fragColor;    // Fragment color

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample the texture using the texture coordinate
    vec4 textureColor = texture(spriteTexture, fragTexCoord);

    // Multiply the texture color with the vertex color (for tinting or blending effects)
    outColor = textureColor * fragColor;
}
