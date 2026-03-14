/*==========================================================================
  interrupt_monitor.cpp  -  Interrupciones del Sistema Operativo
  Muestra IRQs asignadas a dispositivos y estadísticas de interrupciones.
  Usa SetupAPI y NtQuerySystemInformation (ntdll.dll).
  Sistemas Operativos - 2026
==========================================================================*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <winternl.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include "device_monitor.h"

#pragma comment(lib, "setupapi.lib")

// ─── NtQuerySystemInformation ─────────────────────────────────────────────────
typedef NTSTATUS(WINAPI* PfnNtQSI)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
#define SystemInterruptInformation ((SYSTEM_INFORMATION_CLASS)23)

struct SYSTEM_INTERRUPT_INFO {
    ULONG ContextSwitches;
    ULONG DpcCount;
    ULONG DpcRate;
    ULONG TimeIncrement;
    ULONG DpcBypassCount;
    ULONG ApcBypassCount;
};

// ─── Leer string ANSI de propiedad del registro (para interrupt_monitor) ──────
static std::string leerPropRegA(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devData, DWORD prop) {
    DWORD dt = 0, sz = 0;
    SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData, prop, &dt, nullptr, 0, &sz);
    if (sz == 0) return "";
    if (dt != REG_SZ && dt != REG_EXPAND_SZ && dt != REG_MULTI_SZ) return "";
    std::vector<BYTE> buf(sz + 2, 0);
    if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData, prop, &dt, buf.data(), sz, nullptr))
        return "";
    return std::string(reinterpret_cast<char*>(buf.data()));
}

// ─── Obtener IRQs desde AllocConfig del registro ──────────────────────────────
static std::map<int, std::vector<std::string>> obtenerIRQsDispositivos() {
    std::map<int, std::vector<std::string>> irqMap;

    HDEVINFO hDevInfo = SetupDiGetClassDevsW(nullptr, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE) return irqMap;

    SP_DEVINFO_DATA devData;
    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    try {
        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devData); i++) {
            try {
                // Nombre del dispositivo
                std::string dname = leerPropRegA(hDevInfo, devData, SPDRP_FRIENDLYNAME);
                if (dname.empty())
                    dname = leerPropRegA(hDevInfo, devData, SPDRP_DEVICEDESC);

                // ID de instancia (ANSI)
                char instIdA[512] = {};
                SetupDiGetDeviceInstanceIdA(hDevInfo, &devData, instIdA, 511, nullptr);

                // Construir ruta al LogConf
                std::string regPath = std::string("SYSTEM\\CurrentControlSet\\Enum\\")
                                      + instIdA + "\\LogConf";

                HKEY hLC = NULL;
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hLC) != ERROR_SUCCESS)
                    continue;

                BYTE data[8192] = {}; DWORD dataSz = sizeof(data), type = 0;
                if (RegQueryValueExA(hLC, "AllocConfig", nullptr, &type, data, &dataSz) == ERROR_SUCCESS
                    && dataSz > 3) {
                    // Parsear descriptores PnP: tag IRQ pequeño = 0x22 o 0x23
                    for (DWORD k = 0; k + 3 < dataSz; k++) {
                        if (data[k] == 0x22 || data[k] == 0x23) {
                            WORD mask = (WORD)(data[k+1]) | ((WORD)(data[k+2]) << 8);
                            for (int irq = 0; irq < 16; irq++) {
                                if (mask & (1 << irq)) {
                                    std::string devStr = dname.empty() ? instIdA : dname;
                                    irqMap[irq].push_back(devStr);
                                }
                            }
                        }
                    }
                }
                RegCloseKey(hLC);
            }
            catch (...) {}
        }
    }
    catch (...) {}

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return irqMap;
}

// ─── Estadísticas DPC por CPU ─────────────────────────────────────────────────
struct CPUInterruptStats {
    DWORD cpuId;
    ULONG contextSwitches;
    ULONG dpcCount;
    ULONG dpcRate;
};

static std::vector<CPUInterruptStats> obtenerEstadisticasCPU() {
    std::vector<CPUInterruptStats> stats;
    try {
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (!hNtdll) return stats;

        PfnNtQSI NtQuerySysInfo = (PfnNtQSI)GetProcAddress(hNtdll, "NtQuerySystemInformation");
        if (!NtQuerySysInfo) return stats;

        SYSTEM_INFO si;
        GetSystemInfo(&si);
        DWORD cpuCount = si.dwNumberOfProcessors;
        if (cpuCount == 0 || cpuCount > 256) return stats;

        std::vector<SYSTEM_INTERRUPT_INFO> buf(cpuCount);
        ULONG retLen = 0;
        NTSTATUS st = NtQuerySysInfo(SystemInterruptInformation,
            buf.data(), (ULONG)(buf.size() * sizeof(SYSTEM_INTERRUPT_INFO)), &retLen);

        if (st == 0) {
            for (DWORD c = 0; c < cpuCount; c++) {
                CPUInterruptStats s;
                s.cpuId           = c;
                s.contextSwitches = buf[c].ContextSwitches;
                s.dpcCount        = buf[c].DpcCount;
                s.dpcRate         = buf[c].DpcRate;
                stats.push_back(s);
            }
        }
    }
    catch (...) {}
    return stats;
}

// ─── Descripciones IRQ estándar x86 ──────────────────────────────────────────
static const char* IRQ_DESCRIPCION[] = {
    /* 0 */ "Temporizador del sistema (PIT)",
    /* 1 */ "Controlador de teclado (8042 KBD)",
    /* 2 */ "Cascada IRQ (PIC esclavo 8-15)",
    /* 3 */ "Puerto serie COM2 / COM4",
    /* 4 */ "Puerto serie COM1 / COM3",
    /* 5 */ "Puerto paralelo LPT2 / Sonido",
    /* 6 */ "Controlador de disquetera (FDC)",
    /* 7 */ "Puerto paralelo LPT1 / Sonido",
    /* 8 */ "Reloj en tiempo real (RTC CMOS)",
    /* 9 */ "IRQ2 redirigida / ACPI",
    /*10 */ "Disponible / Red / USB",
    /*11 */ "Disponible / USB / PCI",
    /*12 */ "Raton PS/2 (8042 AUX)",
    /*13 */ "Coprocesador matematico / FPU",
    /*14 */ "Controlador IDE primario",
    /*15 */ "Controlador IDE secundario"
};

