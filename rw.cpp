#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <system_error>
#include <functional>
#include <iomanip>
#include <cstdlib>
#include <cerrno>
#include <algorithm>

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

using Reloj = std::chrono::steady_clock;

/** 
 * Convierte un texto a long. False si no es un entero decimal completo o si desborda.
 */
static bool leer_long(const char* s, long& salida) {

    if (s == nullptr || *s == '\0') return false;

    char* fin = nullptr;
    errno = 0;
    long v = std::strtol(s, &fin, 10);

    if (errno != 0 || *fin != '\0') return false;

    salida = v;
    return true;
}

/**
 * R1. Parsea y valida los argumentos de la línea de comandos.
 * Retorna true si son válidos, false en caso contrario.
 */
bool parsear_argumentos(int argc, char* argv[], Parametros& params) {
    
    // Verificar cantidad de argumentos (8 o 9 si incluye "notraza")
    if (argc != 8 && argc != 9) {

        std::cerr << "Error: cantidad de argumentos invalida\n";
        return false;
    }

    // Validar modo ("cond" o "atomic")
    params.modo = argv[1];

    if (params.modo != "cond" && params.modo != "atomic") {
        std::cerr << "Error: el modo debe ser 'cond' o 'atomic'\n";
        return false;
    }

    // Validar politica ("lectores", "escritores", "fifo")
    params.politica_str = argv[2];

    if (params.politica_str == "lectores") {
        params.politica = Politica::Lectores;

    } else if (params.politica_str == "escritores") {
        params.politica = Politica::Escritores;

    } else if (params.politica_str == "fifo") {
        params.politica = Politica::Fifo;

    } else {
        std::cerr << "Error: politica debe ser 'lectores', 'escritores' o 'fifo'\n";
        return false;
    }

    // Rechazar combinación no implementada: modo "atomic" con política distinta de "escritores"
    if (params.modo == "atomic" && params.politica != Politica::Escritores) {
        std::cerr << "Error: el modo 'atomic' solo implementa la politica 'escritores'\n";
        return false;
    }   
    
    // Validar que L, E, K, dur_lec_us y dur_esc_us sean números enteros (long).    
    if (!leer_long(argv[3], params.L) || !leer_long(argv[4], params.E) || !leer_long(argv[5], params.K) 
    || !leer_long(argv[6], params.dur_lec_us) || !leer_long(argv[7], params.dur_esc_us)) {
        std::cerr << "Error: L, E, K, dur_lec_us y dur_esc_us deben ser enteros validos\n";
        return false;
    }

    // Validar rangos: L >= 1, E >= 1, L + E <= 1000, K >= 1, dur_lec_us >= 0, dur_esc_us >= 0
    if (params.L < 1 || params.E < 1 || params.L + params.E > 1000) {
        std::cerr << "Error: se requiere que L >= 1, E >= 1 y que L+E <= 1000\n";
        return false;
    }

    if (params.K < 1) {
        std::cerr << "Error: se requiere que K >= 1\n";
        return false;
    }

    if (params.dur_esc_us < 0 || params.dur_lec_us < 0) {
        std::cerr << "Error: se requiere que las duraciones sean >= 0\n";
        return false;
    }

    // Comprobar que 2*E*K y (L+E)*K quepan en un `long` sin desbordamiento
    const long maxl  = std::numeric_limits<long>::max();

    if (params.K > maxl / (2*params.E) || params.K > maxl / (params.L + params.E)) {
        std::cerr << "Error: 2*E*K o (L+E)*K no caben en un long\n";
        return false;
    }

    // Si hay 9no argumento, verificar que sea exactamente "notraza"
    params.traza_activa = true;

    if (argc == 9) {
        if (std::string(argv[8]) != "notraza") {
            std::cerr << "Error: el 9no argumento solo puede ser 'notraza'\n";
            return false;
        }

        params.traza_activa = false;
    }

    return true;
}

/** 
 * Registra el máximo de lectoras simultáneas de forma atómica
*/
static void actualizar_maximo(std::atomic<long>& maximo, long valor) {

    long actual = maximo.load();

    while (valor > actual && !maximo.compare_exchange_weak(actual, valor)) {
    }
}

