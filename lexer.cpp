#include "lexer.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& input) : input(input), pos(0), line(1), column(1) {
    initKeywords();
    initTransitions();
}

void Lexer::initKeywords() {
    keywords = {
        "int", "float", "char", "if", "else", "while", "for", 
        "return", "void", "struct", "input", "output",
        
        // Ключевые слова для работы с массивами согласно лекции
        "read", "write", "mem1", "mem2"
    };
}

void Lexer::initTransitions() {
    // Реализация таблицы переходов согласно 115.md
    using LS = LexState;
    using CC = CharCategory;
    
    // Состояние S (начальное)
    transitions[{LS::S, CC::LETTER}] = {LS::ID, 1};      // ID/1
    transitions[{LS::S, CC::DIGIT}] = {LS::NUM, 2};      // NUM/2
    transitions[{LS::S, CC::PLUS}] = {LS::OP, 3};        // OP/3
    transitions[{LS::S, CC::MINUS}] = {LS::OP, 4};       // OP/4
    transitions[{LS::S, CC::STAR}] = {LS::OP, 5};        // OP/5
    transitions[{LS::S, CC::SLASH}] = {LS::OP, 6};       // OP/6
    transitions[{LS::S, CC::PERCENT}] = {LS::OP, 7};     // OP/7
    transitions[{LS::S, CC::EQUAL}] = {LS::OP, 8};       // OP/8
    transitions[{LS::S, CC::LT}] = {LS::OP, 9};          // OP/9
    transitions[{LS::S, CC::GT}] = {LS::OP, 10};         // OP/10
    transitions[{LS::S, CC::EXCL}] = {LS::OP, 11};       // OP/11
    transitions[{LS::S, CC::AMP}] = {LS::OP, 12};        // OP/12
    transitions[{LS::S, CC::PIPE}] = {LS::OP, 13};       // OP/13
    transitions[{LS::S, CC::SEMICOLON}] = {LS::FIN, 14}; // FIN/14
    transitions[{LS::S, CC::COMMA}] = {LS::FIN, 15};     // FIN/15
    transitions[{LS::S, CC::DOT}] = {LS::OP, 16};        // OP/16
    transitions[{LS::S, CC::LPAREN}] = {LS::FIN, 17};    // FIN/17
    transitions[{LS::S, CC::RPAREN}] = {LS::FIN, 18};    // FIN/18
    transitions[{LS::S, CC::LBRACE}] = {LS::FIN, 19};    // FIN/19
    transitions[{LS::S, CC::RBRACE}] = {LS::FIN, 20};    // FIN/20
    transitions[{LS::S, CC::LBRACKET}] = {LS::FIN, 50};  // FIN/50
    transitions[{LS::S, CC::RBRACKET}] = {LS::FIN, 51};  // FIN/51
    transitions[{LS::S, CC::QUOTE}] = {LS::STR, 21};     // STR/21
    transitions[{LS::S, CC::SQUOTE}] = {LS::STR, 22};    // STR/22
    transitions[{LS::S, CC::SPACE}] = {LS::S, 23};       // S/23
    transitions[{LS::S, CC::NEWLINE}] = {LS::S, 24};     // S/24
    transitions[{LS::S, CC::OTHER}] = {LS::ERR, 25};     // ERR/25
    transitions[{LS::S, CC::END_OF_FILE}] = {LS::FIN, 26}; // FIN/26
    
    // Состояние ID (идентификаторы)
    transitions[{LS::ID, CC::LETTER}] = {LS::ID, 27};    // ID/27
    transitions[{LS::ID, CC::DIGIT}] = {LS::ID, 27};     // ID/27
    // Все остальные символы ведут к FIN/28
    for (CC cat : {CC::PLUS, CC::MINUS, CC::STAR, CC::SLASH, CC::PERCENT, CC::EQUAL, 
                   CC::LT, CC::GT, CC::EXCL, CC::AMP, CC::PIPE, CC::SEMICOLON, 
                   CC::COMMA, CC::DOT, CC::LPAREN, CC::RPAREN, CC::LBRACE, CC::RBRACE,
                   CC::LBRACKET, CC::RBRACKET, CC::QUOTE, CC::SQUOTE, CC::SPACE, 
                   CC::NEWLINE, CC::END_OF_FILE}) {
        transitions[{LS::ID, cat}] = {LS::FIN, 28};
    }
    transitions[{LS::ID, CC::OTHER}] = {LS::ERR, 25};
    
    // Состояние NUM (числа)
    transitions[{LS::NUM, CC::LETTER}] = {LS::ERR, 25};  // ERR/25
    transitions[{LS::NUM, CC::DIGIT}] = {LS::NUM, 29};   // NUM/29
    transitions[{LS::NUM, CC::DOT}] = {LS::NUM, 31};     // NUM/31 (десятичная точка)
    // Все остальные символы ведут к FIN/30
    for (CC cat : {CC::PLUS, CC::MINUS, CC::STAR, CC::SLASH, CC::PERCENT, CC::EQUAL, 
                   CC::LT, CC::GT, CC::EXCL, CC::AMP, CC::PIPE, CC::SEMICOLON, 
                   CC::COMMA, CC::LPAREN, CC::RPAREN, CC::LBRACE, CC::RBRACE,
                   CC::LBRACKET, CC::RBRACKET, CC::QUOTE, CC::SQUOTE, CC::SPACE, 
                   CC::NEWLINE, CC::END_OF_FILE}) {
        transitions[{LS::NUM, cat}] = {LS::FIN, 30};
    }
    transitions[{LS::NUM, CC::OTHER}] = {LS::ERR, 25};
    
    // Состояние STR (строки)
    for (CC cat : {CC::LETTER, CC::DIGIT, CC::PLUS, CC::MINUS, CC::STAR, CC::SLASH, 
                   CC::PERCENT, CC::EQUAL, CC::LT, CC::GT, CC::EXCL, CC::AMP, CC::PIPE, 
                   CC::SEMICOLON, CC::COMMA, CC::DOT, CC::LPAREN, CC::RPAREN, 
                   CC::LBRACE, CC::RBRACE, CC::LBRACKET, CC::RBRACKET, CC::SPACE, 
                   CC::NEWLINE, CC::OTHER}) {
        transitions[{LS::STR, cat}] = {LS::STR, 32}; // STR/32
    }
    transitions[{LS::STR, CC::QUOTE}] = {LS::FIN, 33};   // FIN/33
    transitions[{LS::STR, CC::SQUOTE}] = {LS::FIN, 34};  // FIN/34
    transitions[{LS::STR, CC::END_OF_FILE}] = {LS::ERR, 25};
    
    // Состояние OP (операторы)
    transitions[{LS::OP, CC::EQUAL}] = {LS::OP, 41};     // OP/41 для ==
    transitions[{LS::OP, CC::EXCL}] = {LS::OP, 44};     // OP/44 для !=
    transitions[{LS::OP, CC::AMP}] = {LS::OP, 45};      // OP/45 для &&
    transitions[{LS::OP, CC::PIPE}] = {LS::OP, 46};     // OP/46 для ||
    transitions[{LS::OP, CC::SLASH}] = {LS::COM, -1};   // COM для комментариев
    // Все остальные символы ведут к FIN/35
    for (CC cat : {CC::LETTER, CC::DIGIT, CC::PLUS, CC::MINUS, CC::STAR, CC::PERCENT, 
                   CC::LT, CC::GT, CC::SEMICOLON, CC::COMMA, CC::DOT, CC::LPAREN, 
                   CC::RPAREN, CC::LBRACE, CC::RBRACE, CC::LBRACKET, CC::RBRACKET, 
                   CC::QUOTE, CC::SQUOTE, CC::SPACE, CC::NEWLINE, CC::END_OF_FILE}) {
        transitions[{LS::OP, cat}] = {LS::FIN, 35};
    }
    transitions[{LS::OP, CC::OTHER}] = {LS::ERR, 25};
    
    // Состояние COM (комментарии)
    for (CC cat : {CC::LETTER, CC::DIGIT, CC::PLUS, CC::MINUS, CC::PERCENT, CC::EQUAL, 
                   CC::LT, CC::GT, CC::EXCL, CC::AMP, CC::PIPE, CC::SEMICOLON, 
                   CC::COMMA, CC::DOT, CC::LPAREN, CC::RPAREN, CC::LBRACE, CC::RBRACE,
                   CC::LBRACKET, CC::RBRACKET, CC::QUOTE, CC::SQUOTE, CC::SPACE, 
                   CC::OTHER, CC::SLASH}) {
        transitions[{LS::COM, cat}] = {LS::COM, 47}; // COM/47
    }
    transitions[{LS::COM, CC::STAR}] = {LS::COM, 48};    // COM/48
    transitions[{LS::COM, CC::NEWLINE}] = {LS::COM, 49}; // COM/49
    transitions[{LS::COM, CC::END_OF_FILE}] = {LS::ERR, 25};
}

