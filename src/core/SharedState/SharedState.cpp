#include "SharedState.hpp"

SharedState::SharedState()
{
    mutex_ = xSemaphoreCreateMutex();
}

SharedState::~SharedState()
{
    if (mutex_ != nullptr)
    {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
}

SystemStateData SharedState::snapshot() const
{
    SystemStateData copy{};
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE)
    {
        copy = data_;
        xSemaphoreGive(mutex_);
    }
    return copy;
}

void SharedState::update(const std::function<void(SystemStateData &)> &fn)
{
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE)
    {
        fn(data_);
        xSemaphoreGive(mutex_);
    }
}
