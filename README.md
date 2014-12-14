Trabalho final de Lab Arq 2 - Envelhecimento de Imagem

Filtro para envelhecimento de imagem

Este filtro tem como objetivo o envelhecimento da imagem, ele tem 2 passos básicos que são:

1) Aplicar filtro para deixar a imagem amarelada. <br>
2) Aplicar na imagem um ruído.
Para realização do mesmo vai ser usado como base o seguinte link:

http://mamismd.blogspot.com.br/2014/11/envelhecimento-de-imagem.html

<b>Implementação</b><br>
Para a implementação do filtro, tanto para versão serial quanto para paralela, primeiro é copiado o cabeçalho (tipo da imagem, altura, largura e profundidade) da imagem para a nova imagem a ser gerada. Depois, e calculado o número de pixels da imagem (altura x largura), e alocado um vetor de tamanho 3*pixels para armazena-los, pois cada pixel contem três bytes: R que é a quantidade de vermelho do pixel, G que é a quantidade de verde e B que é a quantidade de azul. 
 
<b>Sequencial</b><br>
A versão sequencial executa o loop para cada pixel da imagem (altura * largura). Primeiro, o valor de B e multiplicado por 3 e, em seguida, dividido por 4 (com shift right 2), resultando num valor tal que B = 0.75 * B. Assim, o fator azul do pixel é reduzido e a imagem se torna amarelada. Em seguida, subtraímos de cada cor um valor randômico de 0 a 40, considerando que o valor da mesma seja superior a 50 para não manchar a imagem com pontos brancos ou pretos. Assim, temos pequenas variações nas cores e a imagem apresentara um leve ruído, aparentando envelhecimento. 
 
<b>Paralelo</b><br>
A versão paralela do filtro executa o loop tratando 8 bytes por vez, ou seja, 2 pixels e mais 2 bytes do terceiro. Para tanto, o vetor alocado deve ser divisível por 8, então adicionamos 8 menos o resto da divisão do total de pixels por 8, resultando em um numero valido. Também precisamos saber quais posições dos 8 bytes são correspondentes a pixels azuis. Para tanto, temos o vetor mult que armazena os 3 estados possíveis para as posições azuis: <br>
>
char mult[24] = {  <br>
                    1, 1, 3, 1, 1, 3, 1, 1, <br>
                    3, 1, 1, 3, 1, 1, 3, 1, <br>
                    1, 3, 1, 1, 3, 1, 1, 3 };
>

<p>Agora, em cada iteração, escolhemos 3 valores aleatórios entre 0 e 40 e os salvamos em um vetor de 8 elementos sempre na mesma ordem: r1, r1, r1, r2, r2, r2, r3, r3. Entretanto, o ideal seria salva-los de acordo com o estado definido pelo vetor mult, mas o resultado não é alterado por isso.<br>
Lidos os valores nos registradores, precisamos fazer uma subtração de 128 (tanto nos bytes de cor, quanto no vetor que possui 50 em todas as posições, simbolizando o limite para a subtração, pois a comparação e sempre feita considerando o sinal e para cores com grandes valores, estes serão considerados erroneamente negativos. <br>
Operamos, agora, um and entre as cores e os limites, tendo como resultado as posições que devem receber o valor randômico marcadas e subtraímos os valores nestas posições, aplicando assim, o ruído. <br>
Agora, temos que aplicar o filtro amarelo. Para tanto, multiplicamos os bytes pelos 8 valores selecionados naquele estado do vetor mult e temos os bytes azuis multiplicados por 3. Para dividi-los por quatro, aplicamos um shift right de 2 bits em todos os bytes, mas temos que usar apenas aqueles que correspondem a cor azul. Assim, pelo vetor mult, sabemos quais são estas posições (marcadas por 3) e as marcamos em um vetor auxiliar. Com este vetor, fazemos um and nos valores que receberam shift e, com isso, transferimos os bytes azuis. Com este mesmo vetor, limpamos os bytes azuis do vetor de multiplicação e fazemos um or para juntar os bytes originais com aqueles que receberam a multiplicação e o shift. Pronto, agora temos apenas os valores azuis multiplicados por 0.75 e todos submetidos ao ruído, então, salvamo-os na memória e reiniciamos o processo com os próximos 8 bytes. <br>
 
<b>Resultados</b><br>
Podemos ver que ao contrario do que se esperava, a versão paralela foi mais lenta. Mas este comportamento se deu pois a cada iteração, além do acesso a memoria para leitura dos pixels, fazemos também a gravação dos 8 valores randômicos e também sua posterior leitura, resultando em 16 acessos extras a memoria. Se executarmos os filtros sem o ruído, a versão paralela chega a ser 3x mais rápida que a sequencial. <br>

<b>Lion</b><br> 
![Lion](http://i.imgur.com/kIOC3Up.png)<br>
![LionProc](http://i.imgur.com/o96cbqi.png)<br>
Tempo versão sequencial: 0.0060000 <br>
Tempo na versão paralela: 0.0080000 <br>
 
<b>Beach</b><br>
![Beach](http://i.imgur.com/NJmmGtq.jpg)<br>
![BeachProc](http://i.imgur.com/XYwgNU5.jpg)<br>
Tempo versão sequencial: 0.043000 <br>
Tempo na versão paralela: 0.050000 <br>
 
<b>Tower</b><br>
![Tower](http://i.imgur.com/ipoFTIx.jpg)<br>
![TowerProc](http://i.imgur.com/5URrY4F.jpg)<br>
Tempo versão sequencial: 0.074000 <br>
Tempo na versão paralela: 0.087000 <br>
 
<b>Grafico</b><br>
![Grafico](http://i.imgur.com/wKUwPzK.png)
