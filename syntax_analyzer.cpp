#include "syntax_analyzer.h"
#include "ops_generator.h"
#include "ops_interpreter.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stack>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#endif

// Конструктор синтаксического анализатора - исправляю порядок инициализации
SyntaxAnalyzer::SyntaxAnalyzer() : opsGenerator(std::make_unique<OPSGenerator>()), currentToken(0), labelCounter(0) {}

// Анализировать токены и генерировать ОПС
std::vector<OPSCommand> SyntaxAnalyzer::analyze(const std::vector<Token>& inputTokens) {
    tokens = inputTokens;
    currentToken = 0;
    stackMachine.reset();
    opsGenerator->reset();
    opsCode.clear();
    labelCounter = 0;
    
    try {
        parseProgram();
    }
    catch (const std::exception& e) {
        std::cerr << "Error during analysis: " << e.what() << std::endl;
    }
    
    return opsGenerator->getGeneratedCode();
}

void SyntaxAnalyzer::parseProgram() {
    int iterationsCount = 0;
    int lastToken = -1;
    
    while (currentToken < tokens.size()) {
        // Защита от бесконечного цикла
        if (currentToken == lastToken) {
            iterationsCount++;
            if (iterationsCount > 1000) {
                throw std::runtime_error("Parser stuck in infinite loop at token " + std::to_string(currentToken));
            }
        } else {
            iterationsCount = 0;
            lastToken = currentToken;
        }
        
        size_t tokenBefore = currentToken;
        parseStatement();
        
        // Если токен не продвинулся, принудительно продвигаем чтобы избежать зависания
        if (currentToken == tokenBefore && currentToken < tokens.size()) {
            currentToken++;
        }
    }
}

void SyntaxAnalyzer::parseStatement() {
    if (currentToken >= tokens.size()) return;
    
    const Token& token = tokens[currentToken];
    
    // Пропускаем EOF токены
    if (token.getType() == "EOF") {
        currentToken++;
        return;
    }
    
    if (token.getType() == "KEYWORD") {
        if (token.getValue() == "int" || token.getValue() == "float" || token.getValue() == "char") {
            parseDeclaration();
        } else if (token.getValue() == "if") {
            parseIfStatement();
        } else if (token.getValue() == "while") {
            parseWhileStatement();
        } else if (token.getValue() == "for") {
            parseForStatement();
        } else if (token.getValue() == "read" || token.getValue() == "input") {
            parseReadStatement();
        } else if (token.getValue() == "write" || token.getValue() == "output") {
            parseWriteStatement();
        } else {
            // Неизвестное ключевое слово - пропускаем
            currentToken++;
        }
    } else if (token.getType() == "IDENTIFIER") {
        parseAssignment();
    } else {
        // Все остальные токены пропускаем
        currentToken++;
    }
}

