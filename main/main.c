/*
    if (alarmaConfigurada)
        {
            ILI9341DrawString(5, 147, "A.CON>>", &font_11x18, ILI9341_GREEN, ILI9341_BLACK);
        }

        if (!alarmaConfigurada)
        {
            ILI9341DrawString(5, 147, "A.DES>>", &font_11x18, ILI9341_RED, ILI9341_BLACK);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
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

// COnfig de la alarma por defecto
static int alarmaHoras = 12;
static int alarmaMinutos = 1;
// bool modoAjusteReloj = false;
bool alarmaActivada = false;
bool alarmaConfigurada = false;

//*******************************************FUNCIONES************************************************ */

void inicializarPantallaCronometro()
{
    PanelMinutosCron = CrearPanel(5, 180, 2, 50, 30, ILI9341_BLUE2, 0x3800, ILI9341_BLACK);
    PanelSegundosCron = CrearPanel(80, 180, 2, 50, 30, ILI9341_BLUE2, 0x3800, ILI9341_BLACK);
    PanelDecimasCron = CrearPanel(155, 180, 1, 50, 30, ILI9341_BLUE2, 0x3800, ILI9341_BLACK);

    Panelparcial1Minutos = CrearPanel(195, 120, 2, 30, 20, ILI9341_PURPLE, 0x3800, ILI9341_BLACK);
    Panelparcial1Segundos = CrearPanel(245, 120, 2, 30, 20, ILI9341_PURPLE, 0x3800, ILI9341_BLACK);
    Panelparcial1Decimas = CrearPanel(295, 120, 1, 30, 20, ILI9341_PURPLE, 0x3800, ILI9341_BLACK);

    Panelparcial2Minutos = CrearPanel(195, 160, 2, 30, 20, ILI9341_CYAN, 0x3800, ILI9341_BLACK);
    Panelparcial2Segundos = CrearPanel(245, 160, 2, 30, 20, ILI9341_CYAN, 0x3800, ILI9341_BLACK);
    Panelparcial2Decimas = CrearPanel(295, 160, 1, 30, 20, ILI9341_CYAN, 0x3800, ILI9341_BLACK);

    Panelparcial3Minutos = CrearPanel(195, 200, 2, 30, 20, ILI9341_GREEN, 0x3800, ILI9341_BLACK);
    Panelparcial3Segundos = CrearPanel(245, 200, 2, 30, 20, ILI9341_GREEN, 0x3800, ILI9341_BLACK);
    Panelparcial3Decimas = CrearPanel(295, 200, 1, 30, 20, ILI9341_GREEN, 0x3800, ILI9341_BLACK);

    ILI9341DrawFilledCircle(74, 190, 2, ILI9341_BLUE2);
    ILI9341DrawFilledCircle(74, 221, 2, ILI9341_BLUE2);
    ILI9341DrawFilledCircle(148, 225, 2, ILI9341_BLUE2);

    DibujarDigito(PanelMinutosCron, 0, 0);
    DibujarDigito(PanelMinutosCron, 1, 0);
    DibujarDigito(PanelSegundosCron, 0, 0);
    DibujarDigito(PanelSegundosCron, 1, 0);
    DibujarDigito(PanelDecimasCron, 0, 0);
}

