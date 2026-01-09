#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #include <wininet.h>
    #pragma comment(lib, "wininet.lib")
    #define sleep(seconds) Sleep(seconds * 1000)
    #define MAX_VARS 1000
    #define MAX_STRINGS 300
    #define MAX_FUNCTIONS 150
    #define MAX_IMPORT_DEPTH 10
#else
    #include <unistd.h>
    #include <curl/curl.h>
    #define MAX_VARS 10000
    #define MAX_STRINGS 1000
    #define MAX_FUNCTIONS 500
    #define MAX_IMPORT_DEPTH 50
#endif

#define STRING_SIZE 1024
#define MAX_FUNC_BODY_SIZE 4096
#define MAX_LINE_LEN 1024
#define MAX_HTTP_RESPONSE 1048576  // 1MB max response
#define MAX_ARRAY_SIZE 1000
#define MAX_ARRAYS 100

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

static unsigned long rand_state = 0;

struct Variable {
    char name[50];
    double value;
    char* str_value;
};

struct Array {
    char name[50];
    double values[MAX_ARRAY_SIZE];
    char* strings[MAX_ARRAY_SIZE];
    int size;
    int is_string_array;
};

static struct Array *arrays = NULL;
static int array_count = 0;
static int array_capacity = 0;

struct Function {
    char name[50];
    char* body;
};

// HTTP response buffer
struct HttpResponse {
    char* data;
    size_t size;
};

static struct Variable *variables = NULL;
static int var_count = 0;
static int var_capacity = 0;

static char **string_pool = NULL;
static int string_count = 0;
static int string_capacity = 0;

static struct Function *functions = NULL;
static int function_count = 0;
static int function_capacity = 0;

static struct HttpResponse last_http_response = {NULL, 0};

void init_memory();
void free_memory();
int add_variable();
int add_string();
int add_function();

// HTTP functions
#ifndef _WIN32
size_t alwex_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct HttpResponse *mem = (struct HttpResponse *)userp;
    
    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if(!ptr) {
        printf("Error: not enough memory for HTTP response\n");
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    
    return realsize;
}
#endif

void http_get(const char* url) {
#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("AlwexScript/2.2", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Error: cannot initialize WinINet\n");
        return;
    }
    
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        printf("Error: cannot connect to URL\n");
        InternetCloseHandle(hInternet);
        return;
    }
    
    if (last_http_response.data) free(last_http_response.data);
    last_http_response.data = malloc(MAX_HTTP_RESPONSE);
    last_http_response.size = 0;
    
    DWORD bytesRead = 0;
    char buffer[4096];
    
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (last_http_response.size + bytesRead < MAX_HTTP_RESPONSE) {
            memcpy(last_http_response.data + last_http_response.size, buffer, bytesRead);
            last_http_response.size += bytesRead;
        }
    }
    
    last_http_response.data[last_http_response.size] = '\0';
    
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
#else
    CURL *curl = curl_easy_init();
    if(!curl) {
        printf("Error: cannot initialize curl\n");
        return;
    }
    
    if (last_http_response.data) free(last_http_response.data);
    last_http_response.data = malloc(1);
    last_http_response.size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, alwex_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&last_http_response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "AlwexScript/2.2");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        printf("Error: HTTP GET failed: %s\n", curl_easy_strerror(res));
    }
    
    curl_easy_cleanup(curl);
#endif
}

void http_post(const char* url, const char* data) {
#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("AlwexScript/2.2", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Error: cannot initialize WinINet\n");
        return;
    }
    
    URL_COMPONENTSA urlComponents;
    char hostname[256], path[1024];
    memset(&urlComponents, 0, sizeof(urlComponents));
    urlComponents.dwStructSize = sizeof(urlComponents);
    urlComponents.lpszHostName = hostname;
    urlComponents.dwHostNameLength = sizeof(hostname);
    urlComponents.lpszUrlPath = path;
    urlComponents.dwUrlPathLength = sizeof(path);
    
    if (!InternetCrackUrlA(url, 0, 0, &urlComponents)) {
        printf("Error: invalid URL\n");
        InternetCloseHandle(hInternet);
        return;
    }
    
    HINTERNET hConnect = InternetConnectA(hInternet, hostname, urlComponents.nPort, 
                                          NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        printf("Error: cannot connect to server\n");
        InternetCloseHandle(hInternet);
        return;
    }
    
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, NULL,
                                          INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) {
        printf("Error: cannot create request\n");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }
    
    const char* headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    BOOL result = HttpSendRequestA(hRequest, headers, strlen(headers), (LPVOID)data, strlen(data));
    
    if (result) {
        if (last_http_response.data) free(last_http_response.data);
        last_http_response.data = malloc(MAX_HTTP_RESPONSE);
        last_http_response.size = 0;
        
        DWORD bytesRead = 0;
        char buffer[4096];
        
        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            if (last_http_response.size + bytesRead < MAX_HTTP_RESPONSE) {
                memcpy(last_http_response.data + last_http_response.size, buffer, bytesRead);
                last_http_response.size += bytesRead;
            }
        }
        
        last_http_response.data[last_http_response.size] = '\0';
    }
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
#else
    CURL *curl = curl_easy_init();
    if(!curl) {
        printf("Error: cannot initialize curl\n");
        return;
    }
    
    if (last_http_response.data) free(last_http_response.data);
    last_http_response.data = malloc(1);
    last_http_response.size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, alwex_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&last_http_response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "AlwexScript/2.2");
    
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        printf("Error: HTTP POST failed: %s\n", curl_easy_strerror(res));
    }
    
    curl_easy_cleanup(curl);
