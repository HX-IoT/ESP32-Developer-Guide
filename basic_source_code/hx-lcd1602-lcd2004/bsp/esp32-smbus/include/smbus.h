/*
 * MIT License
 *
 * Copyright (c) 2017 David Antliff
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
 * @file smbus.h
 * @brief ESP32兼容 SMBus 协议组件的接口定义.
 *
 * This 组件提供结构和函数可用于对SMBus协议兼容 I2C 从设备通讯.
 */

#ifndef SMBUS_H
#define SMBUS_H

#include <stdbool.h>
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_TIMEOUT (1000 / portTICK_RATE_MS)		//定义默认超时时间为1秒

/**
 * @brief 7-bit or 10-bit I2C slave address.
 */
typedef uint16_t i2c_address_t;

/**
 * @brief 包含SMBus 端口相关信息的结构.
 */
typedef struct
{
    bool init;                     ///< 如果struct已被初始化，则为真，否则为假
    i2c_port_t i2c_port;           ///< ESP-IDF I2C 端口号
    i2c_address_t address;         ///< I2C 从设备地址
    portBASE_TYPE timeout;         ///< Number of ticks until I2C operation timeout
} smbus_info_t;

/**
 * @brief构造一个新的SMBus info实例。在调用其他函数之前，应该初始化新实例。
 * @return 指向新设备信息实例的指针，如果不能创建则为空。
 */
smbus_info_t * smbus_malloc(void);

/**
 * @brief 删除现有的SMBus info实例。
 * @param[in,out] smbus_info指针指向SMBus info实例，该实例将被释放并设置为NULL。
 */
void smbus_free(smbus_info_t ** smbus_info);

/**
 * @brief 使用指定的I2C信息初始化SMBus info实例。
 *        I2C超时默认约为1秒。
 * @param[in] smbus_info 指向SMBus信息实例的指针。
 * @param[in] i2c_port 与此SMBus实例相关联的I2C端口。
 * @param[in] address 2C从设备地址。
 */
esp_err_t smbus_init(smbus_info_t * smbus_info, i2c_port_t i2c_port, i2c_address_t address);

/**
 * @brief Set the I2C timeout.
 *        I2C transactions that do not complete within this period are considered an error.
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] timeout Number of ticks to wait until the transaction is considered in error.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_set_timeout(smbus_info_t * smbus_info, portBASE_TYPE timeout);

/**
 * @brief 发送一个bit信号到一个从设备的read/write bit
 *        可用于简单地打开或关闭设备功能，或启用或禁用
 *        a low-power 待机模式. 没有发送或接收数据。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] bit 发送数据位.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_quick(const smbus_info_t * smbus_info, bool bit);

/**
 * @brief 将一个字节发送到从设备。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] data 用于发送到从设备的数据字节8bit
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_send_byte(const smbus_info_t * smbus_info, uint8_t data);

/**
 * @brief 从从设备接收单个字节。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[out] data Data byte received from slave.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_receive_byte(const smbus_info_t * smbus_info, uint8_t * data);

/**
 * @brief 用命令代码将单个字节写入从设备。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[in] data 用于发送到从设备的数据字节8bit
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_write_byte(const smbus_info_t * smbus_info, uint8_t command, uint8_t data);

/**
 * @brief 用命令代码向从设备写入一个单词(两个字节)。
 *        最小有效字节首先被传输。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[in] data Data word to send to slave.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_write_word(const smbus_info_t * smbus_info, uint8_t command, uint16_t data);

/**
 * @brief 用命令代码从从设备读取单个字节。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[out] data Data byte received from slave.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_read_byte(const smbus_info_t * smbus_info, uint8_t command, uint8_t * data);

/**
 * @brief 用命令代码从从设备中读取一个单词(两个字节)。
 *        接收到的第一个字节是最低有效字节。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[out] data Data byte received from slave.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_read_word(const smbus_info_t * smbus_info, uint8_t command, uint16_t * data);

/**
 * @brief 用一个命令代码写多个字节（最高255字节）的数据到从设备
  *        这使用字节计数来协商事务的长度。
 *         数据数组中的第一个字节首先被传输。
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[in] data Data bytes to send to slave.
 * @param[in] len 发送到从设备的字节数.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_write_block(const smbus_info_t * smbus_info, uint8_t command, uint8_t * data, uint8_t len);

/**
 * @brief 用一个命令代码读取从设备多个字节（最高255）的数据
 *        This uses a byte count to negotiate the length of the transaction.
 *        The first byte received is placed in the first array location.
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[out] data Data bytes received from slave.
 * @param[in/out] len Size of data array, and number of bytes actually received.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_read_block(const smbus_info_t * smbus_info, uint8_t command, uint8_t * data, uint8_t * len);

/**
 * @brief Write up to 255 bytes to a slave device with a command code.
 *        No byte count is used - the transaction lasts as long as the master requires.
 *        The first byte in the data array is transmitted first.
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[in] data Data bytes to send to slave.
 * @param[in] len Number of bytes to send to slave.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_i2c_write_block(const smbus_info_t * smbus_info, uint8_t command, uint8_t * data, uint8_t len);

/**
 * @brief Read up to 255 bytes from a slave device with a command code (combined format).
 *        No byte count is used - the transaction lasts as long as the master requires.
 *        The first byte received is placed in the first array location.
 * @param[in] smbus_info 指向初始化的SMBus info实例的指针。
 * @param[in] command 设备特定的命令字节
 * @param[out] data Data bytes received from slave.
 * @param[in/out] len Size of data array. If the slave fails to provide sufficient bytes, ESP_ERR_TIMEOUT will be returned.
 * @return ESP_OK 表示成功，ESP_FAIL or ESP_ERR_* 表示有错误发生
 */
esp_err_t smbus_i2c_read_block(const smbus_info_t * smbus_info, uint8_t command, uint8_t * data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif  // SMBUS_H
