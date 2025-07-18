//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//

#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "ObjModel.h"
#include "Colisoes.h"


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*, bool showAllInfo = false); // Função para debugging


// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);
void TextRendering_ShowShotPower(GLFWwindow* window);
void TextRendering_ShowMenu(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Estrutura para representar uma bola no jogo, com propriedades básicas.
// Usada aqui para depurar o movimento da bola.
// struct GameBall {
//         glm::vec3 position;
//         glm::vec3 velocity;
//         float radius;
//         bool  active;
//         std::string object_name;
//         int   object_id;
//         int texture_unit_index;
//         int   shader_object_id;
//         glm::quat orientation;
//         glm::vec3 angular_velocity;
//     };


// Variável global para armazenar todas as bolas do jogo
std::vector<GameBall> g_Balls;

// Tamanho do passo para o movimento fixo da bola (em unidades do mundo virtual)
float g_BallStepSize = 0.02f; // <<=== Comece com 0.1. Ajuste este valor conforme sua escala.

GameBall g_DebugBall;

// Abaixo definimos variáveis globais utilizadas em várias funções do código.
// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.785f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 2.0f; // Distância da câmera para a origem

// Variáveis para guardar o estado da câmera fixa ao entrar no modo livre
float g_FixedCamRestoreDistance = 3.5f; // Inicialize com um valor padrão ou o valor inicial de g_CameraDistance
float g_FixedCamRestorePhi = 0.0f;
float g_FixedCamRestoreTheta = 0.0f;


// Variáveis para o sistema de mira (linha guia)
float g_AimingAngle = 0.0f; // Ângulo da linha guia no plano XZ
bool  g_AimingMode = false; // true se o modo de mira está ativo


const float g_AimingLineLength = 1.0f; // Comprimento da linha guia (ajuste visualmente)
float g_AimingLineThickness = 0.01f; // Espessura da linha guia (ajuste visualmente)

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variável que controla se o modo de câmera livre está ativo
// bool g_FreeLookMode = false;

// Agora há 3 estados de câmera, freeLook, look_at e menu, e
// Camera mode enum
enum CameraMode {
    LOOKAT_WHITE_BALL, // Look at white ball (fixed mode)
    FREE_CAMERA,       // Free camera with WASD and mouse
    BEZIER             // Bézier curve for game menu
};
// Estou instanciando fora da main para que não precise passar como parametro para as respectivas funcoes
CameraMode g_CameraMode = BEZIER;


// A simple grid structure to store ball indices
std::vector<std::vector<std::vector<size_t>>> spatialGrid;

bool g_CueBallPositioningMode = false;

// Altura fixa em Y para o CENTRO das bolas quando elas estão apoiadas na mesa.
const float BALL_Y_AXIS = -0.2667f; // Valor fornecido pelo usuário.
const float BALL_VIRTUAL_RADIUS = 0.02625f;


const float TABLE_WIDTH = 1.04150000f ;
const float TABLE_DEPTH = 2.28450000f ;
const float TABLE_HALF_WIDTH = TABLE_WIDTH/2;
const float TABLE_HALF_DEPTH = TABLE_DEPTH/2;

const float GRID_CELL_SIZE = BALL_VIRTUAL_RADIUS * 4.0f; // Example: 4 times ball radius
const int GRID_COLS = static_cast<int>(TABLE_WIDTH / GRID_CELL_SIZE) + 1;
const int GRID_ROWS = static_cast<int>(TABLE_DEPTH / GRID_CELL_SIZE) + 1;

// Raio das esferas que representam as caçapas (para visualização e colisão)
const float POCKET_SPHERE_RADIUS = 0.1f;


// Constantes para o posicionamento das bolas no rack triangular
const float RACK_TIP_Z_COORD = -0.60f; // Posição Z do centro da bola na ponta do triângulo (ajuste conforme necessário)
const float BALL_DIAMETER = BALL_VIRTUAL_RADIUS * 2.0f; // Diâmetro da bola
// Distância vertical entre os centros das bolas em linhas adjacentes (para um rack apertado)
const float RACK_ROW_Z_OFFSET = BALL_DIAMETER * glm::sqrt(3.0f) / 2.0f;
// Offset horizontal para o início de cada nova linha do rack
const float RACK_ROW_X_OFFSET = BALL_DIAMETER / 2.0f;


const float TABLE_X_MAX_BALL_CENTER = 0.52025000f; // Exemplo de valor obtido
const float TABLE_X_MIN_BALL_CENTER = -0.52125000f; // Exemplo de valor obtido
const float TABLE_Z_MIN_BALL_CENTER = -1.14725000f; // Exemplo de valor obtido
const float TABLE_Z_MAX_BALL_CENTER = 1.13725000f; // Exemplo de valor obtido


// A altura da superfície do feltro da mesa será o centro da bola menos o raio da bola.
const float FELT_SURFACE_Y_ACTUAL = BALL_Y_AXIS - BALL_VIRTUAL_RADIUS;

// Constantes físicas
const float GRAVITY = 9.8f; // Gravidade (em unidades/s^2, se unidades são metros, 9.8 m/s^2)
const float RESTITUTION_COEFF = 0.8f; // Coeficiente de restituição (0.0 para sem quique, 1.0 para quique perfeito)
const float BALL_FRICTION_FACTOR = 0.99f; // Fator de atrito para desacelerar a bola (por frame)
const float COLLISION_EPSILON = 0.001f; // Pequeno valor para evitar problemas de "colar" na parede.
const float VELOCITY_STOP_THRESHOLD = 0.01f; // Limiar para zerar velocidade quando muito baixa.



const int PHYSICS_SUBSTEPS = 5; // Experiment with this value
float fixedDeltaTime = 1.0f / 60.0f; // Target 60 physics updates per second, for example
float subDeltaTime = fixedDeltaTime / PHYSICS_SUBSTEPS;

const float FIXED_PHYSICS_DELTA_TIME = 1.0f / 120.0f; // Target 120 physics updates per second


// Variáveis para o sistema de barra de força (Power Shot)
bool g_P_KeyHeld = false; // true se a tecla 'P' está sendo mantida pressionada
double g_P_PressStartTime = 0.0; // Tempo em que a tecla 'P' foi pressionada pela primeira vez
float g_CurrentShotPowerPercentage = 0.0f; // Força atual da tacada em porcentagem (0.0 a 100.0)
float g_MaxShotChargeTime = 5.0f; // Tempo (em segundos) para carregar 100% da força
float g_ShotPowerPingPongDirection = 1.0f; // Direção do "ping-pong": 1.0 para carregando (0->100), -1.0 para descarregando (100->0)

// Constantes para o mapeamento da porcentagem para a força real aplicada
const float g_MinShotPowerMagnitude = 0.50f; // Força mínima do impulso (mesmo para um toque rápido)
const float g_MaxShotPowerMagnitude = 12.0f; // Força máxima do impulso



// Limite máximo para a distância da câmera no modo normal (zoom out)
const float MAX_CAMERA_DISTANCE = 5.0f;


// // Estrutura para definir um segmento de reta da tabela
// struct BoundingSegment {
//     glm::vec3 p1; // Ponto inicial do segmento
//     glm::vec3 p2; // Ponto final do segmento
// };


// // Estrutura para representar uma caçapa no jogo
// struct Pocket {
//     glm::vec3 position; // Posição do centro da caçapa no mundo virtual
//     float radius;       // Raio da caçapa (para detecção de colisão)
//     // Você pode adicionar um ID ou nome aqui se quiser renderizá-las depois.
// };

// Segmentos de reta que formam as tabelas internas da mesa (onde a bola colide).
// As coordenadas são o centro da bola quando em contato com a tabela.
std::vector<BoundingSegment> g_TableSegments;

std::vector<BoundingSegment> g_PocketEntrySegments;

std::vector<Pocket> g_Pockets; // Variável global para armazenar todas as caçapas da mesa


#define SPHERE 0
#define PLANE  1
#define TABLE  2
#define LINE   3

// Variáveis para a posição da câmera no modo livre
// Inicie com valores que façam sentido para o seu cenário (ex: acima do plano da mesa)
glm::vec4 g_FreeCameraPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
glm::vec4 g_FreeCameraStartPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
// Velocidade de movimento da câmera em cm/s (ajuste este valor para o que parecer natural)
float g_CameraSpeed = 2.0f; //



// Flags para as teclas WASD (true se a tecla está pressionada)
bool g_W_Pressed = false;
bool g_A_Pressed = false;
bool g_S_Pressed = false;
bool g_D_Pressed = false;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_texture_index_uniform;

GLuint lineVAO;
GLuint lineVBO;
GLuint lineEBO;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;


// Calculate a point on a cubic Bézier curve for 2D (X, Z)
glm::vec2 CalculateBezierPoint(float t, const glm::vec2& p0, const glm::vec2& p1,
                              const glm::vec2& p2, const glm::vec2& p3)
{
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec2 point = uuu * p0;
    point += 3.0f * uu * t * p1;
    point += 3.0f * u * tt * p2;
    point += ttt * p3;

    return point;
}

void updateSpatialGrid(const std::vector<GameBall>& balls) {
    // Clear the grid
    spatialGrid.assign(GRID_COLS, std::vector<std::vector<size_t>>(GRID_ROWS));

    for (size_t i = 0; i < balls.size(); ++i) {
        if (!balls[i].active) continue;

        // Calculate grid cell for the ball's position
        // You'll need to map your table coordinates to grid indices (e.g., 0 to GRID_COLS-1)
        int col = static_cast<int>((balls[i].position.x + TABLE_HALF_WIDTH) / GRID_CELL_SIZE);
        int row = static_cast<int>((balls[i].position.z + TABLE_HALF_DEPTH) / GRID_CELL_SIZE);

        // Clamp to ensure indices are within bounds
        col = glm::clamp(col, 0, GRID_COLS - 1);
        row = glm::clamp(row, 0, GRID_ROWS - 1);

        spatialGrid[col][row].push_back(i);
    }
}

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Sinuca Simulator- v1.0", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Limita o FPS à taxa de atualização do monitor (ativa o V-Sync)
    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    int initial_framebuffer_width, initial_framebuffer_height;
    glfwGetFramebufferSize(window, &initial_framebuffer_width, &initial_framebuffer_height);
    // Agora, chamamos o callback com as dimensões REAIS obtidas.
    FramebufferSizeCallback(window, initial_framebuffer_width, initial_framebuffer_height);


    // Inicialmente, definimos um quadrado 1x1 no plano XZ, centrado em (0,0)
    // Estes são os 4 vértices que formarão o retângulo da linha (serão atualizados dinamicamente)
    float line_vertices_initial[] = {
        // Posição (X, Y, Z) - Y será a altura da bola
        -0.5f, 0.0f, -0.5f,  // Vértice 0 (baixo-esquerda)
        0.5f, 0.0f, -0.5f,  // Vértice 1 (baixo-direita)
        0.5f, 0.0f,  0.5f,  // Vértice 2 (cima-direita)
        -0.5f, 0.0f,  0.5f   // Vértice 3 (cima-esquerda)
    };
    // Índices para formar dois triângulos (um retângulo)
    GLuint line_indices_initial[] = {
        0, 1, 2, // Primeiro triângulo
        0, 2, 3  // Segundo triângulo
    };

    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glGenBuffers(1, &lineEBO);

    glBindVertexArray(lineVAO);


    // VBO para os vértices da linha
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices_initial), line_vertices_initial, GL_STATIC_DRAW);

    // EBO para os índices da linha
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineEBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(line_indices_initial), line_indices_initial, GL_STATIC_DRAW);

    // Supondo que o layout location 0 seja a posição (vec3 a_position no vertex shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); // 3 componentes (X,Y,Z)
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); // Desligar o VAO para evitar modificações acidentais


    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/10523_Pool_Table_v1_Diffuse.jpg");

    //Carregar as texturas das 15 bolas
    for (int i = 1; i < 16; i++)
    {
        std::string filename = "../../data/balls_textures/" + std::to_string(i) + ".jpg";
        LoadTextureImage(filename.c_str());
    }

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel tablemodel("../../data/10523_Pool_Table_v1_L3.obj");
    ComputeNormals(&tablemodel);
    BuildTrianglesAndAddToVirtualScene(&tablemodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }


    // 1. Inicializa o vetor global de bolas
    g_Balls.clear(); // Limpa o vetor se ele já contiver algo

    // 2. Bola Branca (Cue Ball)
    GameBall cueBall;
    cueBall.radius = BALL_VIRTUAL_RADIUS;
    cueBall.position = glm::vec3(-0.0020f, BALL_Y_AXIS, 0.5680f); // Posição inicial da bola branca
    cueBall.angular_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    cueBall.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cueBall.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    cueBall.active = true;
    cueBall.object_name = "the_sphere";
    cueBall.shader_object_id = SPHERE;
    cueBall.texture_unit_index = 0; // Unidade de textura para a bola branca
    g_Balls.push_back(cueBall);

    // === INICIALIZAÇÃO DA BOLA DE DEPURACAO (Temporariamente ÚNICA)
    g_DebugBall.radius = 0.1; // Usa a constante de raio que já existe
    g_DebugBall.position = glm::vec3(-0.03f, BALL_Y_AXIS, 0.5680f); // Posição inicial para começar o debug.
    g_DebugBall.velocity = glm::vec3(0.0f, 0.0f, 0.0f); // Velocidade inicial zero (não será usada na física manual)
    g_DebugBall.active = false;
    g_DebugBall.object_name = "the_sphere";
    g_DebugBall.shader_object_id = SPHERE;
    g_DebugBall.texture_unit_index = 0;

    // === INICIALIZAÇÃO DAS BOLAS NUMERADAS (OBJECT BALLS) NO RACK ===
    int ball_id_counter = 1; // Começa de 1 (para Bola 1, Bola 2, ..., Bola 15)

    for (int row = 0; row < 5; ++row) // O rack triangular tem 5 linhas
    {
        for (int col = 0; col <= row; ++col) // Número de bolas em cada linha (1, 2, 3, 4, 5)
        {
            if (ball_id_counter > 15) break; // Garante que não adicionamos mais de 15 bolas

            GameBall objectBall;
            objectBall.radius = BALL_VIRTUAL_RADIUS;
            objectBall.velocity = glm::vec3(0.0f, 0.0f, 0.0f); // Começa parada
            objectBall.angular_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            objectBall.active = true;
            objectBall.object_name = "the_sphere";
            objectBall.shader_object_id = SPHERE; // Todas as bolas são modelos SPHERE

            // Calcula a posição Z (profundidade) da bola na linha atual do rack
            float current_z = RACK_TIP_Z_COORD - (float)row * RACK_ROW_Z_OFFSET;
            // Calcula a posição X (horizontal) da bola na linha atual do rack
            float current_x = (float)col * BALL_DIAMETER - (float)row * RACK_ROW_X_OFFSET;

            objectBall.position = glm::vec3(current_x, BALL_Y_AXIS, current_z);

            // Atribui o ID da textura: Bola 1 usa TextureImage[1], Bola 2 usa TextureImage[2], etc.
            objectBall.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            objectBall.texture_unit_index = ball_id_counter; // <<=== TEXTURA CORRETA
            g_Balls.push_back(objectBall);

            ball_id_counter++; // Incrementa para a próxima bola numerada
        }
        if (ball_id_counter > 15) break; // Se todas as 15 bolas já foram adicionadas, sai do loop externo também
    }

    // === INICIALIZAÇÃO DOS SEGMENTOS DE TABELA ===
    // Segmento 1
    g_TableSegments.push_back({glm::vec3(TABLE_X_MAX_BALL_CENTER, BALL_Y_AXIS, -0.0730f), glm::vec3(TABLE_X_MAX_BALL_CENTER, BALL_Y_AXIS, -1.0480f)});
    // Segmento 2
    g_TableSegments.push_back({glm::vec3(0.4310f, BALL_Y_AXIS, TABLE_Z_MIN_BALL_CENTER), glm::vec3(-0.4400f, BALL_Y_AXIS, TABLE_Z_MIN_BALL_CENTER)});
    // Segmento 3
    g_TableSegments.push_back({glm::vec3(TABLE_X_MIN_BALL_CENTER, BALL_Y_AXIS, -1.0470f), glm::vec3(TABLE_X_MIN_BALL_CENTER, BALL_Y_AXIS, -0.0730f)});
    // Segmento 4
    g_TableSegments.push_back({glm::vec3(TABLE_X_MIN_BALL_CENTER, BALL_Y_AXIS, 0.0760f), glm::vec3(TABLE_X_MIN_BALL_CENTER, BALL_Y_AXIS, 1.0490f)});
    // Segmento 5
    g_TableSegments.push_back({glm::vec3(-0.4400f, BALL_Y_AXIS, TABLE_Z_MAX_BALL_CENTER), glm::vec3(0.4340f, BALL_Y_AXIS, TABLE_Z_MAX_BALL_CENTER)});
    // Segmento 6
    g_TableSegments.push_back({glm::vec3(TABLE_X_MAX_BALL_CENTER , BALL_Y_AXIS, 1.0520f), glm::vec3(TABLE_X_MAX_BALL_CENTER, BALL_Y_AXIS, 0.0770f)});


    // Caçapa Superior Esquerda
    g_PocketEntrySegments.push_back({glm::vec3(-0.5200f, BALL_Y_AXIS, 1.0530f), glm::vec3(-0.5500f, BALL_Y_AXIS, 1.0780f)});
    g_PocketEntrySegments.push_back({glm::vec3(-0.4400f, BALL_Y_AXIS, 1.1480f), glm::vec3(-0.4650f, BALL_Y_AXIS, 1.1730f)});

    // Caçapa Superior Direita
    g_PocketEntrySegments.push_back({glm::vec3(0.5200f, BALL_Y_AXIS, 1.0520f), glm::vec3(0.5480f, BALL_Y_AXIS, 1.0800f)});
    g_PocketEntrySegments.push_back({glm::vec3(0.4400f, BALL_Y_AXIS, 1.1480f), glm::vec3(0.4600f, BALL_Y_AXIS, 1.1700f)});

    // Caçapa Central Esquerda
    g_PocketEntrySegments.push_back({glm::vec3(-0.5540f, BALL_Y_AXIS, 0.0600f), glm::vec3(-0.5200f, BALL_Y_AXIS, 0.0720f)});
    g_PocketEntrySegments.push_back({glm::vec3(-0.5200f, BALL_Y_AXIS, -0.0700f), glm::vec3(-0.5460f, BALL_Y_AXIS, -0.0620f)});

    // Caçapa Central Direita
    g_PocketEntrySegments.push_back({glm::vec3(0.5180f, BALL_Y_AXIS, 0.0740f), glm::vec3(0.5440f, BALL_Y_AXIS, 0.0620f)});
    g_PocketEntrySegments.push_back({glm::vec3(0.5200f, BALL_Y_AXIS, -0.0720f), glm::vec3(0.5480f, BALL_Y_AXIS, -0.0600f)});

    // Caçapa Inferior Esquerda
    g_PocketEntrySegments.push_back({glm::vec3(-0.5200f, BALL_Y_AXIS, -1.0500f), glm::vec3(-0.5480f, BALL_Y_AXIS, -1.0780f)});
    g_PocketEntrySegments.push_back({glm::vec3(-0.4400f, BALL_Y_AXIS, -1.1480f), glm::vec3(-0.4640f, BALL_Y_AXIS, -1.1740f)});

    // Caçapa Inferior Direita
    g_PocketEntrySegments.push_back({glm::vec3(0.4380f, BALL_Y_AXIS, -1.1480f), glm::vec3(0.4640f, BALL_Y_AXIS, -1.1740f)});
    g_PocketEntrySegments.push_back({glm::vec3(0.5200f, BALL_Y_AXIS, -1.0540f), glm::vec3(0.5480f, BALL_Y_AXIS, -1.0800f)});
    // ========================================================


    // As 6 caçapas com raio 0.1 e BALL_Y_AXIS como coordenada Y
    g_Pockets.push_back({glm::vec3(0.5500f, BALL_Y_AXIS, 1.1900f), POCKET_SPHERE_RADIUS});
    g_Pockets.push_back({glm::vec3(-0.5500f, BALL_Y_AXIS, 1.1900f), POCKET_SPHERE_RADIUS});
    g_Pockets.push_back({glm::vec3(-0.6300f, BALL_Y_AXIS, 0.0000f), POCKET_SPHERE_RADIUS});
    g_Pockets.push_back({glm::vec3(0.6300f, BALL_Y_AXIS, 0.0000f), POCKET_SPHERE_RADIUS});
    g_Pockets.push_back({glm::vec3(-0.5740f, BALL_Y_AXIS, -1.1860f), POCKET_SPHERE_RADIUS});
    g_Pockets.push_back({glm::vec3(0.5700f, BALL_Y_AXIS, -1.1860f), POCKET_SPHERE_RADIUS});
    // ================================

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    GLint maxTextures;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextures);
    //printf("\n\n\nLimite máximo de texturas: %d \n ", maxTextures);

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);


        // === CÁLCULO DE DELTATIME ===
        static double lastFrameTime = glfwGetTime();
        double currentFrameTime = glfwGetTime();
        float deltaTime = (float)(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;
        //fprintf(stdout, "DEBUG: DeltaTime: %.4f\n", deltaTime); // Para depuração, se necessário
        // Accumulator for fixed time steps
        static float physics_accumulator = 0.0f;
        physics_accumulator += deltaTime; // deltaTime from your existing calculation
        SimularColisoes(deltaTime, g_Balls, g_TableSegments, g_PocketEntrySegments, g_Pockets, spatialGrid, g_CueBallPositioningMode);



        // // Loop para garantir que a física seja atualizada em passos de tempo fixos
        // while (physics_accumulator >= FIXED_PHYSICS_DELTA_TIME)
        // {
        //     // Update the spatial grid once per physics step with the current ball positions
        //     // This must happen BEFORE any ball-to-ball checks using the grid.
        //     updateSpatialGrid(g_Balls);

        //     // === LÓGICA DE FÍSICA PARA TODAS AS BOLAS ===
        //     for (size_t i = 0; i < g_Balls.size(); ++i)
        //     {
        //         GameBall& ball_A = g_Balls[i]; // Bola atual (referenciada como ball_A)
        //         if (!ball_A.active) continue; // Pula bolas que não estão ativas (caíram na caçapa)

        //         // --- 1. Física Individual para ball_A (Gravidade, Atrito, Feltro) ---
        //         // IMPORTANT: Use FIXED_PHYSICS_DELTA_TIME here!
        //         ball_A.velocity.y -= GRAVITY * FIXED_PHYSICS_DELTA_TIME;
        //         ball_A.position += ball_A.velocity * FIXED_PHYSICS_DELTA_TIME;
        //         ball_A.velocity.x *= BALL_FRICTION_FACTOR;
        //         ball_A.velocity.z *= BALL_FRICTION_FACTOR;

        //         if (glm::length(glm::vec2(ball_A.velocity.x, ball_A.velocity.z)) < VELOCITY_STOP_THRESHOLD)
        //         {
        //             ball_A.velocity.x = 0.0f;
        //             ball_A.velocity.z = 0.0f;
        //         }
        //         // === CALCULAR VELOCIDADE ANGULAR E ATUALIZAR ORIENTAÇÃO (ROLLING)
        //         glm::vec3 linear_velocity_xz = glm::vec3(ball_A.velocity.x, 0.0f, ball_A.velocity.z);
        //         float linear_speed_xz = glm::length(linear_velocity_xz);
        //         if (linear_speed_xz > VELOCITY_STOP_THRESHOLD) // Só calcula rolamento se a bola estiver se movendo
        //         {
        //             glm::vec3 surface_normal = glm::vec3(0.0f, 1.0f, 0.0f);
        //             ball_A.angular_velocity = glm::cross(surface_normal, linear_velocity_xz) / ball_A.radius;

        //             glm::quat frame_rotation = glm::angleAxis(glm::length(ball_A.angular_velocity) * FIXED_PHYSICS_DELTA_TIME, glm::normalize(ball_A.angular_velocity));
        //             ball_A.orientation = glm::normalize(frame_rotation * ball_A.orientation);
        //         }
        //         else // Bola parada, zera velocidade angular e para de girar
        //         {
        //             ball_A.angular_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        //         }

        //         // Colisão com o Feltro da Mesa (Chão) para ball_A
        //         if (ball_A.position.y - ball_A.radius < FELT_SURFACE_Y_ACTUAL)
        //         {
        //             ball_A.position.y = FELT_SURFACE_Y_ACTUAL + ball_A.radius;
        //             ball_A.velocity.y *= -1.0f * RESTITUTION_COEFF;
        //             if (glm::abs(ball_A.velocity.y) < VELOCITY_STOP_THRESHOLD) {
        //                 ball_A.velocity.y = 0.0f;
        //                 ball_A.position.y = FELT_SURFACE_Y_ACTUAL + ball_A.radius;
        //             }
        //         }

        //         // === OPTIMIZED BALL-TO-BALL COLLISION TEST (using spatial grid) ===
        //         // This block replaces the previous inefficient nested loop for ball-to-ball collisions.
        //         // It iterates over neighboring cells from the spatial grid.
        //         int col_A = static_cast<int>((ball_A.position.x + TABLE_HALF_WIDTH) / GRID_CELL_SIZE);
        //         int row_A = static_cast<int>((ball_A.position.z + TABLE_HALF_DEPTH) / GRID_CELL_SIZE);

        //         col_A = glm::clamp(col_A, 0, GRID_COLS - 1);
        //         row_A = glm::clamp(row_A, 0, GRID_ROWS - 1);

        //         for (int dc = -1; dc <= 1; ++dc) {
        //             for (int dr = -1; dr <= 1; ++dr) {
        //                 int current_col = col_A + dc;
        //                 int current_row = row_A + dr;

        //                 // Ensure neighbor cell is within bounds
        //                 if (current_col >= 0 && current_col < GRID_COLS &&
        //                     current_row >= 0 && current_row < GRID_ROWS)
        //                 {
        //                     for (size_t j_idx : spatialGrid[current_col][current_row]) {
        //                         // Avoid checking ball against itself and avoid duplicate checks (i vs j, then j vs i)
        //                         if (j_idx <= i) continue;

        //                         GameBall& ball_B = g_Balls[j_idx]; //
        //                         if (!ball_B.active) continue; // Pula se a bola B não estiver ativa

        //                         // 1. DETECÇÃO DE COLISÃO
        //                         glm::vec3 distance_vector = ball_A.position - ball_B.position; // Vetor da bola B para a bola A
        //                         float distance = glm::length(distance_vector); // Distância entre os centros
        //                         float sum_of_radii = ball_A.radius + ball_B.radius; // Soma dos raios

        //                         if (distance < sum_of_radii) // Colisão detectada (penetração)!
        //                         {
        //                             // 2. RESOLUÇÃO DE POSIÇÃO (Evitar Penetração)
        //                             float penetration_depth = sum_of_radii - distance;
        //                             glm::vec3 separation_vector = glm::normalize(distance_vector);

        //                             ball_A.position += separation_vector * (penetration_depth / 2.0f);
        //                             ball_B.position -= separation_vector * (penetration_depth / 2.0f);

        //                             // 3. RESOLUÇÃO DE VELOCIDADE (Colisão Elástica/Inelástica)
        //                             glm::vec3 normal_collision_vector = glm::normalize(distance_vector);
        //                             glm::vec3 relative_velocity = ball_A.velocity - ball_B.velocity;
        //                             float velocity_along_normal = glm::dot(relative_velocity, normal_collision_vector);

        //                             if (velocity_along_normal > 0) // Se as bolas já estão se separando, ou não se movendo para colidir, não há nova colisão a resolver.
        //                                 continue;

        //                             float impulse_magnitude = (-(1.0f + RESTITUTION_COEFF) * velocity_along_normal) / (2.0f);
        //                             glm::vec3 impulse = impulse_magnitude * normal_collision_vector;

        //                             ball_A.velocity += impulse;
        //                             ball_B.velocity -= impulse;
        //                         }
        //                     }
        //                 }
        //             }
        //         }

        //         // TESTE COLISAO BOLA E ENTRADA DA CACAPA
        //         for (const auto& segment : g_PocketEntrySegments)
        //         {
        //             glm::vec2 segment_vec = glm::vec2(segment.p2.x - segment.p1.x, segment.p2.z - segment.p1.z);
        //             glm::vec2 ball_to_p1_vec = glm::vec2(ball_A.position.x - segment.p1.x, ball_A.position.z - segment.p1.z);

        //             float t = glm::dot(ball_to_p1_vec, segment_vec) / glm::dot(segment_vec, segment_vec);
        //             t = glm::clamp(t, 0.0f, 1.0f);

        //             glm::vec2 closest_point_on_segment = glm::vec2(segment.p1.x, segment.p1.z) + t * segment_vec;
        //             glm::vec2 normal_vec_2d = glm::vec2(ball_A.position.x, ball_A.position.z) - closest_point_on_segment;
        //             float distance = glm::length(normal_vec_2d);

        //             if (distance < ball_A.radius) // Colisão se distância < raio
        //             {
        //                 glm::vec2 collision_normal_2d = glm::normalize(normal_vec_2d);
        //                 float penetration_depth = ball_A.radius - distance;
        //                 ball_A.position.x += collision_normal_2d.x * penetration_depth;
        //                 ball_A.position.z += collision_normal_2d.y * penetration_depth;

        //                 glm::vec2 velocity_2d = glm::vec2(ball_A.velocity.x, ball_A.velocity.z);
        //                 float dot_product = glm::dot(velocity_2d, collision_normal_2d);

        //                 if (dot_product < 0) // Se movendo contra a parede
        //                 {
        //                     glm::vec2 reflected_velocity_2d = velocity_2d - (2.0f * dot_product * collision_normal_2d);
        //                     reflected_velocity_2d *= RESTITUTION_COEFF;

        //                     ball_A.velocity.x = reflected_velocity_2d.x;
        //                     ball_A.velocity.z = reflected_velocity_2d.y;
        //                 }
        //             }
        //         }

        //         //TESTE DE COLISÃO COM AS TABELAS
        //         for (const auto& segment : g_TableSegments)
        //         {
        //             glm::vec2 segment_vec = glm::vec2(segment.p2.x - segment.p1.x, segment.p2.z - segment.p1.z);
        //             glm::vec2 ball_to_p1_vec = glm::vec2(ball_A.position.x - segment.p1.x, ball_A.position.z - segment.p1.z);
        //             float t = glm::dot(ball_to_p1_vec, segment_vec) / glm::dot(segment_vec, segment_vec);
        //             t = glm::clamp(t, 0.0f, 1.0f);
        //             glm::vec2 closest_point_on_segment = glm::vec2(segment.p1.x, segment.p1.z) + t * segment_vec;
        //             glm::vec2 normal_vec_2d = glm::vec2(ball_A.position.x, ball_A.position.z) - closest_point_on_segment;
        //             float distance = glm::length(normal_vec_2d);

        //             if (distance < ball_A.radius)
        //             {
        //                 glm::vec2 collision_normal_2d = glm::normalize(normal_vec_2d);
        //                 float penetration_depth = ball_A.radius - distance;
        //                 ball_A.position.x += collision_normal_2d.x * penetration_depth;
        //                 ball_A.position.z += collision_normal_2d.y * penetration_depth;

        //                 glm::vec2 velocity_2d = glm::vec2(ball_A.velocity.x, ball_A.velocity.z);
        //                 float dot_product = glm::dot(velocity_2d, collision_normal_2d);

        //                 if (dot_product < 0)
        //                 {
        //                     glm::vec2 reflected_velocity_2d = velocity_2d - (2.0f * dot_product * collision_normal_2d);
        //                     reflected_velocity_2d *= RESTITUTION_COEFF;
        //                     ball_A.velocity.x = reflected_velocity_2d.x;
        //                     ball_A.velocity.z = reflected_velocity_2d.y;
        //                 }
        //             }
        //         }

        //         // === TESTE DE COLISÃO BOLA-CAÇAPA (esferas) ===
        //         for (const auto& pocket : g_Pockets)
        //         {
        //             float distance = glm::length(ball_A.position - pocket.position);
        //             if (distance <= (ball_A.radius + pocket.radius))
        //             {
        //                 if (ball_A.texture_unit_index == 0) // <<=== VERIFICA SE É A BOLA BRANCA
        //                 {
        //                     g_CueBallPositioningMode = true; // Entra no modo de posicionamento
        //                     ball_A.position = glm::vec3(-0.0020f, BALL_Y_AXIS, 0.5680f); // Posição de reposicionamento
        //                     ball_A.velocity = glm::vec3(0.0f, 0.0f, 0.0f); // Zera a velocidade
        //                     fprintf(stdout, "DEBUG: Bola branca encacapada! Entrando no modo de posicionamento.\n");
        //                 }
        //                 else // Se for qualquer outra bola (não a branca)
        //                 {
        //                     ball_A.active = false; // Desativa a bola
        //                     ball_A.position = glm::vec3(1000.0f, 1000.0f, 1000.0f); // Move para longe da cena
        //                     ball_A.velocity = glm::vec3(0.0f, 0.0f, 0.0f); // Zera a velocidade
        //                     fprintf(stdout, "DEBUG: Bola encacapada! Pos: (%.4f, %.4f, %.4f)\n", ball_A.position.x, ball_A.position.y, ball_A.position.z);
        //                 }
        //                 fflush(stdout);
        //                 break; // A bola já caiu, não precisa testar outras caçapas para esta bola.
        //             }
        //         }
        //     }
        //     physics_accumulator -= FIXED_PHYSICS_DELTA_TIME; // Decrement accumulator after each physics step
        // }


        if (g_P_KeyHeld)
        {
            // Calcula o progresso com base no tempo decorrido desde o início do pressionamento
            float time_elapsed_since_press = (float)(glfwGetTime() - g_P_PressStartTime);
            float progress_ratio = time_elapsed_since_press / g_MaxShotChargeTime;

            // Atualiza a porcentagem da força com o efeito "ping-pong"
            g_CurrentShotPowerPercentage += g_ShotPowerPingPongDirection * (100.0f / g_MaxShotChargeTime) * deltaTime;

            // Inverte a direção do ping-pong se atingir os limites
            if (g_CurrentShotPowerPercentage >= 100.0f) {
                g_CurrentShotPowerPercentage = 100.0f; // Garante que não ultrapasse 100%
                g_ShotPowerPingPongDirection = -1.0f; // Começa a descarregar
            } else if (g_CurrentShotPowerPercentage <= 0.0f) {
                g_CurrentShotPowerPercentage = 0.0f; // Garante que não vá abaixo de 0%
                g_ShotPowerPingPongDirection = 1.0f; // Começa a carregar novamente
            }
            //fprintf(stdout, "DEBUG: Forca: %.2f%%\n", g_CurrentShotPowerPercentage); fflush(stdout);
        }



        glm::vec4 camera_position_c;  // Ponto "c", centro da câmera
        glm::vec4 camera_lookat_l;    // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        glm::vec4 camera_view_vector; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eixo Y global)

        if (g_CameraMode == FREE_CAMERA) // Se o modo de câmera livre estiver ativado
        {
            camera_position_c = g_FreeCameraPosition; // Usa a posição atualizada pelo WASD

            // Calcula o vetor "forward" (para onde a câmera está olhando)
            // Isso usa g_CameraPhi e g_CameraTheta que são controlados pelo mouse
            glm::vec4 forward_dir = glm::vec4(cos(g_CameraPhi) * sin(g_CameraTheta),
                                            sin(g_CameraPhi),
                                            cos(g_CameraPhi) * cos(g_CameraTheta), 0.0f);
            forward_dir = glm::normalize(forward_dir);


              // Calcula o vetor "right" (para a direita da câmera)
            glm::vec4 global_up = glm::vec4(0.0f,1.0f,0.0f,0.0f);
            glm::vec4 right_dir = glm::normalize(glm::vec4(glm::cross(glm::vec3(forward_dir), glm::vec3(global_up)), 0.0f));


            // O ponto para onde a câmera "olha" é sua posição atual mais um pouco à frente
            camera_lookat_l = camera_position_c + forward_dir * 1.0f; // Olhe 1 unidade à frente

            // O vetor "view" da câmera é o próprio vetor 'forward_dir'.
            camera_view_vector = forward_dir;

                // === VERIFICAÇÃO DO MOVIMENTO WASD E ATUALIZAÇÃO DA POSIÇÃO ===
            if (g_W_Pressed) {
                g_FreeCameraPosition += forward_dir * g_CameraSpeed * deltaTime;
                //fprintf(stdout, "DEBUG: W ativado. Nova Posicao Cam Livre: (%.2f, %.2f, %.2f)\n", g_FreeCameraPosition.x, g_FreeCameraPosition.y, g_FreeCameraPosition.z); // <<=== DEBUG POSIÇÃO
            }
            if (g_S_Pressed) {
                g_FreeCameraPosition -= forward_dir * g_CameraSpeed * deltaTime;
                //fprintf(stdout, "DEBUG: S ativado. Nova Posicao Cam Livre: (%.2f, %.2f, %.2f)\n", g_FreeCameraPosition.x, g_FreeCameraPosition.y, g_FreeCameraPosition.z); // <<=== DEBUG POSIÇÃO
            }
            if (g_A_Pressed) {
                g_FreeCameraPosition -= right_dir * g_CameraSpeed * deltaTime;
                //fprintf(stdout, "DEBUG: A ativado. Nova Posicao Cam Livre: (%.2f, %.2f, %.2f)\n", g_FreeCameraPosition.x, g_FreeCameraPosition.y, g_FreeCameraPosition.z); // <<=== DEBUG POSIÇÃO
            }
            if (g_D_Pressed) {
                g_FreeCameraPosition += right_dir * g_CameraSpeed * deltaTime;
                //fprintf(stdout, "DEBUG: D ativado. Nova Posicao Cam Livre: (%.2f, %.2f, %.2f)\n", g_FreeCameraPosition.x, g_FreeCameraPosition.y, g_FreeCameraPosition.z); // <<=== DEBUG POSIÇÃO
            }
        }

        else if (g_CameraMode == LOOKAT_WHITE_BALL)// Modo de câmera normal (look-at na origem)
        {
            // O ponto para onde a câmera (look-at) estará sempre olhando: o centro da bola branca.
            // Acessamos a primeira bola do vetor g_Balls (assumindo que g_Balls[0] é a bola branca).
            // É importante verificar se g_Balls não está vazio para evitar erro de índice.
            if (!g_Balls.empty() && g_Balls[0].active) {
                camera_lookat_l = glm::vec4(g_Balls[0].position, 1.0f); // <<=== MUDANÇA CRUCIAL AQUI
            } else {
                // Fallback: Se a bola branca não existir ou estiver inativa, olhe para a origem.
                camera_lookat_l = glm::vec4(0.0f,0.0f,0.0f,1.0f);
            }

            // Calculamos a posição da câmera utilizando coordenadas esféricas,
            // AGORA RELATIVAS ao ponto camera_lookat_l.
            float r = g_CameraDistance;
            float y_rel = r*sin(g_CameraPhi); // Posição Y relativa ao alvo
            float z_rel = r*cos(g_CameraPhi)*cos(g_CameraTheta); // Posição Z relativa ao alvo
            float x_rel = r*cos(g_CameraPhi)*sin(g_CameraTheta); // Posição X relativa ao alvo

            // A posição final da câmera é a posição do alvo MAIS a posição relativa.
            camera_position_c = camera_lookat_l + glm::vec4(x_rel, y_rel, z_rel, 0.0f); // <<=== MUDANÇA CRUCIAL AQUI

            // O vetor "view" agora é (look_at - position), como sempre, mas com os novos pontos.
            camera_view_vector = camera_lookat_l - camera_position_c;
        }

        else if (g_CameraMode == BEZIER)
         {
            // Bézier curve for horizontal panning (X, Z) around table center
            static float t = 0.0f; // Progress along Bézier curve (0 to 1)
            t += 0.1f * deltaTime; // Complete curve in ~10 seconds
            if (t > 1.0f) t -= 1.0f; // Loop the path

            // Define Bézier control points (X, Z) for a near-circular path
            glm::vec2 p0(0.0f, 2.0f);    // Start at (0, 2)
            glm::vec2 p1(2.0f, 0.0f);    // Curve toward (2, 0)
            glm::vec2 p2(0.0f, -2.0f);   // Curve toward (0, -2)
            glm::vec2 p3(-2.0f, 0.0f);   // Curve toward (-2, 0)

            // Calculate X, Z using Bézier curve
            glm::vec2 xz = CalculateBezierPoint(t, p0, p1, p2, p3);

            // Use fixed phi to prevent Y flickering
            float fixedPhi = 0.785f; // Default value from your setup
            float y = g_CameraDistance * sin(fixedPhi); // Fixed Y coordinate (~1.414)

            // Set camera position
            camera_position_c = glm::vec4(xz.x, y, xz.y, 1.0f);

            // Set look-at point to table center
            camera_lookat_l = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            // Calculate view vector
            camera_view_vector = camera_lookat_l - camera_position_c;

            // Update theta for consistency (phi remains fixed)
            glm::vec3 direction = glm::normalize(glm::vec3(camera_lookat_l - camera_position_c));
            g_CameraTheta = atan2(direction.x, direction.z);
            g_CameraPhi = fixedPhi; // Keep phi fixed to avoid flipping

            // Print camera position for debugging
            // printf("Bézier Pan - Camera Position: (%.2f, %.2f, %.2f)\n",
            //     camera_position_c.x, camera_position_c.y, camera_position_c.z);
         }

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -10.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }
        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Desenhamos o modelo do plano
        model = Matrix_Translate(0.0f,-1.0f,0.0f)
              * Matrix_Rotate_Z(g_AngleZ)
              * Matrix_Rotate_Y(g_AngleY)
              * Matrix_Rotate_X(g_AngleX)
              * Matrix_Scale(2.0f, 1.0f, 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("the_plane");

        // Desenhamos o modelo da mesa
        model = Matrix_Translate(0.0f, -1.0f, 0.0f)
        * Matrix_Scale(0.01f, 0.01f, 0.01f)
        * Matrix_Rotate_X(-M_PI/2.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TABLE);
        DrawVirtualObject("10523_Pool_Table_v1_SG");


        // if (g_DebugBall.active)
        // {
        //     glm::mat4 model_debug_ball = Matrix_Translate(g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z)
        //                             * Matrix_Scale(g_DebugBall.radius, g_DebugBall.radius, g_DebugBall.radius)
        //                             * Matrix_Rotate_X(-M_PI/2.0f);
        //     glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model_debug_ball));
        //     glUniform1i(g_object_id_uniform, g_DebugBall.shader_object_id);
        //     glUniform1i(g_texture_index_uniform, g_DebugBall.texture_unit_index);
        //     DrawVirtualObject(g_DebugBall.object_name.c_str());
        // }

        // === DESENHAMOS TODAS AS BOLAS ===
        for (const auto& ball : g_Balls) 
        {

            if (!ball.active) continue; // Só desenha se a bola estiver ativa

            glm::mat4 ball_rotation_matrix = glm::toMat4(ball.orientation); // <<=== ADICIONE ESTA LINHA

            glm::mat4 model_ball = Matrix_Translate(ball.position.x, ball.position.y, ball.position.z)
                                * ball_rotation_matrix // <<=== ADICIONE ESTA LINHA (multiplica a rotação da bola)
                                * Matrix_Scale(ball.radius, ball.radius, ball.radius)
                                * Matrix_Rotate_X(-M_PI/2.0f); // Mantenha a rotação original do modelo OBJ se necessária
                                                                // A ordem importa: rotação do modelo OBJ primeiro, depois a rotação de rolamento.
                                                                // OU: rotação de rolamento, depois a rotação do OBJ. Depende do seu modelo.
                                                                // Experimente:
                                                                // * ball_rotation_matrix * Matrix_Scale(...) * Matrix_Rotate_X(-M_PI/2.0f);
                                                                // OU:
                                                                // * Matrix_Scale(...) * ball_rotation_matrix * Matrix_Rotate_X(-M_PI/2.0f);
                                                                // Se a rotação do modelo OBJ é para "levantá-lo", ela deve ser a última antes do Scale.
                                                                // Mantenha como está por enquanto, mas se a rotação parecer errada, inverta a ordem com ball_rotation_matrix.
                                                                // É mais comum: Translação * Rotação_do_Modelo * Rotação_do_Jogo * Escala.
                                                                // No seu caso: Translação * Rotação_do_Modelo_Original * Rotação_de_Rolamento * Escala
                                                                // Então, melhor seria:
                                                                // * Matrix_Rotate_X(-M_PI/2.0f) * ball_rotation_matrix * Matrix_Scale(...)
                                                                // Mas vamos manter a ordem mais simples e testar.
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model_ball));
            glUniform1i(g_object_id_uniform, ball.shader_object_id);
            glUniform1i(g_texture_index_uniform, ball.texture_unit_index);
            DrawVirtualObject(ball.object_name.c_str());
        }

        // === DESENHAR LINHA GUIA DE MIRA (se o modo de mira estiver ativo) ===
        if (g_AimingMode)
        {
            if (!g_Balls.empty() && g_Balls[0].active)
            {
                glm::vec3 cue_ball_pos = g_Balls[0].position; // Centro da bola branca

                // Direção da mira no plano XZ (normalizado)
                glm::vec3 aim_direction_xz = glm::normalize(glm::vec3(glm::sin(g_AimingAngle), 0.0f, glm::cos(g_AimingAngle)));
                // Vetor perpendicular à direção da mira no plano XZ, para definir a largura do plano
                glm::vec3 perpendicular_direction_xz = glm::vec3(-aim_direction_xz.z, 0.0f, aim_direction_xz.x); // Rotação 90 graus no Y

                // Calculamos os 4 vértices do plano fino (retângulo)
                // P1 e P2 são os pontos ao longo do comprimento da linha
                // Os vértices são deslocados por metade da espessura na direção perpendicular.
                glm::vec3 P1_start = cue_ball_pos;
                glm::vec3 P2_end = cue_ball_pos + aim_direction_xz * g_AimingLineLength; // Ponto final da linha

                // Vértices do retângulo da linha
                float current_line_vertices_data[] = {
                    // X, Y, Z
                    (P1_start - perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).x, P1_start.y, (P1_start - perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).z, // V0 (início, lado esquerdo)
                    (P1_start + perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).x, P1_start.y, (P1_start + perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).z, // V1 (início, lado direito)
                    (P2_end   + perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).x, P2_end.y,   (P2_end   + perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).z, // V2 (fim, lado direito)
                    (P2_end   - perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).x, P2_end.y,   (P2_end   - perpendicular_direction_xz * (g_AimingLineThickness / 2.0f)).z  // V3 (fim, lado esquerdo)
                };
                // Estes vértices correspondem aos índices {0, 1, 2, 0, 2, 3} (V0,V1,V2, V0,V2,V3)

                // Re-ligar o programa de GPU e setar uniforms (se necessário)
                glUseProgram(g_GpuProgramID);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(Matrix_Identity()));
                glUniform1i(g_object_id_uniform, LINE); // ID para o shader (para a cor da linha)

                glBindVertexArray(lineVAO);
                glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
                // Envie os novos dados dos vértices para o VBO da linha
                glBufferData(GL_ARRAY_BUFFER, sizeof(current_line_vertices_data), current_line_vertices_data, GL_STATIC_DRAW);

                // === AGORA DESENHE COMO TRIÂNGULOS USANDO OS ÍNDICES ===
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineEBO); // Liga o EBO
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Desenha 6 índices como triângulos
                // ======================================================

                // Remova glLineWidth() aqui, pois não é mais necessário
                // glLineWidth(3.0f);
                // glLineWidth(1.0f);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Desliga o EBO
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
            }
        }


        // === CHAMADA PARA EXIBIR PORCENTAGEM DE FORÇA NA TELA ===
        TextRendering_ShowShotPower(window);

        //chamada da funcao de menu
        TextRendering_ShowMenu(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    //printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    //printf("Textura carregada na unidade GL_TEXTURE%d (textureunit = %u)\n", textureunit, textureunit);


    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_texture_index_uniform = glGetUniformLocation(g_GpuProgramID, "texture_index_uniform");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);

    for (int i = 0; i < 16; ++i)
    {
        std::string name = "TextureImage[" + std::to_string(i) + "]";
        GLint loc = glGetUniformLocation(g_GpuProgramID, name.c_str());
        glUniform1i(loc, i); // GL_TEXTUREi
    }



    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Lógica para o modo de Câmera Livre (mouse sem botão)
    if (g_CameraMode == FREE_CAMERA)
    {
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   -= 0.01f*dy;

        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;
    }


    if (g_AimingMode)
    {
        g_AimingAngle -= 0.01f*dx; // Movimento horizontal do mouse controla o ângulo da mira
        // Não é necessário clamping para o ângulo da mira, ele pode girar 360+ graus.
        // fprintf(stdout, "DEBUG: Aiming Angle: %.2f\n", g_AimingAngle); fflush(stdout);
    }


    // Lógica para o modo NORMAL (mouse com botão)
    else
    {

        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY; // dy não será usado para mira horizontal


        // Se o botão esquerdo do mouse estiver pressionado no modo NORMAL,
        // ele controla a câmera look-at (movendo g_CameraTheta e g_CameraPhi)
        if (g_LeftMouseButtonPressed)
        {
            // Ajusta a câmera look-at com o mouse
            g_CameraTheta -= 0.01f*dx;
            g_CameraPhi   -= 0.01f*dy;

            // Restrições de phi para a câmera look-at
            float phimax = 3.141592f/2;
            float phimin = -phimax;

            if (g_CameraPhi > phimax)
                g_CameraPhi = phimax;

            if (g_CameraPhi < phimin)
                g_CameraPhi = phimin;
        }
    }

    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Se o modo de câmera livre estiver DESATIVADO, o scroll controla a distância da câmera normal.
    if (g_CameraMode == LOOKAT_WHITE_BALL)
    {
        // Usamos a variável yoffset para simular um zoom da câmera.
        g_CameraDistance -= yoffset;

        // Limita a distância da câmera para evitar valores problemáticos.
        if (g_CameraDistance < 1.0f)
            g_CameraDistance = 1.0f;

        // Limita a distância da câmera para o zoom out máximo.
        if (g_CameraDistance > MAX_CAMERA_DISTANCE)
            g_CameraDistance = MAX_CAMERA_DISTANCE;
        }

    // Se g_FreeLookMode for true, o scroll não fará nada (ou você poderia adicionar
    // outra lógica aqui, como mudar o FOV ou a velocidade da câmera livre,
    // mas por enquanto, vamos mantê-lo sem efeito).
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ======================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    // === CONTROLE DA BOLA BRANCA NO MODO DE POSICIONAMENTO ===
    if (g_CueBallPositioningMode)
    {
        float step = g_BallStepSize; // Um passo maior para posicionamento manual

        if (key == GLFW_KEY_LEFT) {
            g_Balls[0].position.x -= step;
        }
        if (key == GLFW_KEY_RIGHT) {
            g_Balls[0].position.x += step;
        }
        // Para mover para frente/trás na mesa (eixo Z)
        if (key == GLFW_KEY_UP) {
            g_Balls[0].position.z -= step; // Z negativo é "para frente" na mesa
        }
        if (key == GLFW_KEY_DOWN) {
            g_Balls[0].position.z += step; // Z positivo é "para trás" na mesa
        }
        // A altura Y da bola deve permanecer fixa
        g_Balls[0].position.y = BALL_Y_AXIS;

        // Sair do modo de posicionamento e permitir o chute
        if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) // Tecla Enter ou Espaço para confirmar
        {
            g_CueBallPositioningMode = false;
            fprintf(stdout, "DEBUG: Bola branca posicionada. Modo de jogo reativado.\n");
        }
        fprintf(stdout, "DEBUG: Posicao da Bola Branca: (%.4f, %.4f, %.4f)\n", g_Balls[0].position.x, g_Balls[0].position.y, g_Balls[0].position.z);
    }


    /*/ === CONTROLE MANUAL FIXO DA BOLA DE DEPURACAO (g_DebugBall)
    // (Apenas quando a tecla é pressionada ou repetida)
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Reinicia a bola para uma posição conhecida (útil se ela sair do controle)
        if (key == GLFW_KEY_R) // Exemplo: Tecla 'R' para Resetar a bola
        {
            g_DebugBall.position = glm::vec3(-0.0020f, BALL_Y_AXIS, 0.5680f); // Posição inicial
            fprintf(stdout, "DEBUG: Bola resetada para a posicao inicial. Pos: (%.4f, %.4f, %.4f)\n", g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z);
        }

        // Mover em X (esquerda/direita)
        if (key == GLFW_KEY_LEFT) { // Seta para a esquerda
            g_DebugBall.position.x -= g_BallStepSize;
            fprintf(stdout, "DEBUG: posicao atual (%.4f, %.4f, %.4f)\n", g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z);
        }
        if (key == GLFW_KEY_RIGHT) { // Seta para a direita
            g_DebugBall.position.x += g_BallStepSize;
            fprintf(stdout, "DEBUG: posicao atual (%.4f, %.4f, %.4f)\n", g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z);

        }
        if (key == GLFW_KEY_UP) { // Seta para cima
            g_DebugBall.position.z -= g_BallStepSize;
            fprintf(stdout, "DEBUG: posicao atual (%.4f, %.4f, %.4f)\n", g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z);

        }

        if (key == GLFW_KEY_DOWN) { // Seta para baixo
            g_DebugBall.position.z += g_BallStepSize;
            fprintf(stdout, "DEBUG: posicao atual (%.4f, %.4f, %.4f)\n", g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z);

        }

        if (key == GLFW_KEY_PAGE_UP) { // Seta para baixo
            g_DebugBall.position.y += g_BallStepSize;
            fprintf(stdout, "DEBUG: posicao atual (%.4f, %.4f, %.4f)\n", g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z);
        }

        if (key == GLFW_KEY_PAGE_DOWN) { // Seta para baixo
            g_DebugBall.position.y += g_BallStepSize;
            fprintf(stdout, "DEBUG: posicao atual (%.4f, %.4f, %.4f)\n", g_DebugBall.position.x, g_DebugBall.position.y, g_DebugBall.position.z);
        }
        if (key == GLFW_KEY_2)
        {
            g_DebugBall.radius += 0.1;
            fprintf(stdout, "DEBUG: RAIO ATUAL (%.4f,)\n", g_DebugBall.radius);

        }

        if (key == GLFW_KEY_1)
        {
            g_DebugBall.radius -= 0.1;
            fprintf(stdout, "DEBUG: RAIO ATUAL (%.4f,)\n", g_DebugBall.radius);
        }

    }*/

    // Se o usuário apertar a tecla T, alterna o modo de mira
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        // Só permite ativar o modo de mira se a bola branca estiver parada e ativa
        if (!g_AimingMode && !g_Balls.empty() )
        {
            g_AimingMode = true;
            g_AimingAngle = g_CameraTheta + M_PI; // Inicializa o ângulo de mira com base na direção da câmera
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            fprintf(stdout, "DEBUG: Modo de Mira ATIVADO.\n");
        }
        else if (g_AimingMode) // Se já estiver no modo de mira, desativa.
        {
            g_AimingMode = false;
            fprintf(stdout, "DEBUG: Modo de Mira DESATIVADO.\n");
        }
        fflush(stdout);
    }

    // === Lógica para a tecla P (Sistema de Força) ===
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        // Só permite iniciar o carregamento da força se o modo de mira estiver ativo
        // e a bola branca estiver ativa.
        if (g_AimingMode && !g_Balls.empty() && g_Balls[0].active )
        {
            g_P_KeyHeld = true; // Sinaliza que 'P' está pressionada
            g_P_PressStartTime = glfwGetTime(); // Registra o tempo de início
            g_CurrentShotPowerPercentage = 0.0f; // Começa a força em 0%
            g_ShotPowerPingPongDirection = 1.0f; // Começa carregando (incrementando)
            fprintf(stdout, "DEBUG: Carregando Forca...\n");
        }
        fflush(stdout);
    }

    if (key == GLFW_KEY_P && action == GLFW_RELEASE)
    {
        if (g_P_KeyHeld) // Só dispara se 'P' estava de fato sendo pressionada
        {
            g_P_KeyHeld = false; // Sinaliza que 'P' não está mais sendo pressionada
            fprintf(stdout, "DEBUG: Modo de Mira DESATIVADO (Tacada!).\n");

            // Calcula a força final com base na porcentagem atual
            float shot_power_magnitude = g_MinShotPowerMagnitude + (g_MaxShotPowerMagnitude - g_MinShotPowerMagnitude) * (g_CurrentShotPowerPercentage / 100.0f);

            // Calcula o vetor de direção da tacada a partir do g_AimingAngle
            glm::vec3 shoot_direction = glm::vec3(glm::sin(g_AimingAngle), 0.0f, glm::cos(g_AimingAngle));
            shoot_direction = glm::normalize(shoot_direction); // Normaliza para ter comprimento 1

            // Aplica a velocidade à bola branca
            if (!g_Balls.empty() && g_Balls[0].active) {
                g_Balls[0].velocity = shoot_direction * shot_power_magnitude;
                g_AimingMode = false;
                fprintf(stdout, "DEBUG: Tacada! Forca %.2f%%. Vel: (%.2f, %.2f, %.2f)\n",
                        g_CurrentShotPowerPercentage,
                        g_Balls[0].velocity.x, g_Balls[0].velocity.y, g_Balls[0].velocity.z);
            }
            fflush(stdout);
        }
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {

        if( g_UsePerspectiveProjection )
        {
            fprintf(stdout, "DEBUG: Projeção ORTOGRÁFICA DESATIVADA.\n");
            g_UsePerspectiveProjection = false; // Alterna para projeção ortográfica
        }
        else
        {
            g_UsePerspectiveProjection = true; // Alterna para projeção perspectiva
            fprintf(stdout, "DEBUG: Projeção ORTOGRÁFICA ATIVADA.\n");
        }
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    // Se o usuário apertar a tecla C, alterna o modo de câmera.
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        //inicialmente, se estava no menu e apertou C, vai direto para LOOK_AT_WHITE_BALL
        //depois disso vai apenas transicionar entre as duas e não volta mais pra bezier
        if (g_CameraMode == BEZIER){
            g_CameraMode = LOOKAT_WHITE_BALL;
        }
        // Primeiro, alternamos o estado do modo de câmera livre.
        //g_FreeLookMode = !g_FreeLookMode;
        // AGORA é diferente, alterna entre os dois estados, uma vez apertado C
        if (g_CameraMode == LOOKAT_WHITE_BALL){
            g_CameraMode = FREE_CAMERA;
        }
        else if (g_CameraMode == FREE_CAMERA){
            g_CameraMode = LOOKAT_WHITE_BALL;
        }
        if (g_CameraMode == FREE_CAMERA) // AGORA: Estamos no modo LIVRE (acabamos de ativar)
        {
            // === TRANSITION ON: Do modo FIXO para o LIVRE ===
            // 1. SALVAR O ESTADO ATUAL DA CÂMERA FIXA (para restaurar ao sair)
            g_FixedCamRestoreDistance = g_CameraDistance;
            g_FixedCamRestorePhi      = g_CameraPhi;
            g_FixedCamRestoreTheta    = g_CameraTheta;  
            g_AimingAngle = g_CameraTheta + M_PI; // Garante que a mira comece alinhada com a câmera
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            // 2. Captura a posição atual da câmera do modo FIXO (coordenadas cartesianas).
            float r_fixed = g_CameraDistance;
            float y_fixed = r_fixed*sin(g_CameraPhi);
            float z_fixed = r_fixed*cos(g_CameraPhi)*cos(g_CameraTheta);
            float x_fixed = r_fixed*cos(g_CameraPhi)*sin(g_CameraTheta);
            g_FreeCameraPosition = glm::vec4(x_fixed, y_fixed, z_fixed, 1.0f);

            // 3. Salva esta posição como o ponto de retorno para quando desativarmos o modo LIVRE.
            g_FreeCameraStartPosition = g_FreeCameraPosition; // Usado para o debug print e opcionalmente para "resetar" a posição inicial da free cam.

            // 4. Ajusta g_CameraPhi e g_CameraTheta para fazer a CÂMERA LIVRE olhar para o mesmo ponto (origem).
            glm::vec3 current_cam_pos_vec3 = glm::vec3(g_FreeCameraPosition);
            glm::vec3 target_point_vec3 = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::vec3 desired_view_vector = target_point_vec3 - current_cam_pos_vec3;

            g_CameraPhi = atan2(desired_view_vector.y, glm::length(glm::vec2(desired_view_vector.x, desired_view_vector.z)));
            g_CameraTheta = atan2(desired_view_vector.x, desired_view_vector.z);

            float phimax = 3.141592f/2;
            float phimin = -phimax;
            if (g_CameraPhi > phimax) g_CameraPhi = phimax;
            if (g_CameraPhi < phimin) g_CameraPhi = phimin;


            //fprintf(stdout, "DEBUG: Modo Câmera Livre ATIVADO. Pos: (%.2f,%.2f,%.2f). Ângulos ajustados: Phi=%.2f, Theta=%.2f\n",
              //      g_FreeCameraPosition.x, g_FreeCameraPosition.y, g_FreeCameraPosition.z, g_CameraPhi, g_CameraTheta);


        }
        else if (g_CameraMode == LOOKAT_WHITE_BALL) //GORA: Estamos no modo FIXO (acabamos de desativar)
        {
            // === TRANSITION OFF: Do modo LIVRE para o FIXO ===
            // 1. RESTAURAR o estado completo da câmera fixa para a posição anterior
            g_CameraDistance = g_FixedCamRestoreDistance;
            g_CameraPhi      = g_FixedCamRestorePhi;
            g_CameraTheta    = g_FixedCamRestoreTheta;

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            //fprintf(stdout, "DEBUG: Modo Câmera Livre DESATIVADO. Restaurado para: (D=%.2f, P=%.2f, T=%.2f)\n",
              //      g_CameraDistance, g_CameraPhi, g_CameraTheta);

            // Opcional: Mostrar cursor (se você for implementar o bloqueio do cursor)
            // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        fflush(stdout); // Garante que as mensagens sejam exibidas imediatamente
    }

    // Lógica para as flags de WASD (independentemente do modo de câmera livre)
    if (key == GLFW_KEY_W) {
        g_W_Pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        fprintf(stdout, "DEBUG: W key state: %d (Press/Repeat: %d)\n", g_W_Pressed, (action == GLFW_PRESS || action == GLFW_REPEAT)); // <<=== NOVO DEBUG
    }
    if (key == GLFW_KEY_A) {
        g_A_Pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        //fprintf(stdout, "DEBUG: A key state: %d (Press/Repeat: %d)\n", g_A_Pressed, (action == GLFW_PRESS || action == GLFW_REPEAT)); // <<=== NOVO DEBUG
    }
    if (key == GLFW_KEY_S) {
        g_S_Pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        //fprintf(stdout, "DEBUG: S key state: %d (Press/Repeat: %d)\n", g_S_Pressed, (action == GLFW_PRESS || action == GLFW_REPEAT)); // <<=== NOVO DEBUG
    }
    if (key == GLFW_KEY_D) {
        g_D_Pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        //fprintf(stdout, "DEBUG: D key state: %d (Press/Repeat: %d)\n", g_D_Pressed, (action == GLFW_PRESS || action == GLFW_REPEAT)); // <<=== NOVO DEBUG
    }

    fflush(stdout); // Garante que a mensagem seja exibida

}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}


