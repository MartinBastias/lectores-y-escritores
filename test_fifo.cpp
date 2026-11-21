#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "monitor_rw.hpp"
#include "verificador.hpp"

// test para probar el monitor con politica FIFO
// para correr el test pone: g++ -std=c++17 -O2 -Wall -Wextra -pthread test_fifo.cpp monitor_rw.cpp verificador.cpp -o test_fifo
// luego ejecuta con ./test_fifo (nombre), en 'nombre' puedes colocar la politica que quieras usar, como lectores, escritores o fifo

thread_local int id_hebra = -1;   // provisorio: en el programa final lo define rw.cpp

static void pausa_us(long us) {
    if (us > 0) std::this_thread::sleep_for(std::chrono::microseconds(us));
}

int main(int argc, char* argv[]) {
    Politica pol = Politica::Fifo;
    std::string nombre = "fifo";
    if (argc > 1) {
        nombre = argv[1];
        if      (nombre == "lectores")   pol = Politica::Lectores;
        else if (nombre == "escritores") pol = Politica::Escritores;
        else if (nombre == "fifo")       pol = Politica::Fifo;
        else { std::cerr << "Uso: ./test_fifo [lectores|escritores|fifo]\n"; return 1; }
    }

    const int  L = 6, E = 2, K = 20;           // configuración tipo E1: varias lectoras
    const long dur_lec = 2000, dur_esc = 1000; // µs; con pausa para que se solapen las lecturas

    Registro reg;
    MonitorRW mon(pol, true);
    std::atomic<long> lect_act{0}, esc_act{0}, max_lect{0}, incons{0}, viol{0};

    auto lectora = [&](int id) {
        id_hebra = id;
        for (int k = 0; k < K; ++k) {
            mon.entrar_lectura();
            long n = ++lect_act;
            long prev = max_lect.load();
            while (n > prev && !max_lect.compare_exchange_weak(prev, n)) {}
            if (esc_act.load() != 0) ++viol;               // lectora no debe coincidir con escritora
            long v = reg.version, d = reg.doble_version;
            if (d != 2 * v) ++incons;
            pausa_us(dur_lec);
            --lect_act;
            mon.salir_lectura();
        }
    };

    auto escritora = [&](int id) {
        id_hebra = id;
        for (int k = 0; k < K; ++k) {
            mon.entrar_escritura();
            if (++esc_act != 1 || lect_act.load() != 0) ++viol;  // exclusión total
            reg.version++;
            reg.doble_version = 2 * reg.version;
            pausa_us(dur_esc);
            --esc_act;
            mon.salir_escritura();
        }
    };

    std::vector<std::thread> hs;
    for (int i = 0; i < L; ++i) hs.emplace_back(lectora, i);          // ids 0..L-1
    for (int j = 0; j < E; ++j) hs.emplace_back(escritora, L + j);    // ids L..L+E-1
    for (auto& h : hs) h.join();

    auto traza = mon.obtener_traza();
    ResultadoVerificacion r = verificar_traza(traza, pol);

    std::cout << "version=" << reg.version << " (esperado " << E * K << ")\n"
              << "doble_version=" << reg.doble_version << " (esperado " << 2 * E * K << ")\n"
              << "inconsistencias=" << incons << "  violaciones_exclusion=" << viol << "\n"
              << "max_lectoras_simultaneas=" << max_lect << "\n"
              << "politica_ok=" << (r.politica_ok ? "SI" : "NO") << "\n"
              << nombre_evidencia(pol) << "=" << r.evidencia << "\n\n";

    // Fragmento de traza para el anexo (primeras 40 anotaciones)
    const char* acc[] = {"Pide ", "Entra", "Sale "};
    for (size_t i = 0; i < traza.size() && i < 50; ++i) {   // aca puedes cambiar el numero 40 a otro valor para ver la cantidad de turnos mostrados
        const auto& a = traza[i];
        std::cout << i << ": hebra " << a.hebra
                  << (a.tipo == Tipo::Lectora ? " L " : " E ")
                  << acc[static_cast<int>(a.accion)]
                  << " turno " << a.turno << "\n";
    }
}