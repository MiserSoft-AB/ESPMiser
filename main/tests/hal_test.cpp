#include "sensors_update.hpp"
#include "display_update.hpp"

#include <cassert>
#include <cstdio>

class FakeSensor final : public ISensor
{
public:
    explicit FakeSensor(esp_err_t result) : result_(result) {}

    esp_err_t init() override
    {
        return ESP_OK;
    }

    esp_err_t read(SensorReadings& readings) override
    {
        if (result_ == ESP_OK)
        {
            readings = { 22.5f, 45.0f, 1013.0f };
        }
        return result_;
    }

private:
    esp_err_t result_;
};

class FakeDisplay final : public IDisplay
{
public:
    explicit FakeDisplay(esp_err_t init_result = ESP_OK)
        : init_result_(init_result)
    {
    }

    esp_err_t init() override
    {
        ++init_calls;
        ready_ = init_result_ == ESP_OK;
        return init_result_;
    }

    bool is_ready() override
    {
        return ready_;
    }

    esp_err_t set_backlight(bool) override
    {
        return ESP_OK;
    }

    unsigned int init_calls = 0;

private:
    esp_err_t init_result_;
    bool ready_ = false;
};

void test_sensor_update()
{
    FakeSensor working_sensor{ESP_OK};
    const SensorsUpdate success = read_sensor_update(working_sensor);

    assert(success.should_publish(true));
    assert(success.readings.temperature == 22.5f);
    assert(success.readings.humidity == 45.0f);
    assert(success.readings.pressure == 1013.0f);
    assert(!success.should_publish(false));

    FakeSensor failing_sensor{ESP_FAIL};
    const SensorsUpdate failure = read_sensor_update(failing_sensor);

    assert(failure.read_result == ESP_FAIL);
    assert(!failure.should_publish(true));
}

void test_display_initialization()
{
    FakeDisplay display;

    const auto first_result = initialize_display(display);
    assert(first_result == ESP_OK);
    assert(display.is_ready());
    assert(display.init_calls == 1);

    const auto second_result = initialize_display(display);
    assert(second_result == ESP_OK);
    assert(display.init_calls == 1);

    FakeDisplay failing_display{ESP_FAIL};

    const auto failure_result = initialize_display(failing_display);
    assert(failure_result == ESP_FAIL);
    assert(!failing_display.is_ready());
    assert(failing_display.init_calls == 1);
}

int main()
{
    test_sensor_update();
    test_display_initialization();

    std::puts("sensor and display tests passed");
    return 0;
}