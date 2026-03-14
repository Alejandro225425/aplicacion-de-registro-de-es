/*==========================================================================
  main.cpp  -  Monitor de Controladores E/S, DMA e Interrupciones del SO
  Punto de entrada principal con menú interactivo.
  Sistemas Operativos - 2026
==========================================================================*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include "device_monitor.h"

// ─── Limpiar pantalla ─────────────────────────────────────────────────────────
static void limpiar() {
    system("cls");
}

// ─── Menú principal ───────────────────────────────────────────────────────────
static void mostrarMenu() {
    std::cout << "\n" << CYAN << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                   MENÚ PRINCIPAL                         ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════════════╣\n";
    std::cout << "  ║                                                          ║\n";
    std::cout << "  ║   " << GREEN  << "1" << CYAN << "  ─  Registros de Controladores de E/S               ║\n";
    std::cout << "  ║      " << RESET << "     (Mouse, Teclado, Pantalla, USB, Audio...)   " << CYAN << "║\n";
    std::cout << "  ║                                                          ║\n";
    std::cout << "  ║   " << MAGENTA << "2" << CYAN << "  ─  Canales DMA (Direct Memory Access)              ║\n";
    std::cout << "  ║      " << RESET << "     (Estado de los 8 canales DMA del sistema)  " << CYAN << "║\n";
    std::cout << "  ║                                                          ║\n";
    std::cout << "  ║   " << RED    << "3" << CYAN << "  ─  Interrupciones del SO (IRQ)                     ║\n";
    std::cout << "  ║      " << RESET << "     (Tabla IRQ + estadisticas DPC por CPU)     " << CYAN << "║\n";
    std::cout << "  ║                                                          ║\n";
    std::cout << "  ║   " << YELLOW << "4" << CYAN << "  ─  Mostrar TODO (E/S + DMA + Interrupciones)       ║\n";
    std::cout << "  ║                                                          ║\n";
    std::cout << "  ║   " << BLUE   << "5" << CYAN << "  ─  Simular Operaciones de E/S                      ║\n";
    std::cout << "  ║      " << RESET << "     (Teclado, Mouse, Pantalla y Disco Duro)    " << CYAN << "║\n";
    std::cout << "  ║                                                          ║\n";
    std::cout << "  ║   " << WHITE  << "0" << CYAN << "  ─  Salir                                           ║\n";
    std::cout << "  ║                                                          ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    std::cout << RESET;
    std::cout << "\n  " << BOLD << "Seleccione una opción: " << RESET;
}

// ─── Función principal ────────────────────────────────────────────────────────
int main() {
    // Configurar consola para UTF-8 y colores ANSI
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Título de la ventana de consola
    SetConsoleTitleA("Monitor E/S - Controladores, DMA e Interrupciones | SO 2026");

    limpiar();
    mostrarEncabezado();

    std::string opcion;
    // Iniciamos directamente el menú del simulador en lugar del menú general.
    simularOperacionesES();

    std::cout << "\n" << GREEN << BOLD
              << "  Saliendo del monitor... ¡Hasta luego!\n\n" << RESET;

    return 0;
}
