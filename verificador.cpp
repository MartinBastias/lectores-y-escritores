#include "verificador.hpp"
#include <iostream>
#include <unordered_map>

const char* nombre_evidencia(Politica politica) {
    switch (politica) {
    case Politica::Lectores:
        return "lectoras_entraron_con_escritoras_pendientes";
    case Politica::Escritores:
        return "lectoras_pidieron_con_escritoras_pendientes";
    case Politica::Fifo:
        return "lectoras_entraron_con_otra_lectora_activa";
    }
    return "";
}

ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza>& traza, Politica politica) {
    ResultadoVerificacion res;
    long lectoras_activas = 0;
    long escritoras_activas = 0;
    long lectoras_pendientes = 0;
    long escritoras_pendientes = 0;
    long entras = 0;
    long pides = 0;

    // 0 = libre, 1 = pendiente, 2 = activa.
    std::unordered_map<int, int> estado_hebra;

    auto violacion = [&](size_t i, const char* regla) {
        if (res.politica_ok) {
            res.politica_ok = false;
            res.indice_violacion = static_cast<long>(i);
            res.regla = regla;
        }
    };

    for (size_t i = 0; i < traza.size(); ++i) {
        const AnotacionTraza& a = traza[i];
        const bool es_lectora = (a.tipo == Tipo::Lectora);
        int& eh = estado_hebra[a.hebra];

        switch (a.accion) {
        case Accion::Pide:
            if (eh != 0)
                violacion(i, "secuencia de hebra: Pide con una solicitud en curso");
            eh = 1;

            if (politica == Politica::Fifo && a.turno != pides)
                violacion(i, "FIFO: los turnos al pedir deben ser consecutivos desde 0");
            ++pides;

            if (es_lectora) {
                if (politica == Politica::Escritores && escritoras_pendientes > 0)
                    ++res.evidencia;

                if (politica == Politica::Lectores && escritoras_activas == 0) {
                    const bool entra_enseguida =
                        i + 1 < traza.size() && traza[i + 1].hebra == a.hebra &&
                        traza[i + 1].tipo == Tipo::Lectora &&
                        traza[i + 1].accion == Accion::Entra;
                    if (!entra_enseguida)
                        violacion(i, "Lectores: lectora que pide sin escritora activa no entra de inmediato");
                }
                ++lectoras_pendientes;
            } else {
                ++escritoras_pendientes;
            }
            break;

        case Accion::Entra:
            if (eh != 1)
                violacion(i, "secuencia de hebra: Entra sin Pide previo");
            eh = 2;

            if (es_lectora) {
                if (escritoras_activas > 0)
                    violacion(i, "Todas: lectora entra con escritora activa");
                if (politica == Politica::Escritores && escritoras_pendientes > 0)
                    violacion(i, "Escritores: lectora entra con escritoras pendientes");
                if (politica == Politica::Lectores && escritoras_pendientes > 0)
                    ++res.evidencia;
                if (politica == Politica::Fifo && lectoras_activas > 0)
                    ++res.evidencia;

                --lectoras_pendientes;
                ++lectoras_activas;
            } else {
                if (escritoras_activas > 0)
                    violacion(i, "Todas: escritora entra con otra escritora activa");
                if (lectoras_activas > 0)
                    violacion(i, "Todas: escritora entra con lectoras activas");
                if (politica == Politica::Lectores && lectoras_pendientes > 0)
                    violacion(i, "Lectores: escritora entra con lectoras pendientes");

                --escritoras_pendientes;
                ++escritoras_activas;
            }

            if (politica == Politica::Fifo && a.turno != entras)
                violacion(i, "FIFO: el k-esimo Entra debe corresponder al turno k");
            ++entras;
            break;

        case Accion::Sale:
            if (eh != 2)
                violacion(i, "secuencia de hebra: Sale sin Entra previo");
            eh = 0;

            if (es_lectora)
                --lectoras_activas;
            else
                --escritoras_activas;
            break;
        }
    }

    if (!res.politica_ok) {
        std::cerr << "verificar_traza: violacion en la anotacion "
                  << res.indice_violacion << ": " << res.regla << "\n";
    }
    return res;
}