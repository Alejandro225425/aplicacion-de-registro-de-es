/*==========================================================================
  device_monitor.h  -  Estructuras y declaraciones compartidas
  Monitor de Registros de Controladores de E/S, DMA e Interrupciones del SO
  Sistemas Operativos - 2026
==========================================================================*/
#pragma once
#ifndef DEVICE_MONITOR_H
#define DEVICE_MONITOR_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <string>
#include <vector>

// ─── Colores ANSI para la consola ────────────────────────────────────────────
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define DARK_GRAY   "\033[90m"
#define BG_BLUE     "\033[44m"
#define BG_DARK     "\033[40m"

// ─── Estructura: Dispositivo de E/S ──────────────────────────────────────────
struct DeviceInfo {
    std::wstring nombre;       // Nombre descriptivo del dispositivo
    std::wstring clase;        // Clase (Mouse, Keyboard, Monitor…)
    std::wstring fabricante;   // Fabricante
    std::wstring instanceId;   // ID de instancia
    std::wstring estado;       // Estado del controlador
    std::wstring ubicacion;    // Descripción de ubicación
    DWORD        devNodeStatus; // Código de estado del nodo
    std::vector<std::pair<std::wstring, std::wstring>> detallesTecnicos; // Detalles adicionales
};

// ─── Estructura: Canal DMA ────────────────────────────────────────────────────
struct DMAChannel {
    int          canal;        // Número de canal (0-7)
    std::wstring dispositivo;  // Dispositivo asignado
    bool         enUso;        // true si está en uso
};

// ─── Estructura: Línea de Interrupción (IRQ) ──────────────────────────────────
struct IRQInfo {
    int          irq;          // Número de IRQ
    std::wstring dispositivo;  // Dispositivo que usa la IRQ
    std::wstring tipo;         // Tipo: Level / Edge
    bool         compartida;   // Si la IRQ es compartida
};

// ─── Declaraciones de funciones ───────────────────────────────────────────────

// device_monitor.cpp
void mostrarEncabezado();
void mostrarSeparador(const char* titulo, const char* color);
void mostrarDispositivosES();

// dma_monitor.cpp
void mostrarDMA();

// interrupt_monitor.cpp
void mostrarInterrupciones();

// simulation_monitor.cpp
void simularOperacionesES();

// Utilidades
std::string wstrToStr(const std::wstring& wstr);
void        pausar();

#endif // DEVICE_MONITOR_H
