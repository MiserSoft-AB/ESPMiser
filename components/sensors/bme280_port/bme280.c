/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * CHANGES FROM LEGACY i2c_bus DRIVER
 *
 * 1. Replaced `i2c_bus.h` with ESP-IDF's `driver/i2c_master.h`.
 * 2. `bme280_create()` now accepts an `i2c_master_bus_handle_t` and creates an
 *    I2C master device with `i2c_master_bus_add_device()`.
 * 3. `bme280_delete()` removes the device with `i2c_master_bus_rm_device()`.
 * 4. Register reads use `i2c_master_transmit_receive()`:
 *      - transmit the register address,
 *      - repeated-start and read the requested number of bytes.
 * 5. Register writes use `i2c_master_transmit()`:
 *      - transmit the register address followed by the value.
 * 6. Added an optional device probe using `i2c_master_probe()` before adding
 *    the BME280 device to the bus.
 * 7. Multi-byte reads are now performed as one transaction rather than
 *    separate single-byte reads.
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "bme280.h"
#include "math.h"
#include "esp_log.h"

/* Timeout used for each I2C master transaction, in milliseconds. */
#define BME280_I2C_TIMEOUT_MS 100

/* Default SCL frequency used when creating the BME280 device handle. */
#define BME280_I2C_SCL_SPEED_HZ 400000

bme280_handle_t bme280_create(i2c_master_bus_handle_t bus, uint8_t dev_addr)
{
    bme280_dev_t *sens = (bme280_dev_t *) calloc(1, sizeof(bme280_dev_t));
    if (sens == NULL) {
        return NULL;
    }

    /* CHANGED: Probe the 7-bit I2C address before creating the device handle.
     * i2c_master_probe() returns ESP_OK when the device ACKs its address. */
    esp_err_t ret = i2c_master_probe(bus, dev_addr, BME280_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        free(sens);
        return NULL;
    }

    /* CHANGED: Configure the device for the new I2C master driver. */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = BME280_I2C_SCL_SPEED_HZ,
    };

    /* CHANGED: Add the BME280 as an I2C master device. */
    ret = i2c_master_bus_add_device(bus, &dev_cfg, &sens->i2c_dev);
    if (ret != ESP_OK) {
        free(sens);
        return NULL;
    }

    sens->dev_addr = dev_addr;
    return (bme280_handle_t)sens;
}

esp_err_t bme280_delete(bme280_handle_t *sensor)
{
    if (sensor == NULL || *sensor == NULL) {
        return ESP_OK;
    }

    bme280_dev_t *sens = (bme280_dev_t *)(*sensor);

    /* CHANGED: Remove the I2C master device handle. */
    esp_err_t ret = i2c_master_bus_rm_device(sens->i2c_dev);
    free(sens);
    *sensor = NULL;

    return ret;
}

/* CHANGED: Read one register using a write-register-address/read-value
 * transaction. This replaces i2c_bus_read_byte(). */
static esp_err_t bme280_read_register(bme280_handle_t sensor, uint8_t addr, uint8_t *data)
{
    bme280_dev_t *sens = (bme280_dev_t *) sensor;

    return i2c_master_transmit_receive(
        sens->i2c_dev,
        &addr,
        sizeof(addr),
        data,
        sizeof(*data),
        BME280_I2C_TIMEOUT_MS
    );
}

/* CHANGED: Write one register using a register-address/value transaction.
 * This replaces i2c_bus_write_byte(). */
static esp_err_t bme280_write_register(bme280_handle_t sensor, uint8_t addr, uint8_t value)
{
    bme280_dev_t *sens = (bme280_dev_t *) sensor;
    uint8_t buffer[2] = { addr, value };

    return i2c_master_transmit(
        sens->i2c_dev,
        buffer,
        sizeof(buffer),
        BME280_I2C_TIMEOUT_MS
    );
}

/* CHANGED: Read `len` consecutive registers in one transaction. This replaces
 * repeated i2c_bus_read_byte() calls and i2c_bus_read_bytes(). */
static esp_err_t bme280_read_registers(bme280_handle_t sensor, uint8_t addr,
                                       uint8_t *data, size_t len)
{
    bme280_dev_t *sens = (bme280_dev_t *) sensor;

    return i2c_master_transmit_receive(
        sens->i2c_dev,
        &addr,
        sizeof(addr),
        data,
        len,
        BME280_I2C_TIMEOUT_MS
    );
}

/* CHANGED: Uses one two-byte transaction. The original code read each byte
 * separately and assembled it in big-endian order. */
