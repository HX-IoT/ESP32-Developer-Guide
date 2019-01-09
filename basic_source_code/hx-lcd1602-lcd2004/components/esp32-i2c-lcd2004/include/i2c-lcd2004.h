/*
 * MIT License
 *
 * Copyright (c) 2018 David Antliff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 * @brief Interface definitions for the ESP32-compatible I2C LCD2004 component.
 *
 * This component provides structures and functions that are useful for communicating with the device.
 *
 * Technically, the LCD2004 device is an I2C not SMBus device, however some SMBus protocols can be used
 * to communicate with the device, so it makes sense to use an SMBus interface to manage communication.
 */

#ifndef I2C_LCD2004_H
#define I2C_LCD2004_H

#include <stdbool.h>
#include "smbus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 包含与I2C-LCD2004器件相关信息的结构。
 */
typedef struct
{
    bool init;                                          ///< 如果struct已被初始化，则为真，否则为假
    smbus_info_t * smbus_info;                          ///< 指向关联的SMBus信息的指针
    uint8_t backlight_flag;                             ///< 如果要启用背光，则非零，否则为零
    uint8_t display_control_flags;                      ///< 当前活动的显示控件标志
    uint8_t entry_mode_flags;                           ///< 当前活动进入模式标志
} i2c_lcd2004_info_t;

#define I2C_LCD2004_NUM_ROWS               4            ///< 此设备支持的最大行数 4
#define I2C_LCD2004_NUM_COLUMNS            40           ///< 此设备支持的最大列数 40
#define I2C_LCD2004_NUM_VISIBLE_COLUMNS    20           ///< 每次可见的列数 20

// ROM代码A00的特殊字符
#define I2C_LCD2004_CHARACTER_ALPHA        0b11100000   ///< Lower-case alpha symbol
#define I2C_LCD2004_CHARACTER_BETA         0b11100010   ///< Lower-case beta symbol
#define I2C_LCD2004_CHARACTER_THETA        0b11110010   ///< Lower-case theta symbol
#define I2C_LCD2004_CHARACTER_PI           0b11110111   ///< Lower-case pi symbol
#define I2C_LCD2004_CHARACTER_OMEGA        0b11110100   ///< Upper-case omega symbol
#define I2C_LCD2004_CHARACTER_SIGMA        0b11110110   ///< Upper-case sigma symbol
#define I2C_LCD2004_CHARACTER_INFINITY     0b11110011   ///< Infinity symbol
#define I2C_LCD2004_CHARACTER_DEGREE       0b11011111   ///< Degree symbol
#define I2C_LCD2004_CHARACTER_ARROW_RIGHT  0b01111110   ///< Arrow pointing right symbol
#define I2C_LCD2004_CHARACTER_ARROW_LEFT   0b01111111   ///< Arrow pointing left symbol
#define I2C_LCD2004_CHARACTER_SQUARE       0b11011011   ///< Square outline symbol
#define I2C_LCD2004_CHARACTER_DOT          0b10100101   ///< Centred dot symbol
#define I2C_LCD2004_CHARACTER_DIVIDE       0b11111101   ///< Division sign symbol
#define I2C_LCD2004_CHARACTER_BLOCK        0b11111111   ///< 5x8 filled block

/**
 * @brief 用于定义用户定义字符的有效索引的枚举。
 */
typedef enum
{
    I2C_LCD2004_INDEX_CUSTOM_0 = 0,                     ///< Index of first user-defined custom symbol
    I2C_LCD2004_INDEX_CUSTOM_1,                         ///< Index of second user-defined custom symbol
    I2C_LCD2004_INDEX_CUSTOM_2,                         ///< Index of third user-defined custom symbol
    I2C_LCD2004_INDEX_CUSTOM_3,                         ///< Index of fourth user-defined custom symbol
    I2C_LCD2004_INDEX_CUSTOM_4,                         ///< Index of fifth user-defined custom symbol
    I2C_LCD2004_INDEX_CUSTOM_5,                         ///< Index of sixth user-defined custom symbol
    I2C_LCD2004_INDEX_CUSTOM_6,                         ///< Index of seventh user-defined custom symbol
    I2C_LCD2004_INDEX_CUSTOM_7,                         ///< Index of eighth user-defined custom symbol
} i2c_lcd2004_custom_index_t;

