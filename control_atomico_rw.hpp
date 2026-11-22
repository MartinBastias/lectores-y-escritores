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

    // R9: Formato de bits de `estado` (64 bits):
    // - Bits  0..30 (31 bits): número de lectoras activas    (máx. 2^31 - 1 = 2.147.483.647)
    // - Bit  31     ( 1 bit ): bandera de escritora activa   (0 o 1)
    // - Bits 32..63 (32 bits): número de escritoras esperando (máx. 2^32 - 1 = 4.294.967.295)
    //
    // Con L + E <= 1000, tanto las lectoras activas (< 1000) como las escritoras esperando
    // (< 1000) caben holgadamente sin desbordar ni invadir campos adyacentes.
    static constexpr unsigned SHIFT_LECTORAS         = 0;
    static constexpr unsigned BITS_LECTORAS          = 31;
    static constexpr unsigned SHIFT_ESCRITORA_ACTIVA = 31;
    static constexpr unsigned SHIFT_ESCRITORAS_ESP   = 32;

    static constexpr std::uint64_t UNIDAD_LECTORA       = std::uint64_t{1} << SHIFT_LECTORAS;
    static constexpr std::uint64_t MASK_LECTORAS        = (std::uint64_t{1} << BITS_LECTORAS) - 1;
    static constexpr std::uint64_t BIT_ESCRITORA_ACTIVA = std::uint64_t{1} << SHIFT_ESCRITORA_ACTIVA;
    static constexpr std::uint64_t UNIDAD_ESCRITORA_ESP = std::uint64_t{1} << SHIFT_ESCRITORAS_ESP;

    static_assert((MASK_LECTORAS & BIT_ESCRITORA_ACTIVA) == 0);
    static_assert(((MASK_LECTORAS | BIT_ESCRITORA_ACTIVA) & ~(UNIDAD_ESCRITORA_ESP - 1)) == 0);
    static_assert(MASK_LECTORAS >= 1000);
    static_assert((~std::uint64_t{0} >> SHIFT_ESCRITORAS_ESP) >= 1000);

    static constexpr std::uint64_t lectoras_activas(std::uint64_t s) noexcept {
        return s & MASK_LECTORAS;
    }

    static constexpr bool hay_escritora_activa(std::uint64_t s) noexcept {
        return (s & BIT_ESCRITORA_ACTIVA) != 0;
    }

    static constexpr std::uint64_t escritoras_esperando(std::uint64_t s) noexcept {
        return s >> SHIFT_ESCRITORAS_ESP;
    }

    std::atomic<std::uint64_t> estado{0};
};

#endif // CONTROL_ATOMICO_RW_HPP
