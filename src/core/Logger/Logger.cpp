#include "Logger.hpp"

Logger::Logger()
{
    mutex_ = xSemaphoreCreateMutex();
}

Logger::~Logger()
{
    if (mutex_ != nullptr)
    {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
}

void Logger::begin()
{
    info("logger", "Logger online");
}

void Logger::debug(const String &component, const String &message)
{
    push(LogLevel::Debug, component, message);
}

void Logger::info(const String &component, const String &message)
{
    push(LogLevel::Info, component, message);
}

void Logger::warn(const String &component, const String &message)
{
    push(LogLevel::Warning, component, message);
}

void Logger::error(const String &component, const String &message)
{
    push(LogLevel::Error, component, message);
}

String Logger::latestAsJson(size_t limit) const
{
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE)
    {
        return "[]";
    }

    if (limit == 0U || limit > entries_.size())
    {
        limit = entries_.size();
    }

    String json = "[";
    const size_t start = entries_.size() > limit ? entries_.size() - limit : 0U;
    bool first = true;

    for (size_t i = start; i < entries_.size(); ++i)
    {
        const auto &entry = entries_[i];
        if (!first)
        {
            json += ",";
        }
        first = false;

        String msg = entry.message;
        msg.replace("\\", "\\\\");
        msg.replace("\"", "\\\"");
        msg.replace("\n", "\\n");

        String component = entry.component;
        component.replace("\\", "\\\\");
        component.replace("\"", "\\\"");

        json += "{";
        json += "\"ts_ms\":" + String(entry.timestampMs) + ",";
        json += "\"level\":\"" + toString(entry.level) + "\",";
        json += "\"component\":\"" + component + "\",";
        json += "\"message\":\"" + msg + "\"";
        json += "}";
    }

    json += "]";
    xSemaphoreGive(mutex_);
    return json;
}

String Logger::latestAsText(size_t limit) const
{
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE)
    {
        return {};
    }

    if (limit == 0U || limit > entries_.size())
    {
        limit = entries_.size();
    }

    String text;
    const size_t start = entries_.size() > limit ? entries_.size() - limit : 0U;
    for (size_t i = start; i < entries_.size(); ++i)
    {
        text += formatLine(entries_[i]);
        text += "\n";
    }

    xSemaphoreGive(mutex_);
    return text;
}

void Logger::push(LogLevel level, const String &component, const String &message)
{
    const LogEntry entry{millis(), level, component, message};

    Serial.println(formatLine(entry));

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    entries_.push_back(entry);
    while (entries_.size() > kMaxEntries)
    {
        entries_.pop_front();
    }

    appendToFile(entry);
    xSemaphoreGive(mutex_);
}

void Logger::appendToFile(const LogEntry &entry)
{
    File file = LittleFS.open("/events.log", FILE_APPEND);
    if (!file)
    {
        return;
    }
    file.println(formatLine(entry));
    file.close();
}

String Logger::formatLine(const LogEntry &entry) const
{
    String line;
    line.reserve(96);
    line += "[";
    line += entry.timestampMs;
    line += "][";
    line += toString(entry.level);
    line += "][";
    line += entry.component;
    line += "] ";
    line += entry.message;
    return line;
}
