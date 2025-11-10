#ifndef XML_GENERATOR_H
#define XML_GENERATOR_H

#include "xsd_schema.h"
#include "data_generator.h"
#include <pugixml.hpp>
#include <string>
#include <memory>

namespace expocli {

class XmlGenerator {
public:
    XmlGenerator();

    // Generate XML instances from schema
    void generateFiles(
        const XsdSchema& schema,
        int count,
        const std::string& destDir,
        const std::string& prefix = "generated_"
    );

private:
    DataGenerator data_gen_;

    // Generate a single XML document
    pugi::xml_document generateDocument(const XsdSchema& schema);

    // Generate an element based on schema definition
    void generateElement(
        pugi::xml_node& parent,
        const std::shared_ptr<XsdElement>& element
    );

    // Determine how many times to repeat an element (for maxOccurs > 1)
    int determineRepeatCount(const std::shared_ptr<XsdElement>& element);
};

} // namespace expocli

#endif // XML_GENERATOR_H