#define I2C_LCD2004_CHARACTER_CUSTOM_0     0b00000000   ///< User-defined custom symbol in index 0
#define I2C_LCD2004_CHARACTER_CUSTOM_1     0b00000001   ///< User-defined custom symbol in index 1
#define I2C_LCD2004_CHARACTER_CUSTOM_2     0b00000010   ///< User-defined custom symbol in index 2
#define I2C_LCD2004_CHARACTER_CUSTOM_3     0b00000011   ///< User-defined custom symbol in index 3
#define I2C_LCD2004_CHARACTER_CUSTOM_4     0b00000100   ///< User-defined custom symbol in index 4
#define I2C_LCD2004_CHARACTER_CUSTOM_5     0b00000101   ///< User-defined custom symbol in index 5
#define I2C_LCD2004_CHARACTER_CUSTOM_6     0b00000110   ///< User-defined custom symbol in index 6
#define I2C_LCD2004_CHARACTER_CUSTOM_7     0b00000111   ///< User-defined custom symbol in index 7

/**
 * @brief 构造一个新的I2C-LCD2004 info实例。
 *        在调用其他函数之前，应该初始化新实例。
 *
 * @return 指向新设备信息实例的指针，如果不能创建则为空
 */
i2c_lcd2004_info_t * i2c_lcd2004_malloc(void);

/**
 * @brief Delete an existing I2C-LCD2004 info instance.
 *
 * @param[in,out] tsl2561_info Pointer to I2C-LCD2004 info instance that will be freed and set to NULL.
 */
void i2c_lcd2004_free(i2c_lcd2004_info_t ** tsl2561_info);

/**
 * @brief 使用指定的SMBus信息初始化一个I2C-LCD2004 info实例。
 *
 * @param[in] i2c_lcd2004_info Pointer to I2C-LCD2004 info instance.
 * @param[in] smbus_info Pointer to SMBus info instance.
 */
esp_err_t i2c_lcd2004_init(i2c_lcd2004_info_t * i2c_lcd2004_info, smbus_info_t * smbus_info, bool backlight);

