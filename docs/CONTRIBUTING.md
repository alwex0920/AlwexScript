# Руководство по добавлению новых команд в AlwexScript

Отличная работа! Теперь, когда у вас есть работающая версия AlwexScript
как в вашей ОС, так и в standalone-версии для компьютеров, вот
руководство по добавлению новых команд.

## Структура проекта

**alwex.c** - standalone-версия для компьютеров

## Процесс добавления новой команды

### 1. Определение синтаксиса команды

Сначала определите, как будет выглядеть ваша команда. Например, для
команды возведения в степень:
```alw
pow base exponent
```

### 2. Реализация в alwex.c
```c
else if (strncmp(token, "pow ", 4) == 0) {
    char* args = token + 4;
    char base_str[50], exponent_str[50];
    
    if (sscanf(args, "%49s %49s", base_str, exponent_str) == 2) {
        double base = eval_expression(base_str);
        double exponent = eval_expression(exponent_str);
        double result = pow(base, exponent); // Используем стандартную функцию math.h
        
        if (var_count < MAX_VARS) {
            struct Variable* var = &variables[var_count++];
            strncpy(var->name, "_result", sizeof(var->name));
            var->value = result;
            var->str_value = NULL;
            printf("%.4f\n", result);
        }
    } else {
        printf("Error: pow requires 2 arguments\n");
    }
}
```
### 3. Добавление встроенных функций

Если вы хотите добавить встроенную функцию (например, математические
функции), вам нужно:

1.  Добавить распознавание имени функции в **eval_expression()**

2.  Реализовать вычисление функции

Пример для синуса в **eval_expression()**:
```c
// В секции обработки переменных и функций в eval_expression()
if (strcmp(var_name, "sin") == 0) {
    // Ожидаем аргумент в скобках
    char* paren = strchr(p, '(');
    if (paren) {
        char* end_paren = strchr(paren, ')');
        if (end_paren) {
            char arg_expr[100];
            int len = end_paren - paren - 1;
            
            if (len > 0 && len < sizeof(arg_expr)) {
                strncpy(arg_expr, paren + 1, len);
                arg_expr[len] = '\0';
                double arg = eval_expression(arg_expr);
                current = sin(arg); // Из math.h
                has_value = 1;
                p = end_paren + 1;
                continue;
            }
        }
    }
}
```
Важно: Не забудьте добавить #include <math.h>

### 5. Тестирование команды

Создайте тестовый скрипт для проверки новой команды:
```alw
let x = 5

let y = 3

pow x y
```

### 6. Документирование команды

Добавьте описание команды в документацию:

\## Команда pow

Возводит число в степень.

Синтаксис:
```alw
pow base exponent
```
Пример:
```alw
pow 2 3 \# Результат: 8
```

## Советы по разработке

1.  **Обработка ошибок**: Всегда добавляйте проверку входных данных и
    обработку ошибок

2.  **Память**: Следите за использованием памяти

3.  **Тестирование**: Тестируйте команды на наличие ошибок

4.  **Документация**: Обновляйте документацию при добавлении новых
    команд

## Пример добавления команды работы со строками
```c
else if (strncmp(token, "strlen ", 7) == 0) {
    char* var_name = token + 7;
    struct Variable* var = find_variable(var_name);
    
    if (var && var->str_value) {
        int len = strlen(var->str_value);
        printf("%d\n", len);
    } else {
        printf("Error: variable not found or not a string\n");
    }
}
```

## Пример добавления команды задержки
```c
else if (strncmp(token, "wait", 4) == 0) {
    // Пропускаем "wait" и любые пробелы после него
    char* sec_str = token + 4;
    while (*sec_str == ' ' || *sec_str == '\t') sec_str++;
            
    if (*sec_str == '\0') {
        printf("Error: wait command requires argument\n");
    } else {
        int seconds = atoi(sec_str);
        if (seconds > 0) {
            sleep(seconds);
        } else {
            printf("Error: invalid time value\n");
        }
    }
}
```

Теперь вы можете добавлять любые команды, которые вам нужны!
