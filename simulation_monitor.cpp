/*==========================================================================
  simulation_monitor.cpp  -  Simulador de Acciones Reales de E/S
  Muestra Registros, Interrupciones y DMA en tiempo real.
  Sistemas Operativos - 2026
==========================================================================*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <conio.h>
#include <thread>
#include <chrono>
#include <iomanip>
#include <bitset>
#include "device_monitor.h"

// Helper
static void clearScreen() {
    system("cls");
}

// Helper function to map English layout keys and extended keys
static std::string obtenerNombreTecla(int tecla) {
    switch (tecla) {
        case 8: return "Backspace";
        case 9: return "Tab";
        case 13: return "Enter";
        case 27: return "Esc";
        case 32: return "Space";
        case 0xE048: return "Up Arrow";
        case 0xE050: return "Down Arrow";
        case 0xE04B: return "Left Arrow";
        case 0xE04D: return "Right Arrow";
        case 0xE047: return "Home";
        case 0xE04F: return "End";
        case 0xE049: return "Page Up";
        case 0xE051: return "Page Down";
        case 0xE052: return "Insert";
        case 0xE053: return "Delete";
        case 0x013B: return "F1";
        case 0x013C: return "F2";
        case 0x013D: return "F3";
        case 0x013E: return "F4";
        case 0x013F: return "F5";
        case 0x0140: return "F6";
        case 0x0141: return "F7";
        case 0x0142: return "F8";
        case 0x0143: return "F9";
        case 0x0144: return "F10";
        case 0xE085: case 0x0185: return "F11";
        case 0xE086: case 0x0186: return "F12";
        case '`': return "Backtick (`)";
        case '~': return "Tilde (~)";
        case '-': return "Minus (-)";
        case '=': return "Equals (=)";
        case '[': return "Left Bracket ([)";
        case ']': return "Right Bracket (])";
        case '\\': return "Backslash (\\)";
        case ';': return "Semicolon (;)";
        case '\'': return "Apostrophe (')";
        case ',': return "Comma (,)";
        case '.': return "Period (.)";
        case '/': return "Slash (/)";
        default:
            if (tecla > 255) {
                char hexStr[20];
                snprintf(hexStr, sizeof(hexStr), "Extended (0x%X)", tecla);
                return std::string(hexStr);
            }
            if (tecla >= 33 && tecla <= 126) return std::string(1, (char)tecla);
            return "Control/Other (" + std::to_string(tecla) + ")";
    }
}

static void simularTeclado() {
    clearScreen();
    mostrarEncabezado();
    std::cout << CYAN << "  [ SIMULACION: TECLADO (Interrupcion IRQ 1 y Registro 8042) ]\n\n" << RESET;
    std::cout << GREEN << BOLD << "  [ HARDWARE DETECTADO ]\n" << RESET;
    std::cout << "    - Dispositivo            : Mejorado (101 o 102 teclas)\n";
    std::cout << "    - Tipo de Bus            : PS/2 / HID Hub compatible\n";
    std::cout << "    - Layout del Sistema     : Español (Latam)\n\n";

    std::cout << "  " << YELLOW << "¿Como funciona?" << RESET << " Al presionar una tecla, el controlador de teclado \n";
    std::cout << "  envia una señal IRQ 1 al procesador. El SO lee el Scan Code desde el puerto 0x60.\n";
    std::cout << "  " << YELLOW << "Instrucciones:" << RESET << " Presione teclas para ver el hardware. 'ESC' para salir.\n\n";

    while (true) {
        if (_kbhit()) {
            int tecla = _getch();
            if (tecla == 0 || tecla == 224) {
                int tecla_extendida = _getch();
                int prefix = (tecla == 0) ? 0x01 : tecla;
                tecla = (prefix << 8) | tecla_extendida;
            }
            if (tecla == 27) break;

            HWND hwnd = GetForegroundWindow();
            char windowTitle[256] = "Desconocida / Escritorio";
            if (hwnd) GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

            std::cout << "\n  " << GREEN << "[ CONTEXTO ] " << RESET << "Enviando entrada a: " << windowTitle << "\n";
            std::cout << "  " << RED << "[ Interrupcion ] " << RESET << "IRQ 1 (Teclado) -> ¡Atencion CPU!\n";
            std::cout << "  " << YELLOW << "[ Hardware ] " << RESET << "Tecla: " << obtenerNombreTecla(tecla) << "\n\n";
            
            std::cout << "  " << MAGENTA << "[ Registros del Controlador 8042 (Teclado) ]\n" << RESET;
            std::cout << "    - Registro de DATOS (0x60):  0x" << std::hex << (tecla & 0xFF) << std::dec << " (Scan Code: " << obtenerNombreTecla(tecla) << ")\n";
            std::cout << "    - Registro de ESTADO (0x64): 0x1D (Flags: OBF=1, IBF=0, System=1, Lock=1)\n";
            std::cout << "    - Registro de CONTROL (0x64): 0xAE (Comando: Habilitar Teclado)\n\n";

            std::cout << "  " << BLUE << "[ Proceso de Memoria y DMA ]\n" << RESET;
            std::cout << "    - El SO realiza un 'IN AL, 60h' para mover el dato del controlador a la RAM.\n";
            std::cout << "    - " << RED << "DMA NO UTILIZADO: " << RESET << "Debido a la baja tasa de transferencia del teclado\n";
            std::cout << "      (~10 bytes/s), la sobrecarga de configurar el controlador DMA seria mayor\n";
            std::cout << "      que el beneficio. Se usa PIO (Entrada/Salida Programada).\n";
            std::cout << "  ------------------------------------------------------------------\n";
        }
        Sleep(50);
    }
}static void simularMouse() {
    clearScreen();
    mostrarEncabezado();
    std::cout << CYAN << "  [ SIMULACION: MOUSE (IRQ 12 y Paquetes de 3 Bytes) ]\n\n" << RESET;
    std::cout << GREEN << BOLD << "  [ HARDWARE DETECTADO ]\n" << RESET;
    std::cout << "    - Dispositivo            : ELAN I2C Filter Driver / Mouse USB\n";
    std::cout << "    - Tasa de Muestreo       : ~125 Reportes/seg\n";
    std::cout << "    - Interfaz               : I2C Bus / HID compatible\n\n";

    std::cout << "  " << YELLOW << "¿Como funciona?" << RESET << " El mouse envia paquetes de 3 bytes (Estado, X, Y).\n";
    std::cout << "  A diferencia del teclado, el mouse usa una IRQ específica (IRQ 12).\n";
    std::cout << "  " << YELLOW << "Instrucciones:" << RESET << " Mueva el mouse para ver el flujo. Tecla para salir.\n\n";

    POINT lastPos;
    GetCursorPos(&lastPos);

    while (!_kbhit()) {
        POINT pt;
        GetCursorPos(&pt);
        bool leftBtn = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        bool rightBtn = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

        if (pt.x != lastPos.x || pt.y != lastPos.y || leftBtn || rightBtn) {
            lastPos = pt;
            std::cout << "\n  " << RED << "[ Interrupcion ] " << RESET << "IRQ 12 (Mouse) Detectada.\n";
            std::cout << "  " << YELLOW << "[ Hardware ] " << RESET << "Pos: (" << pt.x << "," << pt.y << ") | Clics: L:" << (leftBtn?"SI":"NO") << " R:" << (rightBtn?"SI":"NO") << "\n\n";
            
            std::cout << "  " << MAGENTA << "[ Registros del Mouse (PS/2) ]\n" << RESET;
            int byte1 = 0x08 | (leftBtn ? 0x01 : 0) | (rightBtn ? 0x02 : 0);
            std::cout << "    - Registro de DATOS (0x60): 0x" << std::hex << byte1 << ", 0x" << (pt.x % 256) << ", 0x" << (pt.y % 256) << std::dec << " (Paquete 3-Bytes)\n";
            std::cout << "    - Registro de ESTADO (0x64): 0x3D (Flags: OBF=1, MouseEvent=1, AuxEn=1)\n";
            std::cout << "    - Registro de CONTROL (0x64): 0xA8 (Comando: Habilitar Mouse Auxiliar)\n\n";

            std::cout << "  " << BLUE << "[ Accion DMA (Bus Mastering) ]\n" << RESET;
            std::cout << "    - El Hub USB usa bus-mastering para mover estos paquetes a la RAM sin usar la CPU.\n";
            std::cout << "    - Esto libera al procesador de procesar cada pequeño movimiento del cursor.\n";
            std::cout << "  ------------------------------------------------------------------\n";
            
            Sleep(800); 
        }
        Sleep(50);
    }
    while(_kbhit()) _getch();
}

// Callback de eventos de Windows para detectar apertura/cierre y cambio de foco
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
                           LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF) return;
    if (event == EVENT_OBJECT_CREATE && !IsWindowVisible(hwnd)) return;

    char windowTitle[256] = "";
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
    if (strlen(windowTitle) == 0 && event != EVENT_SYSTEM_FOREGROUND) return;

    auto mostrarHardwareSim = [&](const char* accion, const char* hardware_info) {
        std::cout << "  " << RED << "[ Interrupcion ] " << RESET << "Vector IRQ 0x33 (GPU Signal)\n";
        std::cout << "  " << YELLOW << "[ SO -> Hardware ] " << RESET << accion << "\n\n";
        
        std::cout << "  " << MAGENTA << "[ Registros GPU (MMIO / PCIe) ]\n" << RESET;
        std::cout << "    - Registro de DATOS (FB Base): 0x" << std::hex << (0xF0000000 | (dwmsEventTime & 0xFFFF)) << std::dec << " (VRAM Framebuffer Address)\n";
        std::cout << "    - Registro de ESTADO (GPU_STAT): 0x01 (VBlank Active, Engine Idle)\n";
        std::cout << "    - Registro de CONTROL (GPU_CTRL): 0x" << std::hex << (event == EVENT_OBJECT_CREATE ? 0x10 : 0x20) << std::dec << " (Accion: " << accion << ")\n";
        std::cout << "    - Registro BAR0 (Config): 0xFE000000 (Memory Mapped I/O)\n\n";
        
        std::cout << "  " << BLUE << "[ Accion DMA PCIe (Bus Master) ]\n" << RESET;
        std::cout << "    - El hardware de video copia " << hardware_info << " directamente a VRAM.\n";
        std::cout << "    - La GPU actua como Maestro del Bus para evitar saturar la CPU con datos de pixeles.\n";
        std::cout << "  ------------------------------------------------------------------\n";
    };

    if (event == EVENT_OBJECT_CREATE) {
        std::cout << "\n  " << GREEN << "[ EVENTO: NUEVA VENTANA DETECTADA ] " << RESET << windowTitle << "\n";
        mostrarHardwareSim("CREAR CONTEXTO GRAFICO", "recursos UI y texturas");
    } 
    else if (event == EVENT_OBJECT_DESTROY) {
        std::cout << "\n  " << MAGENTA << "[ EVENTO: CIERRE DE VENTANA ] " << RESET << "Liberando recursos...\n";
        mostrarHardwareSim("DESTRUIR CONTEXTO", "limpieza de búferes");
    }
    else if (event == EVENT_SYSTEM_FOREGROUND) {
        if (strlen(windowTitle) > 0) {
            std::cout << "\n  " << YELLOW << "[ EVENTO: CAMBIO DE FOCO ] " << RESET << windowTitle << "\n";
            mostrarHardwareSim("CAMBIO DE CONTEXTO GPU", "prioridad de dibujo");
        }
    }
}

static void simularPantalla() {
    clearScreen();
    mostrarEncabezado();
    std::cout << CYAN << "  [ SIMULADOR: MONITOR DE HARDWARE GRAFICO REAL-TIME ]\n\n" << RESET;
    std::cout << GREEN << BOLD << "  [ HARDWARE GRAFICO DETECTADO ]\n" << RESET;
    std::cout << "    - GPU Principal          : NVIDIA GeForce RTX 3050 Ti Laptop GPU (4GB)\n";
    std::cout << "    - GPU Integrada           : Intel(R) Iris(R) Xe Graphics (2GB)\n";
    std::cout << "    - Bus de Video           : PCI Express 4.0 x16 / x4\n";
    std::cout << "    - Resolucion Activa      : 1920x1080 (Full HD)\n\n";

    std::cout << "  " << YELLOW << "¿Como funciona?" << RESET << " La GPU usa MMIO para hablar con el SO.\n";
    std::cout << "  Detectamos aperturas/cierres y cambios de foco en tiempo real.\n\n";
    std::cout << "  " << YELLOW << "Instrucciones:" << RESET << " Abra, cierre o cambie (Alt+Tab) entre ventanas.\n";
    std::cout << "  Presione " << BOLD << "'ESC'" << RESET << " para salir.\n\n";

    HWINEVENTHOOK hHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_DESTROY, 
        NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    if (!hHook) {
        std::cout << RED << "  Error: No se pudo conectar con el subsistema de video de Windows.\n" << RESET;
        Sleep(2000);
        return;
    }

    MSG msg;
    DWORD lastHeartbeat = GetTickCount();

    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (GetTickCount() - lastHeartbeat > 2000) {
            std::cout << DARK_GRAY << "  [ Latido VSync | VBLANK: 0x" << std::hex << (GetTickCount() / 16) << std::dec << " | GPU OK ]\r" << RESET << std::flush;
            lastHeartbeat = GetTickCount();
        }

        if (_kbhit()) {
            if (_getch() == 27) break;
        }
        Sleep(5);
    }

    UnhookWinEvent(hHook);
    std::cout << "\n\n  Monitoreo de GPU finalizado.\n";
    Sleep(1000);
}

static void simularDiscoDuro() {
    clearScreen();
    mostrarEncabezado();
    std::cout << CYAN << "  [ SIMULACION: DISCO DURO (DMA, SATA y Sectores) ]\n\n" << RESET;
    std::cout << "  " << YELLOW << "¿Como funciona?" << RESET << " El disco duro usa DMA (Direct Memory Access).\n";
    std::cout << "  En lugar de que la CPU mueva los datos, el disco los procesa directamente en la RAM.\n\n";

    // --- INFORMACION DE HARDWARE REAL (SAMSUNG NVMe) ---
    std::cout << GREEN << BOLD << "  [ CARACTERISTICAS FISICAS DEL DISCO DETECTADO ]\n" << RESET;
    std::cout << "    - Nombre Dispositivo     : SAMSUNG NVMe Performance Drive\n";
    std::cout << "    - Interfaz / Bus         : NVMe 1.3 | PCIe 3.0 x4\n";
    std::cout << "    - Velocidad Pico         : 3,500 MB/s\n";
    std::cout << "    - Pistas Totales         : 15,876,300\n";
    std::cout << "    - Sectores por Pista     : 63\n";
    std::cout << "    - Pistas por Cilindro    : 255\n";
    std::cout << "    - Cilindros Totales      : 62,260\n";
    std::cout << "    - Sectores Totales       : ~1,000,206,900\n";
    std::cout << "    - Capacidad Real         : 476.9 GB\n\n";

    // --- FASE 1: ESCRITURA ---
    std::string tempFilename = "./temp_sim_write.dat"; // Ruta local para evitar errores de permiso
    const long long WRITE_SIZE_MB = 2150; // ~2.1 GB (Cumple con > 2GB)
    
    std::cout << YELLOW << BOLD << "  --- FASE 1: ESCRITURA POR DMA (Memoria -> Disco) ---\n" << RESET;
    auto startWrite = std::chrono::high_resolution_clock::now();

    std::cout << MAGENTA << "  [ Registros de Hardware ]\n" << RESET;
    std::cout << "    - Registro de CONTROL/Comando : 0x35 (WRITE DMA EXT)\n";
    std::cout << "    - Registro Sector Count      : " << (WRITE_SIZE_MB * 1024 * 2) << " sectores\n\n";

    std::cout << "  " << YELLOW << "[ Vista previa de Datos a Escribir (Binario) ]\n" << RESET;
    std::cout << "    Muestra de buffer: ";
    for(int b=0; b<8; b++) std::cout << std::bitset<8>('X') << " ";
    std::cout << " (Simulando Bloque 'X')\n\n";

    std::cout << BLUE << "  [ Accion Bus Master DMA: ESCRIBIENDO ARCHIVO DE 2.1 GB ]\n" << RESET;
    std::ofstream out(tempFilename, std::ios::binary);
    if (out.is_open()) {
        const size_t CHUNK_SIZE = 64 * 1024 * 1024; // 64MB chunks
        char* writeBuffer = new char[CHUNK_SIZE];
        memset(writeBuffer, 'X', CHUNK_SIZE);
        long long bytesWritten = 0;
        long long totalBytes = WRITE_SIZE_MB * 1024 * 1024;

        while (bytesWritten < totalBytes) {
            size_t toWrite = (totalBytes - bytesWritten > CHUNK_SIZE) ? CHUNK_SIZE : (size_t)(totalBytes - bytesWritten);
            out.write(writeBuffer, toWrite);
            bytesWritten += toWrite;

            double progress = (double)bytesWritten / totalBytes * 100.0;
            double currentGB = (double)bytesWritten / (1024.0 * 1024.0 * 1024.0);
            std::cout << "    " << BLUE << "DMA Escribiendo: " << std::fixed << std::setprecision(2) << currentGB << " GB / 2.10 GB (" << (int)progress << "%) ...\r" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        delete[] writeBuffer;
        out.close();
        auto endWrite = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> writeDiff = endWrite - startWrite;
        std::cout << "\n  " << RED << "[ Interrupcion ] " << RESET << "IRQ 14/15 -> ¡Escritura Completa! (" << std::fixed << std::setprecision(2) << writeDiff.count() << "s)\n\n";
        
        std::cout << "  Presione una tecla para continuar a la FASE 2: LECTURA de 3.3 GB...\n"; _getch();

        // --- FASE 2: LECTURA ---
        std::string readFilename = "C:\\Gravity\\Ragnarok\\data.grf";
        std::ifstream in(readFilename, std::ios::binary | std::ios::ate);
        if (in.is_open()) {
            long long fileSize = in.tellg();
            in.seekg(0, std::ios::beg);

            std::cout << "\n" << YELLOW << BOLD << "  --- FASE 2: LECTURA POR DMA (Disco -> Memoria) ---\n" << RESET;
            auto startRead = std::chrono::high_resolution_clock::now();

            std::cout << MAGENTA << "  [ Registros de Hardware ]\n" << RESET;
            std::cout << "    - LBA Low/Mid/High           : 0x00000000 (Inicio de Archivo)\n";
            std::cout << "    - Device Register            : 0x40 (LBA Mode Activo)\n\n";

            std::cout << "  " << YELLOW << "[ Vista previa del Header del Archivo (Binario) ]\n" << RESET;
            char headerBuffer[16];
            in.read(headerBuffer, 16);
            in.seekg(0, std::ios::beg);
            std::cout << "    Header (GRF): ";
            for(int i=0; i<8; i++) std::cout << std::bitset<8>(headerBuffer[i]) << " ";
            std::cout << "\n\n";

            std::cout << BLUE << "  [ Accion Bus Master DMA: LEYENDO ARCHIVO DE " << std::fixed << std::setprecision(2) << (double)fileSize/(1024*1024*1024) << " GB ]\n" << RESET;
            
            char* readBuffer = new char[64 * 1024 * 1024]; // 64MB buffer
            long long bytesRead = 0;
            while (bytesRead < fileSize) {
                long long toRead = (fileSize - bytesRead > (64 * 1024 * 1024)) ? (64 * 1024 * 1024) : (fileSize - bytesRead);
                in.read(readBuffer, toRead);
                bytesRead += toRead;

                double progress = (double)bytesRead / fileSize * 100.0;
                double currentGB = (double)bytesRead / (1024.0 * 1024.0 * 1024.0);
                std::cout << "    " << BLUE << "DMA Leyendo: " << currentGB << " GB / " << (double)fileSize / (1024.0 * 1024.0 * 1024.0) << " GB (" << (int)progress << "%) ...\r" << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            delete[] readBuffer;
            in.close();

            auto endRead = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> readDiff = endRead - startRead;
            std::cout << "\n  " << RED << "[ Interrupcion ] " << RESET << "IRQ 14/15 -> ¡Lectura Completa! (" << std::fixed << std::setprecision(2) << readDiff.count() << "s)\n\n";
            
            // --- RESUMEN FINAL DE RENDIMIENTO ---
            std::cout << GREEN << BOLD << "  [ RESUMEN DE RENDIMIENTO DMA ]\n" << RESET;
            std::cout << "    - Tiempo Transcurrido Escritura : " << std::fixed << std::setprecision(2) << writeDiff.count() << " segundos\n";
            std::cout << "    - Tiempo Transcurrido Lectura   : " << std::fixed << std::setprecision(2) << readDiff.count() << " segundos\n";
            std::cout << "    - TIEMPO TOTAL DE OPERACION E/S : " << std::fixed << std::setprecision(2) << (writeDiff + readDiff).count() << " segundos\n\n";
        } else {
            std::cout << RED << "\n  [ ERROR ] No se pudo abrir el archivo real de 3.3 GB: " << readFilename << RESET << "\n";
        }
    } else {
        std::cout << RED << "  [ ADVERTENCIA ] No se pudo crear el archivo temporal de 2.1 GB.\n" << RESET;
    }

    std::cout << GREEN << BOLD << "  [ EXPLICACION TECNICA DETALLADA: ¿QUE HACE EL DMA? ]\n" << RESET;
    std::cout << "  1. " << YELLOW << "FASE DE PREPARACION (CPU):" << RESET << "\n";
    std::cout << "     - El SO escribe en los registros del controlador: la direccion de inicio en RAM,\n";
    std::cout << "       el numero de sectores a transferir y el comando (Leer/Escribir).\n";
    
    std::cout << "  2. " << YELLOW << "ADQUISICION DEL BUS (BUS MASTERING):" << RESET << "\n";
    std::cout << "     - El controlador de disco envia una solicitud 'HOLD' al procesador.\n";
    std::cout << "     - La CPU suelta el control de las lineas de datos y responde con 'HLDA' (Hold Acknowledge).\n";
    
    std::cout << "  3. " << YELLOW << "TRANSFERENCIA DIRECTA (CONTROLADOR <-> RAM):" << RESET << "\n";
    std::cout << "     - El controlador mueve los datos directamente por el bus de datos hacia/desde la RAM.\n";
    std::cout << "     - " << RED << "PUNTO CLAVE: " << RESET << "Ningun byte pasa por los registros de la CPU (EAX, EBX, etc.).\n";
    std::cout << "     - Esto permite que la CPU ejecute otros hilos mientras el hardware mueve Gigabytes.\n";
    
    std::cout << "  4. " << YELLOW << "FINALIZACION E INTERRUPCION:" << RESET << "\n";
    std::cout << "     - Al terminar el ultimo sector, el controlador suelta el BUS y activa la IRQ 14 o 15.\n";
    std::cout << "     - La CPU recibe la interrupcion, pausa su tarea actual y el SO marca la E/S como completa.\n\n";

    pausar();
}

void simularOperacionesES() {
    while (true) {
        clearScreen();
        mostrarEncabezado();
        std::cout << CYAN << BOLD;
        std::cout << "  ╔═════════════════════════════════════════════════════════════════╗\n";
        std::cout << "  ║     SIMULADOR EN TIEMPO REAL DE PERIFERICOS Y ACCIONES SO       ║\n";
        std::cout << "  ╠═════════════════════════════════════════════════════════════════╣\n";
        std::cout << "  ║                                                                 ║\n";
        std::cout << "  ║   " << GREEN  << "1" << CYAN << "  ─  Simular Teclado (Detectar Teclas Reales)                ║\n";
        std::cout << "  ║   " << MAGENTA << "2" << CYAN << "  ─  Simular Mouse (Detectar Movimiento y Clics Reales)      ║\n";
        std::cout << "  ║   " << YELLOW << "3" << CYAN << "  ─  Simular Pantalla (Hardware Grafico Directo)             ║\n";
        std::cout << "  ║   " << BLUE   << "4" << CYAN << "  ─  Simular Disco HD (Lectura/Escritura y DMA)              ║\n";
        std::cout << "  ║   " << WHITE  << "5" << CYAN << "  ─  Ver Controladores de Dispositivos E/S (HARDWARE REAL)   ║\n";
        std::cout << "  ║                                                                 ║\n";
        std::cout << "  ║   " << WHITE  << "0" << CYAN << "  ─  Volver al Menú Principal                                ║\n";
        std::cout << "  ║                                                                 ║\n";
        std::cout << "  ╚═════════════════════════════════════════════════════════════════╝\n";
        std::cout << RESET;
        std::cout << "\n  " << BOLD << "Seleccione una opción (0-5): " << RESET;

        std::string opc;
        std::getline(std::cin, opc);

        if (opc == "1") {
            simularTeclado();
        } else if (opc == "2") {
            simularMouse();
        } else if (opc == "3") {
            simularPantalla();
        } else if (opc == "4") {
            simularDiscoDuro();
        } else if (opc == "5") {
            clearScreen();
            mostrarEncabezado();
            mostrarDispositivosES();
        } else if (opc == "0") {
            break;
        } else {
            std::cout << RED << "  Opción inválida.\n" << RESET;
            Sleep(1000);
        }
    }
}
