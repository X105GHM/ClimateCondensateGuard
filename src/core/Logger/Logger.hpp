#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <deque>
#include "core/Types.hpp"

struct LogEntry
{
    uint32_t timestampMs{};
    LogLevel level{LogLevel::Info};
    String component;
    String message;
};

class Logger
{
public:
    Logger();
    ~Logger();

    void begin();
    void debug(const String &component, const String &message);
    void info(const String &component, const String &message);
    void warn(const String &component, const String &message);
    void error(const String &component, const String &message);

    String latestAsJson(size_t limit) const;
    String latestAsText(size_t limit) const;

private:
    void push(LogLevel level, const String &component, const String &message);
    void appendToFile(const LogEntry &entry);
    String formatLine(const LogEntry &entry) const;

    mutable SemaphoreHandle_t mutex_{nullptr};
    std::deque<LogEntry> entries_;
    static constexpr size_t kMaxEntries = 300;
};
