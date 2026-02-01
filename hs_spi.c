/**
 * @file      hs_spi.c
 * @brief     SPI 模块源文件
 * @author    huenrong (sgyhy1028@outlook.com)
 * @date      2026-02-01 14:44:15
 *
 * @copyright Copyright (c) 2026 huenrong
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#include "hs_spi.h"

// SPI 对象
struct _hs_spi
{
    int fd;
    hs_spi_cs_control_cb cs_control_cb;
    size_t max_transfer_len;
    pthread_mutex_t mutex;
};

/**
 * @brief SPI 片选控制
 *
 * @param[in] hs_spi: SPI 对象
 * @param[in] enable: 是否使能片选 (true: 使能, false: 失能)
 *
 * @return 0 : 成功
 * @return <0: 失败
 */
static int hs_spi_cs_control(const hs_spi_t *hs_spi, const bool enable)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (hs_spi->cs_control_cb != NULL)
    {
        return hs_spi->cs_control_cb(enable);
    }

    return 0;
}

hs_spi_t *hs_spi_create(void)
{
    hs_spi_t *hs_spi = (hs_spi_t *)malloc(sizeof(hs_spi_t));
    if (hs_spi == NULL)
    {
        return NULL;
    }

    hs_spi->fd = -1;
    hs_spi->cs_control_cb = NULL;
    hs_spi->max_transfer_len = 0;
    pthread_mutex_init(&hs_spi->mutex, NULL);

    return hs_spi;
}

int hs_spi_init(hs_spi_t *hs_spi, const char *spi_name, const hs_spi_mode_e spi_mode, const uint32_t spi_speed_hz,
                const uint8_t spi_bits)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (spi_name == NULL)
    {
        return -2;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd != -1)
    {
        close(hs_spi->fd);
        hs_spi->fd = -1;
    }

    int fd = open(spi_name, O_RDWR);
    if (fd < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -3;
    }

    // 设置 SPI 写模式
    int ret = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode);
    if (ret < 0)
    {
        close(fd);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -4;
    }

    // 设置 SPI 读模式
    ret = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode);
    if (ret < 0)
    {
        close(fd);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -5;
    }

    // 设置 SPI 写最大速率
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed_hz);
    if (ret < 0)
    {
        close(fd);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -6;
    }

    // 设置 SPI 读最大速率
    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed_hz);
    if (ret < 0)
    {
        close(fd);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -7;
    }

    // 设置写 SPI 字长
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
    if (ret < 0)
    {
        close(fd);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -8;
    }

    // 设置读 SPI 字长
    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
    if (ret < 0)
    {
        close(fd);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -9;
    }

    hs_spi->fd = fd;
    hs_spi->max_transfer_len = hs_spi->max_transfer_len == 0 ? 4096 : hs_spi->max_transfer_len;
    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_destroy(hs_spi_t *hs_spi)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd != -1)
    {
        close(hs_spi->fd);
        hs_spi->fd = -1;
    }

    pthread_mutex_unlock(&hs_spi->mutex);
    pthread_mutex_destroy(&hs_spi->mutex);
    free(hs_spi);

    return 0;
}

