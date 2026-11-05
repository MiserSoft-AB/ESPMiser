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
    esp_err_t init() override { return ESP_OK; }
    bool is_ready() override { return true; }

    esp_err_t set_backlight(bool enabled) override
    {
        backlight_enabled = enabled;
        ++backlight_calls;
        return ESP_OK;
    }

    bool backlight_enabled = false;
    unsigned int backlight_calls = 0;
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

void test_display_backlight()
{
    FakeDisplay display;
    constexpr bool expected_states[] = { false, true, false };
    std::uint32_t frame = 0;

    for (const bool expected : expected_states)
    {
        const auto result = update_display_backlight(display, frame);

        assert(result == ESP_OK);
        assert(display.backlight_calls == frame + 1);
        assert(display.backlight_enabled == expected);

        ++frame;
    }
}

int main()
{
    test_sensor_update();
    test_display_backlight();

    std::puts("sensor and display tests passed");
    return 0;
}