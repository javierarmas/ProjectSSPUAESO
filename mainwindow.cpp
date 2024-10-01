#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QTextStream>
#include <cstdlib>
#include <ctime>
#include <QMessageBox>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Inicializa la semilla para numeros aleatorios
    srand(time(NULL));

    // Conectar botones a sus funciones correspondientes
    connect(ui->generarButton, &QPushButton::clicked, this, &MainWindow::generarProceso);
    connect(ui->obtenerResultadosButton, &QPushButton::clicked, this, &MainWindow::obtenerResultados);

    // Configurar el reloj global y el tiempo inicial en 00:00:00
    relojGlobal = new QTimer(this);
    connect(relojGlobal, &QTimer::timeout, this, &MainWindow::actualizarReloj);
    tiempoTranscurrido = QTime(0, 0, 0);  // Inicializar a 0
    ui->labelReloj->setText("Reloj Global: 00:00:00");

    // Validar que solo se puedan ingresar numeros mayores que cero
    QIntValidator *validator = new QIntValidator(1, 1000, this);
    ui->lineEditNumeroProcesos->setValidator(validator);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Funcion para generar y procesar los datos de los procesos
void MainWindow::generarProceso()
{
    // Verificar si el campo de entrada esta vacío
    if (ui->lineEditNumeroProcesos->text().isEmpty()) {
        QMessageBox::warning(this, "ERROR", "El proceso no debe de estar vacio.");
        return;  // Salir si no hay entrada
    }

    // Obtener el numero de procesos ingresado
    int numero = ui->lineEditNumeroProcesos->text().toInt();

    // Validar si el numero es mayor que 0
    if (numero <= 0) {
        QMessageBox::warning(this, "ERROR", "Por favor, ingresa un numero entero mayor que cero.");
        ui->lineEditNumeroProcesos->clear();
        return;
    }

    // Limpiar la lista de procesos terminados
    ui->listWidgetTerminados->clear();

    // Reiniciar el reloj global y el tiempo transcurrido
    tiempoTranscurrido = QTime(0, 0, 0);
    ui->labelReloj->setText("Reloj Global: 00:00:00");
    relojGlobal->stop();

    // Inicializar procesos y lotes pendientes
    numeroProcesos = numero;
    procesosPendientes = (numero > 5) ? 3 : numero - 2;
    lotesPendientes = (numero > 5) ? (numero - 1) / 5 : 0; // (7-1) 6 / 5 = 1

    // Inicializar vectores de procesos y sus TME
    nombres.clear();
    operaciones.clear();
    TMEs.clear();
    TMEOriginales.clear();

    // Generar nombres, operaciones y TME para cada proceso
    for (int i = 1; i <= numero; i++) {
        nombres.push_back(generarNombre());
        operaciones.push_back(generarOperacion());
        int TME = rand() % 8 + 5;  // Generar TME aleatorio entre 5 y 12
        TMEs.push_back(TME);
        TMEOriginales.push_back(TME);
    }

    // Crear el archivo de texto "Datos.txt"
    QFile file("Datos.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        int numeroProcesos = nombres.size(); // Numero total de procesos

        // Recorrer y escribir los procesos por lotes de 5
        for (int i = 0; i < numeroProcesos; i += 5)
        {
            int loteActual = (i / 5) + 1;
            out << "______________________________________________________\n";
            out << "Lote " << loteActual << "\n\n";

            for (int j = i; j < i + 5 && j < numeroProcesos; ++j)
            {
                out << (j + 1) << ". " << nombres[j] << "\n";
                out << operaciones[j] << "\n";
                out << "TME: " << TMEOriginales[j] << "\n\n";
            }
        }
        file.close();
    }

    procesoActual = 0;
    relojGlobal->start(1000);  // Iniciar el reloj y actualizar cada segundo
    actualizarInterfaz();  // Actualizar la interfaz con la informacion generada
}

