import customtkinter as ctk
from tkinter import messagebox
import subprocess
import threading
import time
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import queue

ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

parent_process = None
output_file = None
output_queue = queue.Queue()

class CalculatorApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("Division Calculator with Enhanced Interface")
        self.geometry("800x500")
        self.resizable(False, False)
        self.create_sidebar()
        self.create_main_content()
        self.create_graph_area()

    def create_sidebar(self):
        sidebar = ctk.CTkFrame(self, width=200, corner_radius=0)
        sidebar.grid(row=0, column=0, rowspan=2, sticky="nsew", padx=(0, 10), pady=10)
        title = ctk.CTkLabel(sidebar, text="Calculator", font=("Arial", 20, "bold"))
        title.pack(pady=10)
        instructions = ctk.CTkLabel(sidebar, text="Instructions:\n\n"
                                                  "1. Enter output file name.\n"
                                                  "2. Press 'Start' to initiate.\n"
                                                  "3. Enter two numbers to divide.\n"
                                                  "4. Press 'Send' to calculate.\n"
                                                  "5. View results and graphs.\n"
                                                  "6. Press 'Exit' to quit.",
                                    font=("Arial", 12), justify="left")
        instructions.pack(pady=10, padx=10)

    def create_main_content(self):
        content_frame = ctk.CTkFrame(self)
        content_frame.grid(row=0, column=1, padx=10, pady=10, sticky="nsew")
        file_label = ctk.CTkLabel(content_frame, text="Output File Name:", font=("Arial", 14))
        file_label.grid(row=0, column=0, sticky="w", pady=5)
        self.entry_file = ctk.CTkEntry(content_frame, placeholder_text="Enter file name, e.g., output.txt", width=200)
        self.entry_file.grid(row=0, column=1, sticky="w", pady=5)
        numbers_label = ctk.CTkLabel(content_frame, text="Enter Numbers (e.g., 10 5):", font=("Arial", 14))
        numbers_label.grid(row=1, column=0, sticky="w", pady=5)
        self.entry_numbers = ctk.CTkEntry(content_frame, placeholder_text="Enter numbers here", width=200)
        self.entry_numbers.grid(row=1, column=1, sticky="w", pady=5)
        self.result_text = ctk.CTkTextbox(content_frame, width=400, height=100)
        self.result_text.grid(row=2, column=0, columnspan=2, pady=10)
        self.progress_bar = ctk.CTkProgressBar(content_frame, width=200)
        self.progress_bar.grid(row=3, column=0, columnspan=2, pady=10)
        self.button_start = ctk.CTkButton(content_frame, text="Start", command=self.start_calculation, width=100)
        self.button_start.grid(row=4, column=0, pady=10, sticky="w")
        self.button_send = ctk.CTkButton(content_frame, text="Send", command=self.send_numbers, state="disabled", width=100)
        self.button_send.grid(row=4, column=1, pady=10, sticky="e")
        self.button_exit = ctk.CTkButton(content_frame, text="Exit", command=self.exit_program, width=100)
        self.button_exit.grid(row=5, column=0, columnspan=2, pady=10)

    def create_graph_area(self):
        graph_frame = ctk.CTkFrame(self)
        graph_frame.grid(row=1, column=1, padx=10, pady=(0, 10), sticky="nsew")
        self.fig = Figure(figsize=(4, 2), dpi=100, facecolor="#1c1c1c")
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#1c1c1c")
        self.ax.tick_params(colors="white")
        self.ax.spines["bottom"].set_color("white")
        self.ax.spines["left"].set_color("white")
        self.ax.set_title("Results Over Time", color="white")
        self.ax.set_xlabel("Operation #", color="white")
        self.ax.set_ylabel("Result", color="white")
        self.ax.grid(color="#555555")
        self.canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)
        self.results = []

    def start_calculation(self):
        global parent_process
        file_name = self.entry_file.get()
        
        if not file_name:
            messagebox.showerror("Error", "Please enter an output file name.")
            return
        
        parent_process = subprocess.Popen(
            ["parent_win.exe"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        
        parent_process.stdin.write(file_name + "\n")
        parent_process.stdin.flush()
        
        threading.Thread(target=self.read_stdout, daemon=True).start()
        
        self.button_send.configure(state="normal")
        self.button_start.configure(state="disabled")
        self.result_text.insert("end", f"Process started. Output file: {file_name}\n")

    def send_numbers(self):
        numbers = self.entry_numbers.get()

        if not numbers:
            messagebox.showerror("Error", "Please enter numbers to divide.")
            return

        parent_process.stdin.write(numbers + "\n")
        parent_process.stdin.flush()

        self.progress_bar.set(0.0)
        for i in range(10):
            self.progress_bar.set(i / 10)
            time.sleep(0.1)

    def read_stdout(self):
        global parent_process
        while True:
            output = parent_process.stdout.readline()
            if output:
                output_queue.put(output.strip())
                self.after(100, self.update_result)

    def update_result(self):
        while not output_queue.empty():
            result = output_queue.get()
            self.result_text.insert("end", f"Result: {result}\n")

            try:
                numeric_result = float(result.split('=')[-1].strip())
                self.results.append(numeric_result)
                self.update_graph()
            except ValueError:
                pass

    def update_graph(self):
        self.ax.cla()
        self.ax.set_facecolor("#1c1c1c")
        self.ax.tick_params(colors="white")
        self.ax.spines["bottom"].set_color("white")
        self.ax.spines["left"].set_color("white")
        self.ax.set_title("Results Over Time", color="white")
        self.ax.set_xlabel("Operation #", color="white")
        self.ax.set_ylabel("Result", color="white")
        self.ax.grid(color="#555555")
        self.ax.plot(self.results, marker="o", linestyle="-", color="cyan")
        self.canvas.draw()

    def exit_program(self):
        global parent_process
        if parent_process:
            parent_process.stdin.write("exit\n")
            parent_process.stdin.flush()
            parent_process.terminate()
            parent_process = None

        self.destroy()

if __name__ == "__main__":
    app = CalculatorApp()
    app.mainloop()
