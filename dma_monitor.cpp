/*==========================================================================
  dma_monitor.cpp  -  Información de canales DMA
  Consulta el registro de Windows para listar canales DMA y sus asignaciones.
  Sistemas Operativos - 2026
==========================================================================*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include "device_monitor.h"

#pragma comment(lib, "setupapi.lib")

// Uso estándar de canales DMA ISA clásico
static struct { int canal; const char* uso_tipico; } DMA_TIPICO[] = {
    {0, "Disponible / Sin uso estandar"},
    {1, "Controlador de sonido (SB16 legacy)"},
    {2, "Controlador de disquetera (FDC)"},
    {3, "Controlador de sonido / Puerto paralelo ECP"},
    {4, "CASCADA: Enlaza DMA 0-3 con controlador maestro (DMA 4-7)"},
    {5, "Expansion ISA / Sonido 16-bit"},
    {6, "Disponible / Sin uso estandar"},
    {7, "Expansion ISA / Sin uso estandar"},
    {-1, nullptr}
};

// ─── Leer DMA desde el registro (AllocConfig de PnP) ─────────────────────────
static std::map<int, std::vector<std::string>> obtenerDMADesdeRegistro() {
    std::map<int, std::vector<std::string>> dmaMap;

    HKEY hEnum = NULL;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Enum", 0, KEY_READ, &hEnum) != ERROR_SUCCESS)
        return dmaMap;

    const char* buses[] = {"Root", "ISAPNP", "PCI", nullptr};
    for (int b = 0; buses[b]; b++) {
        HKEY hBus = NULL;
        if (RegOpenKeyExA(hEnum, buses[b], 0, KEY_READ, &hBus) != ERROR_SUCCESS) continue;

        char devId[256] = {};
        for (DWORD di = 0; ; di++) {
            DWORD sz = sizeof(devId) - 1;
            memset(devId, 0, sizeof(devId));
            if (RegEnumKeyExA(hBus, di, devId, &sz, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

            HKEY hDev = NULL;
            if (RegOpenKeyExA(hBus, devId, 0, KEY_READ, &hDev) != ERROR_SUCCESS) continue;

            char instId[256] = {};
            for (DWORD ii = 0; ; ii++) {
                DWORD sz2 = sizeof(instId) - 1;
                memset(instId, 0, sizeof(instId));
                if (RegEnumKeyExA(hDev, ii, instId, &sz2, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;

                // Abrir LogConf de la instancia
                std::string lcSubkey = std::string(instId) + "\\LogConf";
                HKEY hLC = NULL;
                if (RegOpenKeyExA(hDev, lcSubkey.c_str(), 0, KEY_READ, &hLC) != ERROR_SUCCESS) continue;

                BYTE data[4096] = {}; DWORD dataSz = sizeof(data), type = 0;
                if (RegQueryValueExA(hLC, "AllocConfig", nullptr, &type, data, &dataSz) == ERROR_SUCCESS
                    && dataSz > 3) {
                    // Parsear descriptores PnP: tag pequeno DMA = 0x2A
                    for (DWORD k = 0; k + 2 < dataSz; k++) {
                        if (data[k] == 0x2A) {
                            BYTE dmaMask = data[k+1];
                            for (int ch = 0; ch < 8; ch++) {
                                if (dmaMask & (1 << ch)) {
                                    // Nombre del dispositivo
                                    HKEY hInst = NULL;
                                    char friendlyName[256] = {};
                                    if (RegOpenKeyExA(hDev, instId, 0, KEY_READ, &hInst) == ERROR_SUCCESS) {
                                        DWORD fnSz = sizeof(friendlyName) - 1, fnType = 0;
                                        if (RegQueryValueExA(hInst, "FriendlyName", nullptr, &fnType,
                                                (LPBYTE)friendlyName, &fnSz) != ERROR_SUCCESS) {
                                            fnSz = sizeof(friendlyName) - 1;
                                            RegQueryValueExA(hInst, "DeviceDesc", nullptr, &fnType,
                                                (LPBYTE)friendlyName, &fnSz);
                                        }
                                        RegCloseKey(hInst);
                                    }
                                    std::string devName = (strlen(friendlyName) > 0) ? friendlyName : devId;
                                    dmaMap[ch].push_back(devName);
                                }
                            }
                        }
                    }
                }
                RegCloseKey(hLC);
            }
            RegCloseKey(hDev);
        }
        RegCloseKey(hBus);
    }
    RegCloseKey(hEnum);
    return dmaMap;
}

// ─── Función principal: Mostrar información DMA ───────────────────────────────
void mostrarDMA() {
    mostrarSeparador("CANALES DMA (DIRECT MEMORY ACCESS)", MAGENTA);

    std::cout << MAGENTA << BOLD << "\n  Que es el DMA?\n" << RESET
              << "  El DMA permite que los dispositivos transfieran datos directamente\n"
              << "  a/desde la memoria principal sin pasar por la CPU, reduciendo la\n"
              << "  carga del procesador y aumentando el rendimiento del sistema.\n\n";

    std::cout << MAGENTA << BOLD << "  Arquitectura DMA (simplificada):\n" << RESET
              << "  [Dispositivo] --solicitud--> [Controlador DMA] <--bus datos--> [RAM]\n"
              << "                                      |\n"
              << "                         interrupcion al terminar\n"
              << "                                      v\n"
              << "                                   [CPU]\n\n";

    // Leer asignaciones reales del registro
    std::map<int,std::vector<std::string>> dmaReal;
    try { dmaReal = obtenerDMADesdeRegistro(); }
    catch (...) {}

    std::cout << YELLOW << BOLD << "  Tabla de Canales DMA del Sistema:\n\n" << RESET;
    std::cout << CYAN
              << "  +--------+--------------------------------------------------------------+\n"
              << "  | Canal  | Dispositivo / Asignacion                                     |\n"
              << "  +--------+--------------------------------------------------------------+\n"
              << RESET;

    for (int c = 0; c <= 7; c++) {
        bool enUso = (dmaReal.count(c) > 0 && !dmaReal[c].empty());

        std::string usoTipico;
        for (int t = 0; DMA_TIPICO[t].canal >= 0; t++) {
            if (DMA_TIPICO[t].canal == c) { usoTipico = DMA_TIPICO[t].uso_tipico; break; }
        }

        const char* colorCanal = (c == 4) ? YELLOW : (enUso ? GREEN : RESET);

        std::cout << CYAN << "  | " << colorCanal << BOLD
                  << "DMA " << c << RESET << CYAN << "  | " << RESET;

        std::string descripcion;
        if (enUso)       descripcion = dmaReal[c][0];
        else if (c == 4) descripcion = "CASCADA - Enlaza DMA 0-3 con controlador maestro (DMA 4-7)";
        else             descripcion = usoTipico;

        if (descripcion.size() > 62) descripcion = descripcion.substr(0, 60) + "..";

        std::cout << colorCanal << std::left << std::setw(62) << descripcion
                  << RESET << CYAN << " |\n" << RESET;

        if (enUso && dmaReal[c].size() > 1) {
            for (size_t k = 1; k < dmaReal[c].size() && k < 3; k++) {
                std::string extra = "  + " + dmaReal[c][k];
                if (extra.size() > 62) extra = extra.substr(0, 60) + "..";
                std::cout << CYAN << "  |        | " << RESET
                          << GREEN << std::left << std::setw(62) << extra
                          << RESET << CYAN << " |\n" << RESET;
            }
        }
    }

    std::cout << CYAN
              << "  +--------+--------------------------------------------------------------+\n"
              << RESET;

    std::cout << "\n" << BLUE
              << "  Nota: Los sistemas modernos (PCIe/USB) usan bus mastering DMA\n"
              << "  en lugar de los canales ISA clasicos. El DMA legacy (0-7) esta\n"
              << "  presente por compatibilidad pero en desuso en hardware moderno.\n"
              << RESET;

    pausar();
}
