#ifndef MONITOR_RW_HPP
#define MONITOR_RW_HPP

#include <condition_variable>
#include <mutex>
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

    // TODO: Definir variables de condición y estado de admisión privado
    // Ejemplos de lo que podrían necesitar para R3, R4 y R5:
    // - Contadores de lectoras activas / pendientes
    // - Contadores o bandera de escritoras activas / pendientes
    // - Contadores o cola de turnos para la política FIFO (R5)
    // - std::condition_variable para bloquear/despertar lectoras y escritoras
};

#endif // MONITOR_RW_HPP
