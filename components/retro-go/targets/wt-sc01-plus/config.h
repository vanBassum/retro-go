// Target definition
#define RG_TARGET_NAME             "WT-SC01-PLUS"

// Storage
// The WT-SC01 Plus has no built-in SD card slot. Connect one via the extension header,
// or use flash storage by uncommenting RG_STORAGE_FLASH_PARTITION below.
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI2_HOST
#define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_ROOT             "/flash"
// #define RG_STORAGE_FLASH_PARTITION  "vfs"

// Audio
#define RG_AUDIO_USE_INT_DAC        0   // 0 = Disable, 1 = GPIO25, 2 = GPIO26, 3 = Both
#define RG_AUDIO_USE_EXT_DAC        1   // 0 = Disable, 1 = Enable

// Video
#define RG_SCREEN_DRIVER            2   // 2 = 8080 parallel bus (ST7796/ILI9341)
#define RG_SCREEN_SPEED             20000000
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             480
#define RG_SCREEN_HEIGHT            320
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
#define RG_SCREEN_INIT()                                                                         \
    ILI9341_CMD(0xF0, 0xC3);                /* Enable extension command 2 part I */              \
    ILI9341_CMD(0xF0, 0x96);                /* Enable extension command 2 part II */             \
    ILI9341_CMD(0x36, 0x28);                /* Memory Access Control: MV|BGR (landscape) */     \
    ILI9341_CMD(0xB4, 0x01);                /* Display Inversion Control: 1-dot */               \
    ILI9341_CMD(0xB6, 0x80, 0x02, 0x3B);    /* Display Function Control */                      \
    ILI9341_CMD(0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33); /* Display Output Ctrl */ \
    ILI9341_CMD(0xC1, 0x06);                /* Power Control 2 */                                \
    ILI9341_CMD(0xC2, 0xA7);                /* Power Control 3 */                                \
    ILI9341_CMD(0xC5, 0x18);                /* VCOM Control */                                   \
    ILI9341_CMD(0xE0, 0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B); \
    ILI9341_CMD(0xE1, 0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B); \
    ILI9341_CMD(0xF0, 0x3C);                /* Disable extension command 2 part I */             \
    ILI9341_CMD(0xF0, 0x69);                /* Disable extension command 2 part II */


// Input
// The WT-SC01 Plus has no physical buttons (GPIO0/BOOT is used by LCD DC).
// Connect external buttons via the extension header and adjust GPIOs below.
// Refer to rg_input.h to see all available RG_KEY_* and RG_GAMEPAD_*_MAP types
//
// Example with buttons wired to extension header:
//
// #define RG_GAMEPAD_GPIO_MAP {
//     {RG_KEY_UP,     .num = GPIO_NUM_10, .pullup = 1, .level = 0},
//     {RG_KEY_DOWN,   .num = GPIO_NUM_11, .pullup = 1, .level = 0},
//     {RG_KEY_LEFT,   .num = GPIO_NUM_12, .pullup = 1, .level = 0},
//     {RG_KEY_RIGHT,  .num = GPIO_NUM_13, .pullup = 1, .level = 0},
//     {RG_KEY_A,      .num = GPIO_NUM_14, .pullup = 1, .level = 0},
//     {RG_KEY_B,      .num = GPIO_NUM_21, .pullup = 1, .level = 0},
//     {RG_KEY_SELECT, .num = GPIO_NUM_1,  .pullup = 1, .level = 0},
//     {RG_KEY_START,  .num = GPIO_NUM_2,  .pullup = 1, .level = 0},
//     {RG_KEY_MENU,   .num = GPIO_NUM_38, .pullup = 1, .level = 0},
// }

// Battery
#define RG_BATTERY_DRIVER           0

// 8080 Parallel Display
#define RG_GPIO_LCD_D0              GPIO_NUM_9
#define RG_GPIO_LCD_D1              GPIO_NUM_46
#define RG_GPIO_LCD_D2              GPIO_NUM_3
#define RG_GPIO_LCD_D3              GPIO_NUM_8
#define RG_GPIO_LCD_D4              GPIO_NUM_18
#define RG_GPIO_LCD_D5              GPIO_NUM_17
#define RG_GPIO_LCD_D6              GPIO_NUM_16
#define RG_GPIO_LCD_D7              GPIO_NUM_15
#define RG_GPIO_LCD_WR              GPIO_NUM_47
#define RG_GPIO_LCD_DC              GPIO_NUM_0
#define RG_GPIO_LCD_CS              GPIO_NUM_NC
#define RG_GPIO_LCD_RST             GPIO_NUM_4
#define RG_GPIO_LCD_BCKL            GPIO_NUM_45

// SPI SD Card (directly accessible pins on extension header - adjust to your wiring)
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_13
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_11
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_12
#define RG_GPIO_SDSPI_CS            GPIO_NUM_10

// External I2S DAC (accent pins on extension header - adjust to your wiring)
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_1
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_2
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_14

// I2C Bus (FT5x06 touch controller)
#define RG_GPIO_I2C_SDA             GPIO_NUM_6
#define RG_GPIO_I2C_SCL             GPIO_NUM_5
