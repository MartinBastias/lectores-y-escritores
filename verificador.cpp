#include "verificador.hpp"
#include <iostream>

ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza>& traza, Politica politica) {
    ResultadoVerificacion res;

    // TODO (R6): Recorrer `traza` reconstruyendo en cada paso:
    // - lectoras_activas (entraron y no han salido)
    // - escritoras_activas (entraron y no han salido)
    // - lectoras_pendientes (pidieron y no han entrado)
    // - escritoras_pendientes (pidieron y no han entrado)
    //
    // Reglas a comprobar en cada anotación Accion::Entra:
    // 1. Todas las políticas:
    //    - Una escritora entra sin lectoras activas ni escritora activa.
    //    - Una lectora entra sin escritora activa.
    // 2. Politica::Lectores:
    //    - Una escritora entra sin lectoras pendientes.
    //    - Si una lectora pide cuando no hay escritora activa, la anotación siguiente es su Entra.
    // 3. Politica::Escritores:
    //    - Una lectora entra sin escritoras pendientes.
    // 4. Politica::Fifo:
    //    - Los ingresos siguen el orden de los turnos: el k-ésimo Entra (desde 0)
    //      corresponde al turno k.
    //
    // Indicador de evidencia a contar en `res.evidencia`:
    // - Politica::Lectores: lectoras que entraron mientras había escritoras pendientes.
    // - Politica::Escritores: lectoras que pidieron mientras había escritoras pendientes.
    // - Politica::Fifo: lectoras que entraron mientras otra lectora estaba activa.
    //
    // Si se encuentra una violación:
    // - Informar el número de la anotación y la regla incumplida.
    // - Marcar `res.politica_ok = false` y guardar `res.indice_violacion`.

    (void)traza;
    (void)politica;

    return res;
}