void SyntaxAnalyzer::parseDeclaration() {
    // Типизированное объявление: int x = value; ИЛИ int M[size]; ИЛИ float A[10];
    std::string type = tokens[currentToken].getValue(); // int, float, char
    currentToken++; // пропускаем тип
    
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "IDENTIFIER") {
        std::string varName = tokens[currentToken].getValue();
        currentToken++;
        
        // Проверяем на объявление массива M[size]
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
            currentToken++; // пропускаем '['
            
            // Парсим размер массива
            if (currentToken < tokens.size() && tokens[currentToken].getType() == "NUMBER") {
                std::string size1 = tokens[currentToken].getValue();
                currentToken++;
                
                if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
                    if (currentToken < tokens.size()) {
                        error("Expected ']' after array size", tokens[currentToken]);
                    } else {
                        throw std::runtime_error("Unexpected end of input - missing ']'");
                    }
                }
                currentToken++; // пропускаем ']'
                
                // Проверяем на двумерный массив M[size1][size2]
                if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
                    currentToken++; // пропускаем '['
                    
                    if (currentToken < tokens.size() && tokens[currentToken].getType() == "NUMBER") {
                        std::string size2 = tokens[currentToken].getValue();
                        currentToken++;
                        
                        // Генерируем ОПС для объявления двумерного массива
                        // Формат: тип имя_массива строки столбцы alloc_array_2d
                        opsCode.push_back(type);        // тип массива (int/float/char)
                        opsCode.push_back(varName);     // имя массива
                        opsCode.push_back(size1);       // количество строк
                        opsCode.push_back(size2);       // количество столбцов
                        opsCode.push_back("alloc_array_2d"); // команда выделения памяти 2D
                        
                        if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
                            if (currentToken < tokens.size()) {
                                error("Expected ']' after second array dimension", tokens[currentToken]);
                            } else {
                                throw std::runtime_error("Unexpected end of input - missing second ']'");
                            }
                        }
                        currentToken++; // пропускаем ']'
                    } else {
                        error("Expected array size after second '['", tokens[currentToken]);
                    }
                } else {
                    // Одномерный массив
                    // Генерируем ОПС для объявления типизированного массива
                    // Формат: тип имя_массива размер alloc_array
                    opsCode.push_back(type);        // тип массива (int/float/char)
                    opsCode.push_back(varName);     // имя массива
                    opsCode.push_back(size1);       // размер массива
                    opsCode.push_back("alloc_array"); // команда выделения памяти
                }
            } else {
                error("Expected array size after '['", tokens[currentToken]);
            }
        }
        else if (currentToken < tokens.size() && tokens[currentToken].getValue() == "=") {
            // Обычное объявление с инициализацией: int x = 5;
            currentToken++; // пропускаем '='
            parseExpression(); // генерирует ОПС для выражения (значение уже в стеке)
            opsCode.push_back(varName); // добавляем имя переменной
            opsCode.push_back(":="); // добавляем операцию присваивания
        } else {
            // Простое объявление без инициализации: int x;
            // В ОПС это может не генерировать команд, или генерировать команду объявления
            opsCode.push_back(type);     // тип переменной
            opsCode.push_back(varName);  // имя переменной
            opsCode.push_back("declare"); // команда объявления
        }
        
        // пропускаем ';'
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "SEMICOLON") {
            currentToken++;
        }
    }
}

void SyntaxAnalyzer::parseAssignment() {
    // x = value; ИЛИ M[i] = value;
    std::string varName = tokens[currentToken].getValue();
    currentToken++;
    
    // Проверяем на доступ к массиву M[i] или M[i][j]
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
        currentToken++; // пропускаем '['
        
        // Сначала добавляем имя массива
        opsCode.push_back(varName); // имя массива идет ПЕРВЫМ
        
        parseExpression(); // парсим первый индекс (добавляется в стек)
        
        if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
            if (currentToken < tokens.size()) {
                error("Expected ']' after array index", tokens[currentToken]);
            } else {
                throw std::runtime_error("Unexpected end of input - missing ']'");
            }
        }
        currentToken++; // пропускаем ']'
        
        // Проверяем на второй индекс для двумерного массива M[i][j]
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
            currentToken++; // пропускаем '['
            
            parseExpression(); // парсим второй индекс (добавляется в стек)
            
            if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
                if (currentToken < tokens.size()) {
                    error("Expected ']' after second array index", tokens[currentToken]);
                } else {
                    throw std::runtime_error("Unexpected end of input - missing second ']'");
                }
            }
            currentToken++; // пропускаем ']'
            
            if (currentToken < tokens.size() && tokens[currentToken].getValue() == "=") {
                currentToken++; // пропускаем '='
                parseExpression(); // генерирует ОПС для выражения (значение в стеке)
                opsCode.push_back("array_set_2d"); // операция установки элемента 2D массива
            }
        } else {
            // Одномерный массив M[i] = value
            if (currentToken < tokens.size() && tokens[currentToken].getValue() == "=") {
                currentToken++; // пропускаем '='
                parseExpression(); // генерирует ОПС для выражения (значение в стеке)
                opsCode.push_back("array_set"); // операция установки элемента массива
            }
        }
    } else {
        // Обычное присваивание переменной
        if (currentToken < tokens.size() && tokens[currentToken].getValue() == "=") {
            currentToken++; // пропускаем '='
            parseExpression(); // генерирует ОПС для выражения (значение в стеке)
            opsCode.push_back(varName); // добавляем имя переменной ПОСЛЕ значения
            opsCode.push_back(":="); // добавляем присваивание
        }
    }
    
    // пропускаем ';'
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "SEMICOLON") {
        currentToken++;
    }
}

