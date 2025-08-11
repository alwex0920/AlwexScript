#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Реализация strlcpy для Windows
#ifndef _WIN32
#define strlcpy(dest, src, size) strncpy(dest, src, size)
#else
size_t strlcpy(char *dest, const char *src, size_t size) {
    size_t src_len = strlen(src);
    if (size == 0) return src_len;
    
    size_t to_copy = src_len < size - 1 ? src_len : size - 1;
    memcpy(dest, src, to_copy);
    dest[to_copy] = '\0';
    return src_len;
}
#endif

// Основные структуры данных
#define MAX_VARS 100
#define MAX_STRINGS 30
#define STRING_SIZE 128
#define MAX_FUNCTIONS 15
#define MAX_FUNC_BODY_SIZE 1024
#define MAX_LINE_LEN 256
#define MAX_IMPORT_DEPTH 5

struct Variable {
    char name[50];
    double value;
    char* str_value;
};

struct Function {
    char name[50];
    char body[MAX_FUNC_BODY_SIZE];
};

// Глобальные переменные
static struct Variable variables[MAX_VARS];
static int var_count = 0;
static char string_pool[MAX_STRINGS][STRING_SIZE];
static int string_count = 0;
static struct Function functions[MAX_FUNCTIONS];
static int function_count = 0;

// Вспомогательные функции
int my_isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

double str_to_double(const char* s) {
    double res = 0.0;
    double fact = 1.0;
    int point_seen = 0;
    int negative = 0;

    if (*s == '-') {
        negative = 1;
        s++;
    }

    for (; *s; s++) {
        if (*s == '.') {
            point_seen = 1;
            continue;
        }
        int d = *s - '0';
        if (d >= 0 && d <= 9) {
            if (point_seen) {
                fact /= 10.0;
                res = res + (double)d * fact;
            } else {
                res = res * 10.0 + (double)d;
            }
        }
    }
    return negative ? -res : res;
}

void print_double(double n) {
    int integer_part = (int)n;
    double fractional = n - integer_part;
    if (fractional < 0) fractional = -fractional;
    
    int fractional_part = (int)(fractional * 10000);
    printf("%d.%04d", integer_part, fractional_part);
}

void replace_text_operators(char* token) {
    char* op_ptr = token;
    while ((op_ptr = strstr(op_ptr, "plus"))) {
        *op_ptr = '+';
        memmove(op_ptr + 1, op_ptr + 4, strlen(op_ptr + 4) + 1);
    }
    op_ptr = token;
    while ((op_ptr = strstr(op_ptr, "minus"))) {
        *op_ptr = '-';
        memmove(op_ptr + 1, op_ptr + 5, strlen(op_ptr + 5) + 1);
    }
    op_ptr = token;
    while ((op_ptr = strstr(op_ptr, "mul"))) {
        *op_ptr = '*';
        memmove(op_ptr + 1, op_ptr + 3, strlen(op_ptr + 3) + 1);
    }
    op_ptr = token;
    while ((op_ptr = strstr(op_ptr, "div"))) {
        *op_ptr = '/';
        memmove(op_ptr + 1, op_ptr + 3, strlen(op_ptr + 3) + 1);
    }
    op_ptr = token;
    while ((op_ptr = strstr(op_ptr, "inc"))) {
        *op_ptr = '+';
        *(op_ptr + 1) = '+';
        memmove(op_ptr + 2, op_ptr + 3, strlen(op_ptr + 3) + 1);
    }
    op_ptr = token;
    while ((op_ptr = strstr(op_ptr, "dec"))) {
        *op_ptr = '-';
        *(op_ptr + 1) = '-';
        memmove(op_ptr + 2, op_ptr + 3, strlen(op_ptr + 3) + 1);
    }
}

struct Variable* find_variable(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i];
        }
    }
    return NULL;
}

struct Function* find_function(const char* name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            return &functions[i];
        }
    }
    return NULL;
}

