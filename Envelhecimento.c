#define _CRT_SECURE_NO_WARNINGS
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/* Corpo de leitura do arquivo adaptado do arquivo filtro_windows.c no repositÃ³rio. */
int main(int argc, char *argv[]) 
{
    clock_t start, end;
    double cpu_time_used;
    char filetype[256], *ptri, *ptro, *img;
    char r, g, b;
    int width, height, depth, pixels, i;
    int rn; // ruido
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
    ptri = ptro = img = (char *)malloc(pixels * 3);

    fread(img, 3, pixels, fp);
        
    start = clock();    
    for (i = 0; i < pixels; i++) 
    {
        r = *ptri++; // nÃ£o pode ser int ou a conta nÃ£o funciona adequadamente e a conversÃ£o para char borra completamente a imagem
        g = *ptri++;
        b = *ptri++;
	rn = rand() % 40;
               
       __asm {
                movzx eax, r
                movzx ebx, g
                movzx ecx, b

                xchg ecx, eax

		mov esi, 3
		mul esi
                shr eax, 2

                xchg ecx, eax
                
		cmp al, 50 // ignora valores muito baixos para nÃ£o manchar a imagem com pontos pretos ou brancos
                jb skip1
                sub eax, rn

	skip1:	cmp bl, 50 // ignora valores muito baixos para nÃ£o manchar a imagem com pontos pretos ou brancos
                jb skip2
                sub ebx, rn

	skip2:	cmp cl, 50 // ignora valores muito baixos para nÃ£o manchar a imagem com pontos pretos ou brancos
                jb skip3
                sub ecx, rn

	skip3:  mov r, al
                mov g, bl
                mov b, cl
        }

         *ptro++ = r;
         *ptro++ = g;
         *ptro++ = b;
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
