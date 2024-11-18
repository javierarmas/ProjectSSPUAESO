#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);  // Constructor
    ~MainWindow();  // Destructor

private slots:
    // Inicia el procesamiento de los datos cuando se presiona el boton "Generar"
    void generarProceso();

    // Actualiza la interfaz grafica mostrando los procesos en ejecucion y en espera
    void actualizarInterfaz();

    // Actualiza el reloj global para mostrar el tiempo transcurrido
    void actualizarReloj();

    // Genera el archivo de resultados al presionar el boton "Obtener Resultados"
    void obtenerResultados();

    // Interrumpe los procesos que estan en ejecucion
    void interrumpirProceso();

    // Termina los procesos que estan en ejecucion y los manda a "Terminados"
    void terminarConError();

private:
    Ui::MainWindow *ui;  // Interfaz grafica
    QTimer *relojGlobal;  // Control del reloj global
    QTime tiempoTranscurrido;  // Tiempo acumulado en el reloj

    int procesoActual;  // Indice del proceso que esta en ejecucion
    int procesosPendientes;  // Cantidad de procesos pendientes
    int lotesPendientes;  // Numero de lotes pendientes
    int numeroProcesos;  // Total de procesos ingresados por el usuario
    int calcularResultado(QString operacion); // Calcula el resultado de una operacion asignada a un proceso

    std::vector<QString> nombres;  // Almacena los nombres de los procesos
    std::vector<QString> operaciones;  // Almacena las operaciones matematicas de los procesos
    std::vector<int> TMEs;  // Tiempos maximos estimados (TME) para cada proceso
    std::vector<int> TEs;  // Vector para almacenar el Tiempo Ejecutado de cada proceso
    std::vector<int> TMEOriginales;  // Almacena los TME originales para cada proceso
    std::vector<QString> estados;  // Vector para almacenar el estado de cada proceso ("PENDIENTE", "FINALIZADO", "ERROR")
    std::vector<int> ids; // Mantener los numeros originales de cada proceso

    std::vector<int> idsOriginales; // Almacena los ids originales para cada proceso
    std::vector<QString> nombresOriginales; // Almacena los nombres originales para cada proceso
    std::vector<QString> operacionesOriginales; // Almacena las operaciones originales para cada proceso
    std::vector<QString> estadosOriginales; // Almacena los estados originales para cada proceso
    std::vector<bool> interrumpidos;  // Vector para indicar si un proceso ha sido interrumpido

    QString generarNombre(); // Genera nombres aleatorios para los procesos
    QString generarOperacion(); // Genera una operacion matematica aleatoria para los procesos

    QMap<int, int> tmeOriginalMap; // Mapa de ID de proceso a TME original
};

#endif // MAINWINDOW_H
