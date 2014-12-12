cl Envelhecimento.c

for /f %%f in ('dir /b in\') do Envelhecimento in\%%f out\%%f

cl EnvelhecimentoSIMD.c

for /f %%f in ('dir /b in\') do EnvelhecimentoSIMD in\%%f out_simd\%%f