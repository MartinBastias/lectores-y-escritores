#ifndef MONITOR_RW_HPP
#define MONITOR_RW_HPP

#include <cstddef>
#include <deque>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>
#include "control_rw.hpp"
#include "verificador.hpp"

/**
 * Parte I: Monitor con tres políticas (R3: Lectores, R4: Escritores, R5: FIFO).
 *
 * Reglas de monitor:
 * - El estado de admisión (contadores, turnos, banderas) es privado.
 * - Cada método público adquiere el mutex `m` al comenzar (std::unique_lock<std::mutex>)
 *   y lo libera al retornar.
 * - Un método que debe esperar lo hace con std::condition_variable::wait() en un ciclo while.
 */
class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica p, bool registrar_traza = true);

    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

    std::vector<AnotacionTraza> obtener_traza(); // ver R6

private:
    Politica politica;
    bool registrar_traza;
    std::mutex m;                        // mutex del monitor
    std::vector<AnotacionTraza> traza;   // ver R6

    // Variables de condición y estado de admisión privado
    std::size_t lectores_activos = 0;
    std::size_t lectores_pendientes = 0;
    std::size_t escritoras_activas = 0;
    std::size_t escritoras_pendientes = 0;

    // FIFO: cola de tipos y turno numérico. El turno asignado empieza en 0.
    enum class TipoEspera { Lectora, Escritora };
    std::deque<std::pair<long, TipoEspera>> cola_fifo;
    long siguiente_turno = 0;

    std::condition_variable cv_lectores;
    std::condition_variable cv_escritoras;
};

#endif // MONITOR_RW_HPP
