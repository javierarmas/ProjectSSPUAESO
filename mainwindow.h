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

private:
    Ui::MainWindow *ui;  // Interfaz grafica
    QTimer *relojGlobal;  // Control del reloj global
    QTime tiempoTranscurrido;  // Tiempo acumulado en el reloj

    int procesoActual;  // Indice del proceso que esta en ejecucion
    int procesosPendientes;  // Cantidad de procesos pendientes
    int lotesPendientes;  // Numero de lotes pendientes
    int numeroProcesos;  // Total de procesos ingresados por el usuario

    std::vector<QString> nombres;  // Almacena los nombres de los procesos
    std::vector<QString> operaciones;  // Almacena las operaciones matematicas de los procesos
    std::vector<int> TMEs;  // Tiempos maximos estimados (TME) para cada proceso
    std::vector<int> TMEOriginales;  // Almacena los TME originales para cada proceso

    // Genera nombres aleatorios para los procesos
    QString generarNombre();

    // Genera una operacion matematica aleatoria para los procesos
    QString generarOperacion();

    // Calcula el resultado de una operacion asignada a un proceso
    int calcularResultado(QString operacion);
};

#endif // MAINWINDOW_H