void SyntaxAnalyzer::parseIfStatement() {
    // if (condition) { statements } [else { statements }]
    currentToken++; // пропускаем 'if'
    
    // Проверяем наличие открывающей скобки
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "LEFT_PAREN") {
        if (currentToken < tokens.size()) {
            error("Expected '(' after 'if'", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input after 'if'");
        }
    }
    
    currentToken++; // пропускаем '('
    
    parseCondition(); // генерирует ОПС для условия
    
    // Проверяем наличие закрывающей скобки
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_PAREN") {
        if (currentToken < tokens.size()) {
            error("Expected ')' after condition", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input - missing ')'");
        }
    }
    
    std::string elseLabel = "m" + std::to_string(labelCounter++);
    std::string endLabel = "m" + std::to_string(labelCounter++);
    
    opsCode.push_back(elseLabel); // метка для перехода
    opsCode.push_back("jf");      // команда условного перехода
    
    currentToken++; // пропускаем ')'
    
    // Проверяем наличие открывающей фигурной скобки
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "LEFT_BRACE") {
        if (currentToken < tokens.size()) {
            error("Expected '{' after condition", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input - missing '{'");
        }
    }
    
    currentToken++; // пропускаем '{'
    
    // парсим тело if
    while (currentToken < tokens.size() && tokens[currentToken].getType() != "RIGHT_BRACE") {
        parseStatement();
    }
    
    // Проверяем наличие закрывающей фигурной скобки
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACE") {
        throw std::runtime_error("Unexpected end of input - missing '}'");
    }
    
    currentToken++; // пропускаем '}'
    
    // Проверяем на else
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "KEYWORD" && 
        tokens[currentToken].getValue() == "else") {
        
        opsCode.push_back(endLabel); // метка для безусловного перехода
        opsCode.push_back("j");      // команда безусловного перехода
        opsCode.push_back(elseLabel + ":"); // метка начала else
        
        currentToken++; // пропускаем 'else'
        
        // Проверяем наличие открывающей фигурной скобки для else
        if (currentToken >= tokens.size() || tokens[currentToken].getType() != "LEFT_BRACE") {
            if (currentToken < tokens.size()) {
                error("Expected '{' after 'else'", tokens[currentToken]);
            } else {
                throw std::runtime_error("Unexpected end of input after 'else'");
            }
        }
        
        currentToken++; // пропускаем '{'
        
        // парсим тело else
        while (currentToken < tokens.size() && tokens[currentToken].getType() != "RIGHT_BRACE") {
            parseStatement();
        }
        
        // Проверяем наличие закрывающей фигурной скобки для else
        if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACE") {
            throw std::runtime_error("Unexpected end of input - missing '}' after else");
        }
        
        currentToken++; // пропускаем '}'
        
        opsCode.push_back(endLabel + ":"); // метка конца всей конструкции if-else
    } else {
        // Нет else, просто добавляем метку конца if
        opsCode.push_back(elseLabel + ":"); // метка конца if
    }
}

void SyntaxAnalyzer::parseWhileStatement() {
    // while (condition) { statements }
    currentToken++; // пропускаем 'while'
    
    std::string startLabel = "m" + std::to_string(labelCounter++);
    std::string endLabel = "m" + std::to_string(labelCounter++);
    
    opsCode.push_back(startLabel + ":"); // метка начала цикла
    
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_PAREN") {
        currentToken++; // пропускаем '('
        
        parseCondition(); // генерирует ОПС для условия
        
        opsCode.push_back(endLabel); // метка для условного перехода
        opsCode.push_back("jf");     // команда условного перехода на конец
        
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "RIGHT_PAREN") {
            currentToken++; // пропускаем ')'
        }
        
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACE") {
            currentToken++; // пропускаем '{'
            
            // парсим тело while
            while (currentToken < tokens.size() && tokens[currentToken].getType() != "RIGHT_BRACE") {
                parseStatement();
            }
            
            if (currentToken < tokens.size() && tokens[currentToken].getType() == "RIGHT_BRACE") {
                currentToken++; // пропускаем '}'
            }
        }
        
        opsCode.push_back(startLabel); // метка для безусловного перехода
        opsCode.push_back("j");        // команда безусловного перехода на начало
        opsCode.push_back(endLabel + ":"); // метка конца цикла
    }
}

