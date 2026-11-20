#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), registrar_traza(registrar_traza) {}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);

    long turno = -1;
    if (politica == Politica::Fifo) {
        turno = siguiente_turno++;
        cola_fifo.push_back({turno, TipoEspera::Lectora});
    }

    lectores_pendientes++;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Pide, turno});

    switch (politica) {
    case Politica::Lectores:
        while (escritoras_activas > 0)
            cv_lectores.wait(lock);
        break;
    case Politica::Escritores:
        while (escritoras_activas > 0 || escritoras_pendientes > 0)
            cv_lectores.wait(lock);
        break;
    case Politica::Fifo:
        while (escritoras_activas > 0 || cola_fifo.empty() ||
               cola_fifo.front().first != turno ||
               cola_fifo.front().second != TipoEspera::Lectora)
            cv_lectores.wait(lock);
        cola_fifo.pop_front();
        if (!cola_fifo.empty() && cola_fifo.front().second == TipoEspera::Lectora)
            cv_lectores.notify_all();
        break;
    }

    lectores_pendientes--;
    lectores_activos++;

    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Entra, turno});
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);

    lectores_activos--;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Sale, -1});

    if (politica == Politica::Lectores) {
        if (lectores_activos == 0)
            cv_escritoras.notify_one();
    } else if (politica == Politica::Escritores) {
        if (lectores_activos == 0 && escritoras_pendientes > 0)
            cv_escritoras.notify_one();
    } else if (politica == Politica::Fifo) {
        if (lectores_activos == 0 && !cola_fifo.empty()) {
            if (cola_fifo.front().second == TipoEspera::Escritora)
                cv_escritoras.notify_one();
            else
                cv_lectores.notify_all();
        }
    }
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);

    long turno = -1;
    if (politica == Politica::Fifo) {
        turno = siguiente_turno++;
        cola_fifo.push_back({turno, TipoEspera::Escritora});
    }

    escritoras_pendientes++;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Pide, turno});

    switch (politica) {
    case Politica::Lectores:
        while (escritoras_activas > 0 || lectores_activos > 0 || lectores_pendientes > 0)
            cv_escritoras.wait(lock);
        break;
    case Politica::Escritores:
        while (escritoras_activas > 0 || lectores_activos > 0)
            cv_escritoras.wait(lock);
        break;
    case Politica::Fifo:
        while (escritoras_activas > 0 || lectores_activos > 0 ||
               cola_fifo.empty() || cola_fifo.front().first != turno ||
               cola_fifo.front().second != TipoEspera::Escritora)
            cv_escritoras.wait(lock);
        cola_fifo.pop_front();
        break;
    }

    escritoras_pendientes--;
    escritoras_activas = 1;

    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Entra, turno});
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);

    escritoras_activas = 0;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Sale, -1});

    switch (politica) {
    case Politica::Lectores:
        if (lectores_pendientes > 0)
            cv_lectores.notify_all();
        else
            cv_escritoras.notify_one();
        break;
    case Politica::Escritores:
        if (escritoras_pendientes > 0)
            cv_escritoras.notify_one();
        else
            cv_lectores.notify_all();
        break;
    case Politica::Fifo:
        if (!cola_fifo.empty()) {
            if (cola_fifo.front().second == TipoEspera::Lectora)
                cv_lectores.notify_all();
            else
                cv_escritoras.notify_one();
        }
        break;
    }
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
