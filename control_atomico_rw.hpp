#ifndef CONTROL_ATOMICO_RW_HPP
#define CONTROL_ATOMICO_RW_HPP

#include <atomic>
#include <cstdint>
#include "control_rw.hpp"

/**
 * R10. Contadores de intentos de admisión fallidos separados por causa.
 * Cada hebra lleva sus propios contadores locales (por ejemplo en una variable
 * thread_local) y se suman después de los join().
 *
 * Prioridad de conteo si se cumple más de una causa en un mismo intento:
 * contar como `recurso_ocupado`.
 */
struct ContadoresAtomicos {
    long recurso_ocupado = 0;       // escritora activa o (para escritora) lectoras activas
    long escritoras_esperando = 0;  // solo lectoras: había escritoras esperando
    long cas_fallido = 0;           // fallo de compare_exchange (cambio por otra hebra o espurio)
};

extern thread_local ContadoresAtomicos contadores_atomicos_hebra;

/**
 * Parte II: Prioridad de escritores con operaciones atómicas y CAS (R8, R9, R10).
 *
 * Restricciones (R9):
 * - Estado de sincronización en una única palabra std::atomic<std::uint64_t>.
 * - Prohibido usar std::mutex, std::condition_variable, std::shared_mutex, semáforos,
 *   spinlocks de biblioteca, pthread_rwlock_t o std::atomic::wait().
 */
class ControlAtomicoRW : public ControlRW {
public:
    ControlAtomicoRW() = default;

    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

private:
    // R9: Comprobar que la palabra de 64 bits no usa un candado interno
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

    // TODO (R9): Definir máscaras y desplazamientos de bits para empaquetar en `estado`:
    // - Número de lectoras activas
    // - Si hay una escritora activa
    // - Número de escritoras esperando
    // Justificar que ningún campo se desborda con L + E <= 1000.
    std::atomic<std::uint64_t> estado{0};
};

#endif // CONTROL_ATOMICO_RW_HPP
