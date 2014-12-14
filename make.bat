cl Envelhecimento.c
cl EnvelhecimentoSIMD.c

for /f %%f in ('dir /b in\') do Envelhecimento in\%%f out\%%f
for /f %%f in ('dir /b in\') do EnvelhecimentoSIMD in\%%f out\SIMD_%%f