import subprocess
import sys
import os
import re

# Маппинг системных вызовов для расшифровки
SYS_CALL_DESCRIPTIONS = {
    # Управление процессами
    "execve": "Запуск новой программы. Аргументы: путь к исполняемому файлу, аргументы программы, окружение.",
    "clone": "Создание нового процесса или потока.",
    "fork": "Создание нового процесса, копирующего текущий процесс.",
    "vfork": "Создание нового процесса для дальнейшего выполнения execve.",
    "exit_group": "Завершение всех потоков в текущей группе процессов.",
    "wait4": "Ожидание завершения дочернего процесса и получение его статуса.",
    "getpid": "Получение идентификатора текущего процесса.",
    "getppid": "Получение идентификатора родительского процесса.",
    "gettid": "Получение идентификатора текущего потока.",
    "setpgid": "Установка идентификатора группы процессов.",
    "getpgid": "Получение идентификатора группы процессов.",
    "setsid": "Создание новой сессии и группы процессов.",
    "getpriority": "Получение приоритета процесса.",
    "setpriority": "Установка приоритета процесса.",
    "sched_getaffinity": "Получение текущей привязки процессора для процесса.",
    "sched_setaffinity": "Установка привязки процессора для процесса.",
    "sched_yield": "Освобождение процессора текущим потоком.",
    "kill": "Отправка сигнала процессу или группе процессов.",
    "tkill": "Отправка сигнала потоку.",
    "tgkill": "Отправка сигнала конкретному потоку в группе процессов.",
    "times": "Получение времени выполнения процесса.",
    "setns": "Присоединение к другому пространству имён.",

    # Работа с потоками
    "pthread_create": "Создание нового потока (интерфейс POSIX).",
    "pthread_join": "Ожидание завершения потока (интерфейс POSIX).",
    "pthread_exit": "Завершение текущего потока (интерфейс POSIX).",
    "pthread_detach": "Отключение потока от вызывающего процесса.",
    "set_tid_address": "Установка адреса для хранения идентификатора потока.",
    "set_robust_list": "Установка списка надёжных блокировок для потока.",
    "get_robust_list": "Получение списка надёжных блокировок для потока.",

    # Работа с файлами
    "open": "Открытие файла.",
    "openat": "Открытие файла относительно указанного каталога.",
    "read": "Чтение данных из файла или потока.",
    "write": "Запись данных в файл или поток.",
    "pread64": "Чтение из файла по указанному смещению.",
    "pwrite64": "Запись в файл по указанному смещению.",
    "readv": "Чтение из файла с использованием вектора буферов.",
    "writev": "Запись в файл с использованием вектора буферов.",
    "close": "Закрытие файлового дескриптора.",
    "dup": "Дублирование файлового дескриптора.",
    "dup2": "Дублирование файлового дескриптора с указанием нового номера.",
    "dup3": "Дублирование файлового дескриптора с возможностью установки флагов.",
    "unlink": "Удаление файла.",
    "unlinkat": "Удаление файла или символической ссылки относительно указанного каталога.",
    "rename": "Переименование файла.",
    "renameat": "Переименование файла относительно указанного каталога.",
    "mkdir": "Создание нового каталога.",
    "mkdirat": "Создание нового каталога относительно указанного каталога.",
    "rmdir": "Удаление пустого каталога.",
    "getdents": "Чтение списка файлов в каталоге.",
    "stat": "Получение информации о файле.",
    "fstat": "Получение информации о файле через файловый дескриптор.",
    "lstat": "Получение информации о символической ссылке.",
    "fstatat": "Получение информации о файле относительно указанного каталога.",
    "truncate": "Установка нового размера файла.",
    "ftruncate": "Установка нового размера файла через файловый дескриптор.",
    "fsync": "Синхронизация содержимого файла с диском.",
    "fdatasync": "Синхронизация данных файла с диском без обновления метаданных.",
    "access": "Проверка прав доступа к файлу.",
    "faccessat": "Проверка прав доступа к файлу относительно указанного каталога.",
    "chmod": "Изменение прав доступа к файлу.",
    "fchmod": "Изменение прав доступа к файлу через файловый дескриптор.",
    "chown": "Изменение владельца файла.",
    "fchown": "Изменение владельца файла через файловый дескриптор.",
    "symlink": "Создание символической ссылки.",
    "link": "Создание жёсткой ссылки.",
    "readlink": "Чтение содержимого символической ссылки.",
    "readlinkat": "Чтение содержимого символической ссылки относительно указанного каталога.",
    "statfs": "Получение информации о файловой системе.",
    "fstatfs": "Получение информации о файловой системе через файловый дескриптор.",
    "utime": "Изменение временных меток файла.",
    "utimensat": "Изменение временных меток файла относительно указанного каталога.",
    "sendfile": "Копирование данных между файловыми дескрипторами.",

    # Управление ресурсами
    "prlimit64": "Получение или установка ограничений ресурсов процесса.",
    "getrlimit": "Получение текущих ограничений ресурсов процесса.",
    "setrlimit": "Установка ограничений ресурсов процесса.",
    "getrusage": "Получение информации об использовании ресурсов процесса или потока.",
    "sysinfo": "Получение общей информации о системе.",

    # Таймеры и ожидание
    "nanosleep": "Ожидание указанного интервала времени.",
    "clock_gettime": "Получение текущего времени указанного таймера.",
    "clock_settime": "Установка времени указанного таймера.",
    "gettimeofday": "Получение текущего времени.",
    "settimeofday": "Установка текущего времени.",
    "timer_create": "Создание нового таймера.",
    "timer_settime": "Установка параметров таймера.",
    "timer_gettime": "Получение параметров таймера.",
    "timer_delete": "Удаление таймера.",

    # Межпроцессное взаимодействие
    "pipe": "Создание канала для межпроцессного взаимодействия.",
    "pipe2": "Создание канала с указанием флагов (например, O_NONBLOCK или O_CLOEXEC).",
    "socketpair": "Создание пары связанных сокетов для взаимодействия.",
    "shmget": "Создание или получение сегмента разделяемой памяти.",
    "shmat": "Присоединение сегмента разделяемой памяти.",
    "shmdt": "Отключение сегмента разделяемой памяти.",
    "shmctl": "Управление сегментом разделяемой памяти.",
    "semget": "Создание или получение идентификатора семафора.",
    "semop": "Операции с семафорами (увеличение, уменьшение, проверка).",
    "semctl": "Управление параметрами семафора.",
    "msgget": "Создание или получение очереди сообщений.",
    "msgsnd": "Отправка сообщения в очередь.",
    "msgrcv": "Получение сообщения из очереди.",
    "msgctl": "Управление очередью сообщений.",

    # Прочие вызовы
    "mmap": "Отображение файла или памяти в адресное пространство процесса.",
    "munmap": "Удаление отображенной области памяти.",
    "mprotect": "Изменение прав доступа к области памяти.",
    "mremap": "Изменение размера или местоположения области памяти.",
    "getcwd": "Получение текущего рабочего каталога.",
    "chdir": "Изменение текущего рабочего каталога.",
    "renameat2": "Переименование файлов или каталогов с дополнительными флагами.",
    "ioctl": "Управление устройствами через файловые дескрипторы.",
    "fcntl": "Управление файловыми дескрипторами (например, установка флагов).",
    "poll": "Ожидание событий на файловых дескрипторах.",
    "epoll_create": "Создание нового epoll-описателя.",
    "epoll_ctl": "Управление epoll-описателем.",
    "epoll_wait": "Ожидание событий на epoll-описателе.",
    "inotify_init": "Создание нового inotify-описателя.",
    "inotify_add_watch": "Добавление наблюдения за файловым объектом.",
    "inotify_rm_watch": "Удаление наблюдения за файловым объектом."
}