void SyntaxAnalyzer::parseExpression() {
    std::stack<std::string> operatorStack;
    int iterationsCount = 0;
    size_t lastToken = currentToken;
    
    while (currentToken < tokens.size()) {
        // Защита от бесконечного цикла
        if (currentToken == lastToken) {
            iterationsCount++;
            if (iterationsCount > 100) {
                throw std::runtime_error("Expression parser stuck in infinite loop at token " + std::to_string(currentToken));
            }
        } else {
            iterationsCount = 0;
            lastToken = currentToken;
        }
        
        const Token& token = tokens[currentToken];
        
        if (token.getType() == "NUMBER" || token.getType() == "IDENTIFIER") {
            opsCode.push_back(token.getValue()); // добавляем операнд
            currentToken++;
            
            // Проверяем на доступ к массиву M[i]
            if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
                currentToken++; // пропускаем '['
                parseExpression(); // парсим индекс
                
                // Проверяем на второй индекс для двумерного массива M[i][j]
                if (currentToken < tokens.size() && tokens[currentToken].getType() == "RIGHT_BRACKET") {
                    currentToken++; // пропускаем ']'
                    
                    if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
                        currentToken++; // пропускаем '['
                        parseExpression(); // парсим второй индекс
                        opsCode.push_back("array_get_2d"); // операция получения элемента 2D массива
                        
                        if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
                            if (currentToken < tokens.size()) {
                                error("Expected ']' after second array index", tokens[currentToken]);
                            } else {
                                throw std::runtime_error("Unexpected end of input - missing second ']'");
                            }
                        }
                        currentToken++; // пропускаем ']'
                    } else {
                        // Одномерный массив
                        opsCode.push_back("array_get"); // операция получения элемента массива
                    }
                } else {
                    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
                        if (currentToken < tokens.size()) {
                            error("Expected ']' after array index", tokens[currentToken]);
                        } else {
                            throw std::runtime_error("Unexpected end of input - missing ']'");
                        }
                    }
                    currentToken++; // пропускаем ']'
                    opsCode.push_back("array_get"); // операция получения элемента массива
                }
            }
        }
        else if (token.getType() == "OPERATOR") {
            std::string op = token.getValue();
            
            // Преобразуем операторы в формат ОПС
            if (op == "+") op = "+";
            else if (op == "-") op = "-";
            else if (op == "*") op = "*";
            else if (op == "/") op = "/";
            else if (op == ">") op = ">";
            else if (op == "<") op = "<";
            else if (op == "==") op = "==";
            else if (op == "=") break; // присваивание обрабатывается отдельно
            
            // Простая обработка приоритета операторов
            while (!operatorStack.empty() && 
                   getPriority(operatorStack.top()) >= getPriority(op)) {
                opsCode.push_back(operatorStack.top());
                operatorStack.pop();
            }
            operatorStack.push(op);
            currentToken++;
        }
        else if (token.getType() == "LEFT_PAREN") {
            operatorStack.push("(");
            currentToken++;
        }
        else if (token.getType() == "RIGHT_PAREN") {
            while (!operatorStack.empty() && operatorStack.top() != "(") {
                opsCode.push_back(operatorStack.top());
                operatorStack.pop();
            }
            if (!operatorStack.empty()) {
                operatorStack.pop(); // убираем '('
            }
            // НЕ продвигаем currentToken здесь - оставляем RIGHT_PAREN для верхнего уровня
            break;
        }
        else {
            break; // выходим из выражения
        }
    }
    
    // Выгружаем оставшиеся операторы
    while (!operatorStack.empty()) {
        if (operatorStack.top() != "(") {
            opsCode.push_back(operatorStack.top());
        }
        operatorStack.pop();
    }
}