// Menu rendering function
void TextRendering_ShowMenu(GLFWwindow* window)
{
    // Only show menu if UI text is enabled and in BEZIER mode
    if (!g_ShowInfoText || g_CameraMode != BEZIER)
    {
        return;
    }

    // Static buffers for menu text
    static char buffer1[50] = "Aperte C para iniciar";
    static char buffer2[50] = "T para ligar/desligar o taco";
    static char buffer3[50] = "Segure P para dar uma tacada";
    static char buffer4[50] = "O e P para trocar projecoes";

    // Line height and char width for positioning
    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    // Larger font scale for menu
    float scale = 2.0f;

    // Center each line horizontally (NDC: x from -1 to 1)
    // Calculate x offset: -(length * charwidth * scale / 2)
    float x1 = -(strlen(buffer1) * charwidth * scale / 2.0f);
    float x2 = -(strlen(buffer2) * charwidth * scale / 2.0f);
    float x3 = -(strlen(buffer3) * charwidth * scale / 2.0f);
    float x4 = -(strlen(buffer4) * charwidth * scale / 2.0f);

    // Position lines vertically (NDC: y from -1 to 1)
    // Start at y=0.2 and space downward
    float y1 = 0.2f;
    float y2 = y1 - lineheight * scale;
    float y3 = y2 - lineheight * scale;
    float y4 = y3 - lineheight * scale;

    // Render each line
    TextRendering_PrintString(window, buffer1, x1, y1, scale);
    TextRendering_PrintString(window, buffer2, x2, y2, scale);
    TextRendering_PrintString(window, buffer3, x3, y3, scale);
    TextRendering_PrintString(window, buffer4, x4, y4, scale);
}