// Funcion para actualizar la interfaz con el proceso en ejecucion, espera y pendientes
void MainWindow::actualizarInterfaz()
{
    if (procesoActual < numeroProcesos)
    {
        // Mostrar el proceso en ejecucion
        QString textoEjecucion = QString("Proceso %1: %2\n%3\nTME: %4")
                                     .arg(procesoActual + 1)
                                     .arg(nombres[procesoActual])
                                     .arg(operaciones[procesoActual])
                                     .arg(TMEs[procesoActual]);
        if (ui->listWidgetEjecucion->count() == 0 || ui->listWidgetEjecucion->item(0)->text() != textoEjecucion)
        {
            ui->listWidgetEjecucion->clear();
            ui->listWidgetEjecucion->addItem(textoEjecucion);
        }

        // Mostrar el siguiente proceso en espera
        int procesoEnEsperaIndex = procesoActual + 1;
        if (procesoEnEsperaIndex < numeroProcesos)
        {
            QString textoEspera = QString("Proceso %1: %2\n%3\nTME: %4")
            .arg(procesoEnEsperaIndex + 1)
                .arg(nombres[procesoEnEsperaIndex])
                .arg(operaciones[procesoEnEsperaIndex])
                .arg(TMEs[procesoEnEsperaIndex]);
            if (ui->listWidgetEspera->count() == 0 || ui->listWidgetEspera->item(0)->text() != textoEspera)
            {
                ui->listWidgetEspera->clear();
                ui->listWidgetEspera->addItem(textoEspera);
            }
        }
        else
        {
            ui->listWidgetEspera->clear();  // Limpiar si no hay mas procesos en espera
        }

        // Calcular procesos pendientes y lotes
        int loteEnEspera = procesoEnEsperaIndex / 5;
        int finLoteEnEspera = std::min((loteEnEspera + 1) * 5, numeroProcesos); // 0 + 1 * 5 = 5

        int procesosPendientes = (procesoEnEsperaIndex >= finLoteEnEspera - 1)
                                     ? 0 : finLoteEnEspera - procesoEnEsperaIndex - 1; // Ejemplo 10 - 6 - 1 = 3

        if ((procesoActual + 1) % 5 == 0 && procesoActual + 1 != numeroProcesos)
        {
            lotesPendientes--; // Decremento de los lotes cuando estan completos
        }

        // Actualizar las etiquetas de procesos y lotes pendientes
        ui->labelProcesosPendientes->setText("Procesos Pendientes: " + QString::number(std::max(0, procesosPendientes)));
        ui->labelLotesPendientes->setText("# Lotes Pendientes: " + QString::number(std::max(0, lotesPendientes)));
    }
    else
    {
        // Si no hay mas procesos en ejecucion, detener el reloj
        relojGlobal->stop();
        ui->labelReloj->setText("Reloj Global: " + tiempoTranscurrido.toString("hh:mm:ss"));
        ui->listWidgetEjecucion->clear();
        ui->listWidgetEspera->clear();
    }

    // Mover el proceso terminado a la lista de "Terminados"
    if (procesoActual > 0 && procesoActual <= numeroProcesos)
    {
        QString operacion = operaciones[procesoActual - 1];
        int resultado = calcularResultado(operacion);

        // Validar si es una division por 0
        if (operacion.contains('/') && operacion.split(" ")[2].toInt() == 0) {
            return;  // Omitir la operacion en caso de division por 0
        }

        QString textoTerminado = QString("Proceso %1: %2\n%3 = %4\nTME: %5\n\n")
                                     .arg(procesoActual)
                                     .arg(nombres[procesoActual - 1])
                                     .arg(operacion)
                                     .arg(resultado)
                                     .arg(TMEOriginales[procesoActual - 1]);
        ui->listWidgetTerminados->addItem(textoTerminado);
    }
}

// Funcion que actualiza el reloj global y controla el avance de los procesos
void MainWindow::actualizarReloj()
{
    tiempoTranscurrido = tiempoTranscurrido.addSecs(1);
    ui->labelReloj->setText("Reloj Global: " + tiempoTranscurrido.toString("hh:mm:ss"));

    if (procesoActual < numeroProcesos) {
        if (TMEs[procesoActual] > 0) {
            TMEs[procesoActual]--;  // Decrementar el TME del proceso actual

            // Actualizar visualmente el proceso en ejecucion
            QString textoEjecucion = QString("Proceso %1: %2\n%3\nTME: %4")
                                         .arg(procesoActual + 1)
                                         .arg(nombres[procesoActual])
                                         .arg(operaciones[procesoActual])
                                         .arg(TMEs[procesoActual]);
            ui->listWidgetEjecucion->clear();
            ui->listWidgetEjecucion->addItem(textoEjecucion);
        } else {
            procesoActual++;  // Avanzar al siguiente proceso cuando el TME llega a 0
            actualizarInterfaz();
        }
    }
}

// Funcion que genera el archivo Resultados.txt con los resultados de las operaciones
void MainWindow::obtenerResultados()
{
    QFile file("Resultados.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        int numeroProcesos = nombres.size();

        for (int i = 0; i < numeroProcesos; i += 5)
        {
            int loteActual = (i / 5) + 1;
            out << "______________________________________________________\n";
            out << "Lote " << loteActual << "\n\n";

            for (int j = i; j < i + 5 && j < numeroProcesos; ++j)
            {
                QString operacion = operaciones[j];
                int resultado = calcularResultado(operacion);

                if (operacion.contains('/') && operacion.split(" ")[2].toInt() == 0) {
                    continue;  // Omitir si es una division por 0
                }

                out << (j + 1) << ". " << nombres[j] << "\n";
                out << operacion << " = " << resultado << "\n\n";
            }
        }
        file.close();
    }
    QMessageBox::information(this, "EXITO", "Archivo creado con exito.");
}

// Genera nombres aleatorios para los procesos
QString MainWindow::generarNombre() {
    QString nombres[] = {"Carlos", "Carolina", "Juan", "Jose"};
    return nombres[rand() % 4];
}

// Genera una operacion matematica aleatoria para los procesos
QString MainWindow::generarOperacion() {
    int num1 = rand() % 11;  // Generar valores entre 0 y 10
    int num2 = rand() % 11;
    char operaciones[] = {'+', '-', '*', '/'};
    char operacion = operaciones[rand() % 4];

    /* Si quiero que desde el principio no se muestren operaciones erroneas
    if (operacion == '/') {
        num2 = (num2 == 0) ? 1 : num2;
    }*/

    return QString("%1 %2 %3").arg(num1).arg(operacion).arg(num2);
}

// Calcula el resultado de la operacion asignada a un proceso
int MainWindow::calcularResultado(QString operacion) {
    QStringList partes = operacion.split(" ");
    if (partes.size() != 3) {
        return 0;  // Retorna 0 si la operacion no es valida
    }

    int num1 = partes[0].toInt();
    char operador = partes[1].toLatin1().at(0);
    int num2 = partes[2].toInt();
    int resultado = 0;

    switch (operador) {
    case '+': resultado = num1 + num2; break;
    case '-': resultado = num1 - num2; break;
    case '*': resultado = num1 * num2; break;
    case '/':
        if (num2 != 0) {
            resultado = num1 / num2;
        } else {
            return 0;  // Si es division por 0, retornar 0
        }
        break;
    default:
        return 0;  // Retornar 0 si el operador no es valido
    }

    return resultado;
}