// Новый метод для парсинга условий в if/while
void SyntaxAnalyzer::parseCondition() {
    std::stack<std::string> operatorStack;
    int iterationsCount = 0;
    size_t lastToken = currentToken;
    
    while (currentToken < tokens.size()) {
        // Защита от бесконечного цикла
        if (currentToken == lastToken) {
            iterationsCount++;
            if (iterationsCount > 100) {
                throw std::runtime_error("Condition parser stuck in infinite loop at token " + std::to_string(currentToken));
            }
        } else {
            iterationsCount = 0;
            lastToken = currentToken;
        }
        
        const Token& token = tokens[currentToken];
        
        if (token.getType() == "NUMBER" || token.getType() == "IDENTIFIER") {
            opsCode.push_back(token.getValue()); // добавляем операнд
            currentToken++;
        }
        else if (token.getType() == "OPERATOR") {
            std::string op = token.getValue();
            
            // Преобразуем операторы в формат ОПС
            if (op == "+") op = "+";
            else if (op == "-") op = "-";
            else if (op == "*") op = "*";
            else if (op == "/") op = "/";
            else if (op == ">") op = ">";
            else if (op == "<") op = "<";
            else if (op == "==") op = "==";
            else break; // останавливаемся на других операторах
            
            // Простая обработка приоритета операторов
            while (!operatorStack.empty() && 
                   getPriority(operatorStack.top()) >= getPriority(op)) {
                opsCode.push_back(operatorStack.top());
                operatorStack.pop();
            }
            operatorStack.push(op);
            currentToken++;
        }
        else if (token.getType() == "RIGHT_PAREN") {
            // Останавливаемся на закрывающей скобке, не потребляем её
            break;
        }
        else {
            break; // выходим из выражения
        }
    }
    
    // Выгружаем оставшиеся операторы
    while (!operatorStack.empty()) {
        opsCode.push_back(operatorStack.top());
        operatorStack.pop();
    }
}

int SyntaxAnalyzer::getPriority(const std::string& op) {
    if (op == ">" || op == "<" || op == "==") return 1;
    if (op == "+" || op == "-") return 2;
    if (op == "*" || op == "/") return 3;
    return 0;
}

void SyntaxAnalyzer::processToken(const Token& token) {
    // Этот метод теперь не используется, заменен новой логикой парсинга
    InputSymbol inputSymbol = convertTokenType(token.getType());
    
    if (!stackMachine.processSymbol(inputSymbol, token.getValue())) {
        error("Invalid token sequence", token);
    }
    
    opsGenerator->generateCommand(token.getType(), token.getValue());
}

InputSymbol SyntaxAnalyzer::convertTokenType(const std::string& type) const {
    if (type == "NUMBER") return InputSymbol::NUMBER;
    if (type == "IDENTIFIER") return InputSymbol::IDENTIFIER;
    if (type == "OPERATOR") return InputSymbol::OPERATOR;
    if (type == "LEFT_PAREN") return InputSymbol::LEFT_PAREN;
    if (type == "RIGHT_PAREN") return InputSymbol::RIGHT_PAREN;
    if (type == "LEFT_BRACE") return InputSymbol::LEFT_BRACE;
    if (type == "RIGHT_BRACE") return InputSymbol::RIGHT_BRACE;
    if (type == "KEYWORD") return InputSymbol::KEYWORD;
    if (type == "SEMICOLON") return InputSymbol::END;
    return InputSymbol::END;
}

void SyntaxAnalyzer::error(const std::string& message, const Token& token) const {
    std::stringstream ss;
    ss << message << " at line " << token.getLine() 
       << ", column " << token.getColumn() 
       << ": " << token.getType() 
       << "(" << token.getValue() << ")";
    throw std::runtime_error(ss.str());
}

