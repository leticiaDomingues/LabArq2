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
    int width, height, depth, pixels, i;
    int rn[(SIMD_SIZE/3) + 1]; // ruido
	char mult[24] = {
		1, 1, 3, 1, 1, 3, 1, 1,
		1, 3, 1, 1, 3, 1, 1, 3,
		3, 1, 1, 3, 1, 1, 3, 1 };

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
    
    int siz = (pixels * 3) + (SIMD_SIZE - ((3 * pixels) % SIMD_SIZE));

    ptri = ptro = img = (char *)malloc(siz);

    fread(img, 3, pixels, fp);
    
    start = clock();

    for (int j = 0; j < SIMD_SIZE; j++)
        limit[j] = RND_LIMIT;

    for (i = 0; i < (siz / SIMD_SIZE); i++)
    {
        for (int j = 0; j < (SIMD_SIZE / 3) + 1; j++)
            rn[j] = rand() % 40;
        
        for (int j = 0; j < SIMD_SIZE; j++)
            rnds[j] = rn[j / 3];

		int resto = i % 3;
		
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
				movlpd xmm6, [eax + 16]
				movlpd xmm7, [eax + 8]

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

				/*movdqa xmm5, xmm0
				psrlw xmm5, 2

				movq2dq xmm1, mm7
				punpcklbw xmm1, xmm4

				movdqa xmm3, xmm1
                pcmpgtw xmm3, xmm4

				pcmpgtw xmm2, xmm1
                
                pxor xmm3, xmm2
				pand xmm2, xmm5
				pand xmm3, xmm0
				por xmm3, xmm2
                                
                packuswb xmm3, xmm4
                movlpd [edi], xmm3*/
				packuswb xmm0, xmm4
				movlpd[edi], xmm0
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
