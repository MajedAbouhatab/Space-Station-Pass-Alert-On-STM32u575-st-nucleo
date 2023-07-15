#include <board.h>
#include <stdio.h>
#include <webclient.h>
#include <u8g2_port.h>
#include <Logo.h>

#define BUZZER_P    GET_PIN(E, 9)
#define BUZZER_M    GET_PIN(E, 13)

void beep(void)
{
    for (int i = 0; i < 100; i++)
    {
        rt_pin_write(BUZZER_P, !rt_pin_read(BUZZER_P));
        rt_pin_write(BUZZER_M, !rt_pin_read(BUZZER_P));
        rt_thread_mdelay(1);
    }
    rt_thread_mdelay(50);
}

float radians(float d)
{
    return d * 3.14159265359 / 180.0;
}

// Get distance between two coordinates in km
int haversine(float lat1, float lon1, float lat2, float lon2)
{
    float dLat = radians(lat2 - lat1);
    float dLon = radians(lon2 - lon1);
    float a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
    return (int) (6371 * 2 * atan2(sqrt(a), sqrt(1 - a)));
}

// Get a value from a key in response
float K2V(char* r, char* k, char*d)
{
    char* response2 = calloc(strlen(r) + 1, sizeof(char));
    strcpy(response2, r);
    char* token = (char *) strtok(response2, d);
    while (token != NULL)
    {
        if (!strcmp(token, k))
            return atof((char *) strtok(NULL, d));
        token = (char *) strtok(NULL, d);
    }
    free(response2);
}

int main(void)
{
    // Buzzer setup
    rt_pin_mode(BUZZER_P, PIN_MODE_OUTPUT);
    rt_pin_mode(BUZZER_M, PIN_MODE_OUTPUT);

    // OLED setup
    u8g2_t u8g2;
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_rtthread);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_I2C_CLOCK, GET_PIN(B, 8));
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_I2C_DATA, GET_PIN(B, 9));
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);

    // Display logo
    u8g2_ClearDisplay(&u8g2);
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawBitmap(&u8g2, 0, 0, 16, 64, Logo_bmp);
    u8g2_SendBuffer(&u8g2);
    for (int i = 0; i < 3; i++) beep();

    // Connect to Wi-Fi
    if (rt_wlan_connect("SSID", "Password"))
    {
        // [I/WLAN.mgnt] wifi connect failed!
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 10, 20, "Wi-Fi");
        u8g2_DrawStr(&u8g2, 10, 42, "connect");
        u8g2_DrawStr(&u8g2, 10, 64, "failed!");
        u8g2_SendBuffer(&u8g2);
        while (RT_TRUE) beep();
    }

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 10, 20, "Connecting");
    u8g2_DrawStr(&u8g2, 10, 42, "to");
    u8g2_DrawStr(&u8g2, 10, 64, "Wi-Fi");
    u8g2_SendBuffer(&u8g2);
    beep();

    float Lat1, Lon1, Lat2, Lon2;
    char *response = RT_NULL;
    size_t resp_len = 0;

    // Get local coordinates, restart if unsuccessful
    if (!webclient_request("http://ip-api.com/json/?fields=lat,lon", RT_NULL, RT_NULL, 0, (void **) &response, &resp_len)) NVIC_SystemReset;
    Lat1 = K2V(response, "lat", "\":\"");
    Lon1 = K2V(response, "lon", "\":\"");
    web_free(response);

    while (RT_TRUE)
    {
        // Get ISS coordinates, restart if unsuccessful
        if (!webclient_request("http://api.open-notify.org/iss-now.json", RT_NULL, RT_NULL, 0, (void **) &response, &resp_len)) NVIC_SystemReset;
        Lat2 = K2V(response, "latitude", "\": \"");
        Lon2 = K2V(response, "longitude", "\": \"");
        web_free(response);

        // It's getting close
        if (haversine(Lat1, Lon1, Lat2, Lon2) < 1500) beep();

        // Update display
        u8g2_ClearBuffer(&u8g2);
        char buffer[9];
        snprintf(buffer, 9, "%d", haversine(Lat1, Lon1, Lat2, Lon2));
        u8g2_DrawStr(&u8g2, 10, 20, "ISS");
        u8g2_DrawStr(&u8g2, 10, 42, buffer);
        u8g2_DrawStr(&u8g2, 10, 64, "Km");
        u8g2_SendBuffer(&u8g2);
        rt_thread_mdelay(3000);
    }
}
