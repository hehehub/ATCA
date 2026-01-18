#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

void main()
{
    // 简单的Phong光照
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-uLightDir);
    
    // 漫反射
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    
    // 环境光
    vec3 ambient = 0.3 * uLightColor;
    
    // 镜面反射
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.5;
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}

