#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

// коды статусов функций
typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_OUT_OF_MEMORY,
    STATUS_ERR_INVALID_ARG,
    STATUS_ERR_INVALID_FLAG,
    STATUS_ERR_OVERFLOW
} StatusCode;

// флаги
typedef enum {
    FLAG_LENGTH = 'l',
    FLAG_REVERSE = 'r',
    FLAG_UPPER_ODD = 'u',
    FLAG_NORMALIZE = 'n',
    FLAG_CONCAT = 'c',
    FLAG_UNKNOWN = 0
} FlagType;

// длина строки
static StatusCode str_len_custom(const char *str, size_t *out_len) {
    if (!str || !out_len) {
        return STATUS_ERR_NULL_PTR;
    }
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    *out_len = len;
    return STATUS_OK;
}

// копирование
static StatusCode str_copy_custom(const char *src, char *dest, size_t dest_size) {
    if (!src || !dest) {
        return STATUS_ERR_NULL_PTR;
    }
    size_t i = 0;
    while (src[i] != '\0' && i + 1 < dest_size) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    if (src[i] != '\0') {
        return STATUS_ERR_OUT_OF_MEMORY; // Буфер слишком мал
    }
    return STATUS_OK;
}

// символ этобуква?
static bool is_alpha_custom(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// в заглавные буквы
static char to_upper_custom(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

// символ это цифра?
static bool is_digit_custom(char c) {
    return c >= '0' && c <= '9';
}

// обработка unsigned int
static StatusCode parse_unsigned_int(const char *str, unsigned int *out_val) {
    if (!str || !out_val) {
        return STATUS_ERR_NULL_PTR;
    }
    if (str[0] == '\0') {
        return STATUS_ERR_INVALID_ARG;
    }

    unsigned long val = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!is_digit_custom(str[i])) {
            return STATUS_ERR_INVALID_ARG;
        }
        int digit = str[i] - '0';
        if (val > (ULONG_MAX - digit) / 10) {
            return STATUS_ERR_OVERFLOW;
        }
        val = val * 10 + digit;
    }

    if (val > UINT_MAX) {
        return STATUS_ERR_OVERFLOW;
    }

    *out_val = (unsigned int)val;
    return STATUS_OK;
}

// обработка аргументов командной строки
static StatusCode validate_arguments(int argc, char *argv[], FlagType *out_flag) {
    if (argc < 2) {
        return STATUS_ERR_INVALID_ARG;
    }

    const char *flag_str = argv[1];
    if (flag_str[0] != '-' || flag_str[1] == '\0' || flag_str[2] != '\0') {
        return STATUS_ERR_INVALID_FLAG;
    }

    FlagType flag = (FlagType)flag_str[1];
    *out_flag = flag;

    switch (flag) {
        case FLAG_LENGTH:
        case FLAG_REVERSE:
        case FLAG_UPPER_ODD:
        case FLAG_NORMALIZE:
            if (argc < 3) {
                return STATUS_ERR_INVALID_ARG; // Требуется строка-аргумент
            }
            break;
        case FLAG_CONCAT:
            if (argc < 4) {
                return STATUS_ERR_INVALID_ARG; // Требуется seed и хотя бы одна строка
            }
            // Проверка валидности seed (третий аргумент)
            {
                unsigned int dummy;
                StatusCode status = parse_unsigned_int(argv[3], &dummy);
                if (status != STATUS_OK) {
                    return status;
                }
            }
            break;
        default:
            return STATUS_ERR_INVALID_FLAG;
    }

    return STATUS_OK;
}

// -l длина стр
static StatusCode op_length(const char *str, size_t *out_len) {
    return str_len_custom(str, out_len);
}

// -r ревёрс
static StatusCode op_reverse(const char *str, char **out_result) {
    if (!str || !out_result) {
        return STATUS_ERR_NULL_PTR;
    }
    size_t len = 0;
    StatusCode status = str_len_custom(str, &len);
    if (status != STATUS_OK) return status;

    char *res = (char *)malloc(len + 1);
    if (!res) {
        return STATUS_ERR_OUT_OF_MEMORY;
    }

    for (size_t i = 0; i < len; i++) {
        res[i] = str[len - 1 - i];
    }
    res[len] = '\0';

    *out_result = res;
    return STATUS_OK;
}

// -u нечётные позиции в заглавные буквы
static StatusCode op_upper_odd(const char *str, char **out_result) {
    if (!str || !out_result) {
        return STATUS_ERR_NULL_PTR;
    }
    size_t len = 0;
    StatusCode status = str_len_custom(str, &len);
    if (status != STATUS_OK) return status;

    char *res = (char *)malloc(len + 1);
    if (!res) {
        return STATUS_ERR_OUT_OF_MEMORY;
    }

    for (size_t i = 0; i < len; i++) {
        if (i % 2 != 0) { // Нечётная позиция (индекс 1, 3, 5...)
            res[i] = to_upper_custom(str[i]);
        } else {
            res[i] = str[i];
        }
    }
    res[len] = '\0';

    *out_result = res;
    return STATUS_OK;
}

