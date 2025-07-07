#version 330 core

// Inputs from vertex shader
in vec4 position_world;
in vec4 normal;
in vec4 position_model;
in vec2 texcoords;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int texture_index_uniform;
uniform int object_id;
uniform vec4 bbox_min;
uniform vec4 bbox_max;
uniform sampler2D TextureImage[16]; // Keep the sampler array

// Output color
out vec4 color;

// Constants
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

// Object IDs
#define SPHERE 0
#define PLANE  1
#define TABLE  2
#define LINE   3

void main()
{
    // Common lighting variables
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;
    vec4 p = position_world;
    vec4 n = normalize(normal);
    vec4 l = normalize(vec4(1.0, 1.0, 0.0, 0.0));
    vec4 v = normalize(camera_position - p);

    vec3 Kd0;

    if (object_id == SPHERE)
    {
        // Compute spherical UV coordinates
        vec3 bbox_center = ((bbox_min + bbox_max) * 0.5).xyz;
        vec3 p = position_model.xyz;
        vec3 p_rel = p - bbox_center;
        float rho = length(p_rel);
        float phi = asin(p_rel.y / rho);
        float theta = atan(p_rel.x, p_rel.z);
        float U = 0.5 + theta / (2.0 * M_PI);
        float V = 0.5 + phi / M_PI;

        // Use switch for texture selection
        switch (texture_index_uniform) {
            case 0: // White sphere (Cue Ball)
                Kd0 = vec3(1.0, 1.0, 1.0); // Solid white color
                break;
            case 1:
                Kd0 = texture(TextureImage[1], vec2(U, V)).rgb;
                break;
            case 2:
                Kd0 = texture(TextureImage[2], vec2(U, V)).rgb;
                break;
            case 3:
                Kd0 = texture(TextureImage[3], vec2(U, V)).rgb;
                break;
            case 4:
                Kd0 = texture(TextureImage[4], vec2(U, V)).rgb;
                break;
            case 5:
                Kd0 = texture(TextureImage[5], vec2(U, V)).rgb;
                break;
            case 6:
                Kd0 = texture(TextureImage[6], vec2(U, V)).rgb;
                break;
            case 7:
                Kd0 = texture(TextureImage[7], vec2(U, V)).rgb;
                break;
            case 8:
                Kd0 = texture(TextureImage[8], vec2(U, V)).rgb;
                break;
            case 9:
                Kd0 = texture(TextureImage[9], vec2(U, V)).rgb;
                break;
            case 10:
                Kd0 = texture(TextureImage[10], vec2(U, V)).rgb;
                break;
            case 11:
                Kd0 = texture(TextureImage[11], vec2(U, V)).rgb;
                break;
            case 12:
                Kd0 = texture(TextureImage[12], vec2(U, V)).rgb;
                break;
            case 13:
                Kd0 = texture(TextureImage[13], vec2(U, V)).rgb;
                break;
            case 14:
                Kd0 = texture(TextureImage[14], vec2(U, V)).rgb;
                break;
            case 15:
                Kd0 = texture(TextureImage[15], vec2(U, V)).rgb;
                break;
            default:
                Kd0 = vec3(1.0, 0.5, 0.0); // Orange for debugging
                break;
        }
    }
    else if (object_id == PLANE)
    {
        Kd0 = vec3(0.5, 0.5, 0.5); // Solid gray color for the plane
    }
    else if (object_id == LINE)
    {
        Kd0 = vec3(1.0, 1.0, 0.0); // Solid yellow color for lines
    }
    else if (object_id == TABLE)
    {
        Kd0 = texture(TextureImage[0], texcoords).rgb; // Use texcoords for table texture
        vec3 Ks = vec3(0.05);          // Specular
    }
    else
    {
        Kd0 = vec3(1.0, 0.0, 0.0); // Fallback: red color for unknown objects
    }

    // Lighting calculations
    vec3 Kd = Kd0;                // Diffuse
    vec3 Ks = vec3(0.05);          // Specular
    float shininess = 64.0;       // Phong exponent
    vec3 Ia = vec3(1.0);          // Ambient light intensity
    vec3 ambient = 0.1 * Kd;      // Ambient term
    float lambert = max(dot(n.xyz, l.xyz), 0.0); // Diffuse term
    vec3 Id = vec3(1.0);          // Diffuse light intensity
    vec3 diffuse = lambert * Kd * Id;
// assim seria Phon apenas
//    vec3 r = reflect(-l.xyz, n.xyz); // Reflected vector
//  float spec = pow(max(dot(v.xyz, r), 0.0), shininess);
    // assim é Blinn-Phong
    vec3 h = normalize(l.xyz + v.xyz); // Half-vector
    float spec = pow(max(dot(n.xyz, h), 0.0), shininess);

    vec3 Is = vec3(1.0);          // Specular light intensity
    vec3 specular = Ks * spec * Is;

    // Final color
    color.rgb = ambient + diffuse + specular;
    color.a = 1.0;

    // Gamma correction for sRGB
    color.rgb = pow(color.rgb, vec3(1.0 / 2.2));
}