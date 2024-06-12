import tkinter as tk
from tkinter import ttk
import re  # For regular expressions
import serial
from serial.tools import list_ports
import json
import time


port=str()
baudR=str()
sample=str()
duration=str()

# Function to send data to the microcontroller
def send_data():
    # Collect data from the entry widgets
    global sample
    global duration 
    global baudR
    sample = sample_entry.get()
    duration = duration_entry.get()
    mode=mode_combobox.get()
   
    
    # Validate input 
    try:
        sample=int(sample)
        if not (sample>0 and sample<=3600):
            msg0.config(text="Invalid Sample Rate")  
            return  
        else:
            msg0.config(text=" ")
    except ValueError:
        msg0.config(text="Invalid Sample Rate")  
        return  
    
    if not re.match(r"^\d{2}:\d{2}:\d{2}$", duration):
        msg1.config(text="Invalid Duration") 
        return
    
    try:
        baudR=int(baudR_combobox.get())
        if(baudR == 0):
            baudRmsg.config(text=ValueError)
            return
    except ValueError:
        baudRmsg.config(text=ValueError)
        return

    duration=duration.split(":")

    durationInSec=(int(duration[0])*3600) + (int(duration[1])*60) + int(duration[2])


    # Convert the data to a string format suitable for serial transmission
    serial_data =json.dumps( {"mode":mode,"sample":sample,"duration":durationInSec})
    print(serial_data)
    # Set up the serial connection (adjust the port and baudrate as needed)
    ser = serial.Serial(port,baudrate=baudR , timeout=1)
    ser.write(serial_data.encode())  # Send data as bytes
    time.sleep(0.1)
    data = ser.readline()
    print(data)
    

    

# {"mode":"Temperature Data-Log","sample":70,"duration":3600}


# Create the main window
root = tk.Tk()
root.geometry("350x500")
root.configure(bg='aliceblue')
root.resizable(False, False)
root.title("Data Logger Mode transfer")

# Create a custom style for the button
style = ttk.Style()
style.configure("Custom.TButton", background="#3b445c",font=("Arial Bold",10),foreground="#030303")

# function defination

def validate_sample():
    global sample
    sample=sample_entry.get()
    try:
        sample=int(sample)
        if not (sample>0 and sample<=3600):
            msg0.config(text="Invalid Sample Rate")  
            return True  
        else:
            msg0.config(text=" ")
            return True
    except ValueError:
        msg0.config(text="Invalid Sample Rate")  
        return True  
      
def validate_duration():
    P=duration_entry.get()
    if not re.match(r"^\d{2}:\d{2}:\d{2}$", P):
        msg1.config(text="Invalid Duration")  
        return True
    else:
        msg1.config(text=" ")  
        return True
    
def serial_ports():
    return serial.tools.list_ports.comports()

def com_select(event):
    global port
    port =""
    for i in com_combobox.get():
        if i==" ":
            break
        port += i

def baudR_select(event):
    global baudR
    try:
        baudR=int(baudR_combobox.get())
        baudRmsg.config(text=" ")
        
    except ValueError:
        baudRmsg.config(text=ValueError)

    

# Dropdown menu for mode selection
mode_var = tk.StringVar()
mode_var.set("Temperature Data-Log")  # Default value
mode_options = ["Temperature Data-Log", "Analog Input Data-Log"]  # Customize form options

mode_label = ttk.Label(root, text="Select Mode",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
mode_label.pack(padx=10, pady=5)

mode_combobox = ttk.Combobox(root, textvariable=mode_var, values=mode_options,state="readonly",width=30)
mode_combobox.pack(padx=10, pady=5)

# Dropdown menu for baud rate selection
baudR_var = tk.StringVar()
baudR_var.set("")  # Default value
baudR_options = ["2400", "9600","19200","38400","57600","115200"]  # Customize form options

baudR_label = ttk.Label(root, text="Select Baud Rate",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
baudR_label.pack(padx=10, pady=5)

baudR_combobox = ttk.Combobox(root, textvariable=baudR_var, values=baudR_options,state="readonly",width=30)
baudR_combobox.pack(padx=10, pady=5)
baudR_combobox.bind("<<ComboboxSelected>>", baudR_select)

baudRmsg=ttk.Label(root,background='aliceblue',foreground='#880808')
baudRmsg.pack(padx=10,pady=5)

# Dropdown menu for com ort selection
com_label = ttk.Label(root, text="Select COM Port",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
com_label.pack(padx=10, pady=5)

com_combobox = ttk.Combobox(root, values=serial_ports(),state="readonly",width=30)
com_combobox.pack(padx=10, pady=5)
com_combobox.bind("<<ComboboxSelected>>", com_select)

# Create labels and entry widgets for each data field
sample_label = ttk.Label(root, text="Sample per Second(1 to 3600)",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
sample_label.pack(padx=10, pady=5)
sample_entry = ttk.Entry(root,width=30,validate="focusout",validatecommand=validate_sample)
sample_entry.pack(padx=10, pady=5)

msg0=ttk.Label(root,background='aliceblue',foreground='#880808')
msg0.pack(padx=10,pady=5)

duration_label = ttk.Label(root, text="Data Logging Duration (HH:MM:SS)",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
duration_label.pack(padx=10, pady=5)
duration_entry = ttk.Entry(root,validate="focusout",width=30,validatecommand=validate_duration)
duration_entry.pack(padx=10, pady=5)

msg1=ttk.Label(root,background='aliceblue',foreground='#880808')
msg1.pack(padx=10,pady=5)

# Create a send button
send_button = ttk.Button(root, text="Send",style="Custom.TButton", command=send_data)
send_button.pack(padx=10, pady=30)

# Run the application
root.mainloop()