// Escrevemos na tela a porcentagem da força da tacada.
void TextRendering_ShowShotPower(GLFWwindow* window)
{
    // Verifica se o texto informativo global está habilitado (se você usa g_ShowInfoText para a UI)
    if ( !g_ShowInfoText )
        return;

    // Só exibe o texto da força se a tecla 'P' estiver sendo mantida pressionada
    // (A variável g_P_KeyHeld é global e é atualizada no KeyCallback).
    if ( !g_P_KeyHeld )
        return;

    // Variáveis estáticas para o buffer do texto. Elas mantêm seu valor entre as chamadas da função.
    static char buffer[50]; // Buffer para formatar a string de texto (ex: "FORCA: 85.3%")
    static int  numchars = 0; // Número de caracteres na string formatada

    // A porcentagem de força (g_CurrentShotPowerPercentage) é atualizada a cada frame
    // no loop principal (onde g_P_KeyHeld é true). Aqui, apenas formatamos e imprimimos.
    numchars = snprintf(buffer, 50, "FORCA: %.1f%%", g_CurrentShotPowerPercentage);

    // Posicionamento na tela usando coordenadas NDC (Normalized Device Coordinates).
    // lineheight e charwidth são usados para um posicionamento responsivo.
    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    // Posicionamos no canto inferior esquerdo ou onde preferir.
    // Exemplo: 0.5f para mais à direita, -0.8f para perto do fundo.
    // Você pode ajustar X e Y para centralizar ou posicionar melhor.
    TextRendering_PrintString(window, buffer, 0.5f, -0.8f, 1.2f); // Exemplo: Posição ajustada
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model, bool showAllInfo)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());



  if (showAllInfo){
    for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
        printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
            static_cast<const double>(attrib.vertices[3 * v + 0]),
            static_cast<const double>(attrib.vertices[3 * v + 1]),
            static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
        printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
            static_cast<const double>(attrib.normals[3 * v + 0]),
            static_cast<const double>(attrib.normals[3 * v + 1]),
            static_cast<const double>(attrib.normals[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
        printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
            static_cast<const double>(attrib.texcoords[2 * v + 0]),
            static_cast<const double>(attrib.texcoords[2 * v + 1]));
    }

    // For each shape
    for (size_t i = 0; i < shapes.size(); i++) {
        printf("shape[%ld].name = %s\n", static_cast<long>(i),
            shapes[i].name.c_str());
        printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
            static_cast<unsigned long>(shapes[i].mesh.indices.size()));

        size_t index_offset = 0;

        assert(shapes[i].mesh.num_face_vertices.size() ==
            shapes[i].mesh.material_ids.size());

        printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
            static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

        // For each face
        for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
        size_t fnum = shapes[i].mesh.num_face_vertices[f];

        printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
                static_cast<unsigned long>(fnum));

        // For each vertex in the face
        for (size_t v = 0; v < fnum; v++) {
            tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
            printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
                static_cast<long>(v), idx.vertex_index, idx.normal_index,
                idx.texcoord_index);
        }

        printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
                shapes[i].mesh.material_ids[f]);

        index_offset += fnum;
        }

        printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
            static_cast<unsigned long>(shapes[i].mesh.tags.size()));
        for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
        printf("  tag[%ld] = %s ", static_cast<long>(t),
                shapes[i].mesh.tags[t].name.c_str());
        printf(" ints: [");
        for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
            printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
            if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
            printf(", ");
            }
        }
        printf("]");

        printf(" floats: [");
        for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
            printf("%f", static_cast<const double>(
                            shapes[i].mesh.tags[t].floatValues[j]));
            if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
            printf(", ");
            }
        }
        printf("]");

        printf(" strings: [");
        for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
            printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
            if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
            printf(", ");
            }
        }
        printf("]");
        printf("\n");
        }
    }

    for (size_t i = 0; i < materials.size(); i++) {
        printf("material[%ld].name = %s\n", static_cast<long>(i),
            materials[i].name.c_str());
        printf("  material.Ka = (%f, %f ,%f)\n",
            static_cast<const double>(materials[i].ambient[0]),
            static_cast<const double>(materials[i].ambient[1]),
            static_cast<const double>(materials[i].ambient[2]));
        printf("  material.Kd = (%f, %f ,%f)\n",
            static_cast<const double>(materials[i].diffuse[0]),
            static_cast<const double>(materials[i].diffuse[1]),
            static_cast<const double>(materials[i].diffuse[2]));
        printf("  material.Ks = (%f, %f ,%f)\n",
            static_cast<const double>(materials[i].specular[0]),
            static_cast<const double>(materials[i].specular[1]),
            static_cast<const double>(materials[i].specular[2]));
        printf("  material.Tr = (%f, %f ,%f)\n",
            static_cast<const double>(materials[i].transmittance[0]),
            static_cast<const double>(materials[i].transmittance[1]),
            static_cast<const double>(materials[i].transmittance[2]));
        printf("  material.Ke = (%f, %f ,%f)\n",
            static_cast<const double>(materials[i].emission[0]),
            static_cast<const double>(materials[i].emission[1]),
            static_cast<const double>(materials[i].emission[2]));
        printf("  material.Ns = %f\n",
            static_cast<const double>(materials[i].shininess));
        printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
        printf("  material.dissolve = %f\n",
            static_cast<const double>(materials[i].dissolve));
        printf("  material.illum = %d\n", materials[i].illum);
        printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
        printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
        printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
        printf("  material.map_Ns = %s\n",
            materials[i].specular_highlight_texname.c_str());
        printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
        printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
        printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
        printf("  <<PBR>>\n");
        printf("  material.Pr     = %f\n", materials[i].roughness);
        printf("  material.Pm     = %f\n", materials[i].metallic);
        printf("  material.Ps     = %f\n", materials[i].sheen);
        printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
        printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
        printf("  material.aniso  = %f\n", materials[i].anisotropy);
        printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
        printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
        printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
        printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
        printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
        printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
        std::map<std::string, std::string>::const_iterator it(
            materials[i].unknown_parameter.begin());
        std::map<std::string, std::string>::const_iterator itEnd(
            materials[i].unknown_parameter.end());

        for (; it != itEnd; it++) {
        printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
        }
        printf("\n");

    }
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

