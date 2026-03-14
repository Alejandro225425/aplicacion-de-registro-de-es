# Monitor de Registro de E/S, DMA e Interrupciones

Este proyecto es un simulador y monitor de bajo nivel desarrollado en C++ para la asignatura de **Sistemas Operativos (2026)**. Permite inspeccionar y simular el comportamiento de controladores de Entrada/Salida, canales DMA e interrupciones del sistema en un entorno Windows.

## 🚀 Características Principales

- **Monitor de Dispositivos de E/S**: Listado detallado de periféricos (Teclado, Mouse, Pantalla, USB, Audio) utilizando la API de Windows (`setupapi.h`).
- **Gestión de DMA**: Visualización del estado de los 8 canales DMA del sistema.
- **Control de Interrupciones (IRQ)**: Tabla de líneas de interrupción y estadísticas de DPC (Deferred Procedure Calls) por CPU.
- **Simulador de Operaciones**: Demostración interactiva de cómo operan el teclado, mouse, pantalla y disco duro a través de registros y acceso directo a memoria.

## 📁 Estructura del Proyecto

- `main.cpp`: Punto de entrada principal y menú interactivo.
- `device_monitor.cpp / .h`: Lógica para el escaneo y visualización de dispositivos de hardware.
- `dma_monitor.cpp`: Implementación del monitor de canales DMA.
- `interrupt_monitor.cpp`: Visualización de IRQs y manejo de interrupciones.
- `simulation_monitor.cpp`: Simulaciones lógicas de flujo de datos E/S.
- `compile_run.bat`: Script automatizado para compilar con G++ y ejecutar el programa.

## 🛠️ Compilación y Ejecución

Para compilar y ejecutar el proyecto en Windows, simplemente ejecuta el archivo batch incluido:

```bash
compile_run.bat
```

**Requisitos**:
- Compilador de C++ (MinGW/G++ recomendado).
- Sistema Operativo Windows.

## ⚠️ Notas de Desarrollo

Debido al tamaño de algunos archivos de datos generados durante las pruebas, se han excluido del repositorio los siguientes elementos mediante `.gitignore`:
- `*.exe` (Binarios compilados)
- `temp_sim_write.dat` (~2.2 GB)
- `archivo_grande_simulado.dat` (~50 MB)

---
*Desarrollado para fines educativos en el estudio de arquitecturas de Sistemas Operativos.*
