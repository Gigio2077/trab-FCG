# Commit Inicial
## (baseado no laboratório 5)

O que foi feito até agora:
- Alguns trechos do código original do lab foram removidos/modificados
- a classe ObjModel para um arquivo separado, para mais organização (devido a isso é necessario adicionar ./src/ObjModel.cpp no Makefile, e um header no include também)
- Até agora tem uma mesa com a textura mapeada (de acordo com o vt) fonte: https://free3d.com/3d-model/pool-table-v1--600461.html

- Bolas: https://www.turbosquid.com/3d-models/pool-balls-877865


- Funcionalidade de camera livre:
  Ao apertar tecla "C" altera modo da camera de fixa para livre.
    Teclas W, A, S e D sao responsaveis pela movimentacao da camera
    Posicao do cursor movimenta o angulo da camera