/**
 * Calcula el tiempo transcurrido en microsegundos entre t0 y t1, y actualiza
 * tanto la suma acumulada como el tiempo máximo de espera de la hebra.
 */
static void registrar_espera(MetricasHebra& m, Reloj::time_point t0, Reloj::time_point t1) {

    double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    m.suma_espera_us += us;
    m.max_espera_us = std::max(m.max_espera_us, us);
}

/**
 * Función ejecutada por cada hebra lectora.
 */
void rutina_lectora(int id, const Parametros& params, ControlRW& control,
                    const Registro& registro, DiagnosticoR2& diag, MetricasHebra& metricas) {
    id_hebra = id;

    for (long k = 0; k < params.K; ++k) {

        // 1. Medir tiempo de espera de 'control.entrar_lectura()'
        auto t0 = Reloj::now();
        control.entrar_lectura();
        auto t1 = Reloj::now();
        registrar_espera(metricas, t0, t1);

        // 2. Actualizar contadores de diagnóstico atómicos (lectoras_activas, max_lectoras_simultaneas,
        //    y verificar que escritoras_activas == 0)
        long n = diag.lectoras_activas.fetch_add(1) + 1;
        actualizar_maximo(diag.max_lectoras_simultaneas, n);

        if (diag.escritoras_activas.load() != 0) diag.violaciones_exclusion.fetch_add(1);

        // 3. Leer `registro.version` y `registro.doble_version` y verificar `doble_version == 2 * version`
        long v = registro.version;
        long dv = registro.doble_version;

        if (dv != 2*v) diag.inconsistencias.fetch_add(1);

        // 4. Si `params.dur_lec_us > 0`, simular trabajo con std::this_thread::sleep_for(...)
        if (params.dur_lec_us > 0) 
            std::this_thread::sleep_for(std::chrono::microseconds(params.dur_lec_us));

        // 5. Actualizar diagnóstico antes de salir y llamar a `control.salir_lectura()`
        diag.lectoras_activas.fetch_sub(1);
        control.salir_lectura();
    }

    // 6. Al terminar las K operaciones, copiar `contadores_atomicos_hebra` en `metricas.contadores_atomicos`
    metricas.contadores_atomicos = contadores_atomicos_hebra;
}

/**
 * Función ejecutada por cada hebra escritora.
 */
void rutina_escritora(int id, const Parametros& params, ControlRW& control,
                      Registro& registro, DiagnosticoR2& diag, MetricasHebra& metricas) {
    id_hebra = id;

    for (long k = 0; k < params.K; ++k) {

        // 1. Medir tiempo de espera de `control.entrar_escritura()`
        auto t0 = Reloj::now();
        control.entrar_escritura();
        auto t1 = Reloj::now();
        registrar_espera(metricas, t0, t1);
  
        // 2. Actualizar contadores de diagnóstico atómicos (escritoras_activas, y verificar que
        //    no haya otra escritora ni lectoras activas)
        long previas = diag.escritoras_activas.fetch_add(1);

        if (previas != 0 || diag.lectoras_activas.load() != 0) diag.violaciones_exclusion.fetch_add(1);

        // 3. Modificar registro: `registro.version++` y luego `registro.doble_version = 2 * registro.version`
        registro.version++;
        registro.doble_version = 2 * registro.version;
    
        // 4. Si `params.dur_esc_us > 0`, simular trabajo con std::this_thread::sleep_for(...)
        if (params.dur_esc_us > 0) 
            std::this_thread::sleep_for(std::chrono::microseconds(params.dur_esc_us));
    
        // 5. Actualizar diagnóstico antes de salir y llamar a `control.salir_escritura()`
        diag.escritoras_activas.fetch_sub(1);
        control.salir_escritura();
    }
    // 6. Al terminar las K operaciones, copiar `contadores_atomicos_hebra` en `metricas.contadores_atomicos`
    metricas.contadores_atomicos = contadores_atomicos_hebra;
}

/**
 * Imprime una fila "   etiqueta ........ valor" con las etiquetas alineadas.
 */
static void linea(const std::string& etiqueta, const std::string& valor) {

    std::string e = "   " + etiqueta + " ";

    while (e.size() < 33) e += '.';

    std::cout << e << " " << valor << "\n";
}