// -n сортировка цифры-буквы-остальное
static StatusCode op_normalize(const char *str, char **out_result) {
    if (!str || !out_result) {
        return STATUS_ERR_NULL_PTR;
    }
    size_t len = 0;
    StatusCode status = str_len_custom(str, &len);
    if (status != STATUS_OK) return status;

    char *res = (char *)malloc(len + 1);
    if (!res) {
        return STATUS_ERR_OUT_OF_MEMORY;
    }

    size_t idx = 0;

    // 1) сначала цифры
    for (size_t i = 0; i < len; i++) {
        if (is_digit_custom(str[i])) {
            res[idx++] = str[i];
        }
    }
    // 2) затем буквы
    for (size_t i = 0; i < len; i++) {
        if (is_alpha_custom(str[i])) {
            res[idx++] = str[i];
        }
    }
    // 3) остальные символы
    for (size_t i = 0; i < len; i++) {
        if (!is_digit_custom(str[i]) && !is_alpha_custom(str[i])) {
            res[idx++] = str[i];
        }
    }
    res[len] = '\0';

    *out_result = res;
    return STATUS_OK;
}

// -c конкатенация
static StatusCode op_concat_random(int argc, char *argv[], unsigned int seed, char **out_result) {
    if (!argv || !out_result) {
        return STATUS_ERR_NULL_PTR;
    }

    // собираем массив строк для конкатенации:
    // argv[2], а затем argv[4], argv[5], ... до argv[argc-1]
    int num_strings = 1 + (argc > 4 ? (argc - 4) : 0);
    const char **strings = (const char **)malloc(num_strings * sizeof(char *));
    if (!strings) {
        return STATUS_ERR_OUT_OF_MEMORY;
    }

    strings[0] = argv[2];
    int count = 1;
    for (int i = 4; i < argc; i++) {
        strings[count++] = argv[i];
    }

    // общая длина
    size_t total_len = 0;
    size_t *lens = (size_t *)malloc(num_strings * sizeof(size_t));
    if (!lens) {
        free(strings);
        return STATUS_ERR_OUT_OF_MEMORY;
    }

    for (int i = 0; i < num_strings; i++) {
        str_len_custom(strings[i], &lens[i]);
        total_len += lens[i];
    }

    char *res = (char *)malloc(total_len + 1);
    if (!res) {
        free(lens);
        free(strings);
        return STATUS_ERR_OUT_OF_MEMORY;
    }
    res[0] = '\0';

    // перемешиваем массив индексов с помощью seed
    int *indices = (int *)malloc(num_strings * sizeof(int));
    if (!indices) {
        free(res);
        free(lens);
        free(strings);
        return STATUS_ERR_OUT_OF_MEMORY;
    }
    for (int i = 0; i < num_strings; i++) {
        indices[i] = i;
    }

    srand(seed);
    for (int i = num_strings - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    // Конкатенация в случайном порядке
    size_t current_pos = 0;
    for (int i = 0; i < num_strings; i++) {
        int idx = indices[i];
        for (size_t j = 0; j < lens[idx]; j++) {
            res[current_pos++] = strings[idx][j];
        }
    }
    res[total_len] = '\0';

    // освобождение памяти
    free(indices);
    free(lens);
    free(strings);

    *out_result = res;
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    FlagType flag = FLAG_UNKNOWN;

    // обработка аргументов командной строки
    StatusCode val_status = validate_arguments(argc, argv, &flag);
    if (val_status != STATUS_OK) {
        fprintf(stderr, "Error data%d\n", val_status);
        fprintf(stderr, "Use this:\n");
        fprintf(stderr, "  %s -l <string>\n", argv[0]);
        fprintf(stderr, "  %s -r <string>\n", argv[0]);
        fprintf(stderr, "  %s -u <string>\n", argv[0]);
        fprintf(stderr, "  %s -n <string>\n", argv[0]);
        fprintf(stderr, "  %s -c <string> <seed_uint> <string> [string_3 ...]\n", argv[0]);
        return 1;
    }

    // обработка через switch/case в соответствии с требованиями
    StatusCode exec_status = STATUS_OK;
    size_t length_result = 0;
    char *string_result = NULL;

    switch (flag) {
        case FLAG_LENGTH: {
            exec_status = op_length(argv[2], &length_result);
            if (exec_status == STATUS_OK) {
                // вывод данных
                printf("%zu\n", length_result);
            }
            break;
        }
        case FLAG_REVERSE: {
            exec_status = op_reverse(argv[2], &string_result);
            if (exec_status == STATUS_OK) {
                printf("%s\n", string_result);
            }
            break;
        }
        case FLAG_UPPER_ODD: {
            exec_status = op_upper_odd(argv[2], &string_result);
            if (exec_status == STATUS_OK) {
                printf("%s\n", string_result);
            }
            break;
        }
        case FLAG_NORMALIZE: {
            exec_status = op_normalize(argv[2], &string_result);
            if (exec_status == STATUS_OK) {
                printf("%s\n", string_result);
            }
            break;
        }
        case FLAG_CONCAT: {
            unsigned int seed = 0;
            // парсинг seed уже обработался, но повторим для получения значения
            parse_unsigned_int(argv[3], &seed);
            exec_status = op_concat_random(argc, argv, seed, &string_result);
            if (exec_status == STATUS_OK) {
                printf("%s\n", string_result);
            }
            break;
        }
        default:
            exec_status = STATUS_ERR_INVALID_FLAG;
            break;
    }

    // обработка возможных ошибок выполнения функций
    if (exec_status != STATUS_OK) {
        fprintf(stderr, "Error. Status: %d\n", exec_status);
        free(string_result);
        return 1;
    }

    // освобождение динамической памяти
    free(string_result);

    return 0;
}
