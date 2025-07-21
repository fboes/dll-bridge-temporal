# simple_aerofly_controller.py - Controlador Simple para Aerofly FS4
import tkinter as tk
from tkinter import ttk
import socket
import json

class SimpleAeroflyController:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Aerofly FS4 - Controlador Simple")
        self.root.geometry("800x900")
        self.root.configure(bg='#2b2b2b')
        
        self.setup_gui()
        
    def setup_gui(self):
        """Setup the GUI with all control buttons"""
        
        # Title
        title_label = tk.Label(self.root, text="🛩️ AEROFLY FS4 CONTROLLER", 
                              font=('Arial', 16, 'bold'), 
                              fg='white', bg='#2b2b2b')
        title_label.pack(pady=10)
        
        # Status
        self.status_label = tk.Label(self.root, text="Ready to send commands", 
                                   font=('Arial', 10), 
                                   fg='green', bg='#2b2b2b')
        self.status_label.pack(pady=5)
        
        # Main frame
        main_frame = tk.Frame(self.root, bg='#2b2b2b')
        main_frame.pack(fill='both', expand=True, padx=20, pady=10)
        
        # === FLIGHT CONTROLS ===
        self.create_section(main_frame, "🕹️ FLIGHT CONTROLS", 0, [
            ("Pitch UP", "Controls.Pitch.Input", -0.5, '#ff4444'),
            ("Pitch DOWN", "Controls.Pitch.Input", 0.5, '#ff4444'),
            ("Roll LEFT", "Controls.Roll.Input", -0.5, '#ff4444'),
            ("Roll RIGHT", "Controls.Roll.Input", 0.5, '#ff4444'),
            ("Yaw LEFT", "Controls.Yaw.Input", -0.5, '#ff4444'),
            ("Yaw RIGHT", "Controls.Yaw.Input", 0.5, '#ff4444'),
            ("CENTER CONTROLS", "MULTI", [
                ("Controls.Pitch.Input", 0.0),
                ("Controls.Roll.Input", 0.0),
                ("Controls.Yaw.Input", 0.0)
            ], '#666666')
        ])
        
        # === THROTTLE CONTROLS ===
        self.create_section(main_frame, "🔥 THROTTLE", 1, [
            ("THROTTLE 0%", "Controls.Throttle", 0.0, '#4444ff'),
            ("THROTTLE 25%", "Controls.Throttle", 0.25, '#4444ff'),
            ("THROTTLE 50%", "Controls.Throttle", 0.5, '#4444ff'),
            ("THROTTLE 75%", "Controls.Throttle", 0.75, '#4444ff'),
            ("THROTTLE 100%", "Controls.Throttle", 1.0, '#4444ff'),
            ("THROTTLE 1 - 50%", "Controls.Throttle1", 0.5, '#6666ff'),
            ("THROTTLE 2 - 50%", "Controls.Throttle2", 0.5, '#6666ff')
        ])
        
        # === AIRCRAFT SYSTEMS ===
        self.create_section(main_frame, "⚙️ AIRCRAFT SYSTEMS", 2, [
            ("GEAR UP", "Controls.Gear", 0.0, '#ff8800'),
            ("GEAR DOWN", "Controls.Gear", 1.0, '#ff8800'),
            ("FLAPS UP", "Controls.Flaps", 0.0, '#00ff88'),
            ("FLAPS 1", "Controls.Flaps", 0.25, '#00ff88'),
            ("FLAPS 2", "Controls.Flaps", 0.5, '#00ff88'),
            ("FLAPS 3", "Controls.Flaps", 0.75, '#00ff88'),
            ("FLAPS FULL", "Controls.Flaps", 1.0, '#00ff88'),
            ("AIR BRAKE ON", "Controls.AirBrake", 1.0, '#ff4488'),
            ("AIR BRAKE OFF", "Controls.AirBrake", 0.0, '#ff4488')
        ])
        
        # === NAVIGATION ===
        self.create_section(main_frame, "🧭 NAVIGATION", 3, [
            ("COM1 - 122.8", "Communication.COM1Frequency", 122800000, '#88ff44'),
            ("COM1 - 121.5", "Communication.COM1Frequency", 121500000, '#88ff44'),
            ("NAV1 - 110.5", "Navigation.NAV1Frequency", 110500000, '#44ff88'),
            ("NAV1 - 111.0", "Navigation.NAV1Frequency", 111000000, '#44ff88'),
            ("NAV1 Course 90°", "Navigation.SelectedCourse1", 1.57, '#44ff88'),
            ("NAV1 Course 180°", "Navigation.SelectedCourse1", 3.14, '#44ff88'),
            ("NAV1 Course 270°", "Navigation.SelectedCourse1", 4.71, '#44ff88')
        ])
        
        # === AUTOPILOT ===
        self.create_section(main_frame, "🤖 AUTOPILOT", 4, [
            ("AP Speed 50 m/s", "Autopilot.SelectedAirspeed", 50, '#ff44ff'),
            ("AP Speed 70 m/s", "Autopilot.SelectedAirspeed", 70, '#ff44ff'),
            ("AP Heading 090°", "Autopilot.SelectedHeading", 1.57, '#ff44ff'),
            ("AP Heading 180°", "Autopilot.SelectedHeading", 3.14, '#ff44ff'),
            ("AP Altitude 1000m", "Autopilot.SelectedAltitude", 1000, '#ff44ff'),
            ("AP Altitude 3000m", "Autopilot.SelectedAltitude", 3000, '#ff44ff'),
            ("AP VS +5 m/s", "Autopilot.SelectedVerticalSpeed", 5, '#ff44ff'),
            ("AP VS -5 m/s", "Autopilot.SelectedVerticalSpeed", -5, '#ff44ff')
        ])
        
        # === ENGINE CONTROLS ===
        self.create_section(main_frame, "🔧 ENGINES", 5, [
            ("ENGINE 1 MASTER ON", "Aircraft.EngineMaster1", 1.0, '#ffaa00'),
            ("ENGINE 1 MASTER OFF", "Aircraft.EngineMaster1", 0.0, '#ffaa00'),
            ("ENGINE 1 STARTER", "Aircraft.Starter1", 1.0, '#ffaa00'),
            ("ENGINE 1 IGNITION ON", "Aircraft.Ignition1", 1.0, '#ffaa00'),
            ("MIXTURE 1 RICH", "Controls.Mixture1", 1.0, '#aa00ff'),
            ("MIXTURE 1 LEAN", "Controls.Mixture1", 0.5, '#aa00ff'),
            ("PARKING BRAKE ON", "Aircraft.ParkingBrake", 1.0, '#ff0000'),
            ("PARKING BRAKE OFF", "Aircraft.ParkingBrake", 0.0, '#ff0000')
        ])
        
    def create_section(self, parent, title, row, buttons):
        """Create a section with title and buttons"""
        # Section frame
        section_frame = tk.LabelFrame(parent, text=title, 
                                    font=('Arial', 12, 'bold'),
                                    fg='white', bg='#2b2b2b',
                                    relief='ridge', bd=2)
        section_frame.grid(row=row//2, column=row%2, sticky='nsew', padx=10, pady=5)
        
        # Configure grid
        parent.grid_rowconfigure(row//2, weight=1)
        parent.grid_columnconfigure(0, weight=1)
        parent.grid_columnconfigure(1, weight=1)
        
        # Add buttons
        for i, button_data in enumerate(buttons):
            if len(button_data) == 4:
                text, variable, value, color = button_data
                btn = tk.Button(section_frame, text=text,
                              font=('Arial', 10, 'bold'),
                              bg=color, fg='white',
                              relief='raised', bd=3,
                              command=lambda v=variable, val=value: self.send_command(v, val))
            else:
                # Multi-command button
                text, cmd_type, commands, color = button_data
                btn = tk.Button(section_frame, text=text,
                              font=('Arial', 10, 'bold'),
                              bg=color, fg='white',
                              relief='raised', bd=3,
                              command=lambda cmds=commands: self.send_multi_commands(cmds))
            
            btn.grid(row=i//2, column=i%2, sticky='nsew', padx=3, pady=3)
            
        # Configure button grid
        for i in range((len(buttons) + 1) // 2):
            section_frame.grid_rowconfigure(i, weight=1)
        section_frame.grid_columnconfigure(0, weight=1)
        section_frame.grid_columnconfigure(1, weight=1)
    
    def send_command(self, variable, value):
        """Send single command to Aerofly"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2.0)
            sock.connect(('localhost', 12346))
            
            command = {
                "variable": variable,
                "value": value
            }
            
            cmd_data = json.dumps(command).encode('utf-8')
            sock.send(cmd_data)
            sock.close()
            
            self.status_label.config(text=f"✅ SENT: {variable} = {value}", fg='green')
            print(f"Command sent: {variable} = {value}")
            
        except Exception as e:
            self.status_label.config(text=f"❌ ERROR: {e}", fg='red')
            print(f"Error sending command: {e}")
    
    def send_multi_commands(self, commands):
        """Send multiple commands"""
        success_count = 0
        for variable, value in commands:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2.0)
                sock.connect(('localhost', 12346))
                
                command = {
                    "variable": variable,
                    "value": value
                }
                
                cmd_data = json.dumps(command).encode('utf-8')
                sock.send(cmd_data)
                sock.close()
                success_count += 1
                
            except Exception as e:
                print(f"Error in multi-command {variable}: {e}")
        
        self.status_label.config(text=f"✅ SENT {success_count} commands", fg='green')
    
    def run(self):
        """Start the application"""
        self.root.mainloop()

if __name__ == "__main__":
    print("=== AEROFLY FS4 SIMPLE CONTROLLER ===")
    print("Make sure Aerofly FS4 is running with the Bridge DLL loaded")
    print("Commands will be sent to localhost:12346")
    print("=" * 50)
    
    app = SimpleAeroflyController()
    app.run()