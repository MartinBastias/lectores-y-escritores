#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), registrar_traza(registrar_traza) {}

// R6: siempre se llama con el mutex m tomado, por eso el orden del vector es el orden real.
// anotar toma si es escritora o lectora con tipo, la accion de pedir, entrar o salir con Accion, por ultimo el numero del turno
void MonitorRW::anotar(Tipo t, Accion a, long turno) {
    if (registrar_traza)
        traza.push_back({id_hebra, t, a, turno});
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);

    long turno = -1;
    if (politica == Politica::Fifo) {
        turno = turno_sig++;                                  // turno al registrarse
        cola_fifo.push_back({turno, TipoEspera::Lectora});
    }
    lectoras_pend++;
    anotar(Tipo::Lectora, Accion::Pide, turno);               // antes de esperar

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
        if (!cola_fifo.empty() && cola_fifo.front().second == TipoEspera::Lectora)
            cv_lectoras.notify_all();                         // la siguiente lectora entra ya
        break;
    }

    lectoras_pend--;
    lectoras_activas++;
    anotar(Tipo::Lectora, Accion::Entra, turno);
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);

    lectoras_activas--;
    anotar(Tipo::Lectora, Accion::Sale, -1);

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
                cv_escritoras.notify_all();   // notify_all: las escritoras tienen condición distinta (turno)
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
    anotar(Tipo::Escritora, Accion::Pide, turno);

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
        break;
    }

    escritoras_pend--;
    escritora_activa = true;
    anotar(Tipo::Escritora, Accion::Entra, turno);
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);

    escritora_activa = false;
    anotar(Tipo::Escritora, Accion::Sale, -1);

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
                cv_escritoras.notify_all();
        }
        break;
    }
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}