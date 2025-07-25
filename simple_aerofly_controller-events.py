# simple_aerofly_controller-events.py - Controlador de Eventos C172 para Aerofly FS4
import tkinter as tk
from tkinter import ttk
import socket
import json

class C172EventController:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Aerofly FS4 - Cessna 172 Event Controller")
        self.root.geometry("1000x800")
        self.root.configure(bg='#1e1e1e')
        
        self.setup_gui()
        
    def setup_gui(self):
        """Setup the GUI with all C172 event controls"""
        
        # Title
        title_label = tk.Label(self.root, text="🛩️ CESSNA 172 EVENT CONTROLLER", 
                              font=('Arial', 18, 'bold'), 
                              fg='#00ff00', bg='#1e1e1e')
        title_label.pack(pady=10)
        
        # Subtitle
        subtitle_label = tk.Label(self.root, text="Testing Real TMD Event Controls", 
                                font=('Arial', 12), 
                                fg='#ffff00', bg='#1e1e1e')
        subtitle_label.pack(pady=5)
        
        # Status
        self.status_label = tk.Label(self.root, text="Ready to send event commands", 
                                   font=('Arial', 11), 
                                   fg='#00ff00', bg='#1e1e1e')
        self.status_label.pack(pady=5)
        
        # Main frame with scrollbar
        canvas = tk.Canvas(self.root, bg='#1e1e1e')
        scrollbar = ttk.Scrollbar(self.root, orient="vertical", command=canvas.yview)
        scrollable_frame = tk.Frame(canvas, bg='#1e1e1e')
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.pack(side="left", fill="both", expand=True, padx=10)
        scrollbar.pack(side="right", fill="y")
        
        # === ENGINE CONTROLS ===
        self.create_section(scrollable_frame, "🔥 ENGINE CONTROLS", 0, [
            ("MAGNETOS Step +", "Controls.Magnetos1", "step", 1.0, '#ff4444'),
            ("MAGNETOS Step -", "Controls.Magnetos1", "step", -1.0, '#ff4444'),
            ("MAGNETOS OFF", "Controls.Magnetos1", "step", 0.0, '#ff6666'),
            ("MAGNETOS ON", "Controls.Magnetos1", "step", 2.0, '#ff6666'),
            ("FUEL SELECTOR Step +", "Controls.FuelSelector", "step", 1.0, '#ff8800'),
            ("FUEL SELECTOR Step -", "Controls.FuelSelector", "step", -1.0, '#ff8800'),
            ("FUEL SHUT OFF Step", "Controls.FuelShutOff", "step", 1.0, '#ffaa00'),
            ("FUEL SHUT OFF Reset", "Controls.FuelShutOff", "step", 0.0, '#ffaa00')
        ])
        
        # === FLIGHT CONTROLS ===
        self.create_section(scrollable_frame, "🕹️ FLIGHT CONTROLS", 1, [
            ("TRIM UP", "Controls.Trim", "step", 1.0, '#00ff44'),
            ("TRIM DOWN", "Controls.Trim", "step", -1.0, '#00ff44'),
            ("TRIM CENTER", "Controls.Trim", "step", 0.0, '#44ff44'),
            ("PITCH INPUT +", "Controls.Pitch.Input", "step", 0.1, '#4444ff'),
            ("PITCH INPUT -", "Controls.Pitch.Input", "step", -0.1, '#4444ff'),
            ("PITCH INPUT Center", "Controls.Pitch.Input", "offset", 0.0, '#6666ff'),
            ("ROLL INPUT +", "Controls.Roll.Input", "step", 0.1, '#44ffff'),
            ("ROLL INPUT -", "Controls.Roll.Input", "step", -0.1, '#44ffff'),
            ("ROLL INPUT Center", "Controls.Roll.Input", "offset", 0.0, '#66ffff')
        ])
        
        # === YOKE CONTROLS ===
        self.create_section(scrollable_frame, "🎮 YOKE CONTROLS", 2, [
            ("YOKE BUTTON Press", "LeftYoke.Button", "event", 1.0, '#ff00ff'),
            ("HIDE LEFT YOKE", "Controls.HideYoke.Left", "toggle", 1.0, '#ff44ff'),
            ("SHOW LEFT YOKE", "Controls.HideYoke.Left", "toggle", 0.0, '#ff44ff'),
            ("HIDE RIGHT YOKE", "Controls.HideYoke.Right", "toggle", 1.0, '#ff88ff'),
            ("SHOW RIGHT YOKE", "Controls.HideYoke.Right", "toggle", 0.0, '#ff88ff')
        ])
        
        # === AIRCRAFT SYSTEMS ===
        self.create_section(scrollable_frame, "⚙️ AIRCRAFT SYSTEMS", 3, [
            ("PARKING BRAKE ON", "Controls.ParkingBrake", "step", 1.0, '#ffff00'),
            ("PARKING BRAKE OFF", "Controls.ParkingBrake", "step", 0.0, '#ffff00'),
            ("PARKING BRAKE Step", "Controls.ParkingBrake", "step", 0.5, '#ffff44')
        ])
        
        # === CABIN LIGHTING ===
        self.create_section(scrollable_frame, "💡 CABIN LIGHTING", 4, [
            ("LEFT CABIN LIGHT ON", "Controls.Lighting.LeftCabinOverheadLight", "toggle", 1.0, '#00ffff'),
            ("LEFT CABIN LIGHT OFF", "Controls.Lighting.LeftCabinOverheadLight", "toggle", 0.0, '#00ffff'),
            ("RIGHT CABIN LIGHT ON", "Controls.Lighting.RightCabinOverheadLight", "toggle", 1.0, '#44ffff'),
            ("RIGHT CABIN LIGHT OFF", "Controls.Lighting.RightCabinOverheadLight", "toggle", 0.0, '#44ffff')
        ])
        
        # === DOORS CONTROLS ===
        self.create_section(scrollable_frame, "🚪 DOORS & HANDLES", 5, [
            ("LEFT DOOR HANDLE", "Doors.LeftHandle", "step", 1.0, '#88ff00'),
            ("LEFT DOOR HANDLE Reset", "Doors.LeftHandle", "step", 0.0, '#88ff00'),
            ("LEFT DOOR OPEN", "Doors.Left", "step", 1.0, '#aaff00'),
            ("LEFT DOOR CLOSE", "Doors.Left", "step", 0.0, '#aaff00'),
            ("RIGHT DOOR HANDLE", "Doors.RightHandle", "step", 1.0, '#88ff44'),
            ("RIGHT DOOR HANDLE Reset", "Doors.RightHandle", "step", 0.0, '#88ff44'),
            ("RIGHT DOOR OPEN", "Doors.Right", "step", 1.0, '#aaff44'),
            ("RIGHT DOOR CLOSE", "Doors.Right", "step", 0.0, '#aaff44')
        ])
        
        # === WINDOWS CONTROLS ===
        self.create_section(scrollable_frame, "🪟 WINDOWS", 6, [
            ("LEFT WINDOW OPEN", "Windows.Left", "step", 1.0, '#ffaa88'),
            ("LEFT WINDOW CLOSE", "Windows.Left", "step", 0.0, '#ffaa88'),
            ("LEFT WINDOW 50%", "Windows.Left", "step", 0.5, '#ffbb88'),
            ("RIGHT WINDOW OPEN", "Windows.Right", "step", 1.0, '#ffcc88'),
            ("RIGHT WINDOW CLOSE", "Windows.Right", "step", 0.0, '#ffcc88'),
            ("RIGHT WINDOW 50%", "Windows.Right", "step", 0.5, '#ffdd88')
        ])
        
        # === NON-EVENT CONTROLS (for comparison) ===
        self.create_section(scrollable_frame, "🔍 NON-EVENT CONTROLS", 7, [
            ("LEFT SUN BLOCKER", "Controls.LeftSunBlocker", "value", 1.0, '#666666'),
            ("LEFT SUN BLOCKER Off", "Controls.LeftSunBlocker", "value", 0.0, '#666666'),
            ("RIGHT SUN BLOCKER", "Controls.RightSunBlocker", "value", 1.0, '#777777'),
            ("RIGHT SUN BLOCKER Off", "Controls.RightSunBlocker", "value", 0.0, '#777777')
        ])
        
    def create_section(self, parent, title, row, buttons):
        """Create a section with title and buttons"""
        # Section frame
        section_frame = tk.LabelFrame(parent, text=title, 
                                    font=('Arial', 14, 'bold'),
                                    fg='#ffff00', bg='#1e1e1e',
                                    relief='ridge', bd=2)
        section_frame.pack(fill='x', padx=10, pady=10)
        
        # Add buttons in a grid
        for i, button_data in enumerate(buttons):
            text, variable, qualifier, value, color = button_data
            
            # Create button with event info
            btn_text = f"{text}\n[{qualifier.upper()}]"
            
            btn = tk.Button(section_frame, text=btn_text,
                          font=('Arial', 10, 'bold'),
                          bg=color, fg='black',
                          relief='raised', bd=3, width=18, height=3,
                          command=lambda v=variable, q=qualifier, val=value: self.send_event_command(v, q, val))
            
            btn.grid(row=i//4, column=i%4, sticky='nsew', padx=3, pady=3)
            
        # Configure button grid
        for i in range(4):
            section_frame.grid_columnconfigure(i, weight=1)
    
    def send_event_command(self, variable, qualifier, value):
        """Send event command with proper JSON format to Aerofly"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2.0)
            sock.connect(('localhost', 12346))
            
            # Create JSON command based on DLL expectations
            if qualifier == "toggle":
                # Toggle commands don't need a specific value
                command = {
                    "variable": variable,
                    "value": 1.0,  # DLL needs a value field
                    "qualifier": "toggle"
                }
            elif qualifier == "event":
                # Event commands are momentary
                command = {
                    "variable": variable,
                    "value": 1.0,
                    "qualifier": "event"
                }
            elif qualifier == "step":
                # Step commands use the provided value
                command = {
                    "variable": variable,
                    "value": value,
                    "qualifier": "step"
                }
            elif qualifier == "offset":
                # Offset commands for continuous controls
                command = {
                    "variable": variable,
                    "value": value,
                    "qualifier": "offset"
                }
            else:  # "value" (non-event controls)
                # Simple value assignment
                command = {
                    "variable": variable,
                    "value": value
                }
            
            # Send as JSON
            cmd_data = json.dumps(command).encode('utf-8')
            sock.send(cmd_data)
            sock.close()
            
            # Status display
            if "qualifier" in command:
                status_text = f"✅ SENT: {variable} [{qualifier.upper()}] = {value}"
            else:
                status_text = f"✅ SENT: {variable} = {value}"
                
            self.status_label.config(text=status_text, fg='#00ff00')
            print(f"Event command sent: {json.dumps(command)}")
            
        except Exception as e:
            self.status_label.config(text=f"❌ ERROR: {e}", fg='#ff0000')
            print(f"Error sending event command: {e}")
    
    def run(self):
        """Start the application"""
        # Bind mouse wheel to canvas
        def _on_mousewheel(event):
            canvas = event.widget
            canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        
        self.root.bind_all("<MouseWheel>", _on_mousewheel)
        self.root.mainloop()

if __name__ == "__main__":
    print("=== CESSNA 172 EVENT CONTROLLER ===")
    print("Testing real TMD event controls detected by DLL")
    print("Make sure Aerofly FS4 is running with C172 loaded")
    print("Commands will be sent to localhost:12346")
    print("=" * 50)
    print("\nDetected C172 Event Controls:")
    print("- Controls.Magnetos1 (OnRotate, step)")
    print("- Controls.Trim (OnStep, step)")
    print("- Controls.Pitch.Input (OnStep, step + offset)")
    print("- Controls.Roll.Input (OnRotate, step + offset)")
    print("- LeftYoke.Button (OnPush, event)")
    print("- Controls.HideYoke.Left/Right (OnPush, toggle)")
    print("- Controls.FuelSelector (OnRotate, step)")
    print("- Controls.FuelShutOff (OnStep, step)")
    print("- Controls.ParkingBrake (OnStep, step)")
    print("- Controls.Lighting cabin lights (OnPush, toggle)")
    print("- Doors.* (OnStep, step)")
    print("- Windows.* (OnStep, step)")
    print("=" * 50)
    
    app = C172EventController()
    app.run()