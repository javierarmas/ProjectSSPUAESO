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
    connect(ui->InterrupcionButton, &QPushButton::clicked, this, &MainWindow::interrumpirProceso);
    connect(ui->ErrorButton, &QPushButton::clicked, this, &MainWindow::terminarConError);


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
    estados.clear();  // Limpiar el vector de estados antes de guardar nuevos
    ids.clear();

    idsOriginales.clear();
    nombresOriginales.clear();
    operacionesOriginales.clear();
    estadosOriginales.clear();

    // Generar nombres, operaciones y TME para cada proceso
    for (int i = 1; i <= numero; i++) {
        ids.push_back(i);  // Asignar un ID único
        nombres.push_back(generarNombre());
        operaciones.push_back(generarOperacion());
        int TME = rand() % 8 + 5;  // Generar TME aleatorio entre 5 y 12
        TMEs.push_back(TME);
        TMEOriginales.push_back(TME);
        tmeOriginalMap[i] = TME;  // Asignar TME original al mapa con clave ID
        estados.push_back("PENDIENTE");  // Estado inicial de cada proceso

        // Guardar en los vectores originales
        idsOriginales.push_back(i);
        nombresOriginales.push_back(nombres.back());
        operacionesOriginales.push_back(operaciones.back());
        estadosOriginales.push_back("PENDIENTE");
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
        // Mostrar el proceso en ejecución
        QString textoEjecucion = QString("Proceso %1: %2\n%3\nTME: %4")
                                     .arg(ids[procesoActual])
                                     .arg(nombres[procesoActual])
                                     .arg(operaciones[procesoActual])
                                     .arg(TMEs[procesoActual]);

        ui->listWidgetEjecucion->clear();
        ui->listWidgetEjecucion->addItem(textoEjecucion);

        // Mostrar el siguiente proceso en espera
        int procesoEnEsperaIndex = procesoActual + 1;
        if (procesoEnEsperaIndex < numeroProcesos)
        {
            QString textoEspera = QString("Proceso %1: %2\n%3\nTME: %4")
            .arg(ids[procesoEnEsperaIndex])
                .arg(nombres[procesoEnEsperaIndex])
                .arg(operaciones[procesoEnEsperaIndex])
                .arg(TMEs[procesoEnEsperaIndex]);

            ui->listWidgetEspera->clear();
            ui->listWidgetEspera->addItem(textoEspera);
        }
        else
        {
            ui->listWidgetEspera->clear();  // Limpiar si no hay más procesos en espera
        }

        int loteEnEspera = procesoEnEsperaIndex / 5;
        int finLoteEnEspera = std::min((loteEnEspera + 1) * 5, numeroProcesos);

        int procesosPendientes = (procesoEnEsperaIndex >= finLoteEnEspera - 1)
                                     ? 0 : finLoteEnEspera - procesoEnEsperaIndex - 1;

        // Ajuste para evitar negativos
        procesosPendientes = std::max(0, procesosPendientes);

        // Calcular lotes pendientes
        int totalLotes = (numeroProcesos + 4) / 5;  // Redondeo hacia arriba
        int loteActual = procesoActual / 5;
        int lotesPendientes = totalLotes - loteActual - 1;

        // Ajuste de lotes pendientes si el lote actual está completo
        if ((procesoActual + 1) % 5 == 0 && procesoActual + 1 != numeroProcesos)
        {
            lotesPendientes--;
        }

        // Ajuste para evitar negativos
        lotesPendientes = std::max(0, lotesPendientes);

        // Actualizar las etiquetas de procesos y lotes pendientes
        ui->labelProcesosPendientes->setText("Procesos Pendientes: " + QString::number(procesosPendientes));
        ui->labelLotesPendientes->setText("# Lotes Pendientes: " + QString::number(lotesPendientes));
    }
    else
    {
        // Si no hay más procesos en ejecución, detener el reloj
        relojGlobal->stop();
        ui->labelReloj->setText("Reloj Global: " + tiempoTranscurrido.toString("hh:mm:ss"));
        ui->listWidgetEjecucion->clear();
        ui->listWidgetEspera->clear();

        ui->labelProcesosPendientes->setText("Procesos Pendientes: 0");
        ui->labelLotesPendientes->setText("# Lotes Pendientes: 0");
    }

    // Al mover el proceso terminado a la lista de "Terminados"
    if (procesoActual > 0 && procesoActual <= numeroProcesos + 1 && estados[procesoActual - 1] == "PENDIENTE") {
        int idProcesoTerminado = ids[procesoActual - 1];
        int indiceOriginal = idProcesoTerminado - 1;

        if (estadosOriginales[indiceOriginal] == "PENDIENTE") {
            QString operacion = operacionesOriginales[indiceOriginal];
            int resultado = calcularResultado(operacion);

            // Validar si es una división por 0
            if (operacion.contains('/') && operacion.split(" ")[2].toInt() == 0)
            {
                estadosOriginales[indiceOriginal] = "ERROR";
                QString textoTerminado = QString("Proceso %1: %2\n%3 = ERROR (División por cero)\nTME: %4\n\n")
                                             .arg(idProcesoTerminado)
                                             .arg(nombresOriginales[indiceOriginal])
                                             .arg(operacion)
                                             .arg(tmeOriginalMap[idProcesoTerminado]);  // Obtener TME original del mapa

                ui->listWidgetTerminados->addItem(textoTerminado);
                return;
            }

            estadosOriginales[indiceOriginal] = "FINALIZADO";
            QString textoTerminado = QString("Proceso %1: %2\n%3 = %4\nTME: %5\n\n")
                                         .arg(idProcesoTerminado)
                                         .arg(nombresOriginales[indiceOriginal])
                                         .arg(operacion)
                                         .arg(resultado)
                                         .arg(tmeOriginalMap[idProcesoTerminado]);  // Obtener TME original del mapa

            ui->listWidgetTerminados->addItem(textoTerminado);
        }
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

            // Actualizar visualmente el proceso en ejecución
            QString textoEjecucion = QString("Proceso %1: %2\n%3\nTME: %4")
                                         .arg(ids[procesoActual])
                                         .arg(nombres[procesoActual])
                                         .arg(operaciones[procesoActual])
                                         .arg(TMEs[procesoActual]);
            ui->listWidgetEjecucion->clear();
            ui->listWidgetEjecucion->addItem(textoEjecucion);
        } else {
            procesoActual++;  // Avanzar al siguiente proceso cuando el TME llega a 0
            actualizarInterfaz();
        }
    } else {
        // No hay más procesos
        relojGlobal->stop();
        ui->labelReloj->setText("Reloj Global: " + tiempoTranscurrido.toString("hh:mm:ss"));
        ui->listWidgetEjecucion->clear();
        ui->listWidgetEspera->clear();
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

                out << (ids[j]) << ". " << nombres[j] << "\n";
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

void MainWindow::interrumpirProceso() {
    if (procesoActual < numeroProcesos) {
        // Calcular el índice del final del lote actual
        int loteActual = procesoActual / 5;
        int finLoteActual = std::min((loteActual + 1) * 5, (int)ids.size());

        // Obtener el proceso en ejecución
        int idProceso = ids[procesoActual];
        QString nombreProceso = nombres[procesoActual];
        QString operacionProceso = operaciones[procesoActual];
        int TMEProceso = TMEs[procesoActual];
        int TMEOriginalProceso = TMEOriginales[procesoActual];
        QString estadoProceso = estados[procesoActual];

        // Remover el proceso en ejecución de las listas
        ids.erase(ids.begin() + procesoActual);
        nombres.erase(nombres.begin() + procesoActual);
        operaciones.erase(operaciones.begin() + procesoActual);
        TMEs.erase(TMEs.begin() + procesoActual);
        TMEOriginales.erase(TMEOriginales.begin() + procesoActual);
        estados.erase(estados.begin() + procesoActual);

        // Insertar el proceso al final de su lote
        int insertIndex = finLoteActual - 1;
        ids.insert(ids.begin() + insertIndex, idProceso);
        nombres.insert(nombres.begin() + insertIndex, nombreProceso);
        operaciones.insert(operaciones.begin() + insertIndex, operacionProceso);
        TMEs.insert(TMEs.begin() + insertIndex, TMEProceso);
        TMEOriginales.insert(TMEOriginales.begin() + insertIndex, TMEOriginalProceso);
        estados.insert(estados.begin() + insertIndex, estadoProceso);

        actualizarInterfaz();
    }
}

void MainWindow::terminarConError() {
    if (procesoActual < numeroProcesos) {
        // Cambiar el estado del proceso a "ERROR"
        estados[procesoActual] = "ERROR";

        // Obtener el proceso en ejecucion y marcarlo como "ERROR"
        int idProceso = ids[procesoActual];  // Utiliza el ID original
        QString nombreProceso = nombres[procesoActual];
        QString operacionProceso = operaciones[procesoActual];

        QString textoTerminado = QString("Proceso %1: %2\n%3 = ERROR\nTME: %4\n\n")
                                     .arg(idProceso)  // Aquí utilizamos el ID original
                                     .arg(nombreProceso)
                                     .arg(operacionProceso)
                                     .arg(TMEOriginales[procesoActual]);

        ui->listWidgetTerminados->addItem(textoTerminado);

        // Avanzar al siguiente proceso
        procesoActual++;
        actualizarInterfaz();
    }
}
