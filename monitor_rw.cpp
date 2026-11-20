#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), registrar_traza(registrar_traza) {}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);

    long turno = -1;
    if (politica == Politica::Fifo) {
        turno = turno_sig++;
        cola_fifo.push_back({turno, TipoEspera::Lectora});
    }

    lectoras_pend++;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Pide, turno});

    switch (politica) {
    case Politica::Lectores:
        while (escritora_activa)
            cv_lectoras.wait(lock);
        break;
    case Politica::Escritores:
        while (escritora_activa || escritoras_pend > 0)
            cv_lectoras.wait(lock);
        break;
    case Politica::Fifo:
        while (escritora_activa || cola_fifo.empty() ||
               cola_fifo.front().first != turno ||
               cola_fifo.front().second != TipoEspera::Lectora)
            cv_lectoras.wait(lock);
        cola_fifo.pop_front();
        turno_atendido = turno;
        if (!cola_fifo.empty() && cola_fifo.front().second == TipoEspera::Lectora)
            cv_lectoras.notify_all();
        break;
    }

    lectoras_pend--;
    lectoras_activas++;

    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Entra, turno});
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);

    lectoras_activas--;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Sale, -1});

    switch (politica) {
    case Politica::Lectores:
        if (lectoras_activas == 0)
            cv_escritoras.notify_one();
        break;
    case Politica::Escritores:
        if (lectoras_activas == 0 && escritoras_pend > 0)
            cv_escritoras.notify_one();
        break;
    case Politica::Fifo:
        if (lectoras_activas == 0 && !cola_fifo.empty()) {
            if (cola_fifo.front().second == TipoEspera::Escritora)
                cv_escritoras.notify_one();
            else
                cv_lectoras.notify_all();
        }
        break;
    }
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);

    long turno = -1;
    if (politica == Politica::Fifo) {
        turno = turno_sig++;
        cola_fifo.push_back({turno, TipoEspera::Escritora});
    }

    escritoras_pend++;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Pide, turno});

    switch (politica) {
    case Politica::Lectores:
        while (escritora_activa || lectoras_activas > 0 || lectoras_pend > 0)
            cv_escritoras.wait(lock);
        break;
    case Politica::Escritores:
        while (escritora_activa || lectoras_activas > 0)
            cv_escritoras.wait(lock);
        break;
    case Politica::Fifo:
        while (escritora_activa || lectoras_activas > 0 || cola_fifo.empty() ||
               cola_fifo.front().first != turno ||
               cola_fifo.front().second != TipoEspera::Escritora)
            cv_escritoras.wait(lock);
        cola_fifo.pop_front();
        turno_atendido = turno;
        break;
    }

    escritoras_pend--;
    escritora_activa = true;

    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Entra, turno});
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);

    escritora_activa = false;
    if (registrar_traza)
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Sale, -1});

    switch (politica) {
    case Politica::Lectores:
        if (lectoras_pend > 0)
            cv_lectoras.notify_all();
        else
            cv_escritoras.notify_one();
        break;
    case Politica::Escritores:
        if (escritoras_pend > 0)
            cv_escritoras.notify_one();
        else
            cv_lectoras.notify_all();
        break;
    case Politica::Fifo:
        if (!cola_fifo.empty()) {
            if (cola_fifo.front().second == TipoEspera::Lectora)
                cv_lectoras.notify_all();
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