static esp_err_t bme280_read_uint16(bme280_handle_t sensor, uint8_t addr, uint16_t *data)
{
    uint8_t buf[2];
    esp_err_t ret = bme280_read_registers(sensor, addr, buf, sizeof(buf));
    if (ret != ESP_OK) {
        return ret;
    }

    /* Preserve the original big-endian behavior. */
    *data = ((uint16_t)buf[0] << 8) | buf[1];
    return ESP_OK;
}

/* CHANGED: Uses one two-byte transaction. The original code read each byte
 * separately and assembled it in little-endian order. */
static esp_err_t bme280_read_uint16_le(bme280_handle_t sensor, uint8_t addr, uint16_t *data)
{
    uint8_t buf[2];
    esp_err_t ret = bme280_read_registers(sensor, addr, buf, sizeof(buf));
    if (ret != ESP_OK) {
        return ret;
    }

    /* Preserve the original little-endian behavior. */
    *data = ((uint16_t)buf[1] << 8) | buf[0];
    return ESP_OK;
}

unsigned int bme280_getconfig(bme280_handle_t sensor)
{
    bme280_dev_t *sens = (bme280_dev_t *) sensor;
    return (sens->config_t.t_sb << 5) | (sens->config_t.filter << 3) | sens->config_t.spi3w_en;
}

unsigned int bme280_getctrl_meas(bme280_handle_t sensor)
{
    bme280_dev_t *sens = (bme280_dev_t *) sensor;
    return (sens->ctrl_meas_t.osrs_t << 5) | (sens->ctrl_meas_t.osrs_p << 3) | sens->ctrl_meas_t.mode;
}

unsigned int bme280_getctrl_hum(bme280_handle_t sensor)
{
    bme280_dev_t *sens = (bme280_dev_t *) sensor;
    return (sens->ctrl_hum_t.osrs_h);
}

bool bme280_is_reading_calibration(bme280_handle_t sensor)
{
    uint8_t rstatus = 0;

    /* CHANGED: Legacy i2c_bus_read_byte() replaced with the master-driver
     * register-read helper. */
    if (bme280_read_register(sensor, BME280_REGISTER_STATUS, &rstatus) != ESP_OK) {
        return false;
    }

    return (rstatus & (1 << 0)) != 0;
}

