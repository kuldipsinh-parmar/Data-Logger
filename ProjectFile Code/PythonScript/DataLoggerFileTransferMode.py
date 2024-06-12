import tkinter as tk
from tkinter import ttk
import serial
from serial.tools import list_ports
import json
import time
from tkinter.filedialog import asksaveasfile
import csv

port=str()
baudR=str()
filePath=str()
msg=dict()


# Function to send data to the microcontroller
def send_data():

    global baudR
    global msg
    global port
    global filePath
    
    # Validate input 
    try:
        baudR=int(baudR_combobox.get())
        if(baudR == 0):
            baudRmsg.config(text=ValueError)
            return
    except ValueError:
        baudRmsg.config(text=ValueError)
        return
# {"msg":"File Transfer Mode"}
    # Convert the data to a string format suitable for serial transmission
    msg =json.dumps( {"msg":"File Transfer Mode"})
    print(msg)
    # Set up the serial connection (adjust the port and baudrate as needed)
    ser = serial.Serial(port,baudrate=baudR , timeout=1)
    ser.write(msg.encode())  # Send data as bytes
    time.sleep(1.5)


    msg  = ser.readline()
    print(msg)
    try:
         msg = json.loads(msg)
    except json.JSONDecodeError:
                print("Waiting for ACK")
                return
    except KeyError:
                print("Key 'msg' not found in JSON")
                return

    if msg["msg"] == "ACK":
                # print("ACK condition")
                msg = json.dumps({"msg": "R"})
                ser.write(msg.encode())
    elif msg["msg"] == "NACK":
                alert.config(text="Data error. Send Request Again", foreground='#880808')
                return
    else:
                alert.config(text="Data error. Send Request Again", foreground='#880808')
                return
   
    try:
        file = open(filePath, "w", newline='')  # Open file in write mode
        csvFile = csv.writer(file)
        csvFile.writerow(["Date(DD-MM-YYYY)", "Time(HH:MM:SS)", "Sensor Value"])

        while True:
            time.sleep(0.1)
            msg = ser.readline().strip()  # Read a line and remove leading/trailing whitespace
            try:
                msg = json.loads(msg)
            except json.JSONDecodeError:
                print("JSON decoding error:", msg)
                continue  # Skip to the next iteration of the loop if decoding fails

            print("Received:", msg)

            if msg.get("msg") == "START":
                alert.config(text="Data Transfer started..", foreground='#020812')
                while True:
                    time.sleep(0.02)
                    msg = ser.readline().strip()
                    try:
                        msg = json.loads(msg)
                    except json.JSONDecodeError:
                        print("JSON decoding error:", msg)
                        continue  # Skip to the next iteration of the loop if decoding fails

                    print("Received:", msg)

                    if msg.get("msg") == "END":
                        alert.config(text="Data Received Successfully", foreground='#016113')
                        break  # Exit the inner loop when "END" message is received

                    data = str(msg.get("msg")).split(",")
                    print("Data:", data)
                    csvFile.writerow(data)

                break  # Exit the outer loop when "END" message is received

    except Exception as e:
       print("An error occurred:", e)
    finally:
        file.close()  # Close the file regardless of whether an error occurred

        
       
    
    
# {"sensor":"gps","time":1351824120,"data":[48.756080,2.302038]}
# {"msg":"Temperature Data-Log","sample":1,"duration":120}


# Create the main window
root = tk.Tk()
root.geometry("360x500")
root.configure(bg='aliceblue')
root.resizable(False, False)
root.title("Data Logger File Transfer Mode")

# Create a custom style for the shart button
style = ttk.Style()
style.configure("Custom.TButton", background="#3b445c",font=("Arial Bold",11),foreground="#030303")

# function defination
def select_path(event=None):
    file = asksaveasfile(initialfile = 'Untitled.csv',defaultextension='.csv')
    name=str(file)
    name=name.split("'")
    global filePath
    filePath = name[1]
    if file:
        path_entry.delete(0, tk.END)  # Clear any existing text
        path_entry.insert(0, file)  # Insert the selected file path  

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

    

# File save path 
File_label = ttk.Label(root, text="Select the File Path",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
File_label.grid(row=0, column=0,columnspan=2, padx=5, pady=5)

path_entry = ttk.Entry(root, width=40)
path_entry.grid(row=1, column=0, padx=10, pady=5)

browse_button = ttk.Button(root, text='Browse', command=select_path)
browse_button.grid(row=1, column=1, pady=5,padx=5)

# Dropdown menu for baud rate selection
baudR_var = tk.StringVar()
baudR_var.set("")  # Default value
baudR_options = ["2400", "9600","19200","38400","57600","115200"]  # Customize form options

baudR_label = ttk.Label(root, text="Select Baud Rate",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
baudR_label.grid(row=2, column=0, columnspan=2,padx=10,pady=5)

baudR_combobox = ttk.Combobox(root, textvariable=baudR_var, values=baudR_options,state="readonly",width=30)
baudR_combobox.grid(row=3, column=0,columnspan=2, padx=10,pady=5)
baudR_combobox.bind("<<ComboboxSelected>>", baudR_select)

baudRmsg=ttk.Label(root,background='aliceblue',foreground='#880808')
baudRmsg.grid(row=4, column=0,columnspan=2,padx=10,pady=5)

# Dropdown menu for com ort selection
com_label = ttk.Label(root, text="Select COM Port",font=("Arial Bold",10),foreground="#030303",background="aliceblue")
com_label.grid(row=5, column=0,columnspan=2,padx=10,pady=5)

com_combobox = ttk.Combobox(root, values=serial_ports(),state="readonly",width=30)
com_combobox.grid(row=6, column=0,columnspan=2,padx=10,pady=5)
com_combobox.bind("<<ComboboxSelected>>", com_select)

# Create a send button
send_button = ttk.Button(root, text="Start",style="Custom.TButton", command=send_data)
send_button.grid(row=7, column=0,columnspan=2,padx=10,pady=20,ipadx=5,ipady=5)

alert=ttk.Label(root,text="",font=("Arial Bold",10),background='aliceblue',foreground='#016113')
alert.grid(row=8, column=0,columnspan=2,padx=10,pady=40)

# Run the application
root.mainloop()




