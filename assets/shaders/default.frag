#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture1; // Your wall or floor texture
uniform vec3 lightPos;      // Position of the light source
uniform vec3 viewPos;       // Camera/Player position
uniform vec3 lightColor;    // Color of the light (e.g., white)

void main() {
    // 1. Ambient Lighting (the base light in the maze)
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // 2. Diffuse Lighting (light based on the angle of the wall)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // 3. Specular Lighting (the "shine" on surfaces)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // 32 is shininess
    vec3 specular = specularStrength * spec * lightColor;

    // Sample the texture color
    vec4 texColor = texture(texture1, TexCoord);

    // Combine everything: (Ambient + Diffuse + Specular) * Texture
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}