std::vector<OPSCommand> SyntaxAnalyzer::getOPSCode() const {
    return opsGenerator->getGeneratedCode();
}

void SyntaxAnalyzer::printOPSCode() const {
    if (opsCode.empty()) {
        std::cout << "No OPS code generated." << std::endl;
        return;
    }

    // Выводим ОПС в правильном формате
    for (size_t i = 0; i < opsCode.size(); ++i) {
        std::cout << opsCode[i];
        if (i < opsCode.size() - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    file.close();
    return content;
}

void processCode(const std::string& code, const std::string& description) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "АНАЛИЗ: " << description << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "Исходный код:" << std::endl;
    std::cout << code << std::endl;
    
    try {
        // Лексический анализ
        std::cout << std::string(30, '-') << std::endl;
        std::cout << "1) ЛЕКСИЧЕСКИЙ АНАЛИЗ (конечный автомат):" << std::endl;
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        
        std::cout << "Токены:" << std::endl;
        for (const auto& token : tokens) {
            if (token.getType() != "EOF") {
                std::cout << "  " << token.getType() << ": '" << token.getValue() << "'" << std::endl;
            }
        }
        
        // Синтаксический анализ + генерация ОПС
        std::cout << std::string(30, '-') << std::endl;
        std::cout << "2) СИНТАКСИЧЕСКИЙ АНАЛИЗ (магазинный автомат + генератор ОПС):" << std::endl;
        
        SyntaxAnalyzer analyzer;
        std::vector<OPSCommand> result = analyzer.analyze(tokens);
        
        std::cout << "Сгенерированная ОПС:" << std::endl;
        std::cout << "  ";
        analyzer.printOPSCode();
        
        // Выполнение ОПС интерпретатором
        std::cout << std::string(30, '-') << std::endl;
        std::cout << "3) ВЫПОЛНЕНИЕ ОПС (стековая машина):" << std::endl;
        
        try {
            OPSInterpreter interpreter;
            
            // Преобразуем ОПС код в вектор строк
            std::vector<std::string> opsCommands;
            std::stringstream ss;
            analyzer.printOPSCode();
            // Получаем ОПС код из analyzer.opsCode
            for (const auto& cmd : analyzer.opsCode) {
                opsCommands.push_back(cmd);
            }
            
            if (!opsCommands.empty()) {
                interpreter.execute(opsCommands);
            } else {
                std::cout << "❌ Нет команд ОПС для выполнения" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cout << "❌ ОШИБКА ВЫПОЛНЕНИЯ ОПС: " << e.what() << std::endl;
            std::cout << "⚠️  Выполнение остановлено." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cout << "❌ ОШИБКА: " << e.what() << std::endl;
    }
}

int main() {
    #ifdef _WIN32
    // Set console output codepage to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    std::cout << "🚀 КОМПИЛЯТОР: Лексический + Синтаксический анализатор" << std::endl;
    std::cout << "Версия: 1.0" << std::endl;
    std::cout << "Согласно лекциям по методам компиляции" << std::endl;

    // Ищем файл input.txt
    std::string inputFile = "input.txt";
    
    // Простая проверка существования файла
    std::ifstream fileCheck(inputFile);
    if (fileCheck.good()) {
        fileCheck.close();
        std::cout << "\n📁 Анализ кода из файла: " << inputFile << std::endl;
        
        try {
            std::string fileContent = readFile(inputFile);
            processCode(fileContent, "Код из файла " + inputFile);
    }
    catch (const std::exception& e) {
            std::cout << "❌ Ошибка чтения файла: " << e.what() << std::endl;
        }
    } else {
        std::cout << "\n❌ Файл " << inputFile << " не найден!" << std::endl;
        std::cout << "Создайте файл 'input.txt' с кодом для анализа." << std::endl;
        std::cout << "\nПример содержимого:" << std::endl;
        std::cout << "if (x > 5) {" << std::endl;
        std::cout << "    y = 10;" << std::endl;
        std::cout << "}" << std::endl;
    }

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "✅ Анализ завершен!" << std::endl;
    
    std::cout << "\nНажмите Enter для выхода..." << std::endl;
    std::cin.get();
    return 0;
}

// ============== НОВЫЕ МЕТОДЫ ДЛЯ МАССИВОВ (согласно лекции) ==============

void SyntaxAnalyzer::parseReadStatement() {
    // read(a) → a r  ИЛИ  read(M[i]) → M i array_read  ИЛИ  read(M[i][j]) → M i j array_read_2d
    currentToken++; // пропускаем 'read'
    
    // Ожидаем '('
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "LEFT_PAREN") {
        if (currentToken < tokens.size()) {
            error("Expected '(' after 'read'", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input after 'read'");
        }
    }
    currentToken++; // пропускаем '('
    
    // Парсим переменную или доступ к массиву
    if (currentToken >= tokens.size()) {
        throw std::runtime_error("Expected identifier after 'read('");
    }
    
    const Token& varToken = tokens[currentToken];
    if (varToken.getType() != "IDENTIFIER") {
        error("Expected identifier in read statement", varToken);
    }
    
    opsCode.push_back(varToken.getValue()); // добавляем переменную/массив
    currentToken++;
    
    // Проверяем на доступ к массиву M[i] или M[i][j]
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
        currentToken++; // пропускаем '['
        parseExpression(); // парсим первый индекс
        
        if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
            if (currentToken < tokens.size()) {
                error("Expected ']' after array index", tokens[currentToken]);
            } else {
                throw std::runtime_error("Unexpected end of input - missing ']'");
            }
        }
        currentToken++; // пропускаем ']'
        
        // Проверяем на второй индекс для двумерного массива M[i][j]
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACKET") {
            currentToken++; // пропускаем '['
            parseExpression(); // парсим второй индекс
            
            if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_BRACKET") {
                if (currentToken < tokens.size()) {
                    error("Expected ']' after second array index", tokens[currentToken]);
                } else {
                    throw std::runtime_error("Unexpected end of input - missing second ']'");
                }
            }
            currentToken++; // пропускаем ']'
            
            // Команда для чтения в двумерный массив (пока такой нет в спецификации, используем array_read_2d)
            opsCode.push_back("array_read_2d"); // операция чтения в элемент 2D массива
        } else {
            // Одномерный массив M[i]
            opsCode.push_back("array_read"); // операция чтения в элемент массива
        }
    } else {
        // Обычная переменная
        opsCode.push_back("r"); // операция чтения для обычной переменной
    }
    
    // Ожидаем ')'
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_PAREN") {
        if (currentToken < tokens.size()) {
            error("Expected ')' after read argument", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input - missing ')'");
        }
    }
    currentToken++; // пропускаем ')'
    
    // Ожидаем ';'
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "SEMICOLON") {
        currentToken++;
    }
}

