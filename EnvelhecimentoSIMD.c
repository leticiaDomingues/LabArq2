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
		1, 3, 1, 1, 3, 1, 1, 3 };

	char *m = mult;
	
	char clear[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
	char xorFix[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
		
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
    
    siz = (pixels * 3) + (SIMD_SIZE - ((3 * pixels) % SIMD_SIZE));

    ptri = ptro = img = (char *)malloc(siz);

    fread(img, 3, pixels, fp);
    
    start = clock();

    for (j = 0; j < SIMD_SIZE; j++)
        limit[j] = RND_LIMIT;

    for (i = 0; i < (siz / SIMD_SIZE); i++)
    {
		k = 0;
		for (j = 0; j < (SIMD_SIZE/3) + 1; j++)
		{
			rnd = rand() % 40;			
			for (n = 0; n < 3 && k < SIMD_SIZE; n++)
				rnds[k++] = rnd;
		}

		resto = i % 3;
		
        __asm {
				emms
				mov esi, ptri
				mov edi, ptro

				cmp i, 0
				jne skip
				movq mm2, limit
				movq mm5, xorFix
				movq mm7, clear

				mov eax, m
				movlpd xmm5, [eax]
				movlpd xmm6, [eax + 8]
				movlpd xmm7, [eax + 16]

				pxor mm2, mm5

		skip :  cmp resto, 2
				jne skip2
				movdq2q mm6, xmm7
				jmp read

		skip2:  cmp resto, 1
				jne skip3
				movdq2q mm6, xmm6
				jmp read

		skip3:  movdq2q mm6, xmm5				
				
		read:   movq mm0, [esi]
                movq mm1, rnds             
				movq mm3, mm0
                
				pxor mm3, mm5

                pcmpgtb mm3, mm2
                pand mm3, mm1
                psubb mm0, mm3

                movq2dq mm0, xmm0
                punpcklbw xmm0, xmm4
				
                movq2dq xmm2, mm6
                punpcklbw xmm2, xmm4

                pmullw xmm0, xmm2

				movdqa xmm1, xmm0  // copia x0 em x1
				psrlw xmm1, 2 // faz divisão dos bytes por 4
				
				movq2dq xmm3, mm7 // move o vetor clear (composto por 1s para x3)
				punpcklbw xmm3, xmm4
				pcmpgtw xmm2, xmm3 // marca em x2 quais as words cujo multiplicador é maior que 1 (quais são as posições azuis)

                pcmpgtw xmm3, xmm4 // faz x3 todo igual a FF
                
                pxor xmm3, xmm2 // faz as words de x3 que tem posição do azul serem 0
				pand xmm2, xmm1 // copia os 2 ou 3 bytes azuis para x2
				pand xmm3, xmm0 // copia os demais bytes para x3, deixando os bytes azuis como 0
				por xmm3, xmm2 // junta os bytes em x3, resultando em todos os bytes sem shift, exceto por aqueles marcados nas posições azuis
                                
                packuswb xmm3, xmm4
                movlpd [edi], xmm3
		}

        ptri += SIMD_SIZE;
        ptro += SIMD_SIZE;
    }

    end = clock();

    fwrite(img, 3, pixels, fo);

    fclose(fp);
    fclose(fo);

    free(img);

    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    fprintf(stderr, "tempo = %f segundos\n", cpu_time_used);
    return 0;
}
