#!/bin/bash
# Ejecuta E1: cada caso 5 veces. Uso: ./run_e1.sh [ejecutable]  (por defecto ./rw)
# Guarda la salida completa en e1_resultados.txt y un resumen en e1_resumen.csv
RW=${1:-./rw}
REPS=5
CONFIGS=("1 1 20 500 500" "6 2 100 200 500" "2 6 100 500 200" "8 8 100 0 0" "8 1 200 1000 100")
CASOS=("cond lectores" "cond escritores" "cond fifo" "atomic escritores")
 
: > e1_resultados.txt
echo "modo,politica,L,E,K,dur_lec,dur_esc,rep,correcto,politica_ok,evidencia,max_lectoras,pared_ms,cpu_ms" > e1_resumen.csv
 
for caso in "${CASOS[@]}"; do
  for cfg in "${CONFIGS[@]}"; do
    for r in $(seq 1 $REPS); do
      echo "### $caso $cfg (rep $r)" >> e1_resultados.txt
      salida=$($RW $caso $cfg 2>&1)
      echo "$salida" >> e1_resultados.txt
      correcto=$(echo "$salida" | sed -n 's/^correcto=//p')
      pok=$(echo "$salida" | sed -n 's/^politica_ok=//p')
      evid=$(echo "$salida" | sed -n 's/^evidencia=\([0-9]*\).*/\1/p')
      maxl=$(echo "$salida" | grep "Max. lectoras simultaneas" | awk '{print $NF}')
      pared=$(echo "$salida" | grep "Pared" | awk '{print $(NF-1)}')
      cpu=$(echo "$salida" | grep "CPU del proceso" | awk '{print $(NF-1)}')
      echo "${caso// /,},${cfg// /,},$r,$correcto,$pok,$evid,$maxl,$pared,$cpu" >> e1_resumen.csv
    done
  done
done
echo "Listo: e1_resultados.txt y e1_resumen.csv"
 
