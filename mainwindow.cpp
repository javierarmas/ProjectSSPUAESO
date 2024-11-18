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
#include <QShortcut>

// Constructor de la clase MainWindow
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
    connect(ui->InterrupcionButton, &QPushButton::clicked, this, &MainWindow::interrumpirProceso);  // Nuevo boton de interrupcion
    connect(ui->ErrorButton, &QPushButton::clicked, this, &MainWindow::terminarConError);  // Nuevo boton para terminar con error
    // Para si queremos hacerlo con teclas la interrupcion y error
    QShortcut *shortcutInterrumpir = new QShortcut(QKeySequence("I"), this);
    connect(shortcutInterrumpir, &QShortcut::activated, this, &MainWindow::interrumpirProceso);
    QShortcut *shortcutError = new QShortcut(QKeySequence("E"), this);
    connect(shortcutError, &QShortcut::activated, this, &MainWindow::terminarConError);


    // Configurar el reloj global y el tiempo inicial en 00:00:00
    relojGlobal = new QTimer(this);
    connect(relojGlobal, &QTimer::timeout, this, &MainWindow::actualizarReloj);
    tiempoTranscurrido = QTime(0, 0, 0);  // Inicializar a 0
    ui->labelReloj->setText("Reloj Global: 00:00:00");

    // Validar que solo se puedan ingresar numeros mayores que cero
    QIntValidator *validator = new QIntValidator(1, 1000, this);
    ui->lineEditNumeroProcesos->setValidator(validator);
}

// Destructor de la clase MainWindow
MainWindow::~MainWindow()
{
    delete ui;
}

