# Tarea 2: Lectores y escritores

**Políticas de prioridad: monitores y operaciones atómicas**  
*Sistemas Operativos, 2026*

## Estructura modular del proyecto

El proyecto está dividido en módulos independientes siguiendo las especificaciones del enunciado para facilitar el trabajo colaborativo del equipo:

| Archivo | Descripción y Responsabilidad | Requisitos |
| :--- | :--- | :--- |
| `control_rw.hpp` | Interfaz base abstracta `ControlRW`, `enum class Politica`, estructura compartida `Registro` y declaración de `thread_local int id_hebra`. | Base común, R2, R6 |
| `monitor_rw.hpp` / `monitor_rw.cpp` | Clase `MonitorRW` (Parte I) que utiliza `std::mutex` y `std::condition_variable` para implementar las 3 políticas (`Lectores`, `Escritores`, `Fifo`) y registrar la traza de admisión. | Parte I: R3, R4, R5, R6, R7 |
| `control_atomico_rw.hpp` / `control_atomico_rw.cpp` | Clase `ControlAtomicoRW` (Parte II) que implementa la política de prioridad de escritores empaquetando el estado en un `std::atomic<std::uint64_t>` con CAS y contabilizando intentos fallidos por causa. | Parte II: R8, R9, R10 |
| `verificador.hpp` / `verificador.cpp` | Estructuras `AnotacionTraza`, `ResultadoVerificacion` y función `verificar_traza()` para comprobar invariantes de admisión, prioridad de cada política e indicador de evidencia. | R6 |
| `rw.cpp` | Programa principal: validación de argumentos de línea de comandos, creación y `join()` de hebras lectoras/escritoras, verificación de invariantes sobre `Registro` y medición de tiempos de pared/CPU/espera. | R1, R2, E1, E2 |
| `Makefile` | Reglas de compilación estándar (`make`), con ThreadSanitizer (`make tsan`) y limpieza (`make clean`). | Sección 5 |
| `readme.txt` | Instrucciones de compilación y ejecución requeridas para la entrega. | Sección 5 |

## Compilación rápida

```bash
make
```

O manualmente con `g++`:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pthread rw.cpp monitor_rw.cpp control_atomico_rw.cpp verificador.cpp -o rw
```
