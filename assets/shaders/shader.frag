#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec3 viewPos;
uniform float batteryRatio; // 1.0 = Full, 0.0 = Dead
uniform float flicker;      // Used for stuttering effect

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
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(spotLight.position - FragPos);

    // Spotlight Soft Edges
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

    // Distance Attenuation
    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));

    // Battery & Flicker Effect
    float powerFactor = clamp(batteryRatio, 0.0, 1.0);
    if (batteryRatio < 0.2) {
        // Stutter when low battery
        powerFactor *= flicker;
    }

    // Lighting Components
    vec3 ambient = spotLight.ambient * texture(texture1, TexCoord).rgb;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = spotLight.diffuse * diff * texture(texture1, TexCoord).rgb;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spotLight.specular * spec * texture(texture1, TexCoord).rgb;

    // Combine
    vec3 result = ambient + (diffuse + specular) * intensity * attenuation * powerFactor;

    // Kafkaesque Fog (Black Void)
    float fogStart = 0.5;
    float fogEnd = 7.0;
    float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = vec3(0.005, 0.005, 0.01);

    FragColor = vec4(mix(result, fogColor, fogFactor), 1.0);
}