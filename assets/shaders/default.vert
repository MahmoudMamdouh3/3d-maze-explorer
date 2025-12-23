#version 330 core
layout (location = 0) in vec3 aPos;       // Vertex Position
layout (location = 1) in vec2 aTexCoord;  // Texture Coordinates
layout (location = 2) in vec3 aNormal;    // Normal Vectors for Lighting

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Calculate the fragment position in world space for lighting
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Transform normals to world space (handles scaling/rotation)
    Normal = mat3(transpose(inverse(model))) * aNormal;

    TexCoord = aTexCoord;

    // Final position on screen: Projection * View * Model * Position
    gl_Position = projection * view * vec4(FragPos, 1.0);
}