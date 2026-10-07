#version 450

// Sprite quads arrive in world or screen pixels; RendererVK pushes the
// matrix that maps the current render list (camera, parallax) to clip space.
layout(push_constant) uniform Transform {
    mat4 viewProjection;
};

// Vertex input attributes
layout(location = 0) in vec2 inPosition; // Vertex position in pixels
layout(location = 1) in vec2 inTexCoord; // Texture coordinate
layout(location = 2) in vec4 inColor;    // Vertex color (tint)

// Outputs to fragment shader
layout(location = 0) out vec2 fragTexCoord; // Texture coordinate
layout(location = 1) out vec4 fragColor;    // Fragment color

void main() {
    gl_Position = viewProjection * vec4(inPosition, 0.0, 1.0);

    // Pass texture coordinate and color to fragment shader
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
