/*

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
#include <time.h>
#include "esp_system.h"
#include <sys/time.h>

#define DIGITO_ANCHO 40
#define DIGITO_ALTO 80
#define DIGITO_ENCENDIDO ILI9341_BLUE2
#define DIGITO_APAGADO 0x3800
#define DIGITO_FONDO ILI9341_BLACK

QueueHandle_t colaDigitos;
QueueHandle_t colaEstadosCronometro;
QueueHandle_t colaDigitosParciales;
SemaphoreHandle_t semaforoAccesoDigitos;

bool flagPausa = true;
bool flagParcial = false;

panel_t PanelMinutosCron, PanelSegundosCron, PanelDecimasCron;
panel_t Panelparcial1Minutos, Panelparcial1Segundos, Panelparcial1Decimas;
panel_t Panelparcial2Minutos, Panelparcial2Segundos, Panelparcial2Decimas;
panel_t Panelparcial3Minutos, Panelparcial3Segundos, Panelparcial3Decimas;
panel_t PanelHoras, PanelMinutos, PanelSegundos;
panel_t PanelDia, PanelMes, PanelAnio;
panel_t PanelHorasAlarma, PanelMinutosAlarma;

static int alarmaHoras = 12;
static int alarmaMinutos = 1;

/******************** FUNCIONES ***************************************/
void inicializarPantallaCronometro()
{
    PanelMinutosCron = CrearPanel(5, 180, 2, 50, 30, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    PanelSegundosCron = CrearPanel(80, 180, 2, 50, 30, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    PanelDecimasCron = CrearPanel(155, 180, 1, 50, 30, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    Panelparcial1Minutos = CrearPanel(195, 120, 2, 30, 20, ILI9341_PURPLE, DIGITO_APAGADO, DIGITO_FONDO);
    Panelparcial1Segundos = CrearPanel(245, 120, 2, 30, 20, ILI9341_PURPLE, DIGITO_APAGADO, DIGITO_FONDO);
    Panelparcial1Decimas = CrearPanel(295, 120, 1, 30, 20, ILI9341_PURPLE, DIGITO_APAGADO, DIGITO_FONDO);

    Panelparcial2Minutos = CrearPanel(195, 160, 2, 30, 20, ILI9341_CYAN, DIGITO_APAGADO, DIGITO_FONDO);
    Panelparcial2Segundos = CrearPanel(245, 160, 2, 30, 20, ILI9341_CYAN, DIGITO_APAGADO, DIGITO_FONDO);
    Panelparcial2Decimas = CrearPanel(295, 160, 1, 30, 20, ILI9341_CYAN, DIGITO_APAGADO, DIGITO_FONDO);

    Panelparcial3Minutos = CrearPanel(195, 200, 2, 30, 20, ILI9341_GREEN, DIGITO_APAGADO, DIGITO_FONDO);
    Panelparcial3Segundos = CrearPanel(245, 200, 2, 30, 20, ILI9341_GREEN, DIGITO_APAGADO, DIGITO_FONDO);
    Panelparcial3Decimas = CrearPanel(295, 200, 1, 30, 20, ILI9341_GREEN, DIGITO_APAGADO, DIGITO_FONDO);

    ILI9341DrawFilledCircle(74, 190, 2, DIGITO_ENCENDIDO);
    ILI9341DrawFilledCircle(74, 221, 2, DIGITO_ENCENDIDO);
    ILI9341DrawFilledCircle(148, 225, 2, DIGITO_ENCENDIDO);

    DibujarDigito(PanelMinutosCron, 0, 0);
    DibujarDigito(PanelMinutosCron, 1, 0);
    DibujarDigito(PanelSegundosCron, 0, 0);
    DibujarDigito(PanelSegundosCron, 1, 0);
    DibujarDigito(PanelDecimasCron, 0, 0);
}
void inicializarPantallaReloj()
{
    PanelHoras = CrearPanel(11, 0, 2, DIGITO_ALTO, DIGITO_ANCHO, ILI9341_GREENYELLOW, DIGITO_APAGADO, DIGITO_FONDO);
    PanelMinutos = CrearPanel(115, 0, 2, DIGITO_ALTO, DIGITO_ANCHO, ILI9341_GREENYELLOW, DIGITO_APAGADO, DIGITO_FONDO);
    PanelSegundos = CrearPanel(225, 0, 2, DIGITO_ALTO, DIGITO_ANCHO, ILI9341_GREENYELLOW, DIGITO_APAGADO, DIGITO_FONDO);

    PanelDia = CrearPanel(5, 100, 2, 30, 20, ILI9341_WHITE, DIGITO_APAGADO, DIGITO_FONDO);
    PanelMes = CrearPanel(55, 100, 2, 30, 20, ILI9341_WHITE, DIGITO_APAGADO, DIGITO_FONDO);
    PanelAnio = CrearPanel(110, 100, 4, 30, 20, ILI9341_WHITE, DIGITO_APAGADO, DIGITO_FONDO);

    PanelHorasAlarma = CrearPanel(90, 140, 2, 30, 20, ILI9341_RED, DIGITO_APAGADO, DIGITO_FONDO);
    PanelMinutosAlarma = CrearPanel(145, 140, 2, 30, 20, ILI9341_RED, DIGITO_APAGADO, DIGITO_FONDO);

    ILI9341DrawCircle(103, 25, 3, ILI9341_GREENYELLOW);
    ILI9341DrawCircle(103, 55, 3, ILI9341_GREENYELLOW);
    ILI9341DrawCircle(210, 25, 3, ILI9341_GREENYELLOW);
    ILI9341DrawCircle(210, 55, 3, ILI9341_GREENYELLOW);

    ILI9341DrawString(97, 105, "-", &font_11x18, ILI9341_WHITE, DIGITO_FONDO);
    ILI9341DrawString(45, 105, "-", &font_11x18, ILI9341_WHITE, DIGITO_FONDO);

    ILI9341DrawString(5, 147, "ALARMA", &font_11x18, ILI9341_RED, DIGITO_FONDO);
    ILI9341DrawString(133, 147, ":", &font_11x18, ILI9341_RED, DIGITO_FONDO);
    ILI9341DrawString(195, 90, "PARCIALES", &font_11x18, ILI9341_RED, DIGITO_FONDO);
}

/******************** FUNCIONES ***************************************/

typedef enum
{
    PAUSAR,
    REINICIAR,
    REINICIARTODO,
    PARCIAL
} estadosCronometro_t;

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

void actualizarPantallaCronometro(void *p)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    inicializarPantallaCronometro();

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
                    DibujarDigito(PanelMinutosCron, 0, digitosActuales.decenasMinutos);
                if (digitosActuales.unidadesMinutos != digitosPrevios.unidadesMinutos)
                    DibujarDigito(PanelMinutosCron, 1, digitosActuales.unidadesMinutos);
                if (digitosActuales.decenasSegundos != digitosPrevios.decenasSegundos)
                    DibujarDigito(PanelSegundosCron, 0, digitosActuales.decenasSegundos);
                if (digitosActuales.unidadesSegundos != digitosPrevios.unidadesSegundos)
                    DibujarDigito(PanelSegundosCron, 1, digitosActuales.unidadesSegundos);
                if (digitosActuales.decimasSegundo != digitosPrevios.decimasSegundo)
                    DibujarDigito(PanelDecimasCron, 0, digitosActuales.decimasSegundo);

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

void actualizarPantallaReloj(void *p)
{
    inicializarPantallaReloj();
    TickType_t xLastWakeTime = xTaskGetTickCount();

    struct tm timeinfo, timeinfo_prev;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    timeinfo_prev = timeinfo;

    DibujarDigito(PanelHoras, 0, timeinfo.tm_hour / 10);
    DibujarDigito(PanelHoras, 1, timeinfo.tm_hour % 10);
    DibujarDigito(PanelMinutos, 0, timeinfo.tm_min / 10);
    DibujarDigito(PanelMinutos, 1, timeinfo.tm_min % 10);
    DibujarDigito(PanelSegundos, 0, timeinfo.tm_sec / 10);
    DibujarDigito(PanelSegundos, 1, timeinfo.tm_sec % 10);
    DibujarDigito(PanelDia, 0, timeinfo.tm_mday / 10);
    DibujarDigito(PanelDia, 1, timeinfo.tm_mday % 10);
    DibujarDigito(PanelMes, 0, (timeinfo.tm_mon + 1) / 10);
    DibujarDigito(PanelMes, 1, (timeinfo.tm_mon + 1) % 10);

    int year = timeinfo.tm_year + 1900;
    int year_prev = year;

    DibujarDigito(PanelAnio, 0, (year / 1000) % 10);
    DibujarDigito(PanelAnio, 1, (year / 100) % 10);
    DibujarDigito(PanelAnio, 2, (year / 10) % 10);
    DibujarDigito(PanelAnio, 3, year % 10);

    int horasAlarmaAnterior = alarmaHoras;
    int minutosAlarmaAnterior = alarmaMinutos;

    DibujarDigito(PanelMinutosAlarma, 0, (alarmaMinutos / 10) % 10);
    DibujarDigito(PanelMinutosAlarma, 1, alarmaMinutos % 10);

    DibujarDigito(PanelHorasAlarma, 0, (alarmaHoras / 10) % 10);
    DibujarDigito(PanelHorasAlarma, 1, alarmaHoras % 10);

    while (1)
    {
        time(&now);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_hour != timeinfo_prev.tm_hour)
        {
            DibujarDigito(PanelHoras, 0, timeinfo.tm_hour / 10);
            DibujarDigito(PanelHoras, 1, timeinfo.tm_hour % 10);
        }
        if (timeinfo.tm_min != timeinfo_prev.tm_min)
        {
            DibujarDigito(PanelMinutos, 0, timeinfo.tm_min / 10);
            DibujarDigito(PanelMinutos, 1, timeinfo.tm_min % 10);
        }
        if (timeinfo.tm_sec != timeinfo_prev.tm_sec)
        {
            DibujarDigito(PanelSegundos, 0, timeinfo.tm_sec / 10);
            DibujarDigito(PanelSegundos, 1, timeinfo.tm_sec % 10);
        }
        if (timeinfo.tm_mday != timeinfo_prev.tm_mday)
        {
            DibujarDigito(PanelDia, 0, timeinfo.tm_mday / 10);
            DibujarDigito(PanelDia, 1, timeinfo.tm_mday % 10);
        }
        if (timeinfo.tm_mon != timeinfo_prev.tm_mon)
        {
            DibujarDigito(PanelMes, 0, (timeinfo.tm_mon + 1) / 10);
            DibujarDigito(PanelMes, 1, (timeinfo.tm_mon + 1) % 10);
        }
        year = timeinfo.tm_year + 1900;
        if (year != year_prev)
        {
            DibujarDigito(PanelAnio, 0, (year / 1000) % 10);
            DibujarDigito(PanelAnio, 1, (year / 100) % 10);
            DibujarDigito(PanelAnio, 2, (year / 10) % 10);
            DibujarDigito(PanelAnio, 3, year % 10);
            year_prev = year;
        }
        timeinfo_prev = timeinfo;
        if (alarmaHoras != horasAlarmaAnterior)
        {
            DibujarDigito(PanelHorasAlarma, 0, (alarmaHoras / 10) % 10);
            DibujarDigito(PanelHorasAlarma, 1, alarmaHoras % 10);
            horasAlarmaAnterior = alarmaHoras;
        }
        if (alarmaMinutos != minutosAlarmaAnterior)
        {
            DibujarDigito(PanelMinutosAlarma, 0, (alarmaMinutos / 10) % 10);
            DibujarDigito(PanelMinutosAlarma, 1, alarmaMinutos % 10);
            minutosAlarmaAnterior = alarmaMinutos;
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}

void leerBotones(void *p)
{

    bool estadoanteriorPausa = true;
    bool estadoanteriorReiniciar, presioneReiniciar = true;
    bool estadoanteriorTiempoParcial = true;

    TickType_t tiempoPresionadoPausa = 0;
    TickType_t tiempoPresionadoReiniciar = 0;
    TickType_t tiempoPresionadoParcial = 0;

    while (1)
    {
        if ((gpio_get_level(TEC1_Pausa) == 0) && (estadoanteriorPausa))
        {
            tiempoPresionadoPausa = xTaskGetTickCount();
            estadoanteriorPausa = false;
            while (gpio_get_level(TEC1_Pausa) == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            estadoanteriorPausa = true;

            estadosCronometro_t evento = PAUSAR;
            xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);

            ESP_LOGI("BOTON", "TEC1_Pausa");
            ESP_LOGI("TIEMPO", "%lu", (xTaskGetTickCount() - tiempoPresionadoPausa) * portTICK_PERIOD_MS);
            ESP_LOGI("INFO", "PAUSAR");
            ESP_LOGI("", "************************************");
        }

        if ((gpio_get_level(TEC2_Reiniciar) == 0) && (estadoanteriorReiniciar))
        {
            tiempoPresionadoReiniciar = xTaskGetTickCount();
            estadoanteriorReiniciar = false;

            while (gpio_get_level(TEC2_Reiniciar) == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            estadoanteriorReiniciar = true;

            if (presioneReiniciar && (xTaskGetTickCount() - tiempoPresionadoReiniciar) < pdMS_TO_TICKS(2000))
            {
                estadosCronometro_t evento = REINICIAR;
                xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);

                ESP_LOGI("BOTON", "TEC2_Reiniciar");
                ESP_LOGI("TIEMPO", "%lu", (xTaskGetTickCount() - tiempoPresionadoReiniciar) * portTICK_PERIOD_MS);
                ESP_LOGI("INFO", "REINICIE SOLO CRONOMETRO");
                ESP_LOGI("", "************************************");
            }
            if ((xTaskGetTickCount() - tiempoPresionadoReiniciar) > pdMS_TO_TICKS(2000))
            {
                estadosCronometro_t evento = REINICIARTODO;
                xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);

                ESP_LOGI("BOTON", "TEC2_Reiniciar");
                ESP_LOGI("TIEMPO", "%lu", (xTaskGetTickCount() - tiempoPresionadoReiniciar) * portTICK_PERIOD_MS);
                ESP_LOGI("INFO", "REINICIE PARCIALES Y CRONOMETRO");
                ESP_LOGI("", "************************************");
            }
        }
        if ((gpio_get_level(TEC3_Parcial) == 0) && (estadoanteriorTiempoParcial))
        {
            tiempoPresionadoParcial = xTaskGetTickCount();
            estadoanteriorTiempoParcial = false;

            while (gpio_get_level(TEC3_Parcial) == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            estadoanteriorTiempoParcial = true;
            estadosCronometro_t evento = PARCIAL;
            xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
            ESP_LOGI("BOTON", "TEC3_Parcial");
            ESP_LOGI("TIEMPO", "%lu", (xTaskGetTickCount() - tiempoPresionadoParcial) * portTICK_PERIOD_MS);
            ESP_LOGI("INFO", "CAPTURE PARCIAL");
            ESP_LOGI("", "************************************");
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main()
{
    ConfigurarSalidasLed();
    apagarLeds();
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    colaDigitosParciales = xQueueCreate(3, sizeof(digitos_t));
    colaDigitos = xQueueCreate(5, sizeof(digitos_t));
    colaEstadosCronometro = xQueueCreate(5, sizeof(estadosCronometro_t));
    semaforoAccesoDigitos = xSemaphoreCreateMutex();

    configuracion_pin_t configuraciones[] = {
        {TEC1_Pausa, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {TEC2_Reiniciar, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {TEC3_Parcial, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY}};

    ConfigurarTeclas(configuraciones, sizeof(configuraciones) / sizeof(configuraciones[0]));
    struct tm timeinfo = {
        .tm_year = 2025 - 1900,
        .tm_mon = 0,
        .tm_mday = 1,
        .tm_hour = 12,
        .tm_min = 0,
        .tm_sec = 0};

    time_t t = mktime(&timeinfo);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);

    xTaskCreate(leerBotones, "LecturaBotonera", 2048, NULL, 3, NULL);
    xTaskCreate(manejoEstadosCronometro, "Tiempo100ms", 2048, NULL, 4, NULL);
    xTaskCreate(actualizarPantallaCronometro, "ActualizarPantalla", 4096, NULL, 2, NULL);
    xTaskCreate(actualizarPantallaReloj, "ActualizarPantallaReloj", 4096, NULL, 1, NULL);
    //    xTaskCreate(manejoLedRGB, "LedsTestigos", 4096, NULL, 1, NULL);
}