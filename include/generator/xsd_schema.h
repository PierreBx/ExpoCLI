#ifndef XSD_SCHEMA_H
#define XSD_SCHEMA_H

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace expocli {

// Represents basic XSD data types
enum class XsdType {
    STRING,
    INTEGER,
    DECIMAL,
    BOOLEAN,
    DATE,
    DATETIME,
    COMPLEX  // For elements with child elements
};

// Represents an XSD element
struct XsdElement {
    std::string name;
    XsdType type;
    int minOccurs = 1;  // Default is required
    int maxOccurs = 1;  // Default is single occurrence (-1 means unbounded)

    // For complex types
    std::vector<std::shared_ptr<XsdElement>> children;

    // For simple types with restrictions
    std::string pattern;  // Regex pattern if specified
    int minLength = 0;
    int maxLength = -1;  // -1 means no limit

    bool isOptional() const { return minOccurs == 0; }
    bool isUnbounded() const { return maxOccurs == -1; }
    bool isRepeatable() const { return maxOccurs > 1 || maxOccurs == -1; }
};

// Represents an XSD schema
class XsdSchema {
public:
    XsdSchema() = default;

    // Root element of the schema
    void setRootElement(std::shared_ptr<XsdElement> root);
    std::shared_ptr<XsdElement> getRootElement() const;

    // Schema namespace
    void setTargetNamespace(const std::string& ns);
    std::string getTargetNamespace() const;

private:
    std::shared_ptr<XsdElement> root_element_;
    std::string target_namespace_;
};

} // namespace expocli

#endif // XSD_SCHEMA_H