/**
 * Formatea un double con 3 decimales
 */
static std::string formato(double x) {
    
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << x;
    return os.str();
}

/**
 * Obtiene el tiempo acumulado de CPU consumido por el proceso actual en milisegundos.
 */
static double cpu_proceso_ms() {

    timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/**
 * Punto de entrada principal: valida parámetros, orquesta la ejecución concurrente de 
 * las hebras lectoras/escritoras, verifica invariantes y reporta métricas.
 * Retorna 0 si la ejecución es válida, 1 por error de argumentos o hebras, o 
 * 2 si fallan invariantes.
 */
int main(int argc, char* argv[]) {
    Parametros params;
    if (!parsear_argumentos(argc, argv, params)) {
        std::cerr << "Uso: " << (argc > 0 ? argv[0] : "./rw")
                  << " <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]\n";
        return 1;
    }

    // 1. Instanciar `MonitorRW` o `ControlAtomicoRW` según `params.modo`
    std::unique_ptr<ControlRW> control;
    MonitorRW* monitor = nullptr;

    if (params.modo == "cond") {
        auto mon = std::make_unique<MonitorRW>(params.politica, params.traza_activa);
        monitor = mon.get();
        control = std::move(mon);

    } else {
        control = std::make_unique<ControlAtomicoRW>();

    }

    // 2. Inicializar `Registro` en cero y `DiagnosticoR2`
    Registro registro;
    DiagnosticoR2 diag;
    const long total = params.L + params.E;
    std::vector<MetricasHebra> metricas(static_cast<size_t>(total));

    // 3. Iniciar medición de tiempo de pared (std::chrono::steady_clock) y tiempo de CPU
    //    (clock_gettime con CLOCK_PROCESS_CPUTIME_ID)
    auto pared0 = Reloj::now();
    double cpu0 = cpu_proceso_ms();

    // 4. Crear las L + E hebras con std::thread (manejando posibles excepciones de creación:
    //    si falla, hacer join() de las ya creadas, informar error y terminar)
    std::vector<std::thread> hebras;
    hebras.reserve(static_cast<size_t>(total));
    bool fallo_creacion = false;

    try {
        for (long i = 0; i < total; ++i) {
            if (i < params.L) {
                hebras.emplace_back(rutina_lectora, static_cast<int>(i), std::cref(params),
                 std::ref(*control), std::cref(registro), std::ref(diag), std::ref(metricas[static_cast<size_t>(i)]));

            } else {
                hebras.emplace_back(rutina_escritora, static_cast<int>(i), std::cref(params),
                 std::ref(*control), std::ref(registro), std::ref(diag), std::ref(metricas[static_cast<size_t>(i)]));
            }
        }
    } catch (const std::system_error& e){
        std::cerr <<  "Error al crear hebras: " << e.what() << "\n";
        fallo_creacion = true;
    }

    // 5. Esperar a todas las hebras con join()
    for(auto& h : hebras) {
        if (h.joinable()) h.join();
    }

    if (fallo_creacion) return 1;

    // 6. Detener medición de tiempo de pared y CPU
    double cpu_ms = cpu_proceso_ms() - cpu0;
    double pared_ms = std::chrono::duration<double, std::milli>(Reloj::now() - pared0).count();

    // 7. Comprobar invariantes finales de R2:
    //    - version == E * K
    //    - doble_version == 2 * E * K
    //    - inconsistencias == 0 y sin violaciones de exclusión
    const long esperado_version = params.E * params.K;
    const long esperado_doble = 2 * params.E * params.K;
    const long incons = diag.inconsistencias.load();
    const long viol = diag.violaciones_exclusion.load();
    const bool invariantes_ok = registro.version == esperado_version &&
                                registro.doble_version == esperado_doble &&
                                incons == 0 && viol == 0 &&
                                diag.lectoras_activas.load() == 0 &&
                                diag.escritoras_activas.load() == 0;

    // Agregación de métricas (lectoras: ids 0..L-1, escritoras: L..L+E-1)
    double suma_l = 0, max_l = 0, suma_e = 0, max_e = 0;
    ContadoresAtomicos fallos;
    for (long i = 0; i < total; ++i) {
        const MetricasHebra& m = metricas[static_cast<size_t>(i)];
        if (i < params.L) {
            suma_l += m.suma_espera_us;
            max_l = std::max(max_l, m.max_espera_us);
        } else {
            suma_e += m.suma_espera_us;
            max_e = std::max(max_e, m.max_espera_us);
        }
        fallos.recurso_ocupado += m.contadores_atomicos.recurso_ocupado;
        fallos.escritoras_esperando += m.contadores_atomicos.escritoras_esperando;
        fallos.cas_fallido += m.contadores_atomicos.cas_fallido;
    }
    const double prom_l = suma_l / static_cast<double>(params.L * params.K);
    const double prom_e = suma_e / static_cast<double>(params.E * params.K);

    // 8. Si es modo "cond" y `params.traza_activa`, llamar a `verificar_traza()`
    bool verifico_traza = (params.modo == "cond" && params.traza_activa);
    ResultadoVerificacion ver;
    if (verifico_traza)
        ver = verificar_traza(monitor->obtener_traza(), params.politica);

    // 9. Imprimir resumen con todos los datos solicitados en R2, R6, R10, E1 y E2.
    const bool correcto = invariantes_ok && (!verifico_traza || ver.politica_ok);

    std::cout << "============================================================\n"
              << " RESUMEN DE EJECUCION\n"
              << "============================================================\n";

    std::cout << "\n Configuracion\n";
    linea("Modo", params.modo);
    linea("Politica", params.politica_str);
    linea("Lectoras (L)", std::to_string(params.L));
    linea("Escritoras (E)", std::to_string(params.E));
    linea("Operaciones por hebra (K)", std::to_string(params.K));
    linea("Duracion lectura", std::to_string(params.dur_lec_us) + " us");
    linea("Duracion escritura", std::to_string(params.dur_esc_us) + " us");
    linea("Traza", params.traza_activa ? "SI" : "NO");

    std::cout << "\n Operaciones y registro compartido\n";
    linea("Operaciones de lectura", std::to_string(params.L * params.K));
    linea("Operaciones de escritura", std::to_string(params.E * params.K));
    linea("version final", std::to_string(registro.version) +
                           "  (esperado " + std::to_string(esperado_version) + ")");
    linea("doble_version final", std::to_string(registro.doble_version) +
                                 "  (esperado " + std::to_string(esperado_doble) + ")");
    linea("Inconsistencias", std::to_string(incons));
    linea("Violaciones de exclusion", std::to_string(viol));
    linea("Max. lectoras simultaneas", std::to_string(diag.max_lectoras_simultaneas.load()));

    std::cout << "\n Tiempos\n";
    linea("Pared", formato(pared_ms) + " ms");
    linea("CPU del proceso", formato(cpu_ms) + " ms");

    std::cout << "\n Espera por solicitud (us)      promedio        maximo\n";
    std::cout << std::fixed << std::setprecision(3)
              << "   Lectoras ................ " << std::setw(12) << prom_l << std::setw(14) << max_l << "\n"
              << "   Escritoras .............. " << std::setw(12) << prom_e << std::setw(14) << max_e << "\n";

    if (params.modo == "atomic") {
        std::cout << "\n Intentos de admision fallidos\n";
        linea("Recurso ocupado", std::to_string(fallos.recurso_ocupado));
        linea("Escritoras esperando", std::to_string(fallos.escritoras_esperando));
        linea("CAS fallido", std::to_string(fallos.cas_fallido));
    }

    std::cout << "\n Verificacion\n";
    if (verifico_traza) {
        std::cout << "politica_ok=" << (ver.politica_ok ? "SI" : "NO") << "\n"
                  << "evidencia=" << ver.evidencia
                  << "  (" << nombre_evidencia(params.politica) << ")\n";
        if (!ver.politica_ok)
            std::cout << "violacion en anotacion " << ver.indice_violacion
                      << ": " << ver.regla << "\n";
    }
    std::cout << "correcto=" << (correcto ? "SI" : "NO") << "\n";

    return correcto ? 0 : 2;
}   
