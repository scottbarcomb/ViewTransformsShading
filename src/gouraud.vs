#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vec3 FragPos = vec3(model * vec4(aPos, 1.0)); // fragment position in world space
    vec3 Normal = mat3(transpose(inverse(model))) * aNormal;

    // ambient component calculation
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse component calculation
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, -lightDir), 0.0); // if dot product is negative, maximum is 0.0
    vec3 diffuse = diff * lightColor;

    // specular component calculation
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // exponent is the shininess value
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    Color = vec4(result, 1.0);
}


