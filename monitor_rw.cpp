#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), registrar_traza(registrar_traza) {
    // TODO: Inicializar estado de admisión si corresponde
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO (R6): Si `registrar_traza` está activo, registrar anotación Accion::Pide
    // antes de esperar en una variable de condición (asignar turno si politica == Politica::Fifo, o -1).
    (void)politica;
    (void)registrar_traza;

    // TODO (R3, R4, R5, R7): Esperar con while + wait(lock) según la política:
    // - Politica::Lectores (R3): esperar solo si hay escritora activa.
    // - Politica::Escritores (R4): esperar si hay escritora activa o escritoras esperando.
    // - Politica::Fifo (R5): respetar orden de turnos (admitiendo lectoras consecutivas
    //   mientras no haya escritora con turno anterior pendiente ni activa).

    // TODO (R6): Si `registrar_traza` está activo, registrar anotación Accion::Entra
    // justo antes de retornar.
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO: Actualizar estado de admisión al salir una lectora
    // TODO (R6): Si `registrar_traza` está activo, registrar anotación Accion::Sale
    // TODO (R7): Notificar a las hebras habilitadas según la política
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO (R6): Si `registrar_traza` está activo, registrar anotación Accion::Pide
    // antes de esperar en una variable de condición (asignar turno si politica == Politica::Fifo, o -1).

    // TODO (R3, R4, R5, R7): Esperar con while + wait(lock) según la política:
    // - Politica::Lectores (R3): esperar si hay lectoras activas, escritora activa o lectoras pendientes.
    // - Politica::Escritores (R4): esperar si hay lectoras activas o escritora activa.
    // - Politica::Fifo (R5): esperar turno correspondiente y ausencia de lectoras/escritora activas.

    // TODO (R6): Si `registrar_traza` está activo, registrar anotación Accion::Entra
    // justo antes de retornar.
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO: Actualizar estado de admisión al salir una escritora
    // TODO (R6): Si `registrar_traza` está activo, registrar anotación Accion::Sale
    // TODO (R7): Notificar a las hebras habilitadas según la política
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