void SyntaxAnalyzer::parseWriteStatement() {
    // write(S) → S w
    currentToken++; // пропускаем 'write'
    
    // Ожидаем '('
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "LEFT_PAREN") {
        if (currentToken < tokens.size()) {
            error("Expected '(' after 'write'", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input after 'write'");
        }
    }
    currentToken++; // пропускаем '('
    
    // Парсим выражение
    parseExpression(); // генерирует ОПС для выражения
    opsCode.push_back("w"); // операция записи
    
    // Ожидаем ')'
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "RIGHT_PAREN") {
        if (currentToken < tokens.size()) {
            error("Expected ')' after write argument", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input - missing ')'");
        }
    }
    currentToken++; // пропускаем ')'
    
    // Ожидаем ';'
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "SEMICOLON") {
        currentToken++;
    }
}

void SyntaxAnalyzer::parseArrayAccess() {
    // M[i] → M i i  (используется в parseExpression)
    // Этот метод будет вызываться из parseExpression при обнаружении '['
}

// ============== КОНЕЦ НОВЫХ МЕТОДОВ ============== 

void SyntaxAnalyzer::parseSimpleExpression() {
    // Парсим простое выражение: число или переменную (без операторов)
    if (currentToken >= tokens.size()) {
        throw std::runtime_error("Expected expression");
    }
    
    const Token& token = tokens[currentToken];
    
    if (token.getType() == "NUMBER" || token.getType() == "IDENTIFIER") {
        opsCode.push_back(token.getValue()); // добавляем операнд
        currentToken++;
    } else {
        error("Expected number or identifier in expression", token);
    }
}

