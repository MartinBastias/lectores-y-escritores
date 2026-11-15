#include "control_atomico_rw.hpp"
#include <thread>

thread_local ContadoresAtomicos contadores_atomicos_hebra;

void ControlAtomicoRW::entrar_lectura() {
    // TODO (R9, R10):
    // - Leer `estado` con load()
    // - Comprobar que no haya escritora activa ni escritoras esperando:
    //   * Si hay escritora activa -> contadores_atomicos_hebra.recurso_ocupado++
    //   * Si no, pero hay escritoras esperando -> contadores_atomicos_hebra.escritoras_esperando++
    // - Si puede ingresar, intentar incrementar lectoras activas mediante CAS
    //   (compare_exchange_weak o compare_exchange_strong).
    //   * Si el CAS falla -> contadores_atomicos_hebra.cas_fallido++
    // - Reintentar activamente (opcionalmente con std::this_thread::yield()).
    (void)estado;
}

void ControlAtomicoRW::salir_lectura() {
    // TODO (R9): Decrementar el número de lectoras activas en `estado` de forma atómica.
}

void ControlAtomicoRW::entrar_escritura() {
    // TODO (R9, R10):
    // - Registrar la escritora como esperando antes de reintentar su admisión,
    //   para bloquear el ingreso de nuevas lectoras.
    // - Mientras haya escritora activa o lectoras activas:
    //   * contadores_atomicos_hebra.recurso_ocupado++
    // - Cuando el recurso esté libre, intentar mediante un mismo CAS descontarla
    //   de las esperando y marcarla como escritora activa.
    //   * Si el CAS falla -> contadores_atomicos_hebra.cas_fallido++
}

void ControlAtomicoRW::salir_escritura() {
    // TODO (R9): Desmarcar la escritora activa en `estado` de forma atómica.
}
