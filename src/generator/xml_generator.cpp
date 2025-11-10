#include "generator/xml_generator.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <random>

namespace expocli {

XmlGenerator::XmlGenerator() {
}

void XmlGenerator::generateFiles(
    const XsdSchema& schema,
    int count,
    const std::string& destDir,
    const std::string& prefix
) {
    std::cout << "Generating " << count << " XML files..." << std::endl;

    for (int i = 0; i < count; ++i) {
        // Generate document
        pugi::xml_document doc = generateDocument(schema);

        // Create filename with zero-padded number
        std::ostringstream filename;
        filename << destDir << "/"
                 << prefix
                 << std::setfill('0') << std::setw(4) << (i + 1)
                 << ".xml";

        // Save document
        if (!doc.save_file(filename.str().c_str())) {
            std::cerr << "Error: Failed to save file " << filename.str() << std::endl;
            continue;
        }

        // Progress indicator (every 10%)
        if (count >= 10 && (i + 1) % (count / 10) == 0) {
            int percent = ((i + 1) * 100) / count;
            std::cout << "Progress: " << percent << "% (" << (i + 1) << "/" << count << ")" << std::endl;
        }
    }

    std::cout << "Successfully generated " << count << " XML files in " << destDir << std::endl;
}

pugi::xml_document XmlGenerator::generateDocument(const XsdSchema& schema) {
    pugi::xml_document doc;

    // Add XML declaration
    auto decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    // Generate root element
    auto root = schema.getRootElement();
    if (root) {
        auto rootNode = doc.append_child(root->name.c_str());

        // Generate children for complex type
        if (root->type == XsdType::COMPLEX) {
            for (const auto& child : root->children) {
                generateElement(rootNode, child);
            }
        } else {
            // Simple type root (rare but possible)
            rootNode.text().set(data_gen_.generateValue(root->type).c_str());
        }
    }

    return doc;
}

void XmlGenerator::generateElement(
    pugi::xml_node& parent,
    const std::shared_ptr<XsdElement>& element
) {
    // Determine how many times to create this element
    int count = determineRepeatCount(element);

    for (int i = 0; i < count; ++i) {
        auto node = parent.append_child(element->name.c_str());

        if (element->type == XsdType::COMPLEX) {
            // Complex type - generate children
            for (const auto& child : element->children) {
                generateElement(node, child);
            }
        } else {
            // Simple type - generate value
            std::string value = data_gen_.generateValue(element->type);
            node.text().set(value.c_str());
        }
    }
}

int XmlGenerator::determineRepeatCount(const std::shared_ptr<XsdElement>& element) {
    // If element is optional (minOccurs=0), randomly decide whether to include it
    if (element->isOptional()) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> include_dist(0, 1);

        // 70% chance to include optional elements
        if (include_dist(gen) < 0.7) {
            return 0;  // Skip this element
        }
    }

    // If not repeatable, return minOccurs (usually 1)
    if (!element->isRepeatable()) {
        return element->minOccurs > 0 ? element->minOccurs : 1;
    }

    // For repeatable elements, generate a random count between minOccurs and maxOccurs
    static std::random_device rd;
    static std::mt19937 gen(rd());

    int minCount = element->minOccurs;
    int maxCount = element->maxOccurs;

    // For unbounded, use a reasonable limit (1-10)
    if (element->isUnbounded()) {
        maxCount = 10;
    }

    // Ensure minCount is at least 1 if not optional
    if (minCount == 0) {
        minCount = 1;
    }

    // Cap maxCount at a reasonable number
    if (maxCount > 20) {
        maxCount = 20;
    }

    if (minCount >= maxCount) {
        return minCount;
    }

    std::uniform_int_distribution<> count_dist(minCount, maxCount);
    return count_dist(gen);
}

} // namespace expocli