void inicializarPantallaReloj()
{
    PanelHoras = CrearPanel(11, 0, 2, 80, 40, ILI9341_GREENYELLOW, 0x3800, ILI9341_BLACK);
    PanelMinutos = CrearPanel(115, 0, 2, 80, 40, ILI9341_GREENYELLOW, 0x3800, ILI9341_BLACK);
    PanelSegundos = CrearPanel(225, 0, 2, 80, 40, ILI9341_GREENYELLOW, 0x3800, ILI9341_BLACK);

    PanelDia = CrearPanel(5, 100, 2, 30, 20, ILI9341_WHITE, 0x3800, ILI9341_BLACK);
    PanelMes = CrearPanel(55, 100, 2, 30, 20, ILI9341_WHITE, 0x3800, ILI9341_BLACK);
    PanelAnio = CrearPanel(110, 100, 4, 30, 20, ILI9341_WHITE, 0x3800, ILI9341_BLACK);

    PanelHorasAlarma = CrearPanel(90, 140, 2, 30, 20, ILI9341_RED, 0x3800, ILI9341_BLACK);
    PanelMinutosAlarma = CrearPanel(145, 140, 2, 30, 20, ILI9341_RED, 0x3800, ILI9341_BLACK);

    ILI9341DrawCircle(103, 25, 3, ILI9341_GREENYELLOW);
    ILI9341DrawCircle(103, 55, 3, ILI9341_GREENYELLOW);
    ILI9341DrawCircle(210, 25, 3, ILI9341_GREENYELLOW);
    ILI9341DrawCircle(210, 55, 3, ILI9341_GREENYELLOW);

    ILI9341DrawString(97, 105, "-", &font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341DrawString(45, 105, "-", &font_11x18, ILI9341_WHITE, ILI9341_BLACK);

    // ILI9341DrawString(5, 147, "ALARMA", &font_11x18, ILI9341_RED, ILI9341_BLACK);
    ILI9341DrawString(133, 147, ":", &font_11x18, ILI9341_RED, ILI9341_BLACK);
    ILI9341DrawString(195, 90, "<PARCIALES>", &font_11x18, ILI9341_RED, ILI9341_BLACK);
}

//*******************************************FUNCIONES************************************************ */
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

void actualizarCronometro(void *p)
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
        if (alarmaConfigurada)
        {
            ILI9341DrawString(5, 147, "<A.CON>", &font_11x18, ILI9341_GREEN, ILI9341_BLACK);
        }

        if (!alarmaConfigurada)
        {
            ILI9341DrawString(5, 147, "<A.DES>", &font_11x18, ILI9341_RED, ILI9341_BLACK);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}

void verificarAlarma(void *p)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    time_t now;
    struct tm timeinfo;

    while (true)
    {
        time(&now);
        localtime_r(&now, &timeinfo);

        if ((alarmaConfigurada) && (timeinfo.tm_hour == alarmaHoras && timeinfo.tm_min == alarmaMinutos && timeinfo.tm_sec == 0))
        {
            ESP_LOGI("ALARMA", "❗❗❗ ALARMA ACTIVADA ❗❗❗ Son las %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            alarmaActivada = true;
            PrenderLedAzul(true);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void leerBotonesCronometro(void *p)
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
            }
            if ((xTaskGetTickCount() - tiempoPresionadoReiniciar) > pdMS_TO_TICKS(2000))
            {
                estadosCronometro_t evento = REINICIARTODO;
                xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
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
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void leerBotonesReloj(void *p)
{
    struct tm timeinfo;
    time_t t;

    bool modoAjusteReloj = false;
    bool modoAjusteAlarma = false;
    int modoConfig = -1;
    TickType_t tiempoInicioPresionado = 0;

    while (1)
    {
        if (!gpio_get_level(TEC4_Config))
        {
            if (tiempoInicioPresionado == 0)
            {
                tiempoInicioPresionado = xTaskGetTickCount();
            }
            else if ((xTaskGetTickCount() - tiempoInicioPresionado) >= pdMS_TO_TICKS(2000))
            {
                modoAjusteReloj = false;
                modoAjusteAlarma = false;
                ESP_LOGI("CONFIG", "✅ ✅ ✅ Alarma configurada ✅ ✅ ✅ para las-> %02d:%02d", alarmaHoras, alarmaMinutos);
                tiempoInicioPresionado = 0;
                alarmaConfigurada = true;
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
        else
        {
            if (tiempoInicioPresionado != 0)
            {
                if (alarmaActivada)
                {
                    alarmaActivada = false;
                    ESP_LOGI("CONFIG", "❌ ❌ ❌ Alarma desactivada por teclado ❌ ❌ ❌");
                    alarmaConfigurada = false;
                    apagarLeds();
                }
                modoConfig = (modoConfig + 1) % 7;
                modoAjusteReloj = (modoConfig < 5);
                modoAjusteAlarma = (modoConfig >= 5);

                ESP_LOGI("AJUSTE", "Se ajusta: %s",
                         (modoConfig == 0) ? "Minutos" : (modoConfig == 1) ? "Horas"
                                                     : (modoConfig == 2)   ? "Día"
                                                     : (modoConfig == 3)   ? "Mes"
                                                     : (modoConfig == 4)   ? "Año"
                                                     : (modoConfig == 5)   ? "Minutos de la Alarma"
                                                                           : "Horas de la Alarma");

                tiempoInicioPresionado = 0;
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }

        if (modoAjusteReloj || modoAjusteAlarma)
        {
            if (!gpio_get_level(TEC5_Inc))
            {
                time(&t);
                localtime_r(&t, &timeinfo);

                if (modoAjusteReloj)
                {
                    switch (modoConfig)
                    {
                    case 0:
                        timeinfo.tm_min = (timeinfo.tm_min + 1) % 60;
                        break;
                    case 1:
                        timeinfo.tm_hour = (timeinfo.tm_hour + 1) % 24;
                        break;
                    case 2:
                        timeinfo.tm_mday = (timeinfo.tm_mday % 31) + 1;
                        break;
                    case 3:
                        timeinfo.tm_mon = (timeinfo.tm_mon + 1) % 12;
                        break;
                    case 4:
                        timeinfo.tm_year = timeinfo.tm_year + 1;
                        break;
                    }
                    t = mktime(&timeinfo);
                    struct timeval now = {.tv_sec = t};
                    settimeofday(&now, NULL);

                    ESP_LOGI("CONFIG", "Hora %02d, Minuto %02d, fecha %02d/%02d/%04d, alarma %02d:%02d",
                             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_mday, timeinfo.tm_mon + 1,
                             timeinfo.tm_year + 1900, alarmaHoras, alarmaMinutos);
                }
                else
                {
                    if (modoConfig == 5)
                        alarmaMinutos = (alarmaMinutos + 1) % 60;
                    if (modoConfig == 6)
                        alarmaHoras = (alarmaHoras + 1) % 24;

                    ESP_LOGI("CONFIG", "Hora %02d, Minuto %02d, fecha %02d/%02d/%04d, alarma %02d:%02d",
                             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_mday, timeinfo.tm_mon + 1,
                             timeinfo.tm_year + 1900, alarmaHoras, alarmaMinutos);
                }

                vTaskDelay(pdMS_TO_TICKS(300));
            }

            if (!gpio_get_level(TEC6_Dec))
            {
                time(&t);
                localtime_r(&t, &timeinfo);

                if (modoAjusteReloj)
                {
                    switch (modoConfig)
                    {
                    case 0:
                        timeinfo.tm_min = (timeinfo.tm_min == 0 ? 59 : timeinfo.tm_min - 1);
                        break;
                    case 1:
                        timeinfo.tm_hour = (timeinfo.tm_hour == 0 ? 23 : timeinfo.tm_hour - 1);
                        break;
                    case 2:
                        timeinfo.tm_mday = (timeinfo.tm_mday == 1 ? 31 : timeinfo.tm_mday - 1);
                        break;
                    case 3:
                        timeinfo.tm_mon = (timeinfo.tm_mon == 0 ? 11 : timeinfo.tm_mon - 1);
                        break;
                    case 4:
                        timeinfo.tm_year = timeinfo.tm_year - 1;
                        break;
                    }
                    t = mktime(&timeinfo);
                    struct timeval now = {.tv_sec = t};
                    settimeofday(&now, NULL);

                    ESP_LOGI("CONFIG", "Hora %02d, Minuto %02d, fecha %02d/%02d/%04d, alarma %02d:%02d",
                             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_mday, timeinfo.tm_mon + 1,
                             timeinfo.tm_year + 1900, alarmaHoras, alarmaMinutos);
                }
                else
                {
                    if (modoConfig == 5)
                        alarmaMinutos = (alarmaMinutos == 0 ? 59 : alarmaMinutos - 1);
                    if (modoConfig == 6)
                        alarmaHoras = (alarmaHoras == 0 ? 23 : alarmaHoras - 1);

                    ESP_LOGI("CONFIG", "Hora %02d, Minuto %02d, fecha %02d/%02d/%04d, alarma %02d:%02d",
                             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_mday, timeinfo.tm_mon + 1,
                             timeinfo.tm_year + 1900, alarmaHoras, alarmaMinutos);
                }

                vTaskDelay(pdMS_TO_TICKS(300));
            }
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
        {TEC3_Parcial, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {TEC4_Config, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {TEC5_Inc, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY},
        {TEC6_Dec, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY}};

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

    xTaskCreate(leerBotonesCronometro, "Control Cronometro", 2048, NULL, 4, NULL);
    xTaskCreate(manejoEstadosCronometro, "Tiempo 100ms", 2048, NULL, 5, NULL);
    xTaskCreate(actualizarCronometro, "Informacion Cronometro Parciales", 2048, NULL, 2, NULL);
    xTaskCreate(actualizarPantallaReloj, "Informacion Fecha Hora Alarma", 2048, NULL, 1, NULL);
    xTaskCreate(leerBotonesReloj, "Ajustes reloj alarma", 2048, NULL, 3, NULL);
    xTaskCreate(verificarAlarma, "Verificar si se debe disparar Alarma", 2048, NULL, 1, NULL);
}