#ifndef OPS_INTERPRETER_H
#define OPS_INTERPRETER_H

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <iostream>

// Тип данных для значений
enum class ValueType {
    INT,
    DOUBLE
};

// Структура для хранения значения с типом
struct Value {
    ValueType type;
    union {
        int intValue;
        double doubleValue;
    };
    
    Value() : type(ValueType::INT), intValue(0) {}
    Value(int val) : type(ValueType::INT), intValue(val) {}
    Value(double val) : type(ValueType::DOUBLE), doubleValue(val) {}
    
    // Проверка типа
    bool isInt() const { return type == ValueType::INT; }
    bool isDouble() const { return type == ValueType::DOUBLE; }
    
    // Преобразование в int (по умолчанию)
    int asInt() const {
        return type == ValueType::INT ? intValue : static_cast<int>(doubleValue);
    }
    
    // Преобразование в double
    double asDouble() const {
        return type == ValueType::DOUBLE ? doubleValue : static_cast<double>(intValue);
    }
    
    // Строковое представление
    std::string toString() const {
        if (type == ValueType::INT) {
            return std::to_string(intValue);
        } else {
            return std::to_string(doubleValue);
        }
    }
    
    // Арифметические операции
    Value operator+(const Value& other) const {
        if (type == ValueType::DOUBLE || other.type == ValueType::DOUBLE) {
            return Value(asDouble() + other.asDouble());
        } else {
            return Value(intValue + other.intValue);
        }
    }
    
    Value operator-(const Value& other) const {
        if (type == ValueType::DOUBLE || other.type == ValueType::DOUBLE) {
            return Value(asDouble() - other.asDouble());
        } else {
            return Value(intValue - other.intValue);
        }
    }
    
    Value operator*(const Value& other) const {
        if (type == ValueType::DOUBLE || other.type == ValueType::DOUBLE) {
            return Value(asDouble() * other.asDouble());
        } else {
            return Value(intValue * other.intValue);
        }
    }
    
    Value operator/(const Value& other) const {
        if (type == ValueType::DOUBLE || other.type == ValueType::DOUBLE) {
            return Value(asDouble() / other.asDouble());
        } else {
            return Value(intValue / other.intValue);
        }
    }
    
    // Оператор вывода
    friend std::ostream& operator<<(std::ostream& os, const Value& val) {
        if (val.type == ValueType::INT) {
            os << val.intValue;
        } else {
            os << val.doubleValue;
        }
        return os;
    }
};

// Интерпретатор ОПС (Обратной Польской записи)
class OPSInterpreter {
public:
    OPSInterpreter();
    
    // Выполнить последовательность команд ОПС
    void execute(const std::vector<std::string>& opsCommands);
    
    // Установить значение переменной (для тестирования)
    void setVariable(const std::string& name, int value);
    void setVariable(const std::string& name, double value);
    void setVariable(const std::string& name, const Value& value);
    
    // Получить значение переменной
    Value getVariable(const std::string& name) const;
    
    // Вывести состояние интерпретатора
    void printState() const;
    
    // Очистить состояние
    void reset();

private:
    std::stack<Value> operandStack;                    // Стек операндов (теперь Value)
    std::unordered_map<std::string, Value> variables;  // Таблица переменных (теперь Value)
    std::unordered_map<std::string, std::vector<Value>> arrays; // Таблица одномерных массивов
    std::unordered_map<std::string, std::vector<std::vector<Value>>> arrays2D; // Таблица двумерных массивов
    std::unordered_map<std::string, size_t> labels;  // Таблица меток
    std::vector<std::string> commands;               // Команды ОПС
    size_t programCounter;                           // Счетчик команд
    bool running;                                    // Флаг выполнения
    
    // Вспомогательные методы
    void parseLabels();                              // Найти все метки в коде
    bool isNumber(const std::string& str) const;     // Проверка на число
    bool isDoubleNumber(const std::string& str) const; // Проверка на число с плавающей точкой
    bool isVariable(const std::string& str) const;   // Проверка на переменную
    bool isOperator(const std::string& str) const;   // Проверка на оператор
    bool isCommand(const std::string& str) const;    // Проверка на команду
    
    // Выполнение операций
    void executeArithmetic(const std::string& op);   // Арифметические операции
    void executeComparison(const std::string& op);   // Операции сравнения
    void executeAssignment();                        // Присваивание (:=)
    void executeRead();                              // Чтение (r)
    void executeWrite();                             // Запись (w)
    void executeArrayAlloc();                        // Выделение памяти массива (alloc_array)
    void executeArrayGet();                          // Получение элемента массива (array_get)
    void executeArraySet();                          // Установка элемента массива (array_set)
    void executeArrayRead();                         // Чтение в элемент массива (array_read)
    void executeArrayRead2D();                       // Чтение в элемент 2D массива (array_read_2d)
    void executeArrayAlloc2D();                      // Выделение памяти 2D массива (alloc_array_2d)
    void executeArrayGet2D();                        // Получение элемента 2D массива (array_get_2d)
    void executeArraySet2D();                        // Установка элемента 2D массива (array_set_2d)
    void executeDeclare();                           // Объявление переменной (declare)
    void executeDeclareAssign();                     // Объявление переменной с типизированным присваиванием (declare_assign)
    void executeJump(const std::string& label);      // Безусловный переход (j)
    void executeConditionalJump(const std::string& label); // Условный переход (jf)
    
    // Работа со стеком
    Value popStack();                                // Извлечь значение из стека
    void pushStack(const Value& value);              // Поместить значение в стек
    void error(const std::string& message) const;    // Обработка ошибок
    
    // Вспомогательные методы для массивов
    bool isArrayName(const std::string& name) const; // Проверка на имя массива
};

#endif // OPS_INTERPRETER_H 