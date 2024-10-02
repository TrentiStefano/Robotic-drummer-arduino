import customtkinter as ctk
import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk

# Define global variables
root = ctk.CTk()
ui_label = None
speed_slider = None
volume_slider = None
selected_button = None
initial_text = "Initial Text"
initial_text_color = "blue"
message_label = ctk.CTkLabel(root, text=initial_text, text_color=initial_text_color)
state = 0  # Variable to hold the state
arduino = None


def list_serial_ports():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]


def initialize_serial(port):
    try:
        arduino = serial.Serial(port, 9600, timeout=1)
        return arduino
    except serial.SerialException as e:
        print(f"Error opening serial port {port}: {e}")
        return None


def send_data():
    global speed_slider, volume_slider, selected_button, arduino, message_label, state

    if arduino and arduino.is_open:
        speed = speed_slider.get()
        volume = volume_slider.get()
        button_state = selected_button.get()
        data = f"{int(speed)},{int(volume)},{button_state},{state}\n"
        arduino.write(data.encode())
        message = f"Sending data: Speed={int(speed)}, Volume={int(volume)}, Button={button_state}, State={state}"
        message_label.configure(text=message, text_color="green")
        #print(message)
    else:
        message_label.configure(text="Arduino not connected", text_color="red")
        #print("Arduino not connected")


def on_button_press(button_value):
    global selected_button
    selected_button.set(button_value)
    send_data()


def periodic_send():
    send_data()
    root.after(50, periodic_send)  # Schedule the function to be called again after 50ms


def start():
    global state
    state = 1
    send_data()


def stop():
    global state
    state = 0
    send_data()


    
def create_gui():
    global speed_slider, volume_slider, selected_button, message_label, root

    # Create main window
    root.title("RoboDrummer - Control Interface")
    root.geometry("600x400")  # Set window size
    
    ui_label = ctk.CTkLabel(root, text="RoboDrummer - GUI", text_color="blue", font=("Arial", 16))
    ui_label.pack(padx=10, pady=(10, 5))

    # Create and pack speed slider with label
    speed_label = ctk.CTkLabel(root, text="Speed (BPM)", text_color="white")
    speed_label.pack(padx=10, pady=(10, 5))

    speed_frame = ctk.CTkFrame(root)
    speed_frame.pack(padx=10, pady=5)

    speed_min_label = ctk.CTkLabel(speed_frame, text='0', text_color='white')
    speed_min_label.pack(side='left')

    speed_slider = ctk.CTkSlider(speed_frame, from_=0, to=40, orientation='horizontal', command=lambda val: send_data(), width=500)
    speed_slider.set(30)
    speed_slider.pack(side='left')

    speed_max_label = ctk.CTkLabel(speed_frame, text='40', text_color='white')
    speed_max_label.pack(side='left')

    # Create and pack volume slider with label
    volume_label = ctk.CTkLabel(root, text="Volume", text_color="white")
    volume_label.pack(padx=10, pady=5)

    volume_frame = ctk.CTkFrame(root)
    volume_frame.pack(padx=10, pady=5)

    volume_min_label = ctk.CTkLabel(volume_frame, text='0', text_color='white')
    volume_min_label.pack(side='left')

    volume_slider = ctk.CTkSlider(volume_frame, from_=0, to=100, orientation='horizontal', command=lambda val: send_data(), width=500)
    volume_slider.set(50)
    volume_slider.pack(side='left')

    volume_max_label = ctk.CTkLabel(volume_frame, text='100', text_color='white')
    volume_max_label.pack(side='left')

    # Create a variable to store the selected button value
    selected_button = ctk.StringVar(value='m')

    # Create and pack buttons
    buttons_frame = ctk.CTkFrame(root)
    buttons_frame.pack(padx=10, pady=10)

    muffled_button = ctk.CTkRadioButton(buttons_frame, text="Muffled", variable=selected_button, value='m', command=lambda: on_button_press('m'))
    muffled_button.grid(row=0, column=0, padx=5)

    open_button = ctk.CTkRadioButton(buttons_frame, text="Open", variable=selected_button, value='o', command=lambda: on_button_press('o'))
    open_button.grid(row=0, column=1, padx=5)

    rolled_button = ctk.CTkRadioButton(buttons_frame, text="Rolled", variable=selected_button, value='r', command=lambda: on_button_press('r'))
    rolled_button.grid(row=0, column=2, padx=5)

    # Set the default button
    open_button.invoke()

    # Create and pack message label
    message_label = ctk.CTkLabel(root, text="Arduino not connected", text_color="red")
    message_label.pack(pady=(5, 10))

    # Create start and stop buttons
    start_button = ctk.CTkButton(root, text="Start", command=start)
    start_button.pack(pady=(10, 5))
    
    stop_button = ctk.CTkButton(root, text="Stop", command=stop)
    stop_button.pack(pady=(5, 10))

    # Start the periodic sending of data
    periodic_send()

    # Run the main loop
    root.mainloop()


if __name__ == "__main__":
    # Initialize Arduino serial connection
    available_ports = list_serial_ports()
    print(f"Available ports: {available_ports}")
    if 'COM3' in available_ports:
        arduino_port = 'COM3'
    else:
        arduino_port = available_ports[0] if available_ports else None

    if arduino_port:
        arduino = initialize_serial(arduino_port)
    else:
        print("No available COM ports found.")

    # Start GUI
    create_gui()

    # Close the serial connection when the GUI is closed
    if arduino and arduino.is_open:
        arduino.close()
