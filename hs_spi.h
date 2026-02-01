/**
 * @file      hs_spi.h
 * @brief     SPI 模块头文件
 * @author    huenrong (sgyhy1028@outlook.com)
 * @date      2026-02-01 14:44:20
 *
 * @copyright Copyright (c) 2026 huenrong
 *
 */

#ifndef __HS_SPI_H
#define __HS_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define HS_SPI_CPHA 0x01
#define HS_SPI_CPOL 0x02

/**
 * @brief SPI 片选脚控制回调函数类型
 *
 * @param[in] enable: 是否使能片选脚(true: 使能; false: 失能)
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
typedef int (*hs_spi_cs_control_cb)(bool enable);

// SPI模式定义
// 其实在文件 "linux/spi/spidev.h" 中有定义
typedef enum hs_spi_mode
{
    E_HS_SPI_MODE_0 = (0 | 0),
    E_HS_SPI_MODE_1 = (0 | HS_SPI_CPHA),
    E_HS_SPI_MODE_2 = (HS_SPI_CPOL | 0),
    E_HS_SPI_MODE_3 = (HS_SPI_CPOL | HS_SPI_CPHA),
} hs_spi_mode_e;

// SPI 对象
typedef struct _hs_spi hs_spi_t;

/**
 * @brief 创建 SPI 对象
 *
 * @return 成功: SPI 对象
 * @return 失败: NULL
 */
hs_spi_t *hs_spi_create(void);

/**
 * @brief 初始化 SPI 对象
 *
 * @note 1. 该函数支持重复调用，重复调用时会关闭并重新打开 SPI 设备
 *       2. 调用该函数前必须确保没有其他线程正在使用该 SPI 对象，否则可能导致未定义行为
 *
 * @param[in,out] hs_spi      : SPI 对象
 * @param[in]     spi_name    : SPI 设备名称（如：/dev/spidev0.0）
 * @param[in]     spi_mode    : SPI 模式
 * @param[in]     spi_speed_hz: SPI 速率（单位：Hz）
 * @param[in]     spi_bits    : SPI 数据位宽
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_init(hs_spi_t *hs_spi, const char *spi_name, const hs_spi_mode_e spi_mode, const uint32_t spi_speed_hz,
                const uint8_t spi_bits);

/**
 * @brief 销毁 SPI 对象
 *
 * @note 1. 调用该函数前必须确保没有其他线程正在使用该 SPI 对象，否则可能导致未定义行为
 *       2. 销毁后，该 SPI 对象将不再可用
 *
 * @param[in,out] hs_spi: SPI 对象
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_destroy(hs_spi_t *hs_spi);

/**
 * @brief 设置 SPI 片选脚控制回调函数
 *
 * @note 1. 设置后，由模块内部控制片选脚，无需用户手动控制（推荐此方案）
 *       2. 未设置，由用户手动控制或内核控制
 *       3. 因内部传输数据会分片，即分多次调用 ioctl() 函数，且每次调用 ioctl() 函数后内核会自动控制片选脚，
 *          可能导致分片传输时部分外设 SPI 时序与外设要求不匹配，导致通信异常
 *
 * @param[in,out] hs_spi       : SPI 对象
 * @param[in]     cs_control_cb: SPI 片选脚控制回调函数
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_set_cs_control_cb(hs_spi_t *hs_spi, hs_spi_cs_control_cb cs_control_cb);

/**
 * @brief 设置 SPI 单次最大传输长度
 *
 * @note 未设置或设置值为 0 使用内部默认值 4096 字节
 *
 * @param[in,out] hs_spi          : SPI 对象
 * @param[in]     max_transfer_len: SPI 单次最大传输长度（单位：字节）
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_set_max_transfer_len(hs_spi_t *hs_spi, const size_t max_transfer_len);

/**
 * @brief 向无寄存器地址的 SPI 设备写数据
 *
 * @param[in,out] hs_spi        : SPI 对象
 * @param[in]     write_data    : 待写入的数据
 * @param[in]     write_data_len: 待写入的数据长度
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_write_data(hs_spi_t *hs_spi, const uint8_t *write_data, const size_t write_data_len);

/**
 * @brief 从无寄存器地址的 SPI 设备读数据
 *
 * @param[in,out] hs_spi       : SPI 对象
 * @param[out]    read_data    : 读取到的数据
 * @param[in]     read_data_len: 指定读取数据长度
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_read_data(hs_spi_t *hs_spi, uint8_t *read_data, const size_t read_data_len);

/**
 * @brief 无寄存器地址的 SPI 设备读写（全双工）
 *
 * @note 需要硬件支持全双工，函数内部不判断是否支持全双工
 *
 * @param[in,out] hs_spi        : SPI 对象
 * @param[in]     write_data    : 待写入的数据
 * @param[in]     write_data_len: 待写入的数据长度
 * @param[out]    read_data     : 读取到的数据
 * @param[in]     read_data_len : 指定读取数据长度
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_write_read_data(hs_spi_t *hs_spi, const uint8_t *write_data, const size_t write_data_len, uint8_t *read_data,
                           const size_t read_data_len);

/**
 * @brief 向有寄存器地址的 SPI 设备写数据
 *
 * @param[in,out] hs_spi        : SPI 对象
 * @param[in]     reg_addr      : 寄存器地址
 * @param[in]     write_data    : 待写入的数据
 * @param[in]     write_data_len: 待写入的数据长度
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_write_data_sub(hs_spi_t *hs_spi, const uint8_t reg_addr, const uint8_t *write_data,
                          const size_t write_data_len);

/**
 * @brief 从有寄存器地址的 SPI 设备读数据
 *
 * @param[in,out] hs_spi       : SPI 对象
 * @param[in]     reg_addr     : 寄存器地址
 * @param[out]    rad_data     : 读取到的数据
 * @param[in]     read_data_len: 指定读取数据长度
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_read_data_sub(hs_spi_t *hs_spi, const uint8_t reg_addr, uint8_t *rad_data, const size_t read_data_len);

/**
 * @brief 无寄存器地址的 SPI 设备读写（全双工）
 *
 * @note 需要硬件支持全双工，函数内部不判断是否支持全双工
 *
 * @param[in,out] hs_spi        : SPI 对象
 * @param[in]     reg_addr      : 寄存器地址
 * @param[in]     write_data    : 待写入的数据
 * @param[in]     write_data_len: 待写入的数据长度
 * @param[out]    read_data     : 读取到的数据
 * @param[in]     read_data_len : 指定读取数据长度
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
int hs_spi_write_read_data_sub(hs_spi_t *hs_spi, const uint8_t reg_addr, const uint8_t *write_data,
                               const size_t write_data_len, uint8_t *read_data, const size_t read_data_len);

#ifdef __cplusplus
}
#endif

#endif // __HS_SPI_H
