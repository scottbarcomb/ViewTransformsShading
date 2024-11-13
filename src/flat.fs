#version 450 core
in vec3 FragPos;
flat in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
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
    FragColor = vec4(result, 1.0);
}