/*
Se implementa >>> TECLA 1: Inicio del cronometro - TECLA 2: Reset del cronometro si se presiona una vez, de los
valores y tambien parciales si se mantiene presionado - TECLA 3: Captura de valores parciales (3 en total).
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "ili9341.h"
#include "digitos.h"
#include "teclasconfig.h"
#include "leds.h"
#include "cronometro.h"
#include "esp_log.h"

#define DIGITO_ANCHO 50
#define DIGITO_ALTO 90
#define DIGITO_ENCENDIDO ILI9341_BLUE2
#define DIGITO_APAGADO 0x3800
#define DIGITO_FONDO ILI9341_BLACK

QueueHandle_t colaDigitos;
QueueHandle_t colaEstadosCronometro;
QueueHandle_t colaDigitosParciales;
SemaphoreHandle_t semaforoAccesoDigitos;

bool flagPausa = true;
bool flagParcial = false;

typedef enum
{
    PAUSAR,
    REINICIAR,
    REINICIARTODO,
    PARCIAL,
    MODO_RELOJ
} estadosCronometro_t;

void leerBotones(void *p)
{
    configuracion_pin_t configuraciones[] = {
        {TEC1_Pausa, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {TEC2_Reiniciar, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {TEC3_Parcial, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY}};
    ConfigurarTeclas(configuraciones, sizeof(configuraciones) / sizeof(configuraciones[0]));

    bool estadoanteriorPausa = true;
    bool estadoanteriorReiniciar = true;
    bool estadoanteriorTiempoParcial = true;
    TickType_t tiempoPresionadoPausa = 0;
    TickType_t tiempoPresionadoReiniciar = 0;

    while (1)
    {
        // --- TEC1: PAUSAR / DETECCIÓN DE PRESIÓN PROLONGADA ---
        if (gpio_get_level(TEC1_Pausa) == 0)
        {
            if (estadoanteriorPausa)
            {
                tiempoPresionadoPausa = xTaskGetTickCount(); // Registrar el tiempo de inicio
                estadoanteriorPausa = false;
            }
            else if ((xTaskGetTickCount() - tiempoPresionadoPausa) > pdMS_TO_TICKS(2000)) // Más de 2 segundos
            {
                estadosCronometro_t evento = MODO_RELOJ; // Nuevo evento para cambiar a modo reloj
                xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);

                ESP_LOGI("BOTONES", "⏱ Cambio a MODO RELOJ");
                tiempoPresionadoPausa = 0;
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
        else
        {
            if (!estadoanteriorPausa && tiempoPresionadoPausa > 0 &&
                (xTaskGetTickCount() - tiempoPresionadoPausa) <= pdMS_TO_TICKS(2000))
            {
                estadosCronometro_t evento = PAUSAR;
                xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
            }
            tiempoPresionadoPausa = 0;
            estadoanteriorPausa = true;
        }

        // --- TEC2: REINICIAR / REINICIAR TODO ---
        if (gpio_get_level(TEC2_Reiniciar) == 0)
        {
            if (estadoanteriorReiniciar)
            {
                tiempoPresionadoReiniciar = xTaskGetTickCount();
                estadoanteriorReiniciar = false;
            }
            else if ((xTaskGetTickCount() - tiempoPresionadoReiniciar) > pdMS_TO_TICKS(2000)) // Más de 2 segundos
            {
                estadosCronometro_t evento = REINICIARTODO;
                xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
                ESP_LOGI("BOTONES", "🔄 REINICIAR TODO");
                tiempoPresionadoReiniciar = 0;
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        else
        {
            if (!estadoanteriorReiniciar && tiempoPresionadoReiniciar > 0 &&
                (xTaskGetTickCount() - tiempoPresionadoReiniciar) <= pdMS_TO_TICKS(2000))
            {
                estadosCronometro_t evento = REINICIAR;
                xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
                ESP_LOGI("BOTONES", "🔄 REINICIAR");
            }
            tiempoPresionadoReiniciar = 0;
            estadoanteriorReiniciar = true;
        }

        // --- TEC3: TIEMPO PARCIAL ---
        if ((gpio_get_level(TEC3_Parcial) == 0) && estadoanteriorTiempoParcial)
        {
            estadosCronometro_t evento = PARCIAL;
            xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
            flagParcial = true;
            ESP_LOGI("BOTONES", "⏳ Tiempo Parcial Registrado");
            estadoanteriorTiempoParcial = false;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if (gpio_get_level(TEC3_Parcial) != 0)
        {
            estadoanteriorTiempoParcial = true;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void manejoEstadosCronometro(void *p)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    digitos_t digitosActuales = {0, 0, 0, 0, 0};
    bool enPausa = true;

    while (1)
    {
        estadosCronometro_t eventoRecibido;

        if (xQueueReceive(colaEstadosCronometro, &eventoRecibido, 0))
        {
            switch (eventoRecibido)
            {
            case PAUSAR:
                enPausa = !enPausa;
                break;

            case REINICIAR:
                digitosActuales = (digitos_t){0, 0, 0, 0, 0};
                enPausa = true;
                xQueueSend(colaDigitos, &digitosActuales, portMAX_DELAY);
                break;

            case REINICIARTODO:
                digitosActuales = (digitos_t){0, 0, 0, 0, 0};
                enPausa = true;
                xQueueSend(colaDigitos, &digitosActuales, portMAX_DELAY);

                digitosParciales = (digitos_t){0, 0, 0, 0, 0};
                xQueueSend(colaDigitosParciales, &digitosParciales, portMAX_DELAY);
                break;

            case PARCIAL:
                xQueueSend(colaDigitosParciales, &digitosActuales, portMAX_DELAY);
                break;
            case MODO_RELOJ:
                // Vaciar las colas antes de reiniciar los valores
                xQueueReset(colaDigitos);
                xQueueReset(colaDigitosParciales);

                // Reiniciar los valores de los dígitos
                digitosActuales = (digitos_t){0, 0, 0, 0, 0};
                enPausa = true;
                xQueueSend(colaDigitos, &digitosActuales, portMAX_DELAY);

                digitosParciales = (digitos_t){0, 0, 0, 0, 0};
                xQueueSend(colaDigitosParciales, &digitosParciales, portMAX_DELAY);

                ESP_LOGI("CRONOMETRO", "⏱ MODO RELOJ ACTIVADO - Datos de la cola eliminados y dígitos reiniciados");
                break;

                break;
            }
        }

        if (!enPausa)
        {
            ActualizarCronometro(&digitosActuales);
            xQueueSend(colaDigitos, &digitosActuales, portMAX_DELAY);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

void actualizarPantalla(void *p)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    panel_t PanelMinutos = CrearPanel(12, 0, 2, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t PanelSegundos = CrearPanel(132, 0, 2, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t PanelDecimas = CrearPanel(252, 0, 1, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    panel_t Panelparcial1Minutos = CrearPanel(80, 100, 2, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t Panelparcial1Segundos = CrearPanel(150, 100, 2, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t Panelparcial1Decimas = CrearPanel(220, 100, 1, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    panel_t Panelparcial2Minutos = CrearPanel(80, 145, 2, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t Panelparcial2Segundos = CrearPanel(150, 145, 2, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t Panelparcial2Decimas = CrearPanel(220, 145, 1, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    panel_t Panelparcial3Minutos = CrearPanel(80, 200, 2, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t Panelparcial3Segundos = CrearPanel(150, 200, 2, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t Panelparcial3Decimas = CrearPanel(220, 200, 1, 30, 20, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    ILI9341DrawFilledCircle(121, 22, 3, DIGITO_ENCENDIDO);
    ILI9341DrawFilledCircle(121, 62, 3, DIGITO_ENCENDIDO);
    ILI9341DrawFilledCircle(244, 80, 3, DIGITO_ENCENDIDO);

    DibujarDigito(PanelMinutos, 0, 0);
    DibujarDigito(PanelMinutos, 1, 0);
    DibujarDigito(PanelSegundos, 0, 0);
    DibujarDigito(PanelSegundos, 1, 0);
    DibujarDigito(PanelDecimas, 0, 0);

    digitos_t digitosPrevios = {-1, -1, -1, -1, -1};
    digitos_t parcialesPanel1 = {-1, -1, -1, -1, -1};
    digitos_t parcialesPanel2 = {-1, -1, -1, -1, -1};
    digitos_t parcialesPanel3 = {-1, -1, -1, -1, -1};

    while (1)
    {
        if (xQueueReceive(colaDigitos, &digitosActuales, portMAX_DELAY))
        {
            if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY))
            {

                if (digitosActuales.decenasMinutos != digitosPrevios.decenasMinutos)
                    DibujarDigito(PanelMinutos, 0, digitosActuales.decenasMinutos);
                if (digitosActuales.unidadesMinutos != digitosPrevios.unidadesMinutos)
                    DibujarDigito(PanelMinutos, 1, digitosActuales.unidadesMinutos);
                if (digitosActuales.decenasSegundos != digitosPrevios.decenasSegundos)
                    DibujarDigito(PanelSegundos, 0, digitosActuales.decenasSegundos);
                if (digitosActuales.unidadesSegundos != digitosPrevios.unidadesSegundos)
                    DibujarDigito(PanelSegundos, 1, digitosActuales.unidadesSegundos);
                if (digitosActuales.decimasSegundo != digitosPrevios.decimasSegundo)
                    DibujarDigito(PanelDecimas, 0, digitosActuales.decimasSegundo);

                digitosPrevios = digitosActuales;

                digitos_t nuevoParcial;
                if (xQueueReceive(colaDigitosParciales, &nuevoParcial, 0))
                {
                    parcialesPanel3 = parcialesPanel2;
                    parcialesPanel2 = parcialesPanel1;
                    parcialesPanel1 = nuevoParcial;

                    DibujarDigito(Panelparcial3Minutos, 0, parcialesPanel3.decenasMinutos);
                    DibujarDigito(Panelparcial3Minutos, 1, parcialesPanel3.unidadesMinutos);
                    DibujarDigito(Panelparcial3Segundos, 0, parcialesPanel3.decenasSegundos);
                    DibujarDigito(Panelparcial3Segundos, 1, parcialesPanel3.unidadesSegundos);
                    DibujarDigito(Panelparcial3Decimas, 0, parcialesPanel3.decimasSegundo);

                    DibujarDigito(Panelparcial2Minutos, 0, parcialesPanel2.decenasMinutos);
                    DibujarDigito(Panelparcial2Minutos, 1, parcialesPanel2.unidadesMinutos);
                    DibujarDigito(Panelparcial2Segundos, 0, parcialesPanel2.decenasSegundos);
                    DibujarDigito(Panelparcial2Segundos, 1, parcialesPanel2.unidadesSegundos);
                    DibujarDigito(Panelparcial2Decimas, 0, parcialesPanel2.decimasSegundo);

                    DibujarDigito(Panelparcial1Minutos, 0, parcialesPanel1.decenasMinutos);
                    DibujarDigito(Panelparcial1Minutos, 1, parcialesPanel1.unidadesMinutos);
                    DibujarDigito(Panelparcial1Segundos, 0, parcialesPanel1.decenasSegundos);
                    DibujarDigito(Panelparcial1Segundos, 1, parcialesPanel1.unidadesSegundos);
                    DibujarDigito(Panelparcial1Decimas, 0, parcialesPanel1.decimasSegundo);
                }
                xSemaphoreGive(semaforoAccesoDigitos);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

void app_main()
{
    ConfigurarSalidasLed();
    apagarLeds();

    colaDigitosParciales = xQueueCreate(3, sizeof(digitos_t));
    colaDigitos = xQueueCreate(5, sizeof(digitos_t));
    colaEstadosCronometro = xQueueCreate(5, sizeof(estadosCronometro_t));
    semaforoAccesoDigitos = xSemaphoreCreateMutex();

    xTaskCreate(leerBotones, "LecturaBotonera", 2048, NULL, 1, NULL);
    xTaskCreate(manejoEstadosCronometro, "Tiempo100ms", 2048, NULL, 3, NULL);
    xTaskCreate(actualizarPantalla, "ActualizarPantalla", 4096, NULL, 2, NULL);
    //    xTaskCreate(manejoLedRGB, "LedsTestigos", 4096, NULL, 1, NULL);
}