double eval_expression(const char* expr) {
    double result = 0;
    double current = 0;
    char op = '+';
    int has_value = 0;
    
    const char* p = expr;
    while (*p) {
        if (my_isspace(*p)) {
            p++;
            continue;
        }
        
        if (isdigit(*p) || *p == '.') {
            current = str_to_double(p);
            has_value = 1;
            while (isdigit(*p) || *p == '.') p++;
            continue;
        }
        
        if (isalpha(*p) || *p == '_') {
            char var_name[50] = {0};
            int i = 0;
            
            while (isalnum(*p) || *p == '_') {
                if (i < sizeof(var_name) - 1) {
                    var_name[i++] = *p;
                }
                p++;
            }
            var_name[i] = '\0';
            
            struct Variable* v = find_variable(var_name);
            if (v) {
                current = v->str_value ? 0 : v->value;
            } else {
                current = 0;
            }
            has_value = 1;
            continue;
        }
        
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            if (has_value) {
                switch (op) {
                    case '+': result += current; break;
                    case '-': result -= current; break;
                    case '*': result *= current; break;
                    case '/': 
                        if (current != 0) result /= current;
                        else printf("Error: division by zero\n");
                        break;
                }
                op = *p;
                has_value = 0;
            }
            p++;
            continue;
        }
        
        if (*p == '+' && *(p+1) == '+') {
            p += 2;
            continue;
        }
        if (*p == '-' && *(p+1) == '-') {
            p += 2;
            continue;
        }
        
        p++;
    }
    
    if (has_value) {
        switch (op) {
            case '+': result += current; break;
            case '-': result -= current; break;
            case '*': result *= current; break;
            case '/': 
                if (current != 0) result /= current;
                else printf("Error: division by zero\n");
                break;
        }
    }
    
    return result;
}

int eval_condition(const char* cond) {
    const char* p = cond;
    char left[50] = {0};
    char right[50] = {0};
    char op[3] = {0};
    int op_pos = 0;
    
    while (*p) {
        if (strchr("=><!&|", *p)) {
            if (op_pos < 2) op[op_pos++] = *p;
        } else {
            if (op_pos == 0) strncat(left, p, 1);
            else strncat(right, p, 1);
        }
        p++;
    }
    
    char* ltrim = left;
    while (my_isspace(*ltrim)) ltrim++;
    char* rtrim = right;
    while (my_isspace(*rtrim)) rtrim++;
    
    double left_val = str_to_double(ltrim);
    double right_val = str_to_double(rtrim);
    
    if (strcmp(op, "==") == 0) return left_val == right_val;
    if (strcmp(op, "!=") == 0) return left_val != right_val;
    if (strcmp(op, ">") == 0) return left_val > right_val;
    if (strcmp(op, "<") == 0) return left_val < right_val;
    if (strcmp(op, ">=") == 0) return left_val >= right_val;
    if (strcmp(op, "<=") == 0) return left_val <= right_val;
    
    return 0;
}

void import_library(const char* libname, int import_depth);

