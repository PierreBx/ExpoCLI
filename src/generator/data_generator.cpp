#include "generator/data_generator.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace expocli {

const std::vector<std::string> DataGenerator::sample_words_ = {
    "Product", "Item", "Service", "Widget", "Gadget", "Tool", "Device",
    "Component", "Module", "System", "Package", "Bundle", "Kit", "Set"
};

const std::vector<std::string> DataGenerator::sample_names_ = {
    "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta",
    "Premium", "Standard", "Basic", "Pro", "Plus", "Ultra", "Max", "Lite"
};

DataGenerator::DataGenerator()
    : rng_(std::random_device{}()),
      char_dist_(97, 122)  // lowercase a-z
{
}

std::string DataGenerator::generateValue(XsdType type) {
    switch (type) {
        case XsdType::STRING:
            return generateString();
        case XsdType::INTEGER:
            return generateInteger();
        case XsdType::DECIMAL:
            return generateDecimal();
        case XsdType::BOOLEAN:
            return generateBoolean();
        case XsdType::DATE:
            return generateDate();
        case XsdType::DATETIME:
            return generateDateTime();
        default:
            return generateString();
    }
}

std::string DataGenerator::generateString(int minLength, int maxLength) {
    // Mix of using sample data and random strings
    std::uniform_int_distribution<int> choice_dist(0, 2);
    int choice = choice_dist(rng_);

    if (choice == 0 && !sample_words_.empty()) {
        // Use a sample word
        std::uniform_int_distribution<size_t> word_dist(0, sample_words_.size() - 1);
        return sample_words_[word_dist(rng_)];
    } else if (choice == 1 && !sample_names_.empty()) {
        // Use a sample name
        std::uniform_int_distribution<size_t> name_dist(0, sample_names_.size() - 1);
        return sample_names_[name_dist(rng_)];
    } else {
        // Generate random string
        std::uniform_int_distribution<int> len_dist(minLength, maxLength);
        int length = len_dist(rng_);

        std::string result;
        result.reserve(length);

        for (int i = 0; i < length; ++i) {
            result += static_cast<char>(char_dist_(rng_));
        }

        // Capitalize first letter
        if (!result.empty()) {
            result[0] = static_cast<char>(std::toupper(result[0]));
        }

        return result;
    }
}

std::string DataGenerator::generateInteger(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return std::to_string(dist(rng_));
}

std::string DataGenerator::generateDecimal(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << dist(rng_);
    return oss.str();
}

std::string DataGenerator::generateBoolean() {
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(rng_) ? "true" : "false";
}

std::string DataGenerator::generateDate() {
    // Generate a random date in the past 5 years
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    std::uniform_int_distribution<int> days_dist(0, 365 * 5);
    int days_ago = days_dist(rng_);

    auto past = now - std::chrono::hours(24 * days_ago);
    auto past_time_t = std::chrono::system_clock::to_time_t(past);

    std::tm tm;
    #ifdef _WIN32
        localtime_s(&tm, &past_time_t);
    #else
        localtime_r(&past_time_t, &tm);
    #endif

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm.tm_year + 1900) << "-"
        << std::setw(2) << (tm.tm_mon + 1) << "-"
        << std::setw(2) << tm.tm_mday;

    return oss.str();
}

std::string DataGenerator::generateDateTime() {
    // Similar to date but with time component
    auto now = std::chrono::system_clock::now();

    std::uniform_int_distribution<int> days_dist(0, 365 * 5);
    int days_ago = days_dist(rng_);

    auto past = now - std::chrono::hours(24 * days_ago);
    auto past_time_t = std::chrono::system_clock::to_time_t(past);

    std::tm tm;
    #ifdef _WIN32
        localtime_s(&tm, &past_time_t);
    #else
        localtime_r(&past_time_t, &tm);
    #endif

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm.tm_year + 1900) << "-"
        << std::setw(2) << (tm.tm_mon + 1) << "-"
        << std::setw(2) << tm.tm_mday << "T"
        << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min << ":"
        << std::setw(2) << tm.tm_sec;

    return oss.str();
}

} // namespace expocli