esp_err_t bme280_read_coefficients(bme280_handle_t sensor)
{
    uint8_t data = 0;
    uint8_t data1 = 0;
    uint16_t data16 = 0;
    bme280_dev_t *sens = (bme280_dev_t *) sensor;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_T1, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_t1 = data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_T2, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_t2 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_T3, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_t3 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P1, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p1 = data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P2, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p2 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P3, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p3 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P4, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p4 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P5, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p5 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P6, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p6 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P7, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p7 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P8, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p8 = (int16_t) data16;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_P9, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_p9 = (int16_t) data16;

    /* CHANGED: Single-byte register reads now use i2c_master_transmit_receive(). */
    if (bme280_read_register(sensor, BME280_REGISTER_DIG_H1, &data) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_h1 = data;

    if (bme280_read_uint16_le(sensor, BME280_REGISTER_DIG_H2, &data16) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_h2 = (int16_t) data16;

    if (bme280_read_register(sensor, BME280_REGISTER_DIG_H3, &data) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_h3 = data;

    if (bme280_read_register(sensor, BME280_REGISTER_DIG_H4, &data) != ESP_OK) {
        return ESP_FAIL;
    }
    if (bme280_read_register(sensor, BME280_REGISTER_DIG_H4 + 1, &data1) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_h4 = (data << 4) | (data1 & 0xF);

    if (bme280_read_register(sensor, BME280_REGISTER_DIG_H5 + 1, &data) != ESP_OK) {
        return ESP_FAIL;
    }
    if (bme280_read_register(sensor, BME280_REGISTER_DIG_H5, &data1) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_h5 = (data << 4) | (data1 >> 4);

    if (bme280_read_register(sensor, BME280_REGISTER_DIG_H6, &data) != ESP_OK) {
        return ESP_FAIL;
    }
    sens->data_t.dig_h6 = (int8_t) data;

    return ESP_OK;
}

esp_err_t bme280_set_sampling(bme280_handle_t sensor, bme280_sensor_mode mode,
                              bme280_sensor_sampling tempSampling,
                              bme280_sensor_sampling pressSampling,
                              bme280_sensor_sampling humSampling,
                              bme280_sensor_filter filter,
                              bme280_standby_duration duration)
{
    bme280_dev_t *sens = (bme280_dev_t *) sensor;

    sens->ctrl_meas_t.mode = mode;
    sens->ctrl_meas_t.osrs_t = tempSampling;
    sens->ctrl_meas_t.osrs_p = pressSampling;
    sens->ctrl_hum_t.osrs_h = humSampling;
    sens->config_t.filter = filter;
    sens->config_t.t_sb = duration;

    /* You must also set REGISTER_CONTROL after setting CONTROLHUMID,
     * otherwise the humidity oversampling value is not applied (DS 5.4.3). */

    /* CHANGED: Legacy register writes replaced with i2c_master_transmit(). */
    if (bme280_write_register(sensor, BME280_REGISTER_CONTROLHUMID,
                              bme280_getctrl_hum(sensor)) != ESP_OK) {
        return ESP_FAIL;
    }

    if (bme280_write_register(sensor, BME280_REGISTER_CONFIG,
                              bme280_getconfig(sensor)) != ESP_OK) {
        return ESP_FAIL;
    }

    if (bme280_write_register(sensor, BME280_REGISTER_CONTROL,
                              bme280_getctrl_meas(sensor)) != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t bme280_default_init(bme280_handle_t sensor)
{
    uint8_t chipid = 0;

    /* Check that the sensor reports the expected chip ID. */
    if (bme280_read_register(sensor, BME280_REGISTER_CHIPID, &chipid) != ESP_OK) {
        ESP_LOGI("BME280:", "bme280_default_init->read CHIPID failed: 0x%x", chipid);
        return ESP_FAIL;
    }

    if (chipid != BME280_DEFAULT_CHIPID) {
        ESP_LOGI("BME280:", "Unexpected chip ID: 0x%x", chipid);
        return ESP_FAIL;
    }

    /* Soft-reset the sensor; this clears IIR filtering and other state. */
    if (bme280_write_register(sensor, BME280_REGISTER_SOFTRESET, 0xB6) != ESP_OK) {
        return ESP_FAIL;
    }

    /* Wait for the chip to wake up after reset. */
    vTaskDelay(300 / portTICK_PERIOD_MS);

    /* Wait until calibration data are available. */
    while (bme280_is_reading_calibration(sensor)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    /* Read trimming parameters (BME280 datasheet section 4.2.2). */
    if (bme280_read_coefficients(sensor) != ESP_OK) {
        return ESP_FAIL;
    }

    /* Apply default sampling configuration. */
    if (bme280_set_sampling(sensor,
                            BME280_MODE_NORMAL,
                            BME280_SAMPLING_X16,
                            BME280_SAMPLING_X16,
                            BME280_SAMPLING_X16,
                            BME280_FILTER_OFF,
                            BME280_STANDBY_MS_0_5) != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t bme280_take_forced_measurement(bme280_handle_t sensor)
{
    uint8_t data = 0;

    if (((bme280_dev_t *)sensor)->ctrl_meas_t.mode == BME280_MODE_FORCED) {
        /* Trigger the next forced measurement. */
        if (bme280_write_register(sensor, BME280_REGISTER_CONTROL,
                                  bme280_getctrl_meas(sensor)) != ESP_OK) {
            return ESP_FAIL;
        }

        /* Wait until the measuring bit clears; otherwise the previous
         * measurement could be read. */
        if (bme280_read_register(sensor, BME280_REGISTER_STATUS, &data) != ESP_OK) {
            return ESP_FAIL;
        }

        while (data & 0x08) {
            if (bme280_read_register(sensor, BME280_REGISTER_STATUS, &data) != ESP_OK) {
                return ESP_FAIL;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

    return ESP_OK;
}

esp_err_t bme280_read_temperature(bme280_handle_t sensor, float *temperature)
{
    int32_t var1, var2;
    uint8_t data[3] = { 0 };
    bme280_dev_t *sens = (bme280_dev_t *) sensor;

    /* CHANGED: One 3-byte register read transaction replaces
     * i2c_bus_read_bytes(). */
    if (bme280_read_registers(sensor, BME280_REGISTER_TEMPDATA,
                              data, sizeof(data)) != ESP_OK) {
        return ESP_FAIL;
    }

    int32_t adc_T = (data[0] << 16) | (data[1] << 8) | data[2];

    /* 0x800000 means temperature measurement was disabled. */
    if (adc_T == 0x800000) {
        return ESP_FAIL;
    }

    adc_T >>= 4;

    var1 = ((((adc_T >> 3) - ((int32_t) sens->data_t.dig_t1 << 1)))
            * ((int32_t) sens->data_t.dig_t2)) >> 11;

    var2 = (((((adc_T >> 4) - ((int32_t) sens->data_t.dig_t1))
              * ((adc_T >> 4) - ((int32_t) sens->data_t.dig_t1))) >> 12)
            * ((int32_t) sens->data_t.dig_t3)) >> 14;

    sens->t_fine = var1 + var2;
    *temperature = ((sens->t_fine * 5 + 128) >> 8) / 100.0;

    return ESP_OK;
}

esp_err_t bme280_read_pressure(bme280_handle_t sensor, float *pressure)
{
    int64_t var1, var2, p;
    uint8_t data[3] = { 0 };
    bme280_dev_t *sens = (bme280_dev_t *) sensor;
    float temp = 0.0;

    /* Temperature must be read first because pressure compensation uses t_fine. */
    if (bme280_read_temperature(sensor, &temp) != ESP_OK) {
        return ESP_FAIL;
    }

    /* CHANGED: One 3-byte register read transaction replaces
     * i2c_bus_read_bytes(). */
    if (bme280_read_registers(sensor, BME280_REGISTER_PRESSUREDATA,
                              data, sizeof(data)) != ESP_OK) {
        return ESP_FAIL;
    }

    int32_t adc_P = (data[0] << 16) | (data[1] << 8) | data[2];

    /* 0x800000 means pressure measurement was disabled. */
    if (adc_P == 0x800000) {
        return ESP_FAIL;
    }

    adc_P >>= 4;

    var1 = ((int64_t) sens->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t) sens->data_t.dig_p6;
    var2 = var2 + ((var1 * (int64_t) sens->data_t.dig_p5) << 17);
    var2 = var2 + (((int64_t) sens->data_t.dig_p4) << 35);
    var1 = ((var1 * var1 * (int64_t) sens->data_t.dig_p3) >> 8) +
           ((var1 * (int64_t) sens->data_t.dig_p2) << 12);
    var1 = (((((int64_t) 1) << 47) + var1)) * ((int64_t) sens->data_t.dig_p1) >> 33;

    if (var1 == 0) {
        return ESP_FAIL; /* Avoid division by zero. */
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t) sens->data_t.dig_p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t) sens->data_t.dig_p8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t) sens->data_t.dig_p7) << 4);
    p = p >> 8; /* Divide by 256. */

    *pressure = (float) p / 100;

    return ESP_OK;
}

esp_err_t bme280_read_humidity(bme280_handle_t sensor, float *humidity)
{
    uint16_t data16;
    bme280_dev_t *sens = (bme280_dev_t *) sensor;
    float temp = 0.0;

    /* Temperature must be read first because humidity compensation uses t_fine. */
    if (bme280_read_temperature(sensor, &temp) != ESP_OK) {
        return ESP_FAIL;
    }

    if (bme280_read_uint16(sensor, BME280_REGISTER_HUMIDDATA, &data16) != ESP_OK) {
        return ESP_FAIL;
    }

    int32_t adc_H = data16;

    /* 0x8000 means humidity measurement was disabled. */
    if (adc_H == 0x8000) {
        return ESP_FAIL;
    }

    int32_t v_x1_u32r;
    v_x1_u32r = (sens->t_fine - ((int32_t) 76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t) sens->data_t.dig_h4) << 20)
                    - (((int32_t) sens->data_t.dig_h5) * v_x1_u32r))
                   + ((int32_t) 16384)) >> 15)
                 * (((((((v_x1_u32r * ((int32_t) sens->data_t.dig_h6)) >> 10)
                        * (((v_x1_u32r * ((int32_t) sens->data_t.dig_h3)) >> 11) +
                           ((int32_t) 32768))) >> 10) + ((int32_t) 2097152))
                     * ((int32_t) sens->data_t.dig_h2) + 8192) >> 14));

    v_x1_u32r = (v_x1_u32r
                 - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
                     * ((int32_t) sens->data_t.dig_h1)) >> 4));

    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;

    *humidity = (v_x1_u32r >> 12) / 1024.0;

    return ESP_OK;
}

esp_err_t bme280_read_altitude(bme280_handle_t sensor, float seaLevel, float *altitude)
{
    float pressure = 0.0;
    float temp = 0.0;

    if (bme280_read_pressure(sensor, &temp) != ESP_OK) {
        return ESP_FAIL;
    }

    float atmospheric = pressure / 100.0F;
    *altitude = 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.1903));

    return ESP_OK;
}

esp_err_t bme280_calculates_pressure(bme280_handle_t sensor, float altitude,
                                     float atmospheric, float *pressure)
{
    *pressure = atmospheric / pow(1.0 - (altitude / 44330.0), 5.255);
    return ESP_OK;
}
