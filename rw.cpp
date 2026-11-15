#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "control_atomico_rw.hpp"
#include "control_rw.hpp"
#include "monitor_rw.hpp"
#include "verificador.hpp"

// R6: Variable global por hebra con el identificador lógico
thread_local int id_hebra = -1;

/**
 * R1. Parámetros de ejecución recibidos por línea de comandos:
 * ./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]
 */
struct Parametros {
    std::string modo;         // "cond" o "atomic"
    Politica politica;        // Lectores, Escritores, Fifo
    std::string politica_str; // "lectores", "escritores", "fifo"
    long L = 0;               // L >= 1 lectoras
    long E = 0;               // E >= 1 escritoras (L + E <= 1000)
    long K = 0;               // K >= 1 operaciones por hebra
    long dur_lec_us = 0;      // duración simulada de lectura (>= 0 us)
    long dur_esc_us = 0;      // duración simulada de escritura (>= 0 us)
    bool traza_activa = true; // false si se pasa "notraza"
};

/**
 * R2. Contadores de diagnóstico atómicos para verificar invariantes
 * (separados del estado interno de admisión de las clases).
 */
struct DiagnosticoR2 {
    std::atomic<long> lectoras_activas{0};
    std::atomic<long> escritoras_activas{0};
    std::atomic<long> max_lectoras_simultaneas{0};
    std::atomic<long> inconsistencias{0};
    std::atomic<long> violaciones_exclusion{0};
};

/**
 * E2. Métricas locales recolectadas por cada hebra para agregar al final sin contención.
 */
struct MetricasHebra {
    double suma_espera_us = 0.0;
    double max_espera_us = 0.0;
    ContadoresAtomicos contadores_atomicos;
};

/**
 * R1. Parsea y valida los argumentos de la línea de comandos.
 * Retorna true si son válidos, false en caso contrario.
 */
bool parsear_argumentos(int argc, char* argv[], Parametros& params) {
    // TODO (R1):
    // - Verificar cantidad de argumentos (8 o 9 si incluye "notraza")
    // - Validar modo ("cond" o "atomic") y politica ("lectores", "escritores", "fifo")
    // - Rechazar combinación no implementada: modo "atomic" con política distinta de "escritores"
    // - Validar rangos: L >= 1, E >= 1, L + E <= 1000, K >= 1, dur_lec_us >= 0, dur_esc_us >= 0
    // - Comprobar que 2*E*K y (L+E)*K quepan en un `long` sin desbordamiento
    // - Si hay 9no argumento, verificar que sea exactamente "notraza"
    (void)argc;
    (void)argv;
    (void)params;
    return false;
}

/**
 * Función ejecutada por cada hebra lectora.
 */
void rutina_lectora(int id, const Parametros& params, ControlRW& control,
                    const Registro& registro, DiagnosticoR2& diag, MetricasHebra& metricas) {
    id_hebra = id;

    // TODO (R2, E2): Ejecutar K iteraciones:
    // 1. Medir tiempo de espera de `control.entrar_lectura()`
    // 2. Actualizar contadores de diagnóstico atómicos (lectoras_activas, max_lectoras_simultaneas,
    //    y verificar que escritoras_activas == 0)
    // 3. Leer `registro.version` y `registro.doble_version` y verificar `doble_version == 2 * version`
    // 4. Si `params.dur_lec_us > 0`, simular trabajo con std::this_thread::sleep_for(...)
    // 5. Actualizar diagnóstico antes de salir y llamar a `control.salir_lectura()`
    // 6. Al terminar las K operaciones, copiar `contadores_atomicos_hebra` en `metricas.contadores_atomicos`
    (void)params;
    (void)control;
    (void)registro;
    (void)diag;
    (void)metricas;
}

/**
 * Función ejecutada por cada hebra escritora.
 */
void rutina_escritora(int id, const Parametros& params, ControlRW& control,
                      Registro& registro, DiagnosticoR2& diag, MetricasHebra& metricas) {
    id_hebra = id;

    // TODO (R2, E2): Ejecutar K iteraciones:
    // 1. Medir tiempo de espera de `control.entrar_escritura()`
    // 2. Actualizar contadores de diagnóstico atómicos (escritoras_activas, y verificar que
    //    no haya otra escritora ni lectoras activas)
    // 3. Modificar registro: `registro.version++` y luego `registro.doble_version = 2 * registro.version`
    // 4. Si `params.dur_esc_us > 0`, simular trabajo con std::this_thread::sleep_for(...)
    // 5. Actualizar diagnóstico antes de salir y llamar a `control.salir_escritura()`
    // 6. Al terminar las K operaciones, copiar `contadores_atomicos_hebra` en `metricas.contadores_atomicos`
    (void)params;
    (void)control;
    (void)registro;
    (void)diag;
    (void)metricas;
}

int main(int argc, char* argv[]) {
    Parametros params;
    if (!parsear_argumentos(argc, argv, params)) {
        std::cerr << "Uso: " << (argc > 0 ? argv[0] : "./rw")
                  << " <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]\n";
        return 1;
    }

    // TODO (R1, R2, E1, E2):
    // 1. Instanciar `MonitorRW` o `ControlAtomicoRW` según `params.modo`
    // 2. Inicializar `Registro` en cero y `DiagnosticoR2`
    // 3. Iniciar medición de tiempo de pared (std::chrono::steady_clock) y tiempo de CPU
    //    (clock_gettime con CLOCK_PROCESS_CPUTIME_ID)
    // 4. Crear las L + E hebras con std::thread (manejando posibles excepciones de creación:
    //    si falla, hacer join() de las ya creadas, informar error y terminar)
    // 5. Esperar a todas las hebras con join()
    // 6. Detener medición de tiempo de pared y CPU
    // 7. Comprobar invariantes finales de R2:
    //    - version == E * K
    //    - doble_version == 2 * E * K
    //    - inconsistencias == 0 y sin violaciones de exclusión
    // 8. Si es modo "cond" y `params.traza_activa`, llamar a `verificar_traza()`
    // 9. Imprimir resumen con todos los datos solicitados en R2, R6, R10, E1 y E2.

    return 0;
}
