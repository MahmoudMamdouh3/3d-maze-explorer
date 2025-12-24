#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec3 viewPos;
uniform float batteryRatio;
uniform float flicker;
uniform bool isUnlit; // NEW: If true, ignore lighting (for the Key/Badge)

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform SpotLight spotLight;

void main() {
    vec4 texColor = texture(texture1, TexCoord);

    // 1. GLOWING OBJECTS (The Key)
    // If it's the key, we skip lighting calc and just show the texture bright
    if (isUnlit) {
        FragColor = texColor;
        return;
    }

    // 2. STANDARD LIGHTING
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(spotLight.position - FragPos);

    // Soft Edge Spotlight
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

    // Distance Attenuation
    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));

    // Battery & Flicker
    float powerFactor = clamp(batteryRatio, 0.0, 1.0);
    if (batteryRatio < 0.2) {
        powerFactor *= flicker;
    }

    // DARKER AMBIENT: 0.002 is "Barely Seeable"
    vec3 ambient = vec3(0.002) * texColor.rgb;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = spotLight.diffuse * diff * texColor.rgb;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spotLight.specular * spec * texColor.rgb;

    vec3 result = ambient + (diffuse + specular) * intensity * attenuation * powerFactor;

    // FOG (Pitch Black Void)
    float fogStart = 1.0;
    float fogEnd = 8.0;
    float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = vec3(0.0); // Pure black fog

    FragColor = vec4(mix(result, fogColor, fogFactor), 1.0);
}