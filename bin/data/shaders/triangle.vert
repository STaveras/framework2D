#version 450

// Vertex input attributes
layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec2 inTexCoord; // Texture coordinate
layout(location = 2) in vec4 inColor;    // Vertex color

// Uniform block for transformations
layout(binding = 0) uniform Transformations {
    mat4 modelMatrix; // Model transformation
    mat4 viewMatrix;  // View transformation (camera)
    mat4 projMatrix;  // Projection transformation
};

// Outputs to fragment shader
layout(location = 0) out vec2 fragTexCoord; // Texture coordinate
layout(location = 1) out vec4 fragColor;    // Fragment color

void main() {
    // Apply transformations to position the vertex
    vec4 worldPosition = modelMatrix * vec4(inPosition, 1.0);
    vec4 viewPosition = viewMatrix * worldPosition;
    gl_Position = projMatrix * viewPosition;

    // Pass texture coordinate and color to fragment shader
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