CharCategory Lexer::getCharCategory(char c) const {
    if (std::isalpha(c) || c == '_') return CharCategory::LETTER;
    if (std::isdigit(c)) return CharCategory::DIGIT;
    
    switch (c) {
        case '+': return CharCategory::PLUS;
        case '-': return CharCategory::MINUS;
        case '*': return CharCategory::STAR;
        case '/': return CharCategory::SLASH;
        case '%': return CharCategory::PERCENT;
        case '=': return CharCategory::EQUAL;
        case '<': return CharCategory::LT;
        case '>': return CharCategory::GT;
        case '!': return CharCategory::EXCL;
        case '&': return CharCategory::AMP;
        case '|': return CharCategory::PIPE;
        case ';': return CharCategory::SEMICOLON;
        case ',': return CharCategory::COMMA;
        case '.': return CharCategory::DOT;
        case '(': return CharCategory::LPAREN;
        case ')': return CharCategory::RPAREN;
        case '{': return CharCategory::LBRACE;
        case '}': return CharCategory::RBRACE;
        case '[': return CharCategory::LBRACKET;
        case ']': return CharCategory::RBRACKET;
        case '"': return CharCategory::QUOTE;
        case '\'': return CharCategory::SQUOTE;
        case ' ': case '\t': case '\r': return CharCategory::SPACE;
        case '\n': return CharCategory::NEWLINE;
        case '\0': return CharCategory::END_OF_FILE;
        default: return CharCategory::OTHER;
    }
}