// Funcion para generar y procesar los datos de los procesos
void MainWindow::generarProceso()
{
    // Verificar si el campo de entrada esta vacio
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
    procesosPendientes = (numero > 5) ? 3 : numero - 2; // 5 > = 3 || 4 - 2 = 2
    lotesPendientes = (numero > 5) ? (numero - 1) / 5 : 0; // (7-1) 6 / 5 = 1 || 0

    // Inicializar vectores de procesos y sus TME
    nombres.clear();
    operaciones.clear();
    TMEs.clear();
    TMEOriginales.clear();
    TEs.clear();
    interrumpidos.clear();
    estados.clear();
    ids.clear();
    idsOriginales.clear();
    nombresOriginales.clear();
    operacionesOriginales.clear();
    estadosOriginales.clear();

    // Generar nombres, operaciones y TME para cada proceso
    for (int i = 1; i <= numero; i++) {
        ids.push_back(i);  // Asignar un ID unico (p2)
        nombres.push_back(generarNombre());
        operaciones.push_back(generarOperacion());
        int TME = rand() % 8 + 5;  // Generar TME aleatorio entre 5 y 12
        TMEs.push_back(TME);
        TMEOriginales.push_back(TME);
        TEs.push_back(0);  // Inicializar TE a 0 para cada proceso
        interrumpidos.push_back(false);  // Inicializar como no interrumpido
        tmeOriginalMap[i] = TME;  // Asignar TME original al mapa con clave ID (p2)
        estados.push_back("PENDIENTE");  // Estado inicial de cada proceso

        // Guardar en los vectores originales (p2)
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
        QString textoEjecucion = QString("Proceso %1: %2\n%3\nTME: %4\nTE: %5")
                                     .arg(ids[procesoActual])
                                     .arg(nombres[procesoActual])
                                     .arg(operaciones[procesoActual])
                                     .arg(TMEs[procesoActual])
                                     .arg(TEs[procesoActual]);

        ui->listWidgetEjecucion->clear();
        ui->listWidgetEjecucion->addItem(textoEjecucion);

        // Mostrar el siguiente proceso en espera
        int procesoEnEsperaIndex = procesoActual + 1;
        if (procesoEnEsperaIndex < numeroProcesos)
        {
            QString textoEspera;

            if (interrumpidos[procesoEnEsperaIndex]) {
                // Si el proceso ha sido interrumpido, mostrar TR
                textoEspera = QString("Proceso %1: %2\n%3\nTME: %4\nTR: %5")
                                  .arg(ids[procesoEnEsperaIndex])
                                  .arg(nombres[procesoEnEsperaIndex])
                                  .arg(operaciones[procesoEnEsperaIndex])
                                  .arg(TMEOriginales[procesoEnEsperaIndex])
                                  .arg(TMEs[procesoEnEsperaIndex]);  // Mostrar el TME restante como TR
            } else {
                // Si no ha sido interrumpido, mostrar sin TR
                textoEspera = QString("Proceso %1: %2\n%3\nTME: %4")
                                  .arg(ids[procesoEnEsperaIndex])
                                  .arg(nombres[procesoEnEsperaIndex])
                                  .arg(operaciones[procesoEnEsperaIndex])
                                  .arg(TMEs[procesoEnEsperaIndex]);
            }

            ui->listWidgetEspera->clear();
            ui->listWidgetEspera->addItem(textoEspera);
        }
        else
        {
            ui->listWidgetEspera->clear();  // Limpiar si no hay más procesos en espera
        }

        // Calcular procesos pendientes y lotes pendientes
        int loteEnEspera = procesoEnEsperaIndex / 5; // 0 a 4 / 5 = 0
        int finLoteEnEspera = std::min((loteEnEspera + 1) * 5, numeroProcesos); // 0 + 1 * 5 = 5

        int procesosPendientes = (procesoEnEsperaIndex >= finLoteEnEspera - 1)
                                     ? 0 : finLoteEnEspera - procesoEnEsperaIndex - 1; // Ejemplo 10 - 6 - 1 = 3

        // Ajuste para evitar negativos
        procesosPendientes = std::max(0, procesosPendientes);

        // Calcular lotes pendientes para interrumpidos
        int totalLotes = (numeroProcesos + 4) / 5; // (13 + 4) / 5 = 17 / 5 = 3
        int loteActual = procesoActual / 5; // 7 / 5 = 1
        int lotesPendientes = totalLotes - loteActual - 1; // 3 - 1 - 1 = 1


        // Ajuste de lotes pendientes si el lote actual esta completo
        if ((procesoActual + 1) % 5 == 0 && procesoActual + 1 != numeroProcesos)
        {
            lotesPendientes--; // Decremento de los lotes cuando estan completos
        }

        // Ajuste para evitar negativos
        lotesPendientes = std::max(0, lotesPendientes);

        // Actualizar las etiquetas de procesos y lotes pendientes
        ui->labelProcesosPendientes->setText("Procesos Pendientes: " + QString::number(procesosPendientes));
        ui->labelLotesPendientes->setText("# Lotes Pendientes: " + QString::number(lotesPendientes));
    }
    else
    {
        // Si no hay mas procesos en ejecucion, detener el reloj
        relojGlobal->stop();
        ui->labelReloj->setText("Reloj Global: " + tiempoTranscurrido.toString("hh:mm:ss"));
        ui->listWidgetEjecucion->clear();
        ui->listWidgetEspera->clear();
    }

    // Al mover el proceso terminado a la lista de "Terminados"
    if (procesoActual > 0 && procesoActual <= numeroProcesos + 1 && estados[procesoActual - 1] == "PENDIENTE") {
        int idProcesoTerminado = ids[procesoActual - 1];
        int indiceOriginal = idProcesoTerminado - 1;

        if (estadosOriginales[indiceOriginal] == "PENDIENTE") {
            QString operacion = operacionesOriginales[indiceOriginal];
            int resultado = calcularResultado(operacion);

            // Validar si es una division por cero
            if (operacion.contains('/') && operacion.split(" ")[2].toInt() == 0)
            {
                /* Desactivar comentario si queremos que se muestre el proceso si fue division por cero
                estadosOriginales[indiceOriginal] = "ERROR";
                QString textoTerminado = QString("Proceso %1: %2\n%3 = ERROR (Division por cero)\nTME: %4\n\n")
                                             .arg(idProcesoTerminado)
                                             .arg(nombresOriginales[indiceOriginal])
                                             .arg(operacion)
                                             .arg(tmeOriginalMap[idProcesoTerminado]);  // Obtener TME original del mapa

                ui->listWidgetTerminados->addItem(textoTerminado);*/
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
            TEs[procesoActual]++;   // Incrementar el TE del proceso actual

            // Actualizar visualmente el proceso en ejecucion
            QString textoEjecucion = QString("Proceso %1: %2\n%3\nTME: %4\nTE: %5")
                                         .arg(ids[procesoActual])
                                         .arg(nombres[procesoActual])
                                         .arg(operaciones[procesoActual])
                                         .arg(TMEs[procesoActual])
                                         .arg(TEs[procesoActual]);  // Mostrar el TE
            ui->listWidgetEjecucion->clear();
            ui->listWidgetEjecucion->addItem(textoEjecucion);
        } else {
            procesoActual++;  // Avanzar al siguiente proceso cuando el TME llega a 0
            actualizarInterfaz();
        }
    } else {
        // No hay mas procesos
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
                QString estado = estados[j];
                if (estado == "ERROR") {
                    out << (ids[j]) << ". " << nombres[j] << "\n";
                    out << operacion << " = ERROR\n\n";
                } else {
                    int resultado = calcularResultado(operacion);

                    // Validar si es una division por cero para que no la imprima
                    if (operacion.contains('/') && operacion.split(" ")[2].toInt() == 0) {
                        continue;
                    }
                    out << (ids[j]) << ". " << nombres[j] << "\n";
                    out << operacion << " = " << resultado << "\n\n";
                }
            }
        }
        file.close();
    }
    QMessageBox::information(this, "ÉXITO", "Archivo creado con éxito.");
}


