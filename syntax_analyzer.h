#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include <string>
#include <vector>
#include <memory>
#include "stack_machine.h"
#include "lexer.h"

// Forward declaration of OPSCommand and OPSGenerator
struct OPSCommand;
class OPSGenerator;

// Основной класс синтаксического анализатора
class SyntaxAnalyzer {
public:
    SyntaxAnalyzer();
    
    // Анализ последовательности токенов
    std::vector<OPSCommand> analyze(const std::vector<Token>& tokens);
    
    // Получить сгенерированный код ОПС
    std::vector<OPSCommand> getOPSCode() const;
    
    // Вывод сгенерированного кода
    void printOPSCode() const;

private:
    StackMachine stackMachine;
    std::unique_ptr<OPSGenerator> opsGenerator;
    std::vector<Token> tokens;
    size_t currentToken;
    std::vector<std::string> opsCode;
    int labelCounter;
    
    // Вспомогательные методы
    void processToken(const Token& token);
    InputSymbol convertTokenType(const std::string& type) const;
    void error(const std::string& message, const Token& token) const;
    
    // Новые методы парсинга
    void parseProgram();
    void parseStatement();
    void parseDeclaration();
    void parseAssignment();
    void parseIfStatement();
    void parseWhileStatement();
    void parseExpression();
    int getPriority(const std::string& op);
    void parseSimpleExpression();  // для простых аргументов (числа, переменные)
    
    // Методы парсинга для массивов (согласно лекции)
    void parseReadStatement();    // read(a) → a r
    void parseWriteStatement();   // write(S) → S w  
    void parseMem1Statement();    // mem1(a, S) → a S m1
    void parseMem2Statement();    // mem2(a, S, S) → a S S m2
    void parseArrayAccess();      // M[i] → M i i
};

#endif // SYNTAX_ANALYZER_H 
