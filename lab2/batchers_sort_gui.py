import tkinter as tk
from tkinter import messagebox, ttk
import subprocess
import threading
import re
import matplotlib.pyplot as plt

class BitonicSortGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Bitonic Sort GUI")
        self.times = {}
        self.create_widgets()

    def create_widgets(self):
        frame = tk.Frame(self)
        frame.pack(pady=10)

        tk.Label(frame, text="Количество потоков:").grid(row=0, column=0, padx=5, pady=5)
        self.threads_entry = tk.Entry(frame)
        self.threads_entry.grid(row=0, column=1, padx=5, pady=5)
        self.threads_entry.insert(0, "1")

        run_button = tk.Button(frame, text="Запустить сортировку", command=self.run_sort)
        run_button.grid(row=0, column=2, padx=5, pady=5)

        autotest_button = tk.Button(frame, text="Автотестирование", command=self.run_autotest)
        autotest_button.grid(row=0, column=3, padx=5, pady=5)

        self.output_text = tk.Text(self, width=80, height=20)
        self.output_text.pack(padx=5, pady=5)

        self.table = ttk.Treeview(self, columns=("Threads", "Time"), show="headings")
        self.table.heading("Threads", text="Потоки")
        self.table.heading("Time", text="Время (сек)")
        self.table.pack(padx=5, pady=5)

    def run_sort(self):
        try:
            num_threads = int(self.threads_entry.get())
            if num_threads <= 0:
                raise ValueError
        except ValueError:
            messagebox.showerror("Ошибка", "Введите корректное количество потоков")
            return

        self.output_text.delete('1.0', tk.END)
        self.output_text.insert(tk.END, f"Запуск сортировки с {num_threads} поток(ами)...\n")

        threading.Thread(target=self.execute_sort, args=(num_threads,)).start()

    def execute_sort(self, num_threads):
        command = ["bitonic_sort.exe", str(num_threads)]
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        stdout, stderr = process.communicate()
        self.after(0, self.update_output, stdout, num_threads)

    def update_output(self, output, num_threads):
        self.output_text.insert(tk.END, output)
        match = re.search(r"Sorting time: ([\d\.]+) seconds", output)
        if match:
            time = float(match.group(1))
            self.times[num_threads] = time
            self.update_table(num_threads, time)
            self.plot_results()
        else:
            self.output_text.insert(tk.END, "Не удалось получить время сортировки\n")

    def update_table(self, num_threads, time):
        for row in self.table.get_children():
            if self.table.item(row)["values"][0] == num_threads:
                self.table.item(row, values=(num_threads, time))
                return
        self.table.insert("", tk.END, values=(num_threads, time))

    def run_autotest(self):
        self.output_text.delete('1.0', tk.END)
        self.output_text.insert(tk.END, "Запуск автотестирования...\n")
        threading.Thread(target=self.execute_autotest).start()

    def execute_autotest(self):
        thread_counts = [1, 2, 4, 8, 16, 32]
        for num_threads in thread_counts:
            self.output_text.insert(tk.END, f"Запуск сортировки с {num_threads} поток(ами)...\n")
            command = ["batchers_sort.exe", str(num_threads)]
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
            stdout, stderr = process.communicate()
            self.after(0, self.update_output_autotest, stdout, num_threads)
        self.after(0, self.plot_results)

    def update_output_autotest(self, output, num_threads):
        self.output_text.insert(tk.END, output)
        match = re.search(r"Sorting time: ([\d\.]+) seconds", output)
        if match:
            time = float(match.group(1))
            self.times[num_threads] = time
            self.update_table(num_threads, time)
        else:
            self.output_text.insert(tk.END, f"Не удалось получить время сортировки для {num_threads} поток(ов)\n")

    def plot_results(self):
        if self.times:
            plt.figure(figsize=(8, 6))
            threads = sorted(self.times.keys())
            times = [self.times[t] for t in threads]
            plt.plot(threads, times, marker='o')
            plt.title("Зависимость времени сортировки от количества потоков")
            plt.xlabel("Количество потоков")
            plt.ylabel("Время сортировки (секунды)")
            plt.grid(True)
            plt.show()

if __name__ == "__main__":
    app = BitonicSortGUI()
    app.mainloop()
