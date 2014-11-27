#define _CRT_SECURE_NO_WARNINGS
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Corpo de leitura do arquivo adaptado do arquivo filtro_windows.c no reposit�rio. */
int main(int argc, char *argv[]) {
    clock_t start, end;
    double cpu_time_used;
    char filetype[256], *ptri, *ptro, *img;
    char r, g, b;
    int width, height, depth, pixels, i;
    int rn ; // ruido
    FILE *fp = fopen(argv[1], "rb"); // leitura do arquivo passado com nome no prompt

    srand((unsigned)time(NULL)); // para valores randomicos
    


    fscanf(fp, "%s\n", filetype);
    fprintf(stdout, "%s\n", filetype);

    fscanf(fp, "%d %d\n%d\n", &width, &height, &depth);
    fprintf(stdout, "%d %d\n%d\n", width, height, depth);

    pixels = width * height;
    ptri = ptro = img = (char *)malloc(pixels * 3);

    fread(img, 3, pixels, fp);
        
    
    start = clock();
    
    for (i = 0; i < pixels; i++) {
        r = *ptri++; // n�o pode ser int ou a conta n�o funciona adequadamente e a convers�o para char borra completamente a imagem
        g = *ptri++;
        b = *ptri++;
	   rn = rand() % 30;

       
       __asm {
                movzx eax, r
                movzx ebx, g
                movzx ecx, b
                                
                shl ebx, 1 // g *= 2
                add eax, ebx
                add eax, ecx // r = r + g + b
                shr eax, 2 // r /= 4

                cmp al, 40 // ignora valores muito baixos para n�o manchar a imagem com pontos pretos ou brancos
                jb skip
                sub eax, rn
                
          skip: mov ebx, eax
                mov ecx, eax

			 mov edx, 2
			 mul edx
			 mov esi, 3
			 mov edx, 0
			 div esi

			 xchg ebx, eax

			 shr ecx, 1 // torna a imagem avermelhada
			 shr ecx, 1 // torna a imagem amarelada

			 cmp al, 40 // ignora valores muito baixos para n�o manchar a imagem com pontos pretos ou brancos
                jb skip1
                sub eax, rn

	
	skip1:	 cmp bl, 40 // ignora valores muito baixos para n�o manchar a imagem com pontos pretos ou brancos
                jb skip2
                sub ebx, rn

	
	skip2:	 cmp cl, 40 // ignora valores muito baixos para n�o manchar a imagem com pontos pretos ou brancos
                jb skip3
                sub ecx, rn

      
	skip3:     mov r, al
                mov g, bl
                mov b, cl		 

        }

         *ptro++ = r;
         *ptro++ = g;
         *ptro++ = b;
    }
    
    end = clock();

    fwrite(img, 3, pixels, stdout);

    free(img);

    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    fprintf(stderr, "tempo = %f segundos\n", cpu_time_used);
    return 0;
}