void SyntaxAnalyzer::parseForStatement() {
    // for (init; condition; increment) { body }
    // Трансформируется в: init; while(condition) { body; increment; }
    currentToken++; // пропускаем 'for'
    
    // Ожидаем '('
    if (currentToken >= tokens.size() || tokens[currentToken].getType() != "LEFT_PAREN") {
        if (currentToken < tokens.size()) {
            error("Expected '(' after 'for'", tokens[currentToken]);
        } else {
            throw std::runtime_error("Unexpected end of input after 'for'");
        }
    }
    currentToken++; // пропускаем '('
    
    // 1. Парсим инициализацию (может быть объявление или присваивание)
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "KEYWORD") {
        parseDeclaration(); // int i = 0;
    } else if (currentToken < tokens.size() && tokens[currentToken].getType() == "IDENTIFIER") {
        parseAssignment(); // i = 0;
    } else {
        // Пропускаем ';' если инициализация пустая
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "SEMICOLON") {
            currentToken++;
        }
    }
    
    // Проверяем ';' после инициализации (если не была обработана в parseDeclaration/parseAssignment)
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "SEMICOLON") {
        currentToken++; // пропускаем ';'
    }
    
    // Генерируем метки для цикла
    std::string startLabel = "m" + std::to_string(labelCounter++);
    std::string endLabel = "m" + std::to_string(labelCounter++);
    std::string incrementLabel = "m" + std::to_string(labelCounter++);
    
    opsCode.push_back(startLabel + ":"); // метка начала цикла
    
    // 2. Парсим условие
    parseCondition(); // генерирует ОПС для условия
    
    opsCode.push_back(endLabel); // метка для условного перехода
    opsCode.push_back("jf");     // команда условного перехода на конец
    
    // Пропускаем ';' после условия
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "SEMICOLON") {
        currentToken++; // пропускаем ';'
    }
    
    // 3. Сохраняем инкремент для обработки после тела цикла
    size_t incrementStart = currentToken;
    
    // Пропускаем инкремент, чтобы добраться до тела цикла
    int parenCount = 0;
    while (currentToken < tokens.size()) {
        if (tokens[currentToken].getType() == "LEFT_PAREN") parenCount++;
        if (tokens[currentToken].getType() == "RIGHT_PAREN") {
            if (parenCount == 0) break;
            parenCount--;
        }
        currentToken++;
    }
    
    size_t incrementEnd = currentToken;
    
    // Пропускаем ')'
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "RIGHT_PAREN") {
        currentToken++; // пропускаем ')'
    }
    
    // 4. Парсим тело цикла
    if (currentToken < tokens.size() && tokens[currentToken].getType() == "LEFT_BRACE") {
        currentToken++; // пропускаем '{'
        
        // парсим тело for
        while (currentToken < tokens.size() && tokens[currentToken].getType() != "RIGHT_BRACE") {
            parseStatement();
        }
        
        if (currentToken < tokens.size() && tokens[currentToken].getType() == "RIGHT_BRACE") {
            currentToken++; // пропускаем '}'
        }
    }
    
    // 5. Обрабатываем инкремент
    size_t savedToken = currentToken;
    currentToken = incrementStart;
    
    if (currentToken < incrementEnd) {
        parseAssignment(); // парсим инкремент как присваивание
    }
    
    currentToken = savedToken;
    
    // 6. Генерируем переход на начало цикла
    opsCode.push_back(startLabel); // метка для безусловного перехода
    opsCode.push_back("j");        // команда безусловного перехода на начало
    opsCode.push_back(endLabel + ":"); // метка конца цикла
}

// ============== КОНЕЦ НОВЫХ МЕТОДОВ ==============