int hs_spi_set_cs_control_cb(hs_spi_t *hs_spi, hs_spi_cs_control_cb cs_control_cb)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    hs_spi->cs_control_cb = cs_control_cb;
    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_set_max_transfer_len(hs_spi_t *hs_spi, const size_t max_transfer_len)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    hs_spi->max_transfer_len = max_transfer_len == 0 ? 4096 : max_transfer_len;
    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_write_data(hs_spi_t *hs_spi, const uint8_t *write_data, const size_t write_data_len)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (write_data == NULL)
    {
        return -2;
    }

    if (write_data_len == 0)
    {
        return -3;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -4;
    }

    if (hs_spi_cs_control(hs_spi, true) < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -5;
    }

    // 剩余未发送数据长度
    size_t remain_data_len = write_data_len;
    while (remain_data_len > 0)
    {
        // 本次发送数据长度
        size_t current_len = remain_data_len > hs_spi->max_transfer_len ? hs_spi->max_transfer_len : remain_data_len;
        // 本次发送数据偏移量
        size_t data_offset = write_data_len - remain_data_len;

        struct spi_ioc_transfer spi_transfer = {0};
        spi_transfer.tx_buf = (unsigned long)&write_data[data_offset];
        spi_transfer.len = current_len;
        spi_transfer.cs_change = 0;

        if (ioctl(hs_spi->fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0)
        {
            hs_spi_cs_control(hs_spi, false);
            pthread_mutex_unlock(&hs_spi->mutex);

            return -6;
        }

        remain_data_len -= current_len;
    }

    hs_spi_cs_control(hs_spi, false);

    if (remain_data_len != 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -7;
    }

    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_read_data(hs_spi_t *hs_spi, uint8_t *read_data, const size_t read_data_len)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (read_data == NULL)
    {
        return -2;
    }

    if (read_data_len == 0)
    {
        return -3;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -4;
    }

    if (hs_spi_cs_control(hs_spi, true) < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -5;
    }

    // 剩余未读取数据长度
    size_t remain_data_len = read_data_len;
    while (remain_data_len > 0)
    {
        // 本次读取数据长度
        size_t current_len = remain_data_len > hs_spi->max_transfer_len ? hs_spi->max_transfer_len : remain_data_len;
        // 本次读取数据偏移量
        size_t data_offset = read_data_len - remain_data_len;

        struct spi_ioc_transfer spi_transfer = {0};
        spi_transfer.rx_buf = (unsigned long)&read_data[data_offset];
        spi_transfer.len = current_len;
        spi_transfer.cs_change = 0;

        if (ioctl(hs_spi->fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0)
        {
            hs_spi_cs_control(hs_spi, false);
            pthread_mutex_unlock(&hs_spi->mutex);

            return -6;
        }

        remain_data_len -= current_len;
    }

    hs_spi_cs_control(hs_spi, false);

    if (remain_data_len != 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -7;
    }

    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_write_read_data(hs_spi_t *hs_spi, const uint8_t *write_data, const size_t write_data_len, uint8_t *read_data,
                           const size_t read_data_len)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (write_data == NULL)
    {
        return -2;
    }

    if (write_data_len == 0)
    {
        return -3;
    }

    if (read_data == NULL)
    {
        return -4;
    }

    if (read_data_len == 0)
    {
        return -5;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -6;
    }

    size_t transfer_len = write_data_len > read_data_len ? write_data_len : read_data_len;

    uint8_t *tx_buf = (uint8_t *)malloc(transfer_len);
    if (tx_buf == NULL)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -7;
    }
    memset(tx_buf, 0, transfer_len);
    memcpy(tx_buf, write_data, write_data_len);

    uint8_t *rx_buf = (uint8_t *)malloc(transfer_len);
    if (rx_buf == NULL)
    {
        free(tx_buf);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -8;
    }
    memset(rx_buf, 0, transfer_len);

    if (hs_spi_cs_control(hs_spi, true) < 0)
    {
        free(tx_buf);
        free(rx_buf);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -9;
    }

    // 剩余未传输数据长度
    size_t remain_data_len = transfer_len;
    while (remain_data_len > 0)
    {
        // 本次传输数据长度
        size_t current_len = remain_data_len > hs_spi->max_transfer_len ? hs_spi->max_transfer_len : remain_data_len;
        // 本次传输数据偏移量
        size_t data_offset = transfer_len - remain_data_len;

        struct spi_ioc_transfer spi_transfer = {0};
        spi_transfer.tx_buf = (unsigned long)&tx_buf[data_offset];
        spi_transfer.rx_buf = (unsigned long)&rx_buf[data_offset];
        spi_transfer.len = current_len;
        spi_transfer.cs_change = 0;

        if (ioctl(hs_spi->fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0)
        {
            free(tx_buf);
            free(rx_buf);
            hs_spi_cs_control(hs_spi, false);
            pthread_mutex_unlock(&hs_spi->mutex);

            return -10;
        }

        remain_data_len -= current_len;
    }

    hs_spi_cs_control(hs_spi, false);

    if (remain_data_len != 0)
    {
        free(tx_buf);
        free(rx_buf);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -11;
    }

    memcpy(read_data, rx_buf, read_data_len);
    free(tx_buf);
    free(rx_buf);
    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_write_data_sub(hs_spi_t *hs_spi, const uint8_t reg_addr, const uint8_t *write_data,
                          const size_t write_data_len)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (write_data == NULL)
    {
        return -2;
    }

    if (write_data_len == 0)
    {
        return -3;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -4;
    }

    if (hs_spi_cs_control(hs_spi, true) < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -5;
    }

    struct spi_ioc_transfer spi_transfer[2] = {0};
    spi_transfer[0].tx_buf = (unsigned long)&reg_addr;
    spi_transfer[0].len = 1;
    spi_transfer[0].cs_change = 0;

    // 剩余未写入数据长度
    size_t remain_data_len = write_data_len;
    while (remain_data_len > 0)
    {
        // 本次写入数据长度
        size_t current_len = remain_data_len > hs_spi->max_transfer_len ? hs_spi->max_transfer_len : remain_data_len;
        // 本次写入数据偏移量
        size_t data_offset = write_data_len - remain_data_len;

        int ret = -1;
        // 第一包数据需要单独发送寄存器地址
        if (remain_data_len == write_data_len)
        {
            memset(&spi_transfer[1], 0, sizeof(spi_transfer[1]));
            spi_transfer[1].tx_buf = (unsigned long)&write_data[data_offset];
            spi_transfer[1].len = current_len;
            spi_transfer[1].cs_change = 0;

            ret = ioctl(hs_spi->fd, SPI_IOC_MESSAGE(2), &spi_transfer);
        }
        // 后续数据不需要发送寄存器地址
        else
        {
            memset(&spi_transfer[0], 0, sizeof(spi_transfer[0]));
            spi_transfer[0].tx_buf = (unsigned long)&write_data[data_offset];
            spi_transfer[0].len = current_len;
            spi_transfer[0].cs_change = 0;

            ret = ioctl(hs_spi->fd, SPI_IOC_MESSAGE(1), &spi_transfer[0]);
        }

        if (ret < 0)
        {
            hs_spi_cs_control(hs_spi, false);
            pthread_mutex_unlock(&hs_spi->mutex);

            return -6;
        }

        remain_data_len -= current_len;
    }

    hs_spi_cs_control(hs_spi, false);

    if (remain_data_len != 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -7;
    }

    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_read_data_sub(hs_spi_t *hs_spi, const uint8_t reg_addr, uint8_t *rad_data, const size_t read_data_len)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (rad_data == NULL)
    {
        return -2;
    }

    if (read_data_len == 0)
    {
        return -3;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -4;
    }

    if (hs_spi_cs_control(hs_spi, true) < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -5;
    }

    struct spi_ioc_transfer spi_transfer[2] = {0};
    spi_transfer[0].tx_buf = (unsigned long)&reg_addr;
    spi_transfer[0].len = 1;
    spi_transfer[0].cs_change = 0;

    // 剩余未读取数据长度
    size_t remain_data_len = read_data_len;
    while (remain_data_len > 0)
    {
        // 本次读取数据长度
        size_t current_len = remain_data_len > hs_spi->max_transfer_len ? hs_spi->max_transfer_len : remain_data_len;
        // 本次读取数据偏移量
        size_t data_offset = read_data_len - remain_data_len;

        int ret = -1;
        // 第一包数据需要单独发送寄存器地址
        if (remain_data_len == read_data_len)
        {
            memset(&spi_transfer[1], 0, sizeof(spi_transfer[1]));
            spi_transfer[1].rx_buf = (unsigned long)&rad_data[data_offset];
            spi_transfer[1].len = current_len;
            spi_transfer[1].cs_change = 0;

            ret = ioctl(hs_spi->fd, SPI_IOC_MESSAGE(2), &spi_transfer);
        }
        // 后续数据不需要发送寄存器地址
        else
        {
            memset(&spi_transfer[0], 0, sizeof(spi_transfer[0]));
            spi_transfer[0].rx_buf = (unsigned long)&rad_data[data_offset];
            spi_transfer[0].len = current_len;
            spi_transfer[0].cs_change = 0;

            ret = ioctl(hs_spi->fd, SPI_IOC_MESSAGE(1), &spi_transfer[0]);
        }

        if (ret < 0)
        {
            hs_spi_cs_control(hs_spi, false);
            pthread_mutex_unlock(&hs_spi->mutex);

            return -6;
        }

        remain_data_len -= current_len;
    }

    hs_spi_cs_control(hs_spi, false);

    if (remain_data_len != 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -7;
    }

    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}

int hs_spi_write_read_data_sub(hs_spi_t *hs_spi, const uint8_t reg_addr, const uint8_t *write_data,
                               const size_t write_data_len, uint8_t *read_data, const size_t read_data_len)
{
    if (hs_spi == NULL)
    {
        return -1;
    }

    if (write_data == NULL)
    {
        return -2;
    }

    if (write_data_len == 0)
    {
        return -3;
    }

    if (read_data == NULL)
    {
        return -4;
    }

    if (read_data_len == 0)
    {
        return -5;
    }

    pthread_mutex_lock(&hs_spi->mutex);
    if (hs_spi->fd < 0)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -6;
    }

    size_t transfer_len = write_data_len > read_data_len ? write_data_len : read_data_len;

    uint8_t *tx_buf = (uint8_t *)malloc(transfer_len);
    if (tx_buf == NULL)
    {
        pthread_mutex_unlock(&hs_spi->mutex);

        return -7;
    }
    memset(tx_buf, 0, transfer_len);
    memcpy(tx_buf, write_data, write_data_len);

    uint8_t *rx_buf = (uint8_t *)malloc(transfer_len);
    if (rx_buf == NULL)
    {
        free(tx_buf);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -8;
    }
    memset(rx_buf, 0, transfer_len);

    if (hs_spi_cs_control(hs_spi, true) < 0)
    {
        free(tx_buf);
        free(rx_buf);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -9;
    }

    struct spi_ioc_transfer spi_transfer[2] = {0};
    spi_transfer[0].tx_buf = (unsigned long)&reg_addr;
    spi_transfer[0].len = 1;
    spi_transfer[0].cs_change = 0;

    // 剩余未传输数据长度
    size_t remain_data_len = transfer_len;
    while (remain_data_len > 0)
    {
        // 本次传输数据长度
        size_t current_len = remain_data_len > hs_spi->max_transfer_len ? hs_spi->max_transfer_len : remain_data_len;
        // 本次传输数据偏移量
        size_t data_offset = transfer_len - remain_data_len;

        int ret = -1;
        // 第一包数据需要单独发送寄存器地址
        if (remain_data_len == transfer_len)
        {
            memset(&spi_transfer[1], 0, sizeof(spi_transfer[1]));
            spi_transfer[1].tx_buf = (unsigned long)&tx_buf[data_offset];
            spi_transfer[1].rx_buf = (unsigned long)&rx_buf[data_offset];
            spi_transfer[1].len = current_len;
            spi_transfer[1].cs_change = 0;

            ret = ioctl(hs_spi->fd, SPI_IOC_MESSAGE(2), &spi_transfer);
        }
        // 后续数据不需要发送寄存器地址
        else
        {
            memset(&spi_transfer[0], 0, sizeof(spi_transfer[0]));
            spi_transfer[0].tx_buf = (unsigned long)&tx_buf[data_offset];
            spi_transfer[0].rx_buf = (unsigned long)&rx_buf[data_offset];
            spi_transfer[0].len = current_len;
            spi_transfer[0].cs_change = 0;

            ret = ioctl(hs_spi->fd, SPI_IOC_MESSAGE(1), &spi_transfer[0]);
        }

        if (ret < 0)
        {
            free(tx_buf);
            free(rx_buf);
            hs_spi_cs_control(hs_spi, false);
            pthread_mutex_unlock(&hs_spi->mutex);

            return -10;
        }

        remain_data_len -= current_len;
    }

    hs_spi_cs_control(hs_spi, false);

    if (remain_data_len != 0)
    {
        free(tx_buf);
        free(rx_buf);
        pthread_mutex_unlock(&hs_spi->mutex);

        return -11;
    }

    memcpy(read_data, rx_buf, read_data_len);
    free(tx_buf);
    free(rx_buf);
    pthread_mutex_unlock(&hs_spi->mutex);

    return 0;
}