// Genera nombres aleatorios para los procesos
QString MainWindow::generarNombre() {
    QString nombres[] = {"Carlos", "Carolina", "Juan", "Jose"};
    return nombres[rand() % 4];
}

// Genera una operacion matematica aleatoria para los procesos
QString MainWindow::generarOperacion() {
    int num1 = rand() % 11 + 1;  // Generar valores entre 0 y 10
    int num2 = rand() % 11 + 1;
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

// Funcion para interrumpir el proceso actual y moverlo al final de su lote
void MainWindow::interrumpirProceso() {
    if (procesoActual < numeroProcesos) {
        // Calcular el indice del final del lote actual
        int loteActual = procesoActual / 5; // 3 / 5 = 0
        int finLoteActual = std::min((loteActual + 1) * 5, (int)ids.size()); // (0 + 1) * 5 = 5  finLoteActual = std::min(5, 13) = 5

        // Marcar el proceso como interrumpido
        interrumpidos[procesoActual] = true;

        // Obtener los procesos en ejecucion
        int idProceso = ids[procesoActual];
        QString nombreProceso = nombres[procesoActual];
        QString operacionProceso = operaciones[procesoActual];
        int TMEProceso = TMEs[procesoActual];
        int TMEOriginalProceso = TMEOriginales[procesoActual];
        QString estadoProceso = estados[procesoActual];
        int TEProceso = TEs[procesoActual];  // Obtener el TE del proceso

        // Eliminar los procesos de los actuales vectores
        ids.erase(ids.begin() + procesoActual);
        nombres.erase(nombres.begin() + procesoActual);
        operaciones.erase(operaciones.begin() + procesoActual);
        TMEs.erase(TMEs.begin() + procesoActual);
        TMEOriginales.erase(TMEOriginales.begin() + procesoActual);
        estados.erase(estados.begin() + procesoActual);
        TEs.erase(TEs.begin() + procesoActual);  // Remover el TE
        interrumpidos.erase(interrumpidos.begin() + procesoActual);

        // Insertar los procesos en el indice calculado
        int insertIndex = finLoteActual - 1;
        // Calcular el indice del proceso al final de su lote actual
        ids.insert(ids.begin() + insertIndex, idProceso);
        nombres.insert(nombres.begin() + insertIndex, nombreProceso);
        operaciones.insert(operaciones.begin() + insertIndex, operacionProceso);
        TMEs.insert(TMEs.begin() + insertIndex, TMEProceso);
        TMEOriginales.insert(TMEOriginales.begin() + insertIndex, TMEOriginalProceso);
        estados.insert(estados.begin() + insertIndex, estadoProceso);
        TEs.insert(TEs.begin() + insertIndex, TEProceso);  // Insertar el TE
        interrumpidos.insert(interrumpidos.begin() + insertIndex, true);  // Mantenerlo como interrumpido

        // Actualizar la interfaz de usuario para reflejar los cambios en los procesos
        actualizarInterfaz();
    }
}

// Funcion para terminar el proceso actual con error
void MainWindow::terminarConError() {
    if (procesoActual < numeroProcesos) {
        // Cambiar el estado del proceso a "ERROR"
        estados[procesoActual] = "ERROR";

        // Obtener el proceso en ejecucion y marcarlo como "ERROR"
        int idProceso = ids[procesoActual];  // Utiliza el ID original
        QString nombreProceso = nombres[procesoActual];
        QString operacionProceso = operaciones[procesoActual];

        QString textoTerminado = QString("Proceso %1: %2\n%3 = ERROR\nTME: %4\n\n")
                                     .arg(idProceso)
                                     .arg(nombreProceso)
                                     .arg(operacionProceso)
                                     .arg(TMEOriginales[procesoActual]);

        ui->listWidgetTerminados->addItem(textoTerminado);

        // Avanzar al siguiente proceso
        procesoActual++;
        actualizarInterfaz();
    }
}
