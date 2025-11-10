#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "xsd_schema.h"
#include <string>
#include <random>

namespace expocli {

class DataGenerator {
public:
    DataGenerator();

    // Generate random data based on XSD type
    std::string generateValue(XsdType type);

    // Generate specific data types
    std::string generateString(int minLength = 5, int maxLength = 20);
    std::string generateInteger(int min = 1, int max = 1000);
    std::string generateDecimal(double min = 1.0, double max = 1000.0);
    std::string generateBoolean();
    std::string generateDate();
    std::string generateDateTime();

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> char_dist_;

    // Helper data for realistic values
    static const std::vector<std::string> sample_words_;
    static const std::vector<std::string> sample_names_;
};

} // namespace expocli

#endif // DATA_GENERATOR_H
