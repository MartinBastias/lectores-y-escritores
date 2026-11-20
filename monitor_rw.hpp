#ifndef MONITOR_RW_HPP
#define MONITOR_RW_HPP

#include <condition_variable>
#include <deque>
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
    std::mutex m;
    std::vector<AnotacionTraza> traza;

    // [A] Estado de las políticas Lectores y Escritores.
    long lectoras_activas = 0;
    bool escritora_activa = false;
    long lectoras_pend = 0;
    long escritoras_pend = 0;

    std::condition_variable cv_lectoras;
    std::condition_variable cv_escritoras;

    // [B] Estado de la política FIFO.
    enum class TipoEspera { Lectora, Escritora };
    long turno_sig = 0;
    long turno_atendido = 0;
    std::deque<std::pair<long, TipoEspera>> cola_fifo;
};

#endif // MONITOR_RW_HPP