void execute(const char* code, int import_depth) {
    if (import_depth > MAX_IMPORT_DEPTH) {
        printf("Error: import depth too deep (max %d levels)\n", MAX_IMPORT_DEPTH);
        return;
    }

    char line[MAX_LINE_LEN];
    const char* p = code;

    int skip_block = 0;
    int condition_met = 0;
    int in_loop = 0;
    const char* loop_start = NULL;
    char loop_condition[100] = {0};

    while (*p) {
        int line_len = 0;
        while (p[line_len] != '\n' && p[line_len] != '\0') {
            line_len++;
        }

        if (line_len >= (int)sizeof(line)) line_len = sizeof(line) - 1;
        memcpy(line, p, line_len);
        line[line_len] = '\0';
        p += line_len;
        if (*p == '\n') p++;

        char* token = line;
        while (my_isspace(*token)) token++;

        if (*token == '\0' || *token == '#') continue;

        if (skip_block) {
            if (strncmp(token, "end", 3) == 0) {
                skip_block = 0;
                condition_met = 0;
            }
            else if (strncmp(token, "endloop", 7) == 0) {
                skip_block = 0;
                in_loop = 0;
            }
            continue;
        }

        replace_text_operators(token);

        if (in_loop) {
            if (strncmp(token, "endloop", 7) == 0) {
                if (eval_condition(loop_condition)) {
                    p = loop_start;
                    skip_block = 0;
                    condition_met = 0;
                    continue;
                } else {
                    in_loop = 0;
                }
            }
        }
        else if (strncmp(token, "while ", 6) == 0) {
            char* cond = token + 6;
            if (eval_condition(cond)) {
                in_loop = 1;
                strlcpy(loop_condition, cond, sizeof(loop_condition));
                loop_start = p;
            } else {
                skip_block = 1;
            }
            continue;
        }

        if (strncmp(token, "import ", 7) == 0) {
            char* libname = token + 7;
            while (my_isspace(*libname)) libname++;
            
            // Убираем кавычки если есть
            if (*libname == '"' || *libname == '\'') {
                char quote = *libname;
                libname++;
                char* end_quote = strchr(libname, quote);
                if (end_quote) *end_quote = '\0';
            }
            
            import_library(libname, import_depth + 1);
            continue;
        }

        if (strncmp(token, "if ", 3) == 0) {
            char* cond = token + 3;
            if (eval_condition(cond)) {
                condition_met = 1;
            } else {
                skip_block = 1;
            }
            continue;
        }
        else if (strncmp(token, "else if ", 8) == 0) {
            if (condition_met) {
                skip_block = 1;
            } else {
                char* cond = token + 8;
                if (eval_condition(cond)) {
                    condition_met = 1;
                    skip_block = 0;
                } else {
                    skip_block = 1;
                }
            }
            continue;
        }
        else if (strncmp(token, "else", 4) == 0) {
            if (condition_met) skip_block = 1;
            else skip_block = 0;
            continue;
        }
        else if (strncmp(token, "end", 3) == 0) {
            condition_met = 0;
            continue;
        }

        if (strstr(token, "++") || strstr(token, "--")) {
            char var_name[50] = {0};
            char* op_ptr = token;
            while (*op_ptr && !isalnum(*op_ptr) && !(*op_ptr == '_')) op_ptr++;
            
            int i = 0;
            while (isalnum(*op_ptr) || *op_ptr == '_') {
                if (i < sizeof(var_name) - 1) {
                    var_name[i++] = *op_ptr;
                }
                op_ptr++;
            }
            
            struct Variable* v = find_variable(var_name);
            if (v) {
                if (strstr(token, "++")) v->value += 1;
                else v->value -= 1;
            }
            continue;
        }

        if (strncmp(token, "print ", 6) == 0) {
            char* arg = token + 6;
            while (my_isspace(*arg)) arg++;

            if (*arg == '\'') {
                char* end_quote = strchr(arg + 1, '\'');
                if (end_quote) {
                    *end_quote = '\0';
                    printf("%s\n", arg + 1);
                } else {
                    printf("Error: unclosed string\n");
                }
            } else {
                struct Variable* v = find_variable(arg);
                if (v) {
                    if (v->str_value) printf("%s\n", v->str_value);
                    else {
                        print_double(v->value);
                        printf("\n");
                    }
                } else {
                    printf("Error: variable '%s' not found\n", arg);
                }
            }
        }

        else if (strncmp(token, "let ", 4) == 0) {
            char* eq = strchr(token, '=');
            if (eq) {
                char var_name[50];
                char* name_start = token + 4;
                while (my_isspace(*name_start)) name_start++;
                
                char* name_end = eq;
                while (name_end > name_start && my_isspace(*(name_end - 1))) name_end--;
                
                int name_len = name_end - name_start;
                if (name_len >= (int)sizeof(var_name)) name_len = sizeof(var_name) - 1;
                memcpy(var_name, name_start, name_len);
                var_name[name_len] = '\0';

                char* value_str = eq + 1;
                while (my_isspace(*value_str)) value_str++;

                struct Variable* v = find_variable(var_name);
                if (!v && var_count < MAX_VARS) {
                    v = &variables[var_count++];
                    strncpy(v->name, var_name, sizeof(v->name));
                    v->name[sizeof(v->name) - 1] = '\0';
                }

                if (v) {
                    if (*value_str == '\'') {
                        char* end_quote = strchr(value_str + 1, '\'');
                        if (end_quote) {
                            *end_quote = '\0';
                            if (string_count < MAX_STRINGS) {
                                strncpy(string_pool[string_count], value_str + 1, STRING_SIZE - 1);
                                string_pool[string_count][STRING_SIZE - 1] = '\0';
                                v->str_value = string_pool[string_count];
                                string_count++;
                            } else {
                                printf("Error: string pool full\n");
                            }
                            v->value = 0;
                        } else {
                            printf("Error: unclosed string\n");
                        }
                    } else {
                        v->value = eval_expression(value_str);
                        v->str_value = NULL;
                    }
                }
            } else {
                printf("Error: expected '=' in let statement\n");
            }
        }
        else if (strncmp(token, "inp ", 4) == 0) {
            char* args = token + 4;
            char type[16];
            char var_name[50];
            char prompt[100] = "";
            
            sscanf(args, "%15s %49s", type, var_name);
            
            char* quote_start = strchr(token, '\'');
            if (quote_start) {
                char* quote_end = strrchr(token, '\'');
                if (quote_end && quote_end > quote_start) {
                    *quote_end = '\0';
                    strlcpy(prompt, quote_start + 1, sizeof(prompt));
                }
            }
            
            if (prompt[0] != '\0') {
                printf("%s", prompt);
                fflush(stdout);
            }
            
            char input_buf[64];
            if (fgets(input_buf, sizeof(input_buf), stdin)) {
                input_buf[strcspn(input_buf, "\n")] = '\0';
                
                struct Variable* var = find_variable(var_name);
                if (!var) {
                    if (var_count >= MAX_VARS) {
                        printf("Error: too many variables\n");
                        continue;
                    }
                    var = &variables[var_count++];
                    strncpy(var->name, var_name, sizeof(var->name));
                    var->name[sizeof(var->name)-1] = '\0';
                }
                
                if (strcmp(type, "int") == 0) {
                    var->value = atoi(input_buf);
                    var->str_value = NULL;
                }
                else if (strcmp(type, "float") == 0) {
                    var->value = str_to_double(input_buf);
                    var->str_value = NULL;
                }
                else if (strcmp(type, "string") == 0) {
                    if (string_count < MAX_STRINGS) {
                        strncpy(string_pool[string_count], input_buf, STRING_SIZE);
                        string_pool[string_count][STRING_SIZE-1] = '\0';
                        var->str_value = string_pool[string_count];
                        string_count++;
                        var->value = 0;
                    } else {
                        printf("Error: string pool full\n");
                    }
                }
            }
        }
        else if (strncmp(token, "func ", 5) == 0) {
            char* name = token + 5;
            if (function_count >= MAX_FUNCTIONS) {
                printf("Error: too many functions\n");
                continue;
            }
            
            struct Function* func = &functions[function_count++];
            strncpy(func->name, name, sizeof(func->name));
            
            const char* end_ptr = strstr(p, "end");
            if (!end_ptr) {
                printf("Error: function missing end\n");
                continue;
            }
            
            int body_size = end_ptr - p;
            if (body_size >= MAX_FUNC_BODY_SIZE) body_size = MAX_FUNC_BODY_SIZE - 1;
            memcpy(func->body, p, body_size);
            func->body[body_size] = '\0';
            
            p = end_ptr + 3;
        }
        else if (strncmp(token, "call ", 5) == 0) {
            char* name = token + 5;
            struct Function* func = find_function(name);
            if (func) {
                execute(func->body, import_depth);
            } else {
                printf("Error: function not found: %s\n", name);
            }
        }
    }
}

void import_library(const char* libname, int import_depth) {
    if (import_depth > MAX_IMPORT_DEPTH) {
        printf("Error: import depth too deep for library '%s'\n", libname);
        return;
    }
    
    char filename[100];
    snprintf(filename, sizeof(filename), "%s.alw", libname);
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: library '%s' not found\n", libname);
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* code = malloc(size + 1);
    if (!code) {
        fclose(file);
        printf("Error: memory allocation failed\n");
        return;
    }
    
    fread(code, 1, size, file);
    code[size] = '\0';
    fclose(file);
    
    execute(code, import_depth);
    free(code);
}

int main(int argc, char* argv[]) {
    srand(time(NULL)); // Инициализация генератора случайных чисел
    
    if (argc != 2) {
        printf("Usage: %s <script.alw>\n", argv[0]);
        return 1;
    }
    
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* code = malloc(size + 1);
    if (!code) {
        perror("Memory allocation error");
        fclose(file);
        return 1;
    }
    
    fread(code, 1, size, file);
    code[size] = '\0';
    fclose(file);
    
    execute(code, 0);
    free(code);
    
    return 0;
}