import time
import tkinter as tk
from tkinter import messagebox
import subprocess
import threading
import queue
import os
import sys

class PipeGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Процессы и Пайпы")

        # Родительский процесс
        self.parent_frame = tk.LabelFrame(root, text="Родительский процесс", padx=10, pady=10)
        self.parent_frame.grid(row=0, column=0, padx=20, pady=20)

        self.parent_status = tk.Label(self.parent_frame, text="Статус: Ожидание", fg="orange")
        self.parent_status.grid(row=0, column=0)

        # Дочерний процесс
        self.child_frame = tk.LabelFrame(root, text="Дочерний процесс", padx=10, pady=10)
        self.child_frame.grid(row=0, column=2, padx=20, pady=20)

        self.child_status = tk.Label(self.child_frame, text="Статус: Ожидание", fg="orange")
        self.child_status.grid(row=0, column=0)

        # Пайп между процессами
        self.pipe_label = tk.Label(root, text="--> PIPE -->", font=("Arial", 12))
        self.pipe_label.grid(row=0, column=1)

        # Поле для ввода имени файла
        self.file_label = tk.Label(self.parent_frame, text="Введите имя файла для записи:")
        self.file_label.grid(row=1, column=0, pady=5)
        self.file_entry = tk.Entry(self.parent_frame, width=20)
        self.file_entry.grid(row=2, column=0)

        # Кнопка отправки имени файла
        self.file_button = tk.Button(self.parent_frame, text="Отправить имя файла", command=self.send_file_name)
        self.file_button.grid(row=3, column=0, pady=5)

        # Поле для выбора чисел
        self.input_label = tk.Label(self.parent_frame, text="Выберите два числа:")
        self.input_label.grid(row=4, column=0, pady=5)

        # Ползунки для выбора чисел
        self.num1_scale = tk.Scale(self.parent_frame, from_=1, to=100, orient=tk.HORIZONTAL, label="Число 1")
        self.num1_scale.grid(row=5, column=0)
        self.num2_scale = tk.Scale(self.parent_frame, from_=1, to=100, orient=tk.HORIZONTAL, label="Число 2")
        self.num2_scale.grid(row=6, column=0)

        # Кнопка отправки чисел
        self.send_button = tk.Button(self.parent_frame, text="Отправить числа", command=self.send_to_child)
        self.send_button.grid(row=7, column=0, pady=5)

        # Поле для вывода результата
        self.result_label = tk.Label(self.child_frame, text="Результат:")
        self.result_label.grid(row=1, column=0)
        self.result_display = tk.Text(self.child_frame, height=5, width=30)
        self.result_display.grid(row=2, column=0)

        # Кнопка выхода
        self.exit_button = tk.Button(root, text="Выход", command=self.exit_program)
        self.exit_button.grid(row=3, column=0, pady=20)

        # Переменная для процесса
        self.process = None
        self.output_file = None  # Переменная для хранения имени файла

        # Очередь для вывода
        self.output_queue = queue.Queue()

        self.start_parent_process()

    def start_parent_process(self):
        """Запуск родительского процесса"""
        try:
            # Путь к родительскому процессу (выключаем буферизацию stdout и stderr)
            self.process = subprocess.Popen(
                ['parent_win.exe'],  # Замените это на путь к родительскому процессу
                stdin=subprocess.PIPE, 
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
                bufsize=0  # Отключаем буферизацию
            )

            self.parent_status.config(text="Статус: Активен", fg="green")
            
            # Запуск потока для чтения stdout и stderr
            self.stdout_thread = threading.Thread(target=self.read_from_process)
            self.stdout_thread.start()

            self.stderr_thread = threading.Thread(target=self.read_from_stderr)
            self.stderr_thread.start()
        except Exception as e:
            messagebox.showerror("Ошибка", f"Не удалось запустить процесс: {str(e)}")

    def send_file_name(self):
        """Отправка имени файла в родительский процесс"""
        file_name = self.file_entry.get()
        if not file_name:
            messagebox.showwarning("Ошибка", "Пожалуйста, введите имя файла.")
            return

        self.output_file = file_name.strip()
        
        try:
            self.process.stdin.write(self.output_file + "\n")
            self.process.stdin.flush()
            self.file_button.config(state="disabled")
        except Exception as e:
            messagebox.showerror("Ошибка", f"Ошибка отправки имени файла: {str(e)}")

        self.output_queue.put(f"Файл '{file_name}' создан для записи.\n")

    def send_to_child(self):
        """Отправка чисел в родительский процесс"""
        num1 = self.num1_scale.get()
        num2 = self.num2_scale.get()

        input_data = f"{num1} {num2}"


        try:
            self.process.stdin.write(input_data + "\n")
            self.process.stdin.flush()
            self.output_queue.put(f"Числа отправлены: {num1} и {num2}\n")
            time.sleep(2)  # Задержка для имитации работы процесса

            if num2 == 0:
                result = "Division by zero. Terminating.\n"
            else:
                result = f"Result: {num1} / {num2} = {num1 / num2:.6f}\n"

            self.output_queue.put(result)

        except Exception as e:
            messagebox.showerror("Ошибка", f"Ошибка отправки данных: {str(e)}")

    def read_from_process(self):
        """Чтение данных из stdout родительского процесса"""
        while True:
            output = self.process.stdout.readline()  # Чтение данных по строкам
            if output:
                self.output_queue.put(output)  # Кладем данные в очередь
            elif self.process.poll() is not None:
                break

    def read_from_stderr(self):
        """Чтение данных из stderr родительского процесса"""
        while True:
            error_output = self.process.stderr.readline()  # Чтение данных по строкам
            if error_output:
                print(f"Отладка stderr: {error_output}")  # Вывод в консоль для проверки
                self.output_queue.put(error_output)  # Также кладем в очередь для обработки
            elif self.process.poll() is not None:
                break

    def update_output(self):
        """Обновление вывода в интерфейсе"""
        try:
            while not self.output_queue.empty():
                output = self.output_queue.get_nowait()
                self.result_display.insert(tk.END, output)
                self.result_display.see(tk.END)
        except queue.Empty:
            pass
        # Продолжаем обновлять интерфейс каждые 100 мс
        self.root.after(100, self.update_output)


    def exit_program(self):
        """Корректное завершение программы"""
        try:
            # Отправляем команду 'exit' для завершения процесса
            self.process.stdin.write("exit\n")
            self.process.stdin.flush()

            # Дожидаемся завершения всех потоков
            self.stdout_thread.join()
            self.stderr_thread.join()

            self.process.terminate()

            self.parent_status.config(text="Статус: Завершен", fg="red")
            self.child_status.config(text="Статус: Завершен", fg="red")
            self.root.quit()
        except Exception as e:
            messagebox.showerror("Ошибка", f"Не удалось завершить процесс: {str(e)}")

if __name__ == "__main__":
    root = tk.Tk()
    app = PipeGUI(root)
    root.after(100, app.update_output)  # Запуск обновления вывода
    root.mainloop()
