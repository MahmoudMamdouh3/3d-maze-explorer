#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float time;

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;



    vec2 uv = TexCoords;
    uv *=  1.0 - uv.yx;
    float vig = uv.x*uv.y * 15.0;
    vig = pow(vig, 0.2);
    color *= vec3(vig);


    float strength = 0.05;
    float x = (TexCoords.x + 4.0 ) * (TexCoords.y + 4.0 ) * (time * 10.0);
    vec4 grain = vec4(mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01)-0.005) * strength;
    color += grain.rgb;


    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));

    FragColor = vec4(color, 1.0);
}