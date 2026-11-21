#ifndef MONITOR_RW_HPP
#define MONITOR_RW_HPP

#include <condition_variable>
#include <mutex>
#include <vector>
#include "control_rw.hpp"
#include "verificador.hpp"
#include <deque>
#include <utility>

class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica p, bool registrar_traza = true);

    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

    std::vector<AnotacionTraza> obtener_traza();
private:
    Politica politica;
    bool registrar_traza;
    std::mutex m;
    std::vector<AnotacionTraza> traza;

    void anotar(Tipo t, Accion a, long turno);   // helper de traza (siempre con m tomado)

    long lectoras_activas = 0;
    bool escritora_activa = false;
    long lectoras_pend = 0;
    long escritoras_pend = 0;
    std::condition_variable cv_lectoras;
    std::condition_variable cv_escritoras;

    enum class TipoEspera { Lectora, Escritora };
    long turno_sig = 0;
    std::deque<std::pair<long, TipoEspera>> cola_fifo;

};

#endif // MONITOR_RW_HPP
