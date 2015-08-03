# Mestrado

Bem-vindo(a) à página do código-fonte do projeto de mestrado.

O protótipo que desenvolvemos transforma uma superfície comum (rígida, opaca e não instrumentalizada) - como por exemplo uma parede ou uma mesa - num espaço interativo multi-toque. Denominada **Superfície Mágica**, ou simplesmente superfície interativa, a solução possibilita variadas formas de expressão e é construída com equipamentos portáteis e de baixo custo (ex: projetor de vídeo, câmera Kinect e laptop). O diálogo entre o espaço interativo e o usuário é inspirado nas ações de desenhar, gesticular e agarrar.

Confira os vídeos das aplicações interativas em https://youtube.com/alemart88

Estrutura de pastas:

* **tox:** código C++ responsável por transformar superfícies comuns em espaços interativos multi-toque (aceita dedos de mão, canetas coloridas e apagador) e por detectar uma "varinha mágica" para interação 3D;
* **html5:** código-fonte HTML5 de aplicações diversas que fazem uso da superfície interativa ou da varinha mágica (lousa mágica, tapete mágico, etc);
* **of_v0073_linux64_release:** openFrameworks configurado. Requer as bibliotecas: OpenCV, TUIO e OpenNI (para acessar o Kinect);
* **packages:** bibliotecas e outros requisitos.

Have fun!! ;)
