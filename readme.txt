Tarea 2: Lectores y escritores — Políticas de prioridad: monitores y operaciones atómicas
Sistemas Operativos, 2026

1. Compilación
--------------
Para compilar el ejecutable principal `rw` utilizando Make:

    make

O directamente con g++:

    g++ -std=c++17 -O2 -Wall -Wextra -pthread rw.cpp monitor_rw.cpp control_atomico_rw.cpp verificador.cpp -o rw

Para compilar con ThreadSanitizer (búsqueda de carreras de datos):

    make tsan

2. Ejecución
------------
Formato general (R1):

    ./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]

Parámetros:
  - <modo>       : cond | atomic
  - <politica>   : lectores | escritores | fifo  (atomic solo admite escritores)
  - <L>          : número de hebras lectoras (L >= 1)
  - <E>          : número de hebras escritoras (E >= 1, L + E <= 1000)
  - <K>          : operaciones por hebra (K >= 1)
  - <dur_lec_us> : duración simulada de cada lectura en microsegundos (>= 0)
  - <dur_esc_us> : duración simulada de cada escritura en microsegundos (>= 0)
  - [notraza]    : parámetro opcional para desactivar el registro de traza (E2)

Ejemplos:
    ./rw cond escritores 6 2 100 200 500
    ./rw atomic escritores 6 2 100 200 500
    ./rw cond escritores 6 2 100 200 500 notraza
