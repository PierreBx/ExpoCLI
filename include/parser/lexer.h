#ifndef LEXER_H
#define LEXER_H

#include "ast.h"
#include <string>
#include <vector>

namespace xmlquery {

class Lexer {
public:
    explicit Lexer(const std::string& input);

    // Tokenize the entire input
    std::vector<Token> tokenize();

private:
    std::string input_;
    size_t position_;

    // Helper methods
    char peek() const;
    char advance();
    void skipWhitespace();
    bool isAtEnd() const;

    // Token recognition
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readString();
    TokenType identifyKeyword(const std::string& word) const;
};

} // namespace xmlquery

#endif // LEXER_H
