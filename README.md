# Sinuca Simulator v0.8



## Conceitos aplicados de computação gráfica
- Mapeamento de texturas (por coordenadas UV e sférico)

![alt text](https://github.com/Gigio2077/trab-FCG/blob/main/imagens_readme/print_texturas.jpg)



- Modelos de iluminação de Blinn-Phong e Interpolação de Phong

```C
// Lighting calculations
    vec3 Kd = Kd0;                // Diffuse
    vec3 Ks = vec3(0.05);          // Specular
    float shininess = 64.0;       // Phong exponent
    vec3 Ia = vec3(1.0);          // Ambient light intensity
    vec3 ambient = 0.1 * Kd;      // Ambient term
    float lambert = max(dot(n.xyz, l.xyz), 0.0); // Diffuse term
    vec3 Id = vec3(1.0);          // Diffuse light intensity
    vec3 diffuse = lambert * Kd * Id;
    vec3 h = normalize(l.xyz + v.xyz); // Half-vector
    float spec = pow(max(dot(n.xyz, h), 0.0), shininess);

    vec3 Is = vec3(1.0);          // Specular light intensity
    vec3 specular = Ks * spec * Is;

// Final color
    color.rgb = ambient + diffuse + specular;
```

- Projeção esférica de texturas (feitas em aula no laboratório 5)
```c
// Compute spherical UV coordinates
    vec3 bbox_center = ((bbox_min + bbox_max) * 0.5).xyz;
    vec3 p = position_model.xyz;
    vec3 p_rel = p - bbox_center;
    float rho = length(p_rel);
    float phi = asin(p_rel.y / rho);
    float theta = atan(p_rel.x, p_rel.z);
    float U = 0.5 + theta / (2.0 * M_PI);
    float V = 0.5 + phi / M_PI;

```

## Contribuições de cada membro

- Giovanni se concentrou nos objetos iniciais, mapeamento de texturas, cameras e menu;

- Eric se concentrou mais na parte das colisões entre as bolas, reflexão das tabelas e marcação de pontos (quando aas bolinhas caem na caçapa) e comandos de jogabilidade;





## Manual descrevendo a utilização da aplicação
- No menu inicial, aperte C para jogar

- Aperte C para alternar entre o modo "Look-at" e câmera livre
    - Na câmera livre, caminhe com as teclas W,A,S,D e o mouse

- Aperte T para fazer o Taco aparecer/desaparecer

- Segure e solte  P para fazer uma tacada (apenas quando o taco está ativado)

- Quando a bola branca cai, use as setas para posicioná-la e espaço/enter para definir.

## Utilizamos ferramentas de IA?
Sim, utilizamos Grok e GPT para geração de código, e para rastrear erros.

## Passos necessários para compilação e execução
