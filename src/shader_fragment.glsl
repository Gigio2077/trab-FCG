#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define PLANE  1
#define TABLE  2
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage[16]; // 15 bolas + 1 opcional


// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

vec3 Kd0;

if (object_id == SPHERE){
    vec3 bbox_center = ((bbox_min + bbox_max) * 0.5).xyz;
    vec3 p = position_model.xyz;

    vec3 p_rel = p - bbox_center;

    // p_rep eh o ponto relativo ao centro da esfera
    // e rho eh o raio da esfera
    float rho = length(p_rel);

    // usamos o ponto relativo e o raio para determinar os angulos phi e theta
    // fazendo a inversa da fórmula aprendida em aula:
    // 𝐩𝑥 = 𝜌 cos𝜑 sin 𝜃
    // 𝐩𝑦 = 𝜌 sin 𝜑
    // 𝐩𝑧 = 𝜌 cos𝜑 cos 𝜃

    // 𝐩𝑦 = 𝜌 sin 𝜑 -> 𝜑 = asin( 𝐩𝑦 / 𝜌 )
    float phi = asin(p_rel.y / rho);

    // cálculo de θ usando a função atan2 para obter o ângulo correto no plano xz
    float theta = atan(p_rel.x, p_rel.z);

    U = 0.5 + theta / (2.0 * M_PI);
    V = 0.5 + phi / M_PI;
        
    Kd0 = texture(TextureImage[15], vec2(U,V)).rgb;
}
else if (object_id == TABLE)
{
    Kd0 = texture(TextureImage[0], texcoords).rgb;
}
else // bunny ou outros
{
    Kd0 = texture(TextureImage[0], vec2(U,V)).rgb;
}
    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    //vec3 Kd0 = texture(TextureImage3, vec2(U,V)).rgb;

    // Leitura das luzes noturnas
    //vec3 cityLights = texture(TextureImage1, vec2(U,V)).rgb;

    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

  
    // Pequeno termo ambiente para o dia para não ficar totalmente preto
    float ambient = 0.01;

    // Cor final
    color.rgb =  Kd0 ;
    
    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 

