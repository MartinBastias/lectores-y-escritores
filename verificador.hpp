#ifndef VERIFICADOR_HPP
#define VERIFICADOR_HPP

#include <string>
#include <vector>
#include "control_rw.hpp"

/**
 * R6. Tipos y estructuras para la traza de admisión del monitor.
 */
enum class Tipo {
    Lectora,
    Escritora
};

enum class Accion {
    Pide,
    Entra,
    Sale
};

struct AnotacionTraza {
    int hebra;      // identificador lógico de la hebra (id_hebra)
    Tipo tipo;
    Accion accion;
    long turno;     // turno FIFO (desde 0); -1 en las otras políticas
};

/**
 * Resultado retornado por el verificador de traza (R6).
 */
struct ResultadoVerificacion {
    bool politica_ok = true;
    long evidencia = 0;           // Indicador de evidencia según la política
    long indice_violacion = -1;   // Índice de la anotación donde falló (-1 si no hay fallo)
    std::string regla;            // Regla incumplida en la primera violación
};

/**
 * R6. Recorre la traza reconstruyendo lectoras/escritoras activas y pendientes,
 * comprueba las reglas de exclusión y prioridad en cada anotación Entra,
 * y calcula el indicador de evidencia.
 */
ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza>& traza, Politica politica);

/** Nombre del indicador de evidencia de cada política. */
const char* nombre_evidencia(Politica politica);

#endif // VERIFICADOR_HPP
