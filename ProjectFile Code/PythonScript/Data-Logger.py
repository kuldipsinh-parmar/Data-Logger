import tkinter as tk
from tkinter import filedialog
import serial.tools.list_ports
import csv
import time
import threading

class SerialPortReader:
    def __init__(self, port, baudrate, filename):
        self.ser = serial.Serial(port, baudrate)
        self.filename = filename
        self.running = False

    def start(self):
        self.running = True
        threading.Thread(target=self.read_from_port, daemon=True).start()

    def stop(self):
        self.running = False

    def read_from_port(self):
        with open(self.filename, 'w', newline='') as file:
            writer = csv.writer(file)
            while self.running:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode('utf-8').strip()
                    csv_data = line.split(',')
                    writer.writerow(csv_data)
                    time.sleep(1)

class Application(tk.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.master = master
        self.pack()
        self.create_widgets()

    def create_widgets(self):
        self.baudrate_label = tk.Label(self, text="Baudrate:")
        self.baudrate_label.pack(side="top")

        self.baudrate_entry = tk.Entry(self)
        self.baudrate_entry.pack(side="top")

        self.port_label = tk.Label(self, text="COM Port:")
        self.port_label.pack(side="top")

        self.port_entry = tk.Entry(self)
        self.port_entry.pack(side="top")

        self.file_label = tk.Label(self, text="File:")
        self.file_label.pack(side="top")

        self.file_button = tk.Button(self)
        self.file_button["text"] = "Select File"
        self.file_button["command"] = self.select_file
        self.file_button.pack(side="top")

        self.start_button = tk.Button(self)
        self.start_button["text"] = "Start"
        self.start_button["command"] = self.start_reading
        self.start_button.pack(side="top")

        self.stop_button = tk.Button(self)
        self.stop_button["text"] = "Stop"
        self.stop_button["command"] = self.stop_reading
        self.stop_button.pack(side="top")

        self.quit = tk.Button(self, text="QUIT", fg="red",
                              command=self.master.destroy)
        self.quit.pack(side="bottom")

    def select_file(self):
        self.filename = filedialog.askopenfilename()

    def start_reading(self):
        port = self.port_entry.get()
        baudrate = int(self.baudrate_entry.get())
        self.reader = SerialPortReader(port, baudrate, self.filename)
        self.reader.start()

    def stop_reading(self):
        self.reader.stop()

root = tk.Tk()
app = Application(master=root)
app.mainloop()
