#include "control_atomico_rw.hpp"
#include <thread>

thread_local ContadoresAtomicos contadores_atomicos_hebra;

void ControlAtomicoRW::entrar_lectura() {
    std::uint64_t actual = estado.load(std::memory_order_relaxed);
    while (true) {
        if (hay_escritora_activa(actual)) {
            contadores_atomicos_hebra.recurso_ocupado++;
            std::this_thread::yield();
            actual = estado.load(std::memory_order_relaxed);
            continue;
        }
        if (escritoras_esperando(actual) > 0) {
            contadores_atomicos_hebra.escritoras_esperando++;
            std::this_thread::yield();
            actual = estado.load(std::memory_order_relaxed);
            continue;
        }
        const std::uint64_t nuevo = actual + UNIDAD_LECTORA;
        if (estado.compare_exchange_weak(actual, nuevo,
                                         std::memory_order_acq_rel,
                                         std::memory_order_relaxed)) {
            return;
        }
        contadores_atomicos_hebra.cas_fallido++;
    }
}

void ControlAtomicoRW::salir_lectura() {
    estado.fetch_sub(UNIDAD_LECTORA, std::memory_order_acq_rel);
}

void ControlAtomicoRW::entrar_escritura() {
    std::uint64_t actual =
        estado.fetch_add(UNIDAD_ESCRITORA_ESP, std::memory_order_acq_rel) +
        UNIDAD_ESCRITORA_ESP;

    while (true) {
        if (hay_escritora_activa(actual) || lectoras_activas(actual) > 0) {
            contadores_atomicos_hebra.recurso_ocupado++;
            std::this_thread::yield();
            actual = estado.load(std::memory_order_relaxed);
            continue;
        }
        const std::uint64_t nuevo =
            (actual - UNIDAD_ESCRITORA_ESP) | BIT_ESCRITORA_ACTIVA;
        if (estado.compare_exchange_weak(actual, nuevo,
                                         std::memory_order_acq_rel,
                                         std::memory_order_relaxed)) {
            return;
        }
        contadores_atomicos_hebra.cas_fallido++;
    }
}

void ControlAtomicoRW::salir_escritura() {
    estado.fetch_and(~BIT_ESCRITORA_ACTIVA, std::memory_order_acq_rel);
}