#endif
}

void http_download(const char* url, const char* filename) {
#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("AlwexScript/2.2", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Error: cannot initialize WinINet\n");
        return;
    }
    
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        printf("Error: cannot connect to URL\n");
        InternetCloseHandle(hInternet);
        return;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Error: cannot create file %s\n", filename);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }
    
    DWORD bytesRead = 0;
    char buffer[4096];
    
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }
    
    fclose(file);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    printf("File downloaded: %s\n", filename);
#else
    CURL *curl = curl_easy_init();
    if(!curl) {
        printf("Error: cannot initialize curl\n");
        return;
    }
    
    FILE *fp = fopen(filename, "wb");
    if(!fp) {
        printf("Error: cannot create file %s\n", filename);
        curl_easy_cleanup(curl);
        return;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        printf("Error: download failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("File downloaded: %s\n", filename);
    }
    
    fclose(fp);
    curl_easy_cleanup(curl);
#endif
}

int alwex_rand() {
    rand_state = rand_state * 1103515245 + 12345;
    return (unsigned int)(rand_state / 65536) % 32768;
}

void alwex_srand(unsigned int seed) {
    rand_state = seed;
}

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

struct Array* find_array(const char* name) {
    for (int i = 0; i < array_count; i++) {
        if (strcmp(arrays[i].name, name) == 0) {
            return &arrays[i];
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
        
        if (isdigit(*p) || (*p == '-' && isdigit(*(p+1))) || *p == '.') {
            current = str_to_double(p);
            has_value = 1;
            if (*p == '-') p++;
            while (isdigit(*p) || *p == '.') p++;
            continue;
        }
        
        if (isalpha(*p) || *p == '_') {
            char var_name[50] = {0};
            int i = 0;
            
            while (isalnum(*p) || *p == '_') {
                if (i < (int)(sizeof(var_name) - 1)) {
                    var_name[i++] = *p;
                }
                p++;
            }
            var_name[i] = '\0';

            if (strcmp(var_name, "__rand_internal") == 0) {
                current = (double)alwex_rand();
            } else {
                struct Variable* v = find_variable(var_name);
                if (v) {
                    current = v->str_value ? 0 : v->value;
                } else {
                    current = 0;
                }
            }
            has_value = 1;
            continue;
        }
        
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '%') {
            if (has_value) {
                switch (op) {
                    case '+': result += current; break;
                    case '-': result -= current; break;
                    case '*': result *= current; break;
                    case '%': 
                        if (current != 0) result = (int)result % (int)current;
                        else printf("Error: modulo by zero\n");
                        break;
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
        
        if (*p == '(' || *p == ')') {
            p++;
            continue;
        }
        
        p++;
    }
    
    if (has_value) {
        switch (op) {
            case '+': result += current; break;
            case '-': result -= current; break;
            case '*': result *= current; break;
            case '%': 
                if (current != 0) result = (int)result % (int)current;
                break;
            case '/': 
                if (current != 0) result /= current;
                break;
        }
    }
    
    return result;
}

int eval_condition(const char* cond) {
    const char* operators[] = {"==", "!=", ">=", "<=", ">", "<"};
    int op_count = 6;
    
    for (int i = 0; i < op_count; i++) {
        const char* op_pos = strstr(cond, operators[i]);
        if (op_pos) {
            char left[100] = {0};
            char right[100] = {0};
            
            int left_len = op_pos - cond;
            if (left_len >= 100) left_len = 99;
            strncpy(left, cond, left_len);
            left[left_len] = '\0';
            
            strcpy(right, op_pos + strlen(operators[i]));

            char* ltrim = left;
            while (my_isspace(*ltrim)) ltrim++;
            char* lend = left + strlen(left) - 1;
            while (lend > left && my_isspace(*lend)) *lend-- = '\0';
            
            char* rtrim = right;
            while (my_isspace(*rtrim)) rtrim++;
            char* rend = right + strlen(right) - 1;
            while (rend > right && my_isspace(*rend)) *rend-- = '\0';

            int left_is_string = (*ltrim == '\'' || *ltrim == '"');
            int right_is_string = (*rtrim == '\'' || *rtrim == '"');
            
            if (left_is_string || right_is_string) {
                char left_str[100] = {0};
                char right_str[100] = {0};
                
                if (left_is_string) {
                    strcpy(left_str, ltrim + 1);
                    char* quote_end = strchr(left_str, ltrim[0]);
                    if (quote_end) *quote_end = '\0';
                } else {
                    struct Variable* v = find_variable(ltrim);
                    if (v && v->str_value) {
                        strcpy(left_str, v->str_value);
                    } else if (v) {
                        snprintf(left_str, sizeof(left_str), "%.0f", v->value);
                    }
                }
                
                if (right_is_string) {
                    strcpy(right_str, rtrim + 1);
                    char* quote_end = strchr(right_str, rtrim[0]);
                    if (quote_end) *quote_end = '\0';
                } else {
                    struct Variable* v = find_variable(rtrim);
                    if (v && v->str_value) {
                        strcpy(right_str, v->str_value);
                    } else if (v) {
                        snprintf(right_str, sizeof(right_str), "%.0f", v->value);
                    }
                }
                
                int cmp = strcmp(left_str, right_str);
                
                if (strcmp(operators[i], "==") == 0) return cmp == 0;
                if (strcmp(operators[i], "!=") == 0) return cmp != 0;
                return 0;
            } else {
                double left_val = 0;
                double right_val = 0;

                if (isdigit(*ltrim) || *ltrim == '-') {
                    left_val = str_to_double(ltrim);
                } else {
                    struct Variable* v = find_variable(ltrim);
                    if (v) left_val = v->value;
                }

                if (isdigit(*rtrim) || *rtrim == '-') {
                    right_val = str_to_double(rtrim);
                } else {
                    struct Variable* v = find_variable(rtrim);
                    if (v) right_val = v->value;
                }
                
                if (strcmp(operators[i], "==") == 0) return left_val == right_val;
                if (strcmp(operators[i], "!=") == 0) return left_val != right_val;
                if (strcmp(operators[i], ">=") == 0) return left_val >= right_val;
                if (strcmp(operators[i], "<=") == 0) return left_val <= right_val;
                if (strcmp(operators[i], ">") == 0) return left_val > right_val;
                if (strcmp(operators[i], "<") == 0) return left_val < right_val;
            }
        }
    }
    
    return 0;
}

void import_library(const char* libname, int import_depth);

void init_memory() {
    var_capacity = 10;
    variables = malloc(var_capacity * sizeof(struct Variable));
    
    string_capacity = 10;
    string_pool = malloc(string_capacity * sizeof(char*));
    for (int i = 0; i < string_capacity; i++) {
        string_pool[i] = malloc(STRING_SIZE);
    }
    
    function_capacity = 10;
    functions = malloc(function_capacity * sizeof(struct Function));

    array_capacity = 10;
    arrays = calloc(array_capacity, sizeof(struct Array));
}

void free_memory() {
    free(variables);
    
    for (int i = 0; i < string_capacity; i++) {
        free(string_pool[i]);
    }
    free(string_pool);
    
    for (int i = 0; i < function_count; i++) {
        free(functions[i].body);
    }
    free(functions);

    if (arrays) {
        free(arrays);
    }
    
    if (last_http_response.data) {
        free(last_http_response.data);
    }
}

int add_variable() {
    if (var_count >= var_capacity) {
        var_capacity *= 2;
        variables = realloc(variables, var_capacity * sizeof(struct Variable));
        if (!variables) {
            printf("Error: memory allocation failed for variables\n");
            return -1;
        }
    }
    return var_count++;
}

int add_string() {
    if (string_count >= string_capacity) {
        int new_capacity = string_capacity * 2;
        char **new_pool = realloc(string_pool, new_capacity * sizeof(char*));
        if (!new_pool) {
            printf("Error: memory allocation failed for string pool\n");
            return -1;
        }
        
        for (int i = string_capacity; i < new_capacity; i++) {
            new_pool[i] = malloc(STRING_SIZE);
            if (!new_pool[i]) {
                printf("Error: memory allocation failed for string %d\n", i);
                return -1;
            }
        }
        
        string_pool = new_pool;
        string_capacity = new_capacity;
    }
    return string_count++;
}

int add_function() {
    if (function_count >= function_capacity) {
        function_capacity *= 2;
        functions = realloc(functions, function_capacity * sizeof(struct Function));
        if (!functions) {
            printf("Error: memory allocation failed for functions\n");
            return -1;
        }
    }
    return function_count++;
}

int add_array() {
    if (array_count >= array_capacity) {
        array_capacity *= 2;
        arrays = realloc(arrays, array_capacity * sizeof(struct Array));
    }
    return array_count++;
}

void execute(const char* code, int import_depth) {
    if (import_depth > MAX_IMPORT_DEPTH) {
        printf("Error: import depth too deep (max %d levels)\n", MAX_IMPORT_DEPTH);
        return;
    }

    char line[MAX_LINE_LEN];
    const char* p = code;

    int skip_block = 0;
    int condition_met = 0;
    int condition_stack[20] = {0};
    int condition_depth = 0;
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

        // ==================== ARRAYS ====================
        if (strncmp(token, "let ", 4) == 0 && strchr(token, '[') && strchr(token, ']')) {
            char* eq = strchr(token, '=');
            if (eq) {
                char array_name[50] = {0};
                char* name_start = token + 4;
                while (my_isspace(*name_start)) name_start++;
                char* name_end = eq;
                while (name_end > name_start && my_isspace(*(name_end - 1))) name_end--;
                
                int name_len = name_end - name_start;
                if (name_len >= 50) name_len = 49;
                strncpy(array_name, name_start, name_len);
                array_name[name_len] = '\0';

                char* content = eq + 1;
                while (my_isspace(*content)) content++;

                if (*content == '[') {
                    struct Array* arr = find_array(array_name);
                    if (!arr) {
                        int idx = add_array();
                        if (idx < 0) continue;
                        arr = &arrays[idx];
                        strcpy(arr->name, array_name);
                        arr->size = 0;
                        arr->is_string_array = 0;
                    } else {
                        arr->size = 0;
                    }

                    content++;
                    
                    while (*content && *content != ']') {
                        while (my_isspace(*content) || *content == ',') content++;
                        if (*content == ']') break;

                        if (*content == '\'' || *content == '"') {
                            char quote = *content;
                            content++;
                            char temp_str[STRING_SIZE] = {0};
                            int str_pos = 0;
                            
                            while (*content && *content != quote && str_pos < STRING_SIZE - 1) {
                                temp_str[str_pos++] = *content++;
                            }
                            temp_str[str_pos] = '\0';
                            
                            if (*content == quote) content++;
                            
                            arr->is_string_array = 1;
                            int str_idx = add_string();
                            if (str_idx >= 0 && arr->size < MAX_ARRAY_SIZE) {
                                strcpy(string_pool[str_idx], temp_str);
                                arr->strings[arr->size++] = string_pool[str_idx];
                            }
                        } 

                        else if (isdigit(*content) || *content == '-' || *content == '.') {
                            char num_str[50] = {0};
                            int num_pos = 0;
                            
                            if (*content == '-') num_str[num_pos++] = *content++;
                            
                            while (*content && (isdigit(*content) || *content == '.') && num_pos < 49) {
                                num_str[num_pos++] = *content++;
                            }
                            num_str[num_pos] = '\0';
                            
                            if (arr->size < MAX_ARRAY_SIZE) {
                                arr->values[arr->size++] = str_to_double(num_str);
                            }
                        } else {
                            content++;
                        }
                    }
                }
            }
            continue;
        }

        // arr_get arr 0
        else if (strncmp(token, "arr_get ", 8) == 0) {
            char* args = token + 8;
            char array_name[50] = {0};
            int index = -1;
            
            sscanf(args, "%49s %d", array_name, &index);
            
            struct Array* arr = find_array(array_name);
            if (arr && index >= 0 && index < arr->size) {
                struct Variable* v = find_variable("arr_get_result");
                if (!v) {
                    int idx = add_variable();
                    if (idx < 0) continue;
                    v = &variables[idx];
                    strcpy(v->name, "arr_get_result");
                }
                
                if (arr->is_string_array) {
                    int str_idx = add_string();
                    if (str_idx >= 0) {
                        strcpy(string_pool[str_idx], arr->strings[index]);
                        v->str_value = string_pool[str_idx];
                        v->value = 0;
                    }
                } else {
                    v->value = arr->values[index];
                    v->str_value = NULL;
                }
            }
            continue;
        }

        // arr_set arr 0 100
        else if (strncmp(token, "arr_set ", 8) == 0) {
            char* args = token + 8;
            char array_name[50] = {0};
            int index = -1;
            double value = 0;
            
            sscanf(args, "%49s %d %lf", array_name, &index, &value);
            
            struct Array* arr = find_array(array_name);
            if (arr && index >= 0 && index < arr->size && !arr->is_string_array) {
                arr->values[index] = value;
            }
            continue;
        }

        // arr_length arr
        else if (strncmp(token, "arr_length ", 11) == 0) {
            char* args = token + 11;
            while (my_isspace(*args)) args++;

            char array_name[50] = {0};
            int i = 0;
            while (*args && !my_isspace(*args) && i < 49) {
                array_name[i++] = *args++;
            }
            array_name[i] = '\0';
            
            struct Array* arr = find_array(array_name);
            if (arr) {
                struct Variable* v = find_variable("arr_length_result");
                if (!v) {
                    int idx = add_variable();
                    if (idx < 0) continue;
                    v = &variables[idx];
                    strcpy(v->name, "arr_length_result");
                }
                v->value = arr->size;
                v->str_value = NULL;
            }
            continue;
        }

        // arr_push arr 42
        else if (strncmp(token, "arr_push ", 9) == 0) {
            char* args = token + 9;
            char array_name[50] = {0};
            
            char* space = strchr(args, ' ');
            if (space) {
                int name_len = space - args;
                if (name_len >= 50) name_len = 49;
                strncpy(array_name, args, name_len);
                array_name[name_len] = '\0';
                
                struct Array* arr = find_array(array_name);
                if (arr && arr->size < MAX_ARRAY_SIZE) {
                    char* value_str = space + 1;
                    while (my_isspace(*value_str)) value_str++;
                    
                    if (*value_str == '\'' || *value_str == '"') {
                        char quote = *value_str;
                        value_str++;
                        char* end_quote = strchr(value_str, quote);
                        if (end_quote) {
                            *end_quote = '\0';
                            arr->is_string_array = 1;
                            int str_idx = add_string();
                            if (str_idx >= 0) {
                                strcpy(string_pool[str_idx], value_str);
                                arr->strings[arr->size++] = string_pool[str_idx];
                            }
                        }
                    } else {
                        arr->values[arr->size++] = str_to_double(value_str);
                    }
                }
            }
            continue;
        }

        // ==================== RANDOM ====================
        else if (strcmp(token, "let __rand_internal = 0") == 0 || 
                 strstr(token, "__rand_internal")) {
            struct Variable* v = find_variable("__rand_internal");
            if (!v) {
                int idx = add_variable();
                if (idx < 0) continue;
                v = &variables[idx];
                strcpy(v->name, "__rand_internal");
            }
            v->value = rand() % 32768;
            v->str_value = NULL;
            continue;
        }

        // ==================== ENDLOOP = BREAK ====================
        if (strncmp(token, "endloop", 7) == 0) {
            if (in_loop) {
                in_loop = 0;
                loop_start = NULL;
            }
            continue;
        }

        if (in_loop) {
            if (*p == '\0' || strstr(p, "while ") == p) {
                if (eval_condition(loop_condition)) {
                    p = loop_start;
                    continue;
                } else {
                    in_loop = 0;
                }
            }
        }

        // ==================== WHILE ====================
        if (strncmp(token, "while ", 6) == 0) {
            char* cond = token + 6;
            if (eval_condition(cond)) {
                in_loop = 1;
                strlcpy(loop_condition, cond, sizeof(loop_condition));
                loop_start = p;
            } else {
                skip_block = 1;
                while (*p) {
                    const char* check = p;
                    while (*check == ' ' || *check == '\t') check++;
                    if (strncmp(check, "endloop", 7) == 0) {
                        p = check + 7;
                        while (*p && *p != '\n') p++;
                        if (*p == '\n') p++;
                        break;
                    }
                    p++;
                }
                skip_block = 0;
            }
            continue;
        }

        // ==================== IMPORT ====================
        if (strncmp(token, "import ", 7) == 0) {
            char* libname = token + 7;
            while (my_isspace(*libname)) libname++;
            if (*libname == '"' || *libname == '\'') {
                char quote = *libname;
                libname++;
                char* end_quote = strchr(libname, quote);
                if (end_quote) *end_quote = '\0';
            }
            
            import_library(libname, import_depth + 1);
            continue;
        }

        // ==================== IF/ELSE ====================
        if (strncmp(token, "if ", 3) == 0) {
            char* cond = token + 3;
            int cond_result = eval_condition(cond);

            if (condition_depth < 20) {
                condition_stack[condition_depth++] = cond_result;
            }
            
            if (!cond_result) {
                skip_block = 1;
            }
            continue;
        }
        else if (strncmp(token, "else if ", 8) == 0) {
            int previous_met = 0;
            if (condition_depth > 0) {
                previous_met = condition_stack[condition_depth - 1];
            }
            
            if (previous_met) {
                skip_block = 1;
            } else {
                char* cond = token + 8;
                int cond_result = eval_condition(cond);
                
                if (cond_result) {
                    condition_stack[condition_depth - 1] = 1;
                    skip_block = 0;
                } else {
                    skip_block = 1;
                }
            }
            continue;
        }
        else if (strncmp(token, "else", 4) == 0 && token[4] != ' ' && token[4] != 'i') {
            int previous_met = 0;
            if (condition_depth > 0) {
                previous_met = condition_stack[condition_depth - 1];
            }
            
            if (previous_met) {
                skip_block = 1;
            } else {
                skip_block = 0;
                if (condition_depth > 0) {
                    condition_stack[condition_depth - 1] = 1;
                }
            }
            continue;
        }
        else if (strncmp(token, "end", 3) == 0 && strncmp(token, "endloop", 7) != 0) {
            if (condition_depth > 0) {
                condition_depth--;
            }
            skip_block = 0;
            continue;
        }

        // ==================== INC/DEC ====================
        if (strstr(token, "++") || strstr(token, "--")) {
            char var_name[50] = {0};
            int is_increment = strstr(token, "++") != NULL;

            char* start = token;
            while (my_isspace(*start)) start++;

            int i = 0;
            while (*start && (isalnum(*start) || *start == '_') && i < 49) {
                var_name[i++] = *start++;
            }
            var_name[i] = '\0';

            while (my_isspace(*start)) start++;
            
            if ((is_increment && strncmp(start, "++", 2) == 0) ||
                (!is_increment && strncmp(start, "--", 2) == 0)) {
                
                struct Variable* v = find_variable(var_name);
                if (v) {
                    if (is_increment) {
                        v->value += 1;
                    } else {
                        v->value -= 1;
                    }
                } else {
                    printf("Error: variable '%s' not found\n", var_name);
                }
            }
            continue;
        }

        // ==================== WAIT ====================
        else if (strncmp(token, "wait ", 5) == 0) {
            char* sec_str = token + 5;
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
            continue;
        }

        // ==================== HTTP GET ====================
        else if (strncmp(token, "http_get ", 9) == 0) {
            char* url = token + 9;
            while (my_isspace(*url)) url++;
            
            if (*url == '"' || *url == '\'') {
                char quote = *url;
                url++;
                char* end_quote = strchr(url, quote);
                if (end_quote) *end_quote = '\0';
            }
            
            http_get(url);
            
            struct Variable* v = find_variable("http_response");
            if (!v) {
                int idx = add_variable();
                if (idx >= 0) {
                    v = &variables[idx];
                    strcpy(v->name, "http_response");
                }
            }
            
            if (v && last_http_response.data) {
                int str_idx = add_string();
                if (str_idx >= 0) {
                    strncpy(string_pool[str_idx], last_http_response.data, STRING_SIZE - 1);
                    string_pool[str_idx][STRING_SIZE - 1] = '\0';
                    v->str_value = string_pool[str_idx];
                    v->value = (double)last_http_response.size;
                }
            }
            continue;
        }

        // ==================== HTTP POST ====================
        else if (strncmp(token, "http_post ", 10) == 0) {
            char* args = token + 10;
            while (my_isspace(*args)) args++;
            
            char url[256] = {0};
            char data[512] = {0};
            
            if (*args == '"' || *args == '\'') {
                char quote = *args;
                args++;
                char* end_quote = strchr(args, quote);
                if (end_quote) {
                    strncpy(url, args, end_quote - args);
                    args = end_quote + 1;
                }
            }
            
            while (my_isspace(*args)) args++;
            
            if (*args == '"' || *args == '\'') {
                char quote = *args;
                args++;
                char* end_quote = strchr(args, quote);
                if (end_quote) {
                    strncpy(data, args, end_quote - args);
                }
            } else {
                strcpy(data, args);
            }
            
            http_post(url, data);
            
            struct Variable* v = find_variable("http_response");
            if (!v) {
                int idx = add_variable();
                if (idx >= 0) {
                    v = &variables[idx];
                    strcpy(v->name, "http_response");
                }
            }
            
            if (v && last_http_response.data) {
                int str_idx = add_string();
                if (str_idx >= 0) {
                    strncpy(string_pool[str_idx], last_http_response.data, STRING_SIZE - 1);
                    string_pool[str_idx][STRING_SIZE - 1] = '\0';
                    v->str_value = string_pool[str_idx];
                    v->value = (double)last_http_response.size;
                }
            }
            continue;
        }

        // ==================== HTTP DOWNLOAD ====================
        else if (strncmp(token, "http_download ", 14) == 0) {
            char* args = token + 14;
            while (my_isspace(*args)) args++;
            
            char url[256] = {0};
            char filename[100] = {0};
            
            if (*args == '"' || *args == '\'') {
                char quote = *args;
                args++;
                char* end_quote = strchr(args, quote);
                if (end_quote) {
                    strncpy(url, args, end_quote - args);
                    args = end_quote + 1;
                }
            }
            
            while (my_isspace(*args)) args++;
            
            if (*args == '"' || *args == '\'') {
                char quote = *args;
                args++;
                char* end_quote = strchr(args, quote);
                if (end_quote) {
                    strncpy(filename, args, end_quote - args);
                }
            } else {
                strcpy(filename, args);
            }
            
            http_download(url, filename);
            continue;
        }

        // ==================== PRINT ====================
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
            } 

            else if (strchr(arg, '[')) {
                char array_name[50] = {0};
                int index = -1;

                char* bracket_start = strchr(arg, '[');
                char* bracket_end = strchr(arg, ']');
                
                if (bracket_start && bracket_end) {

                    int name_len = bracket_start - arg;
                    if (name_len >= 50) name_len = 49;
                    strncpy(array_name, arg, name_len);
                    array_name[name_len] = '\0';

                    char* trim_start = array_name;
                    while (*trim_start && my_isspace(*trim_start)) trim_start++;
                    if (trim_start != array_name) {
                        memmove(array_name, trim_start, strlen(trim_start) + 1);
                    }
                    char* trim_end = array_name + strlen(array_name) - 1;
                    while (trim_end > array_name && my_isspace(*trim_end)) {
                        *trim_end = '\0';
                        trim_end--;
                    }

                    char index_str[20] = {0};
                    int idx_len = bracket_end - bracket_start - 1;
                    if (idx_len > 0 && idx_len < 20) {
                        strncpy(index_str, bracket_start + 1, idx_len);
                        index_str[idx_len] = '\0';
                        index = atoi(index_str);
                    }

                    struct Array* arr = find_array(array_name);
                    if (arr) {
                        if (index >= 0 && index < arr->size) {
                            if (arr->is_string_array) {
                                printf("%s\n", arr->strings[index]);
                            } else {
                                print_double(arr->values[index]);
                                printf("\n");
                            }
                        } else {
                            printf("Error: array '%s' index %d out of bounds (size: %d)\n", 
                                array_name, index, arr->size);
                        }
                    } else {
                        printf("Error: array '%s' not found\n", array_name);
                    }
                } else {
                    printf("Error: invalid array syntax\n");
                }
            }
            else {
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
            continue;
        }

        // ==================== LET ====================
        else if (strncmp(token, "let ", 4) == 0 && !strchr(token, '[')) {
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
                if (!v) {
                    int idx = add_variable();
                    if (idx < 0) continue;
                    v = &variables[idx];
                    strncpy(v->name, var_name, sizeof(v->name));
                    v->name[sizeof(v->name) - 1] = '\0';
                }

                if (v) {
                    if (*value_str == '\'') {
                        char* end_quote = strchr(value_str + 1, '\'');
                        if (end_quote) {
                            *end_quote = '\0';
                            int str_idx = add_string();
                            if (str_idx >= 0) {
                                strncpy(string_pool[str_idx], value_str + 1, STRING_SIZE - 1);
                                string_pool[str_idx][STRING_SIZE - 1] = '\0';
                                v->str_value = string_pool[str_idx];
                                v->value = 0;
                            } else {
                                printf("Error: string pool full\n");
                            }
                        } else {
                            printf("Error: unclosed string\n");
                        }
                    } 
                    else if (strstr(value_str, "__rand_internal")) {
                        v->value = eval_expression(value_str);
                        v->str_value = NULL;
                    }
                    else {
                        v->value = eval_expression(value_str);
                        v->str_value = NULL;
                    }
                }
            } else {
                printf("Error: expected '=' in let statement\n");
            }
            continue;
        }

        // ==================== INP ====================
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
                    int idx = add_variable();
                    if (idx < 0) continue;
                    var = &variables[idx];
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
                    int str_idx = add_string();
                    if (str_idx >= 0) {
                        strncpy(string_pool[str_idx], input_buf, STRING_SIZE);
                        string_pool[str_idx][STRING_SIZE-1] = '\0';
                        var->str_value = string_pool[str_idx];
                        var->value = 0;
                    } else {
                        printf("Error: string pool full\n");
                    }
                }
            }
            continue;
        }

        // ==================== FUNC ====================
        else if (strncmp(token, "func ", 5) == 0) {
            char* name = token + 5;
            
            int func_idx = add_function();
            if (func_idx < 0) continue;
            
            struct Function* func = &functions[func_idx];
            strncpy(func->name, name, sizeof(func->name));
            
            const char* end_ptr = strstr(p, "end");
            if (!end_ptr) {
                printf("Error: function missing end\n");
                continue;
            }
            
            int body_size = end_ptr - p;
            func->body = malloc(body_size + 1);
            if (!func->body) {
                printf("Error: memory allocation failed for function body\n");
                continue;
            }
            
            memcpy(func->body, p, body_size);
            func->body[body_size] = '\0';
            
            p = end_ptr + 3;
            continue;
        }

        // ==================== CALL ====================
        else if (strncmp(token, "call ", 5) == 0) {
            char* args = token + 5;
            char func_name[50] = {0};

            int i = 0;
            while (*args && !my_isspace(*args) && i < 49) {
                func_name[i++] = *args++;
            }
            func_name[i] = '\0';
            
            struct Function* func = find_function(func_name);
            if (func) {

                struct Variable saved_vars[10];
                int saved_count = 0;

                while (*args && my_isspace(*args)) args++;
                
                char* param_names[] = {"min", "max", "arr", "index", "value", "len", "a", "b", "c", "d"};
                int param_idx = 0;
                
                while (*args && param_idx < 10) {
                    while (*args && my_isspace(*args)) args++;
                    if (!*args) break;

                    char param_value[100] = {0};
                    int j = 0;
                    while (*args && !my_isspace(*args) && j < 99) {
                        param_value[j++] = *args++;
                    }
                    param_value[j] = '\0';

                    struct Variable* param_var = find_variable(param_names[param_idx]);
                    if (!param_var) {
                        int idx = add_variable();
                        if (idx < 0) break;
                        param_var = &variables[idx];
                        strcpy(param_var->name, param_names[param_idx]);
                    } else {
                        saved_vars[saved_count] = *param_var;
                        saved_count++;
                    }

                    if (isdigit(param_value[0]) || param_value[0] == '-') {
                        param_var->value = str_to_double(param_value);
                        param_var->str_value = NULL;
                    } else {

                        struct Variable* source_var = find_variable(param_value);
                        if (source_var) {
                            param_var->value = source_var->value;
                            param_var->str_value = source_var->str_value;
                        }
                    }
                    
                    param_idx++;
                }

                execute(func->body, import_depth);

                for (int k = 0; k < saved_count; k++) {
                    struct Variable* v = find_variable(saved_vars[k].name);
                    if (v) {
                        *v = saved_vars[k];
                    }
                }
            } else {
                printf("Error: function not found: %s\n", func_name);
            }
            continue;
        }

        // ==================== EXEC ====================
        else if (strncmp(token, "exec ", 5) == 0) {
            char* command = token + 5;
            while (my_isspace(*command)) command++;
            
            #ifdef _WIN32
                system(command);
            #else
                int result = system(command);
                if (result == -1) {
                    printf("Error: failed to execute command\n");
                }
            #endif
            continue;
        }

        // ==================== FILE OPERATIONS ====================
        else if (strncmp(token, "file_write ", 11) == 0) {
            char* args = token + 11;
            while (my_isspace(*args)) args++;

            char filename[100];
            int i = 0;
            while (*args && !my_isspace(*args) && i < (int)(sizeof(filename) - 1)) {
                filename[i++] = *args++;
            }
            filename[i] = '\0';

            while (my_isspace(*args)) args++;

            FILE* file = fopen(filename, "w");
            if (file) {
                fputs(args, file);
                fclose(file);
            } else {
                printf("Error: cannot open file %s for writing\n", filename);
            }
            continue;
        }
        else if (strncmp(token, "file_read ", 10) == 0) {
            char* filename = token + 10;
            while (my_isspace(*filename)) filename++;
            
            FILE* file = fopen(filename, "r");
            if (file) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), file)) {
                    printf("%s", buffer);
                }
                fclose(file);
            } else {
                printf("Error: cannot open file %s for reading\n", filename);
            }
            continue;
        }
        else if (strncmp(token, "file_append ", 12) == 0) {
            char* args = token + 12;
            while (my_isspace(*args)) args++;

            char filename[100];
            int i = 0;
            while (*args && !my_isspace(*args) && i < (int)(sizeof(filename) - 1)) {
                filename[i++] = *args++;
            }
            filename[i] = '\0';

            while (my_isspace(*args)) args++;

            FILE* file = fopen(filename, "a");
            if (file) {
                fputs(args, file);
                fclose(file);
            } else {
                printf("Error: cannot open file %s for appending\n", filename);
            }
            continue;
        }
        else if (strncmp(token, "file_exists ", 12) == 0) {
            char* filename = token + 12;
            while (my_isspace(*filename)) filename++;
            
            FILE* file = fopen(filename, "r");
            if (file) {
                fclose(file);
                printf("true\n");
            } else {
                printf("false\n");
            }
            continue;
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
    alwex_srand((unsigned int)(time(NULL) ^ (clock() << 16)));
    
    if (argc != 2) {
        printf("Usage: %s <script.alw>\n", argv[0]);
        return 1;
    }

    init_memory();
    
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        free_memory();
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* code = malloc(size + 1);
    if (!code) {
        perror("Memory allocation error");
        fclose(file);
        free_memory();
        return 1;
    }
    
    fread(code, 1, size, file);
    code[size] = '\0';
    fclose(file);
    
    execute(code, 0);

    free(code);
    free_memory();
    
    return 0;
}
