# AlwexScript Language Support v3.2.0

Официальная поддержка языка **AlwexScript v3.2.0** для Visual Studio Code.

![Version](https://img.shields.io/badge/version-3.2.0-blue)
![AlwexScript](https://img.shields.io/badge/AlwexScript-v3.2.0-green)
![License](https://img.shields.io/badge/license-MIT-orange)

## Что нового в v3.2.0?

### **Строковые операции**
Добавлена полноценная поддержка работы со строками:

- Конкатенация через `+` или `concat`
- Длина строки: `len(s)` или `length(s)`
- Сравнение строк через `==` и `!=` (уже было, теперь более надёжно)
- Извлечение подстроки: `slice(s, start, end)` или `substr(s, start, end)`
- Проверка вхождения: `contains(s, substring)`
- Замена подстроки: `replace(s, old, new)`
- Разбиение строки в массив: `str_split s delim arr`

### **Улучшения**
- Исправлена обработка строк в выражениях
- Поддержка строковых литералов в кавычках (`'` и `"`)
- Добавлены сниппеты для быстрого написания строковых операций

## Возможности

### Подсветка синтаксиса
- Полная подсветка всех команд
- Комментарии `#`
- Автозакрытие скобок и кавычек
- Массивы и индексы
- HTTP команды (`http_get`, `http_post`, `http_download`)
- Файловые операции
- Математические выражения со скобками
- Классы и объекты
- Строковые функции (`len`, `slice`, `contains`, `replace`, `concat`)
- Команда `str_split`
- Расширение `.alw`

Создано с ❤️ для сообщества AlwexScript