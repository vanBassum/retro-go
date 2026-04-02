// 8080 parallel display driver using ESP32-S3 LCD_CAM peripheral
// Designed for ST7796 and similar controllers on an 8-bit parallel bus

#include <esp_lcd_panel_io.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static esp_lcd_i80_bus_handle_t i80_bus;
static esp_lcd_panel_io_handle_t panel_io;
static QueueHandle_t i80_buffers;
static QueueHandle_t i80_inflight;
static bool mem_write_first;

#define I80_BUFFER_COUNT  (5)
#define I80_BUFFER_LENGTH (LCD_BUFFER_LENGTH * 2) // In bytes

// Command macro - compatible with RG_SCREEN_INIT() sequences
#define ILI9341_CMD(cmd, data...)                                                           \
    {                                                                                       \
        const uint8_t x[] = {data};                                                         \
        esp_lcd_panel_io_tx_param(panel_io, cmd, sizeof(x) ? x : NULL, sizeof(x));          \
    }

static inline uint16_t *i80_take_buffer(void)
{
    uint16_t *buffer;
    if (xQueueReceive(i80_buffers, &buffer, pdMS_TO_TICKS(2500)) != pdTRUE)
        RG_PANIC("display");
    return buffer;
}

IRAM_ATTR
static bool i80_color_trans_done_cb(esp_lcd_panel_io_handle_t io,
    esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    uint16_t *buf;
    BaseType_t high_task_wakeup = pdFALSE;
    if (xQueueReceiveFromISR(i80_inflight, &buf, &high_task_wakeup) == pdTRUE)
        xQueueSendFromISR(i80_buffers, &buf, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

static void i80_init(void)
{
    i80_buffers = xQueueCreate(I80_BUFFER_COUNT, sizeof(uint16_t *));
    i80_inflight = xQueueCreate(I80_BUFFER_COUNT, sizeof(uint16_t *));

    while (uxQueueSpacesAvailable(i80_buffers))
    {
        void *buffer = rg_alloc(I80_BUFFER_LENGTH, MEM_DMA);
        xQueueSend(i80_buffers, &buffer, portMAX_DELAY);
    }

    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = RG_GPIO_LCD_DC,
        .wr_gpio_num = RG_GPIO_LCD_WR,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {
            RG_GPIO_LCD_D0, RG_GPIO_LCD_D1, RG_GPIO_LCD_D2, RG_GPIO_LCD_D3,
            RG_GPIO_LCD_D4, RG_GPIO_LCD_D5, RG_GPIO_LCD_D6, RG_GPIO_LCD_D7,
        },
        .bus_width = 8,
        .max_transfer_bytes = I80_BUFFER_LENGTH,
        .psram_trans_align = 64,
        .sram_trans_align = 4,
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = RG_GPIO_LCD_CS,
        .pclk_hz = RG_SCREEN_SPEED,
        .trans_queue_depth = I80_BUFFER_COUNT,
        .dc_levels = {
            .dc_cmd_level = 0,
            .dc_data_level = 1,
            .dc_dummy_level = 0,
            .dc_idle_level = 0,
        },
        .flags = {
            .swap_color_bytes = false,
        },
        .on_color_trans_done = i80_color_trans_done_cb,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &panel_io));
}

static void i80_deinit(void)
{
    esp_lcd_panel_io_del(panel_io);
    esp_lcd_del_i80_bus(i80_bus);
}

static void lcd_set_backlight(float percent)
{
    float level = RG_MIN(RG_MAX(percent / 100.f, 0), 1.f);
    int error_code = 0;

#if defined(RG_GPIO_LCD_BCKL)
    error_code = ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0x1FFF * level, 50, 0);
#endif

    if (error_code)
        RG_LOGE("failed setting backlight to %d%% (0x%02X)\n", (int)(100 * level), error_code);
    else
        RG_LOGI("backlight set to %d%%\n", (int)(100 * level));
}

static void lcd_set_window(int left, int top, int width, int height)
{
    int right = left + width - 1;
    int bottom = top + height - 1;

    if (left < 0 || top < 0 || right >= display.screen.real_width || bottom >= display.screen.real_height)
        RG_LOGW("Bad lcd window (x0=%d, y0=%d, x1=%d, y1=%d)\n", left, top, right, bottom);

    ILI9341_CMD(0x2A, left >> 8, left & 0xff, right >> 8, right & 0xff); // Column address
    ILI9341_CMD(0x2B, top >> 8, top & 0xff, bottom >> 8, bottom & 0xff); // Page address
    mem_write_first = true;
}

static inline uint16_t *lcd_get_buffer(size_t length)
{
    return i80_take_buffer();
}

static inline void lcd_send_buffer(uint16_t *buffer, size_t length)
{
    if (length > 0)
    {
        // First send after set_window uses 0x2C (Memory Write), subsequent use 0x3C (Memory Write Continue)
        int cmd = mem_write_first ? 0x2C : 0x3C;
        mem_write_first = false;
        xQueueSend(i80_inflight, &buffer, portMAX_DELAY);
        esp_lcd_panel_io_tx_color(panel_io, cmd, buffer, length * sizeof(*buffer));
    }
    else
    {
        // Return unused buffer
        xQueueSend(i80_buffers, &buffer, portMAX_DELAY);
    }
}

static void lcd_sync(void)
{
    // Not needed - the buffer queue provides natural flow control
}

static void lcd_init(void)
{
#ifdef RG_GPIO_LCD_BCKL
    // Initialize backlight at 0% to avoid the lcd reset flash
    ledc_timer_config(&(ledc_timer_config_t){
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
    });
    ledc_channel_config(&(ledc_channel_config_t){
        .gpio_num = RG_GPIO_LCD_BCKL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    #ifdef RG_GPIO_LCD_BCKL_INVERT
        .flags.output_invert = 1,
    #endif
    });
    ledc_fade_func_install(0);
#endif

    i80_init();

#if defined(RG_GPIO_LCD_RST)
    gpio_set_direction(RG_GPIO_LCD_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(RG_GPIO_LCD_RST, 0);
    rg_usleep(100 * 1000);
    gpio_set_level(RG_GPIO_LCD_RST, 1);
    rg_usleep(10 * 1000);
#endif

    ILI9341_CMD(0x01);          // Software Reset
    rg_usleep(5 * 1000);
    ILI9341_CMD(0x3A, 0x55);    // COLMOD: RGB565

#if defined(RG_SCREEN_ROTATION) && defined(RG_SCREEN_RGB_BGR)
    ILI9341_CMD(0x36, (RG_SCREEN_RGB_BGR ? 0x08 : 0x00) | (RG_SCREEN_ROTATION << 5)); // MADCTL
#endif

#ifdef RG_SCREEN_INIT
    RG_SCREEN_INIT();
#else
    #warning "LCD init sequence is not defined for this device!"
#endif

    ILI9341_CMD(0x11);           // Exit Sleep
    rg_usleep(120 * 1000);       // ST7796 needs 120ms after sleep out
    ILI9341_CMD(0x29);           // Display ON
}

static void lcd_deinit(void)
{
#ifdef RG_SCREEN_DEINIT
    RG_SCREEN_DEINIT();
#endif
    i80_deinit();
}

const rg_display_driver_t rg_display_driver_st7796_i80 = {
    .name = "st7796_i80",
};
