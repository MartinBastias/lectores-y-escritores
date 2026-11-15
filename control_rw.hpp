#ifndef CONTROL_RW_HPP
#define CONTROL_RW_HPP

/**
 * Políticas de prioridad soportadas por el problema de Lectores y Escritores.
 */
enum class Politica {
    Lectores,
    Escritores,
    Fifo
};

/**
 * R2. Registro compartido entre hebras lectoras y escritoras.
 * - Escritora: version++ y luego doble_version = 2 * version
 * - Lectora: comprueba que doble_version == 2 * version
 */
struct Registro {
    long version = 0;
    long doble_version = 0;
};

/**
 * R6. Identificador lógico de cada hebra (asignado al iniciar cada std::thread).
 * Definido en rw.cpp.
 */
extern thread_local int id_hebra;

/**
 * Interfaz común abstracta para los controladores de sincronización
 * (tanto MonitorRW en Parte I como ControlAtomicoRW en Parte II).
 */
class ControlRW {
public:
    virtual ~ControlRW() = default;
    virtual void entrar_lectura() = 0;
    virtual void salir_lectura() = 0;
    virtual void entrar_escritura() = 0;
    virtual void salir_escritura() = 0;
};

#endif // CONTROL_RW_HPP
