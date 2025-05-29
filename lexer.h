#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

// Класс для представления токена
class Token {
public:
    Token(const std::string& type, const std::string& value, int line, int column)
        : type(type), value(value), line(line), column(column) {}
    
    std::string getType() const { return type; }
    std::string getValue() const { return value; }
    int getLine() const { return line; }
    int getColumn() const { return column; }

private:
    std::string type;
    std::string value;
    int line;
    int column;
};

// Состояния автомата согласно таблице в 115.md
enum class LexState {
    S,      // начальное
    ID,     // идентификаторы и ключевые слова
    NUM,    // числа
    STR,    // строковые литералы
    COM,    // комментарии
    OP,     // операторы
    ERR,    // ошибка
    FIN     // конечное
};

// Категории символов
enum class CharCategory {
    LETTER,     // буква
    DIGIT,      // цифра
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %
    EQUAL,      // =
    LT,         // <
    GT,         // >
    EXCL,       // !
    AMP,        // &
    PIPE,       // |
    SEMICOLON,  // ;
    COMMA,      // ,
    DOT,        // .
    COLON,      // :
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    LBRACKET,   // [
    RBRACKET,   // ]
    QUOTE,      // "
    SQUOTE,     // '
    SPACE,      // пробел
    NEWLINE,    // \n
    END_OF_FILE,// EOF
    OTHER       // другие символы
};

// Хеш-функция для пары состояний - определяем ДО использования
struct StateTransitionHash {
    size_t operator()(const std::pair<LexState, CharCategory>& p) const {
        return std::hash<int>{}(static_cast<int>(p.first)) ^ 
               (std::hash<int>{}(static_cast<int>(p.second)) << 1);
    }
};

// Лексический анализатор
class Lexer {
public:
    Lexer(const std::string& input);
    
    // Основной метод - токенизация входной строки
    std::vector<Token> tokenize();

private:
    std::string input;
    size_t pos;
    int line;
    int column;
    
    // Ключевые слова языка
    std::unordered_set<std::string> keywords;
    
    // Таблица переходов автомата - используем свою хеш-функцию
    std::unordered_map<std::pair<LexState, CharCategory>, std::pair<LexState, int>, StateTransitionHash> transitions;
    
    // Вспомогательные методы
    void initKeywords();
    void initTransitions();
    CharCategory getCharCategory(char c) const;
    std::string getTokenType(int action, const std::string& lexeme) const;
    char getCurrentChar() const;
    char peekChar(int offset = 1) const;
    void advance();
    void skipWhitespace();
};

#endif // LEXER_H