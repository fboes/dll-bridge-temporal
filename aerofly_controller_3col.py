# simple_aerofly_controller_v3.py - Controlador Simple para Aerofly FS4 (3 Columnas)
import tkinter as tk
from tkinter import ttk
import socket
import json

class SimpleAeroflyController:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Aerofly FS4 - Controlador Simple")
        self.root.geometry("1800x1000")  # Aumentamos el ancho para 3 columnas
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
        
        # Configure 3 columns
        main_frame.grid_columnconfigure(0, weight=1)
        main_frame.grid_columnconfigure(1, weight=1)
        main_frame.grid_columnconfigure(2, weight=1)
        
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
            ], '#666666'),
            ("FULL ELEVATOR UP", "Controls.Pitch.Input", -1.0, '#ff2222')
        ])
        
        # === THROTTLE CONTROLS ===
        self.create_section(main_frame, "🔥 THROTTLE & POWER", 1, [
            ("THROTTLE 0%", "Controls.Throttle", 0.0, '#4444ff'),
            ("THROTTLE 25%", "Controls.Throttle", 0.25, '#4444ff'),
            ("THROTTLE 50%", "Controls.Throttle", 0.5, '#4444ff'),
            ("THROTTLE 75%", "Controls.Throttle", 0.75, '#4444ff'),
            ("THROTTLE 100%", "Controls.Throttle", 1.0, '#4444ff'),
            ("THR1 25%", "Controls.Throttle1", 0.25, '#6666ff'),
            ("THR2 25%", "Controls.Throttle2", 0.25, '#6666ff'),
            ("THR1 75%", "Controls.Throttle1", 0.75, '#6666ff'),
            ("THR2 75%", "Controls.Throttle2", 0.75, '#6666ff'),
            ("THR3 50%", "Controls.Throttle3", 0.5, '#6666ff'),
            ("THR4 50%", "Controls.Throttle4", 0.5, '#6666ff'),
            ("ALL THROTTLES 0%", "MULTI", [
                ("Controls.Throttle1", 0.0),
                ("Controls.Throttle2", 0.0),
                ("Controls.Throttle3", 0.0),
                ("Controls.Throttle4", 0.0)
            ], '#666666')
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
            ("AIR BRAKE 50%", "Controls.AirBrake", 0.5, '#ff4488'),
            ("AIR BRAKE FULL", "Controls.AirBrake", 1.0, '#ff4488'),
            ("AIR BRAKE OFF", "Controls.AirBrake", 0.0, '#ff4488'),
            ("SPEED BRAKE ARM", "Controls.AirBrake.Arm", 1.0, '#ff6666'),
            ("GLIDER BRAKE", "Controls.GliderAirBrake", 1.0, '#ff8888')
        ])
        
        # === BRAKES & STEERING ===
        self.create_section(main_frame, "🛑 BRAKES & STEERING", 3, [
            ("LEFT BRAKE 50%", "Controls.WheelBrake.Left", 0.5, '#ff0000'),
            ("LEFT BRAKE 100%", "Controls.WheelBrake.Left", 1.0, '#ff0000'),
            ("RIGHT BRAKE 50%", "Controls.WheelBrake.Right", 0.5, '#ff0000'),
            ("RIGHT BRAKE 100%", "Controls.WheelBrake.Right", 1.0, '#ff0000'),
            ("BOTH BRAKES 50%", "MULTI", [
                ("Controls.WheelBrake.Left", 0.5),
                ("Controls.WheelBrake.Right", 0.5)
            ], '#ff2222'),
            ("BOTH BRAKES FULL", "MULTI", [
                ("Controls.WheelBrake.Left", 1.0),
                ("Controls.WheelBrake.Right", 1.0)
            ], '#ff2222'),
            ("RELEASE BRAKES", "MULTI", [
                ("Controls.WheelBrake.Left", 0.0),
                ("Controls.WheelBrake.Right", 0.0)
            ], '#666666'),
            ("PARKING BRAKE ON", "Aircraft.ParkingBrake", 1.0, '#aa0000'),
            ("PARKING BRAKE OFF", "Aircraft.ParkingBrake", 0.0, '#aa0000'),
            ("TILLER LEFT", "Controls.Tiller", -0.5, '#ff6600'),
            ("TILLER RIGHT", "Controls.Tiller", 0.5, '#ff6600'),
            ("NOSE WHEEL STEER", "Controls.NoseWheelSteering", 1.0, '#ff6600')
        ])
        
        # === NAVIGATION & COMM ===
        self.create_section(main_frame, "🧭 NAVIGATION & COMM", 4, [
            ("COM1 - 121.5", "Communication.COM1Frequency", 121500000, '#88ff44'),
            ("COM1 - 122.8", "Communication.COM1Frequency", 122800000, '#88ff44'),
            ("COM1 - 124.0", "Communication.COM1Frequency", 124000000, '#88ff44'),
            ("COM2 - 119.1", "Communication.COM2Frequency", 119100000, '#88ff44'),
            ("COM2 - 120.5", "Communication.COM2Frequency", 120500000, '#88ff44'),
            ("NAV1 - 108.5", "Navigation.NAV1Frequency", 108500000, '#44ff88'),
            ("NAV1 - 110.5", "Navigation.NAV1Frequency", 110500000, '#44ff88'),
            ("NAV1 - 112.3", "Navigation.NAV1Frequency", 112300000, '#44ff88'),
            ("NAV2 - 109.7", "Navigation.NAV2Frequency", 109700000, '#44ff88'),
            ("NAV1 Course 000°", "Navigation.SelectedCourse1", 0.0, '#44ffaa'),
            ("NAV1 Course 090°", "Navigation.SelectedCourse1", 1.57, '#44ffaa'),
            ("NAV1 Course 180°", "Navigation.SelectedCourse1", 3.14, '#44ffaa'),
            ("NAV1 Course 270°", "Navigation.SelectedCourse1", 4.71, '#44ffaa'),
            ("NAV2 Course 090°", "Navigation.SelectedCourse2", 1.57, '#44ffaa'),
            ("XPDR 1200", "Communication.TransponderCode", 1200, '#ffaa44'),
            ("XPDR 7000", "Communication.TransponderCode", 7000, '#ffaa44')
        ])
        
        # === AUTOPILOT ===
        self.create_section(main_frame, "🤖 AUTOPILOT", 5, [
            ("AP MASTER ON", "Autopilot.Master", 1.0, '#ff44ff'),
            ("AP MASTER OFF", "Autopilot.Master", 0.0, '#ff44ff'),
            ("AP HEADING MODE", "Autopilot.Heading", 1.0, '#ff44ff'),
            ("AP VS MODE", "Autopilot.VerticalSpeed", 1.0, '#ff44ff'),
            ("AP Speed 40 m/s", "Autopilot.SelectedAirspeed", 40, '#ff66ff'),
            ("AP Speed 60 m/s", "Autopilot.SelectedAirspeed", 60, '#ff66ff'),
            ("AP Speed 80 m/s", "Autopilot.SelectedAirspeed", 80, '#ff66ff'),
            ("AP Heading 000°", "Autopilot.SelectedHeading", 0.0, '#ff66ff'),
            ("AP Heading 090°", "Autopilot.SelectedHeading", 1.57, '#ff66ff'),
            ("AP Heading 180°", "Autopilot.SelectedHeading", 3.14, '#ff66ff'),
            ("AP Heading 270°", "Autopilot.SelectedHeading", 4.71, '#ff66ff'),
            ("AP Alt 500m", "Autopilot.SelectedAltitude", 500, '#ff88ff'),
            ("AP Alt 1500m", "Autopilot.SelectedAltitude", 1500, '#ff88ff'),
            ("AP Alt 3000m", "Autopilot.SelectedAltitude", 3000, '#ff88ff'),
            ("AP VS +10 m/s", "Autopilot.SelectedVerticalSpeed", 10, '#ffaaff'),
            ("AP VS +2 m/s", "Autopilot.SelectedVerticalSpeed", 2, '#ffaaff'),
            ("AP VS 0 m/s", "Autopilot.SelectedVerticalSpeed", 0, '#ffaaff'),
            ("AP VS -2 m/s", "Autopilot.SelectedVerticalSpeed", -2, '#ffaaff'),
            ("AP VS -10 m/s", "Autopilot.SelectedVerticalSpeed", -10, '#ffaaff'),
            ("AUTOTHROTTLE ON", "Autopilot.ThrottleEngaged", 1.0, '#dd44ff')
        ])
        
        # === ENGINES & FUEL ===
        self.create_section(main_frame, "🔧 ENGINES & FUEL", 6, [
            ("ENG1 MASTER ON", "Aircraft.EngineMaster1", 1.0, '#ffaa00'),
            ("ENG1 MASTER OFF", "Aircraft.EngineMaster1", 0.0, '#ffaa00'),
            ("ENG2 MASTER ON", "Aircraft.EngineMaster2", 1.0, '#ffaa00'),
            ("ENG2 MASTER OFF", "Aircraft.EngineMaster2", 0.0, '#ffaa00'),
            ("ENG1 STARTER", "Aircraft.Starter1", 1.0, '#ffcc00'),
            ("ENG2 STARTER", "Aircraft.Starter2", 1.0, '#ffcc00'),
            ("ENG1 IGNITION ON", "Aircraft.Ignition1", 1.0, '#ffdd00'),
            ("ENG2 IGNITION ON", "Aircraft.Ignition2", 1.0, '#ffdd00'),
            ("MIXTURE1 RICH", "Controls.Mixture1", 1.0, '#aa00ff'),
            ("MIXTURE1 LEAN", "Controls.Mixture1", 0.5, '#aa00ff'),
            ("MIXTURE2 RICH", "Controls.Mixture2", 1.0, '#aa00ff'),
            ("MIXTURE2 LEAN", "Controls.Mixture2", 0.5, '#aa00ff'),
            ("PROP1 HIGH", "Controls.PropellerSpeed1", 1.0, '#0088ff'),
            ("PROP2 HIGH", "Controls.PropellerSpeed2", 1.0, '#0088ff'),
            ("THRUST REV1", "Controls.ThrustReverse1", 1.0, '#ff8800'),
            ("THRUST REV2", "Controls.ThrustReverse2", 1.0, '#ff8800'),
            ("ENGINE STARTUP", "MULTI", [
                ("Aircraft.EngineMaster1", 1.0),
                ("Aircraft.Ignition1", 1.0),
                ("Controls.Mixture1", 1.0)
            ], '#44ff44'),
            ("ALL ENGINES ON", "MULTI", [
                ("Aircraft.EngineMaster1", 1.0),
                ("Aircraft.EngineMaster2", 1.0),
                ("Aircraft.Ignition1", 1.0),
                ("Aircraft.Ignition2", 1.0)
            ], '#44ff44')
        ])
        
        # === HELICOPTER CONTROLS ===
        self.create_section(main_frame, "🚁 HELICOPTER", 7, [
            ("COLLECTIVE UP", "Controls.Collective", 0.7, '#ff0088'),
            ("COLLECTIVE MID", "Controls.Collective", 0.5, '#ff0088'),
            ("COLLECTIVE DOWN", "Controls.Collective", 0.3, '#ff0088'),
            ("CYCLIC FWD", "Controls.CyclicPitch", 0.3, '#ff0088'),
            ("CYCLIC AFT", "Controls.CyclicPitch", -0.3, '#ff0088'),
            ("CYCLIC LEFT", "Controls.CyclicRoll", -0.3, '#ff0088'),
            ("CYCLIC RIGHT", "Controls.CyclicRoll", 0.3, '#ff0088'),
            ("TAIL ROTOR LEFT", "Controls.TailRotor", -0.5, '#ff0088'),
            ("TAIL ROTOR RIGHT", "Controls.TailRotor", 0.5, '#ff0088'),
            ("ROTOR BRAKE", "Controls.RotorBrake", 1.0, '#ff0088'),
            ("HELI THR1 75%", "Controls.HelicopterThrottle1", 0.75, '#ff4488'),
            ("HELI THR2 75%", "Controls.HelicopterThrottle2", 0.75, '#ff4488')
        ])
        
        # === TRIM CONTROLS ===
        self.create_section(main_frame, "✂️ TRIM & FLIGHT AIDS", 8, [
            ("PITCH TRIM UP", "Controls.AileronTrim", 0.1, '#00ddff'),
            ("PITCH TRIM DOWN", "Controls.AileronTrim", -0.1, '#00ddff'),
            ("RUDDER TRIM LEFT", "Controls.RudderTrim", -0.1, '#00ddff'),
            ("RUDDER TRIM RIGHT", "Controls.RudderTrim", 0.1, '#00ddff'),
            ("TRIM NEUTRAL", "MULTI", [
                ("Controls.AileronTrim", 0.0),
                ("Controls.RudderTrim", 0.0)
            ], '#666666'),
            ("YAW DAMPER ON", "Aircraft.YawDamperEnabled", 1.0, '#00ffdd'),
            ("AUTO PITCH TRIM", "Aircraft.AutoPitchTrim", 1.0, '#00ffdd'),
            ("DISCONNECT PEDALS", "Controls.PedalsDisconnect", 1.0, '#ffdd00')
        ])
        
        # === SIMULATION CONTROLS ===
        self.create_section(main_frame, "🎮 SIMULATION", 9, [
            ("PAUSE ON", "Simulation.Pause", 1.0, '#888888'),
            ("PAUSE OFF", "Simulation.Pause", 0.0, '#888888'),
            ("LIFT AIRCRAFT", "Simulation.LiftUp", 1.0, '#888888'),
            ("SOUND TOGGLE", "Simulation.Sound", 1.0, '#888888'),
            ("FLIGHT INFO", "Simulation.FlightInformation", 1.0, '#888888'),
            ("MOVING MAP", "Simulation.MovingMap", 1.0, '#888888'),
            ("MOUSE CONTROL", "Simulation.UseMouseControl", 1.0, '#888888'),
            ("TIME CHANGE", "Simulation.TimeChange", 1.0, '#888888')
        ])
        
    def create_section(self, parent, title, index, buttons):
        """Create a section with title and buttons"""
        # Calculate position in 3-column grid
        row = index // 3
        col = index % 3
        
        # Section frame
        section_frame = tk.LabelFrame(parent, text=title, 
                                    font=('Arial', 11, 'bold'),
                                    fg='white', bg='#2b2b2b',
                                    relief='ridge', bd=2)
        section_frame.grid(row=row, column=col, sticky='nsew', padx=5, pady=5)
        
        # Configure grid weights for proper resizing
        parent.grid_rowconfigure(row, weight=1)
        
        # Add buttons
        for i, button_data in enumerate(buttons):
            if len(button_data) == 4:
                text, variable, value, color = button_data
                btn = tk.Button(section_frame, text=text,
                              font=('Arial', 9, 'bold'),
                              bg=color, fg='white',
                              relief='raised', bd=2,
                              command=lambda v=variable, val=value: self.send_command(v, val))
            else:
                # Multi-command button
                text, cmd_type, commands, color = button_data
                btn = tk.Button(section_frame, text=text,
                              font=('Arial', 9, 'bold'),
                              bg=color, fg='white',
                              relief='raised', bd=2,
                              command=lambda cmds=commands: self.send_multi_commands(cmds))
            
            btn.grid(row=i//2, column=i%2, sticky='nsew', padx=2, pady=2)
            
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
    print("=== AEROFLY FS4 SIMPLE CONTROLLER V3 ===")
    print("Make sure Aerofly FS4 is running with the Bridge DLL loaded")
    print("Commands will be sent to localhost:12346")
    print("3-Column Layout for Better Visibility")
    print("=" * 50)
    
    app = SimpleAeroflyController()
    app.run()