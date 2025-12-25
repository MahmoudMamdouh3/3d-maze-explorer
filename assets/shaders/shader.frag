#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec3 viewPos;
uniform float batteryRatio;
uniform float flicker;
uniform bool isUnlit;

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


    if (isUnlit) {
        FragColor = texColor;

        float dist = length(viewPos - FragPos);
        float dens = 0.09;
        float fog = 1.0 / exp(dist * dist * dens * dens);
        FragColor = mix(vec4(0.0, 0.0, 0.0, 1.0), FragColor, clamp(fog, 0.0, 1.0));
        return;
    }


    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(spotLight.position - FragPos);


    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);


    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));


    float powerFactor = clamp(batteryRatio, 0.0, 1.0);
    if (batteryRatio < 0.2) powerFactor *= flicker;


    vec3 ambient = spotLight.ambient * texColor.rgb;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = spotLight.diffuse * diff * texColor.rgb;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spotLight.specular * spec * texColor.rgb;

    vec3 result = ambient + (diffuse + specular) * intensity * attenuation * powerFactor;




    float fogDistance = length(viewPos - FragPos);
    float fogDensity = 0.09;
    float fogFactor = 1.0 / exp(fogDistance * fogDistance * fogDensity * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);


    vec3 atmosphereColor = vec3(0.005, 0.005, 0.01);

    FragColor = vec4(mix(atmosphereColor, result, fogFactor), 1.0);
}