/**
 * @brief 清除整个显示(清除DDRAM)并将光标返回到主位置。
 *       DDRAM内容被清除，CGRAM内容没有改变。
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_clear(const i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 将光标移到主位置。 还可以重置可能发生的任何显示移位。
 *       DDRAM内容没有改变。CGRAM内容不变。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_home(const i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 将光标移动到指定的列和行位置。这是一个新字符将出现的地方。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @param[in] col 目标光标位置的从零开始的水平索引。左边第一列为0.
 * @param[in] row 目标光标位置的从零开始的垂直索引。第一行为0.
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_move_cursor(const i2c_lcd2004_info_t * i2c_lcd2004_info, uint8_t col, uint8_t row);

/**
 * @brief 启用或禁用LED背光。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @param[in] enable 真为启用，假为禁用。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_set_backlight(i2c_lcd2004_info_t * i2c_lcd2004_info, bool enable);

/**
 * @brief 启用或禁用显示。禁用时，背光不受影响，但DDRAM的任何内容都不会显示，光标也不会显示。显示为“空白”。
 *        重新启用显示不会影响DDRAM的内容或光标的状态或位置。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @param[in] enable 真为启用，假为禁用。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_set_display(i2c_lcd2004_info_t * i2c_lcd2004_info, bool enable);

/**
 * @brief 启用或禁用下划线光标的显示。
 *        如果启用，这将直观地指示写入显示的下一个字符将出现在何处。
 *        它可以在闪烁的光标旁边启用，但是视觉效果不佳。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @param[in] enable 真为启用，假为禁用。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_set_cursor(i2c_lcd2004_info_t * i2c_lcd2004_info, bool enable);

/**
 * @brief 启用或禁用闪烁块光标的显示。
 *       如果启用，这将直观地指示写入显示的下一个字符将出现在何处。
 *        它可以在下划线光标旁边启用，但是视觉结果不雅。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @param[in] enable 真为启用，假为禁用。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_set_blink(i2c_lcd2004_info_t * i2c_lcd2004_info, bool enable);

/**
 * @brief 在每个字符写入后设置光标移动方向以产生从左到右的文本。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_set_left_to_right(i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 在每个字符写入后设置光标移动方向，以生成从右到左的文本。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_set_right_to_left(i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 启用或禁用显示的自动滚动。
 *        当启用时，显示将在写入字符时滚动，以保持光标在屏幕上的位置。
 *       从光标位置开始，从左到右的文本将显示为右对齐。
 *        当禁用时，显示将不会滚动，光标将在屏幕上移动。
 *        从光标位置开始，从左到右的文本将显示为左对齐。
 *        
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @param[in] enable 真为启用，假为禁用。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_set_auto_scroll(i2c_lcd2004_info_t * i2c_lcd2004_info, bool enable);

/**
 * @brief 将显示器向左滚动一个位置。屏幕上的文字会向右移动。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_scroll_display_left(const i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 将显示器向右滚动一个位置。屏幕上的文字会向左移动。
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_scroll_display_right(const i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 将光标向左移动一个位置，即使它是不可见的。这将影响写到显示器上的下一个字符的位置。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_move_cursor_left(const i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 将光标向右移动一个位置，即使它是不可见的。这将影响写到显示器上的下一个字符的位置。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_move_cursor_right(const i2c_lcd2004_info_t * i2c_lcd2004_info);

/**
 * @brief 从像素数据数组中定义自定义字符。
 *
 *        T这里有八个可能的自定义字符, 且使用了从零开始的索引
 *        选择要定义的字符。 该索引处的任何现有字符定义都将丢失。
 *        字符宽5像素，高8像素。
 *        pixelmap数组由8个字节组成，每个字节表示每行的像素状态。
 *        第一个字节表示第一行。第8个字节通常为零(为下划线光标留出空间)。
 *       对于每一行，最低的5位表示要照明的像素。最小有效位表示
 *        最右边的像素。空行为零。
 *
 *        NOTE: 在调用此函数之后，将不会选择DDRAM，并且不会定义游标位置. 
 *        因此，如果要将文本写入显示器，那么在此函数之后设置DDRAM地址是很重要的。
 *        这可以通过调用i2c_lcd2004_home()或i2c_lcd2004_move_cursor()来执行。
 *
 *        自定义字符是使用I2C_LCD2004_CHARACTER_CUSTOM_X定义编写的。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @param[in] index 要定义的字符的从零开始的索引。只有0-7的值是有效的。
 * @param[in] pixelmap 为新的字符定义定义像素映射的8字节数组。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_define_char(const i2c_lcd2004_info_t * i2c_lcd2004_info, i2c_lcd2004_custom_index_t index, uint8_t pixelmap[]);

/**
 * @brief 将单个字符写入光标当前位置的显示
 *        根据活动模式的不同，光标可以向左或向右移动，或者显示可以向左或向右移动。
 *        可以使用I2C_LCD2004_CHARACTER_CUSTOM_X定义编写定制字符。
 *
 *        显示为I2C_LCD2004_NUM_COLUMNS宽，在遇到第一行末尾时，显示光标将自动移动到第二行的开头。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_write_char(const i2c_lcd2004_info_t * i2c_lcd2004_info, uint8_t chr);

/**
 * @brief 从光标的当前位置开始，向显示器写入一串字符。写完每个字符后
 *           根据活动模式的不同，光标可以向左或向右移动，或显示可以向左或向右移动，
。
 *        显示为I2C_LCD2004_NUM_COLUMNS宽，在遇到第一行的末尾时，光标将自动移动到第二行的开头。
 *
 * @param[in] i2c_lcd2004_info 指向初始化的I2C-LCD2004 info实例的指针。
 * @return ESP_OK if successful, otherwise an error constant.
 */
esp_err_t i2c_lcd2004_write_string(const i2c_lcd2004_info_t * i2c_lcd2004_info, const char * string);

#ifdef __cplusplus
}
#endif

#endif // I2C_LCD2004_H