def explain_syscall(line):
    """
    Возвращает пояснение для строки системного вызова.
    """
    for syscall, description in SYS_CALL_DESCRIPTIONS.items():
        if syscall in line:
            return f"{line.strip()} - {description}\n"
    return f"{line.strip()} - Неизвестный системный вызов.\n"


def parse_strace_output(strace_output_file, detailed_output_file):
    """
    Парсит файл вывода strace и добавляет пояснения.
    """
    try:
        with open(strace_output_file, "r") as input_file, open(detailed_output_file, "w") as output_file:
            output_file.write("Подробное описание системных вызовов:\n\n")
            for line in input_file:
                explanation = explain_syscall(line)
                output_file.write(explanation)
        print(f"Подробный файл с описанием создан: {detailed_output_file}")
    except Exception as e:
        print(f"Ошибка при парсинге вывода strace: {str(e)}")


def run_with_strace(program, args=None, output_file="strace_output.txt", detailed_file="strace_detailed.txt"):
    """
    Запускает программу с использованием strace, записывает системные вызовы в файл и добавляет их расшифровку.
    """
    if not os.path.exists(program):
        print(f"Ошибка: Программа '{program}' не найдена.")
        sys.exit(1)

    # Формируем команду strace
    strace_command = ["strace", "-o", output_file, "-f"]
    if args:
        strace_command.extend([program] + args)
    else:
        strace_command.append(program)

    try:
        print(f"Запуск {program} с использованием strace...")
        subprocess.run(strace_command)
        print(f"Вывод strace записан в '{output_file}'.")

        # Парсинг вывода strace и создание подробного описания
        parse_strace_output(output_file, detailed_file)
    except FileNotFoundError:
        print("Ошибка: strace не установлен на вашей системе.")
        print("Установите его с помощью команды: sudo apt install strace")
        sys.exit(1)
    except Exception as e:
        print(f"Ошибка: {str(e)}")
        sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(
            "Использование: python3 strace_wrapper.py <программа> [аргументы...]")
        sys.exit(1)

    program_to_trace = sys.argv[1]
    program_args = sys.argv[2:] if len(sys.argv) > 2 else None

    # Файлы для вывода
    output_file = "strace_output.txt"
    detailed_file = "strace_detailed.txt"

    run_with_strace(program_to_trace, args=program_args,
                    output_file=output_file, detailed_file=detailed_file)