std::string Lexer::getTokenType(int action, const std::string& lexeme) const {
    switch (action) {
        case 1: case 27: case 28: // ID actions
            return keywords.count(lexeme) ? "KEYWORD" : "IDENTIFIER";
        case 2: case 29: case 30: case 31: // NUM actions
            return "NUMBER";
        case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:
        case 11: case 12: case 13: case 35: case 40: case 41: case 44: case 45: case 46:
            return "OPERATOR";
        case 14: return "SEMICOLON";
        case 15: return "COMMA";
        case 16: return "DOT";
        case 17: return "LEFT_PAREN";
        case 18: return "RIGHT_PAREN";
        case 19: return "LEFT_BRACE";
        case 20: return "RIGHT_BRACE";
        case 50: return "LEFT_BRACKET";
        case 51: return "RIGHT_BRACKET";
        case 21: case 33: return "STRING";
        case 22: case 34: return "CHAR";
        case 26: return "EOF";
        default: return "UNKNOWN";
    }
}

char Lexer::getCurrentChar() const {
    return pos < input.length() ? input[pos] : '\0';
}

char Lexer::peekChar(int offset) const {
    size_t peek_pos = pos + offset;
    return peek_pos < input.length() ? input[peek_pos] : '\0';
}

void Lexer::advance() {
    if (pos < input.length()) {
        if (input[pos] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        pos++;
    }
}

void Lexer::skipWhitespace() {
    while (pos < input.length()) {
        char c = input[pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    LexState currentState = LexState::S;
    std::string currentLexeme = "";
    int startLine = line;
    int startColumn = column;
    
    while (pos < input.length()) {
        char c = getCurrentChar();
        CharCategory category = getCharCategory(c);
        
        // Пропуск пробелов в начальном состоянии
        if (currentState == LexState::S && (category == CharCategory::SPACE)) {
            skipWhitespace();
            continue;
        }
        
        // Обработка переводов строк в начальном состоянии
        if (currentState == LexState::S && category == CharCategory::NEWLINE) {
            advance();
            continue;
        }
        
        auto transition = transitions.find({currentState, category});
        if (transition == transitions.end()) {
            // Нет перехода - ошибка
            throw std::runtime_error("Lexical error at line " + std::to_string(line) + 
                                   ", column " + std::to_string(column) + 
                                   ": unexpected character '" + c + "' (ASCII: " + std::to_string((int)c) + ")");
        }
        
        LexState nextState = transition->second.first;
        int action = transition->second.second;
        
        // Если переходим в FIN - завершаем токен
        if (nextState == LexState::FIN) {
            if (currentState == LexState::S) {
                // Односимвольный токен
                currentLexeme = c;
                advance();
            }
            // Если не в S состоянии, не потребляем символ - он будет обработан в следующей итерации
            
            std::string tokenType = getTokenType(action, currentLexeme);
            if (tokenType != "UNKNOWN" && action != 26) { // не EOF
                tokens.emplace_back(tokenType, currentLexeme, startLine, startColumn);
            }
            
            if (action == 26) { // EOF
                tokens.emplace_back("EOF", "", line, column);
                break;
            }
            
            // Сброс состояния
            currentState = LexState::S;
            currentLexeme = "";
            startLine = line;
            startColumn = column;
        }
        else if (nextState == LexState::ERR) {
            throw std::runtime_error("Lexical error at line " + std::to_string(line) + 
                                   ", column " + std::to_string(column) + 
                                   ": invalid token '" + currentLexeme + c + "'");
        }
        else {
            // Продолжаем накопление
            if (currentState == LexState::S) {
                startLine = line;
                startColumn = column;
            }
            currentLexeme += c;
            currentState = nextState;
            advance();
        }
    }
    
    // Завершаем последний токен если остался незавершенным
    if (!currentLexeme.empty() && currentState != LexState::S) {
        // Попробуем завершить токен принудительно
        auto finTransition = transitions.find({currentState, CharCategory::END_OF_FILE});
        if (finTransition != transitions.end() && finTransition->second.first == LexState::FIN) {
            std::string tokenType = getTokenType(finTransition->second.second, currentLexeme);
            if (tokenType != "UNKNOWN") {
                tokens.emplace_back(tokenType, currentLexeme, startLine, startColumn);
            }
        }
    }
    
    // Добавляем EOF если его еще нет
    if (tokens.empty() || tokens.back().getType() != "EOF") {
        tokens.emplace_back("EOF", "", line, column);
    }
    
    return tokens;
} 