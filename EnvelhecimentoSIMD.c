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
    char mult[] = { 1, 1, 3, 1, 1, 3, 1, 1 };
    char divs[] = { 0, 0, 1, 0, 0, 1, 0, 0 };

    char clear[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

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

        __asm {
                emms
                mov esi, ptri
                mov edi, ptro
                
                movq mm0, [esi]
                movq mm1, rnds
                movq mm2, limit                
                movq mm3, mm0
                
                pcmpgtb mm3, mm2
                pand mm3, mm1
                paddb mm0, mm3

                movq2dq mm0, xmm0

                punpcklbw xmm0, xmm4

                cmp i, 0
                jne skip
                movq mm2, mult
                movq mm3, divs
                movlpd xmm5, clear
                movhpd xmm5, clear

            skip:
                movq2dq xmm2, mm2
                movq2dq xmm3, mm3
                punpcklbw xmm2, xmm4
                punpcklbw xmm3, xmm4

                pmullw xmm0, xmm2
                
                movdqa xmm1, xmm0
                psrlw xmm1, 2
                
                pcmpgtw xmm3, xmm4
                
                pxor xmm5, xmm3
                pand xmm3, xmm1
                pand xmm5, xmm0
                por xmm5, xmm3

                movq mm5, mm2 // Rotate do vetor de multiplicação em 1 byte
                pslld mm2, 8
                psrld mm5, 24
                por mm2, mm5
                
                movq mm5, mm3 // Rotate do vetor de divisão em 1 byte
                pslld mm3, 8
                psrld mm5, 24
                por mm3, mm5
                                
                packuswb xmm5, xmm4

                movlpd [edi], xmm5
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
