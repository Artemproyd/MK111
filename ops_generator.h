#ifndef OPS_GENERATOR_H
#define OPS_GENERATOR_H

#include <string>
#include <vector>
#include <unordered_map>

// Типы команд ОПС
enum class OPSCommandType {
    DECLARE,
    ASSIGN,
    PUSH_CONST,
    PUSH_VAR,
    ADD,
    SUB,
    MUL,
    DIV,
    COMPARE,
    JUMP,
    LABEL,
    JZ,      // Jump if zero (for if statements)
    GT,      // Greater than
    LT,      // Less than
    EQ,      // Equal
    
    // Команды для массивов согласно лекции
    MEM1,    // m1 - выделение памяти одномерному массиву
    MEM2,    // m2 - выделение памяти двумерному массиву
    READ,    // r - операция чтения со стандартного устройства ввода
    WRITE,   // w - вывод значения в стандартное устройство вывода
    INDEX,   // i - операция индексирования массива
    
    // Дополнительные операции для массивов
    ALLOC_ARRAY,  // выделение массива
    ARRAY_READ,   // ввод в элемент массива
    ARRAY_GET,    // получение элемента массива
    ARRAY_SET     // установка элемента массива
};

// Структура для команды ОПС
struct OPSCommand {
    OPSCommandType type;
    std::vector<std::string> operands;
    
    OPSCommand(OPSCommandType t) : type(t) {}
    OPSCommand(OPSCommandType t, const std::string& op1) : type(t) {
        operands.push_back(op1);
    }
    OPSCommand(OPSCommandType t, const std::string& op1, const std::string& op2) : type(t) {
        operands.push_back(op1);
        operands.push_back(op2);
    }
};

// Класс генератора ОПС
class OPSGenerator {
public:
    OPSGenerator();
    
    // Генерация команды ОПС на основе типа токена и его значения
    void generateCommand(const std::string& tokenType, const std::string& tokenValue);
    
    // Получить сгенерированный код
    std::vector<OPSCommand> getGeneratedCode() const;
    
    // Очистить сгенерированный код
    void reset();

private:
    std::vector<OPSCommand> generatedCode;
    std::string currentType; // Текущий тип для объявления переменной
    std::string lastIdentifier; // Последний обработанный идентификатор
    int ifCount = 0;  // Счетчик операторов if для создания уникальных меток
    
    // Таблица соответствия токенов командам ОПС
    struct CommandTableEntry {
        OPSCommandType commandType;
        int operandCount;
    };
    
    std::unordered_map<std::string, CommandTableEntry> commandTable;
    
    // Инициализация таблицы команд
    void initCommandTable();
};

#endif // OPS_GENERATOR_H 