// ─── Función principal: Mostrar interrupciones ────────────────────────────────
void mostrarInterrupciones() {
    mostrarSeparador("INTERRUPCIONES DEL SISTEMA OPERATIVO (IRQ)", RED);

    std::cout << RED << BOLD << "\n  Que son las interrupciones?\n" << RESET
              << "  Una interrupcion es una senal que un dispositivo envia a la CPU\n"
              << "  para indicar que necesita atencion. El procesador suspende la\n"
              << "  tarea actual, ejecuta el manejador (ISR) y retoma la ejecucion.\n\n";

    std::cout << RED << BOLD << "  Ciclo de interrupcion:\n" << RESET
              << "  Dispositivo --> Senal IRQ --> PIC/APIC --> CPU\n"
              << "      |                                      |\n"
              << "      |                           Guarda contexto (PC, registros)\n"
              << "      |                                      |\n"
              << "      +------------------ ISR <--------------+\n"
              << "                 (Rutina de servicio de interrupcion)\n"
              << "                                      |\n"
              << "                         Restaura contexto -> Reanuda tarea\n\n";

    // IRQs activas
    std::cout << YELLOW << BOLD << "  Tabla de Lineas IRQ Estandar x86 y Asignaciones Actuales:\n\n" << RESET;

    std::map<int,std::vector<std::string>> irqMap;
    try { irqMap = obtenerIRQsDispositivos(); }
    catch (...) {}

    std::cout << CYAN
              << "  +-------+------------------------------------+-----------------------+\n"
              << "  |  IRQ  | Descripcion Estandar               | Dispositivo Asignado  |\n"
              << "  +-------+------------------------------------+-----------------------+\n"
              << RESET;

    for (int irq = 0; irq < 16; irq++) {
        bool asignada = irqMap.count(irq) > 0 && !irqMap[irq].empty();
        const char* colorIRQ = asignada ? GREEN : (irq == 2 ? YELLOW : RESET);

        std::string dispStr = asignada ? irqMap[irq][0] : ((irq == 2) ? "Cascada PIC" : "Sin asignar");
        if (dispStr.size() > 21) dispStr = dispStr.substr(0, 19) + "..";

        std::string desc = std::string(IRQ_DESCRIPCION[irq]);
        if (desc.size() > 34) desc = desc.substr(0, 32) + "..";

        std::cout << CYAN << "  | " << colorIRQ << BOLD
                  << std::right << std::setw(4) << irq << "  " << RESET
                  << CYAN << " | " << RESET
                  << std::left << std::setw(35) << desc
                  << CYAN << "| " << colorIRQ
                  << std::setw(22) << dispStr
                  << RESET << CYAN << "|\n" << RESET;

        if (asignada && irqMap[irq].size() > 1) {
            for (size_t k = 1; k < irqMap[irq].size() && k < 3; k++) {
                std::string extra = irqMap[irq][k];
                if (extra.size() > 21) extra = extra.substr(0, 19) + "..";
                std::cout << CYAN << "  |       | " << RESET
                          << MAGENTA << std::setw(35) << " (compartida)"
                          << RESET << CYAN << "| " << MAGENTA
                          << std::setw(22) << extra
                          << RESET << CYAN << "|\n" << RESET;
            }
        }
    }

    std::cout << CYAN
              << "  +-------+------------------------------------+-----------------------+\n"
              << RESET;

    // Estadísticas DPC por CPU
    std::cout << "\n" << YELLOW << BOLD
              << "  Estadisticas de Interrupciones por Procesador Logico:\n\n" << RESET;

    auto cpuStats = obtenerEstadisticasCPU();

    if (!cpuStats.empty()) {
        std::cout << CYAN
                  << "  +--------+------------------+--------------+------------+\n"
                  << "  |  CPU   | Cambios Contexto |  Conteo DPC  |  Tasa DPC  |\n"
                  << "  +--------+------------------+--------------+------------+\n"
                  << RESET;

        for (auto& s : cpuStats) {
            std::cout << CYAN << "  | " << RESET
                      << GREEN << "CPU " << std::left << std::setw(4) << s.cpuId << RESET
                      << CYAN << " | " << RESET
                      << std::right << std::setw(16) << s.contextSwitches
                      << CYAN << " | " << RESET
                      << std::right << std::setw(12) << s.dpcCount
                      << CYAN << " | " << RESET
                      << std::right << std::setw(10) << s.dpcRate
                      << CYAN << " |\n" << RESET;
        }
        std::cout << CYAN
                  << "  +--------+------------------+--------------+------------+\n"
                  << RESET;

        std::cout << "\n  " << BLUE
                  << "DPC = Deferred Procedure Call (llamada diferida).\n"
                  << "  El SO divide el manejo de interrupciones en 2 fases:\n"
                  << "  1) ISR rapida (nivel hardware) --> 2) DPC (procesamiento posterior)\n"
                  << RESET;
    } else {
        std::cout << YELLOW << "  (Estadisticas de CPU no disponibles)\n" << RESET;
    }

    // Tipos de interrupción
    std::cout << "\n" << MAGENTA << BOLD << "  Tipos de Interrupcion en Windows:\n" << RESET
              << "  +-------------------+--------------------------------------------+\n"
              << "  | Tipo              | Descripcion                                |\n"
              << "  +-------------------+--------------------------------------------+\n"
              << "  | Hardware (IRQ)    | Dispositivos senalan a la CPU via PIC/APIC |\n"
              << "  | Software (INT nn) | Instruccion INT del codigo                 |\n"
              << "  | Excepcion         | Division/0, page fault, GPF, etc.          |\n"
              << "  | NMI               | Non-Maskable: errores HW criticos          |\n"
              << "  | IPI               | Inter-Processor en sistemas multi-nucleo   |\n"
              << "  +-------------------+--------------------------------------------+\n";

    pausar();
}
