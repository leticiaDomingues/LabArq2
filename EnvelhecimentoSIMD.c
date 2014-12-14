#define _CRT_SECURE_NO_WARNINGS
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define RND_LIMIT 50
#define SIMD_SIZE 8

/* Corpo de leitura do arquivo adaptado do arquivo filtro_windows.c no repositório. */
int main(int argc, char *argv[])
{
    clock_t start, end;
    double cpu_time_used;
    char filetype[256], *ptri, *ptro, *img;
    char rnds[SIMD_SIZE];
    char limit[SIMD_SIZE];
    int width, height, depth, pixels, i, j, k, n, resto, siz, rnd;

	char mult[24] = {
		1, 1, 3, 1, 1, 3, 1, 1,
		3, 1, 1, 3, 1, 1, 3, 1,
		1, 3, 1, 1, 3, 1, 1, 3 }; // vetor com as 3 sequencias de multiplicadores usadas (apos a 3a sequencia, a 4a iguala-se a primeira, entao reiniciamos)

	char *m = mult;
	
	char clear[] = { 1, 1, 1, 1, 1, 1, 1, 1 }; // bytes auxiliares
	char xorFix[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 }; // bytes 128 para corrigir a comparacao
		
    FILE *fp;
    FILE *fo;
 
    if (argc < 3)
    {
        printf("Usage: %s input output", argv[0]);
        exit(1);
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("File %s not found!", argv[1]);
        exit(1);
    }

    fo = fopen(argv[2], "wb");
    if (!fo)
    {
        printf("Unable to create file %s!", argv[2]);
        exit(1);
    }

    srand((unsigned)time(NULL)); // para valores randomicos

    fscanf(fp, "%s\n", filetype);
    fprintf(fo, "%s%c", filetype, 10);

    fscanf(fp, "%d %d\n%d\n", &width, &height, &depth);
    fprintf(fo, "%d %d\n%d%c", width, height, depth, 10);

    pixels = width * height;
    
    siz = (pixels * 3) + (SIMD_SIZE - ((3 * pixels) % SIMD_SIZE)); // fazemos o tamanho do malloc ser divisivel por SIMD_SIZE, adicionando o que falta ao resto

    ptri = ptro = img = (char *)malloc(siz);

    fread(img, 3, pixels, fp);
    
    start = clock();

    for (j = 0; j < SIMD_SIZE; j++)
        limit[j] = RND_LIMIT; // preenchemos o vetor que limita a subtracao do ruido com 50

    for (i = 0; i < (siz / SIMD_SIZE); i++) // precisamos de menos iteracoes agora que varios bytes serao tratados por vez
    {
		k = 0;
		for (j = 0; j < (SIMD_SIZE/3) + 1; j++)
		{
			rnd = rand() % 40;			
			for (n = 0; n < 3 && k < SIMD_SIZE; n++) // gera um vetor de numeros aleatorios, sempre repetido r1, r1, r1, r2, r2, r2, r3, r3, embora o ideal tambem seja deslocar esses valores
				rnds[k++] = rnd;
		}

		resto = i % 3;
		
        __asm {
				emms
				mov esi, ptri
				mov edi, ptro

				cmp i, 0 // para evitar acessos a memoria, carregamos esses valores nos registradores apenas na primeira iteracao e o reutilizamos depois
				jne skip
				movq mm2, limit // limitante para o ruido
				movq mm5, xorFix // fator de correcao para a comparacao do ruido
				movq mm7, clear // vetor composto por 1 para concatenar os resultados da multiplicacao azul

				mov eax, m // aqui carregamos os valores dos 3 estagios de multiplicacao
				movlpd xmm5, [eax] 
				movlpd xmm6, [eax + 8]
				movlpd xmm7, [eax + 16]
				
				// fazemos xor entre o limite do ruido e o fator de correcao, pois a comparacao e sempre com sinal e valores elevados serao tratados como negativos
				// entao, subtraimos 128 destes para que a comparacao seja adequada
				pxor mm2, mm5 

		skip :  cmp resto, 2 // verifica o estagio e escolhe os fatores do 3o estagio
				jne skip2
				movdq2q mm6, xmm7
				jmp read

		skip2:  cmp resto, 1 // escolhe o 2o estagio
				jne skip3
				movdq2q mm6, xmm6
				jmp read

		skip3:  movdq2q mm6, xmm5 // escolhe o estagio inicial		
				
		read:   movq mm0, [esi] // lemos os 8 bytes para mm0
                movq mm1, rnds // lemos os valores random para mm1
				movq mm3, mm0 // copia mm0 em mm3
                
				pxor mm3, mm5 // fazemos o xor em mm3 para subtrair 128

                pcmpgtb mm3, mm2 // comparamos com sinal os valores com o limitante
                pand mm3, mm1 // os positivos recebem FF, então fazemos um and com os valores randomicos
                psubb mm0, mm3 // e subtraimos dos bytes originais, assim, aqueles que nao satisfazem o limite sao subtraidos de 0 e continuam iguais

                movq2dq mm0, xmm0 // agora movemos mm0 para xmm0 e o expandimos para words
                punpcklbw xmm0, xmm4
				
                movq2dq xmm2, mm6 // movemos o vetor de multiplicacao escolhido para xmm2 e o expandimos em words
                punpcklbw xmm2, xmm4

                pmullw xmm0, xmm2 // multiplicamos os bytes pelo fator de multiplicacao, resultando apenas nos azuis *3 e os demais *1

				movdqa xmm1, xmm0  // copiamos xmm0 em xmm1
				psrlw xmm1, 2 // e fazemos a divisao das words de xmm1 por 4 (shift right 2)
				
				movq2dq xmm3, mm7 // move o vetor clear (composto por 1s para xmm3) e o expandimos em words
				punpcklbw xmm3, xmm4
				pcmpgtw xmm2, xmm3 // marca em xmm2 quais as words cujo multiplicador é maior que 1 (quais são as posições azuis)

                pcmpgtw xmm3, xmm4 // faz xmm3 todo igual a FF
                
                pxor xmm3, xmm2 // faz as words de xmm3 que tem posição do azul serem 0 e as demais continuam FFFF
				pand xmm2, xmm1 // copia os 2 ou 3 bytes azuis para xmm2
				pand xmm3, xmm0 // copia os demais bytes para xmm3, deixando os bytes azuis como 0
				por xmm3, xmm2 // junta os bytes em xmm3, resultando em todos os bytes sem shift, exceto por aqueles marcados nas posições azuis
				
				// assim, temos os bytes azuis * 0.75 e os compactamos de words para bytes em xmm3                                
                packuswb xmm3, xmm4
                movlpd [edi], xmm3 // gravamos a parte baixa (com os bytes) na memoria
		}

        ptri += SIMD_SIZE;
        ptro += SIMD_SIZE;
    }

    end = clock();

    fwrite(img, 3, pixels, fo); // aqui ignoramos os ultimos pixels que foram usados para deixar a divisao por SIMD_SIZE exata e salvamos apenas os que fazem parte da imagem

    fclose(fp);
    fclose(fo);

    free(img);

    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    fprintf(stderr, "tempo = %f segundos\n", cpu_time_used);
    return 0;
}
