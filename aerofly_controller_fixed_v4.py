# aerofly_controller_fixed_v4.py
import tkinter as tk
from tkinter import ttk
import socket
import json
import time

class SimpleAeroflyController:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Aerofly FS4 - Controlador Simple (Compatible DLL)")
        self.root.geometry("1900x1000")  # Aumentamos ancho para 5 columnas
        self.root.configure(bg='#2b2b2b')
        
        self.setup_gui()
        
    def setup_gui(self):
        """Setup the GUI with all control buttons (NO MULTI commands) - 5 Column Layout"""
        
        # Title
        # title_label = tk.Label(self.root, text="🛩️ AEROFLY FS4 CONTROLLER - DLL COMPATIBLE", 
        #                      font=('Arial', 12, 'bold'), 
        #                      fg='white', bg='#2b2b2b')
        # title_label.pack(pady=10)
        
        # Status
        self.status_label = tk.Label(self.root, text="Ready to send commands (Single commands only)", 
                                   font=('Arial', 10), 
                                   fg='green', bg='#2b2b2b')
        self.status_label.pack(pady=5)
        
        # Main frame
        main_frame = tk.Frame(self.root, bg='#2b2b2b')
        main_frame.pack(fill='both', expand=True, padx=10, pady=10)
        
        # Configure 5 columns
        main_frame.grid_columnconfigure(0, weight=1)
        main_frame.grid_columnconfigure(1, weight=1)
        main_frame.grid_columnconfigure(2, weight=1)
        main_frame.grid_columnconfigure(3, weight=1)
        main_frame.grid_columnconfigure(4, weight=1)
        
        # === FLIGHT CONTROLS ===
        self.create_section(main_frame, "🕹️ FLIGHT CONTROLS", 0, [
            ("Pitch UP", "Controls.Pitch.Input", -0.5, '#ff4444'),
            ("Pitch DOWN", "Controls.Pitch.Input", 0.5, '#ff4444'),
            ("Pitch NEUTRAL", "Controls.Pitch.Input", 0.0, '#666666'),
            ("Roll LEFT", "Controls.Roll.Input", -0.5, '#ff4444'),
            ("Roll RIGHT", "Controls.Roll.Input", 0.5, '#ff4444'),
            ("Roll NEUTRAL", "Controls.Roll.Input", 0.0, '#666666'),
            ("Yaw LEFT", "Controls.Yaw.Input", -0.5, '#ff4444'),
            ("Yaw RIGHT", "Controls.Yaw.Input", 0.5, '#ff4444'),
            ("Yaw NEUTRAL", "Controls.Yaw.Input", 0.0, '#666666'),
            ("FULL ELEVATOR UP", "Controls.Pitch.Input", -1.0, '#ff2222'),
            ("FULL ELEVATOR DOWN", "Controls.Pitch.Input", 1.0, '#ff2222')
        ])
        
        # === THROTTLE CONTROLS ===
        self.create_section(main_frame, "🔥 THROTTLE & POWER", 1, [
            ("THROTTLE 0%", "Controls.Throttle", 0.0, '#4444ff'),
            ("THROTTLE 25%", "Controls.Throttle", 0.25, '#4444ff'),
            ("THROTTLE 50%", "Controls.Throttle", 0.5, '#4444ff'),
            ("THROTTLE 75%", "Controls.Throttle", 0.75, '#4444ff'),
            ("THROTTLE 100%", "Controls.Throttle", 1.0, '#4444ff'),
            ("THR1 0%", "Controls.Throttle1", 0.0, '#6666ff'),
            ("THR1 25%", "Controls.Throttle1", 0.25, '#6666ff'),
            ("THR1 50%", "Controls.Throttle1", 0.5, '#6666ff'),
            ("THR1 75%", "Controls.Throttle1", 0.75, '#6666ff'),
            ("THR1 100%", "Controls.Throttle1", 1.0, '#6666ff'),
            ("THR2 0%", "Controls.Throttle2", 0.0, '#6666ff'),
            ("THR2 25%", "Controls.Throttle2", 0.25, '#6666ff'),
            ("THR2 50%", "Controls.Throttle2", 0.5, '#6666ff'),
            ("THR2 75%", "Controls.Throttle2", 0.75, '#6666ff'),
            ("THR2 100%", "Controls.Throttle2", 1.0, '#6666ff'),
            ("THR3 50%", "Controls.Throttle3", 0.5, '#8888ff'),
            ("THR4 50%", "Controls.Throttle4", 0.5, '#8888ff')
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
            ("AIR BRAKE OFF", "Controls.AirBrake", 0.0, '#ff4488'),
            ("AIR BRAKE 25%", "Controls.AirBrake", 0.25, '#ff4488'),
            ("AIR BRAKE 50%", "Controls.AirBrake", 0.5, '#ff4488'),
            ("AIR BRAKE 75%", "Controls.AirBrake", 0.75, '#ff4488'),
            ("AIR BRAKE FULL", "Controls.AirBrake", 1.0, '#ff4488'),
            ("SPEED BRAKE ARM", "Controls.AirBrake.Arm", 1.0, '#ff6666'),
            ("GLIDER BRAKE", "Controls.GliderAirBrake", 1.0, '#ff8888')
        ])
        
        # === BRAKES & STEERING ===
        self.create_section(main_frame, "🛑 BRAKES & STEERING", 8, [
            ("LEFT BRAKE OFF", "Controls.WheelBrake.Left", 0.0, '#ff0000'),
            ("LEFT BRAKE 25%", "Controls.WheelBrake.Left", 0.25, '#ff0000'),
            ("LEFT BRAKE 50%", "Controls.WheelBrake.Left", 0.5, '#ff0000'),
            ("LEFT BRAKE 100%", "Controls.WheelBrake.Left", 1.0, '#ff0000'),
            ("RIGHT BRAKE OFF", "Controls.WheelBrake.Right", 0.0, '#ff0000'),
            ("RIGHT BRAKE 25%", "Controls.WheelBrake.Right", 0.25, '#ff0000'),
            ("RIGHT BRAKE 50%", "Controls.WheelBrake.Right", 0.5, '#ff0000'),
            ("RIGHT BRAKE 100%", "Controls.WheelBrake.Right", 1.0, '#ff0000'),
            ("PARKING BRAKE ON", "Aircraft.ParkingBrake", 1.0, '#aa0000'),
            ("PARKING BRAKE OFF", "Aircraft.ParkingBrake", 0.0, '#aa0000'),
            ("TILLER CENTER", "Controls.Tiller", 0.0, '#ff6600'),
            ("TILLER LEFT", "Controls.Tiller", -0.5, '#ff6600'),
            ("TILLER RIGHT", "Controls.Tiller", 0.5, '#ff6600'),
            ("NOSE WHEEL STEER", "Controls.NoseWheelSteering", 1.0, '#ff6600')
        ])
        
        # === NAVIGATION & COMM ===
        self.create_section(main_frame, "🧭 NAVIGATION & COMM", 9, [
            ("COM1 - 121.5", "Communication.COM1Frequency", 121500000, '#88ff44'),
            ("COM1 - 122.8", "Communication.COM1Frequency", 122800000, '#88ff44'),
            ("COM1 - 124.0", "Communication.COM1Frequency", 124000000, '#88ff44'),
            ("COM1 - 125.5", "Communication.COM1Frequency", 125500000, '#88ff44'),
            ("COM2 - 119.1", "Communication.COM2Frequency", 119100000, '#88ff44'),
            ("COM2 - 120.5", "Communication.COM2Frequency", 120500000, '#88ff44'),
            ("COM2 - 121.9", "Communication.COM2Frequency", 121900000, '#88ff44'),
            ("NAV1 - 108.5", "Navigation.NAV1Frequency", 108500000, '#44ff88'),
            ("NAV1 - 110.5", "Navigation.NAV1Frequency", 110500000, '#44ff88'),
            ("NAV1 - 112.3", "Navigation.NAV1Frequency", 112300000, '#44ff88'),
            ("NAV2 - 109.7", "Navigation.NAV2Frequency", 109700000, '#44ff88'),
            ("NAV2 - 111.2", "Navigation.NAV2Frequency", 111200000, '#44ff88'),
            ("NAV1 Course 000°", "Navigation.SelectedCourse1", 0.0, '#44ffaa'),
            ("NAV1 Course 090°", "Navigation.SelectedCourse1", 1.57, '#44ffaa'),
            ("NAV1 Course 180°", "Navigation.SelectedCourse1", 3.14, '#44ffaa'),
            ("NAV1 Course 270°", "Navigation.SelectedCourse1", 4.71, '#44ffaa'),
            ("NAV2 Course 090°", "Navigation.SelectedCourse2", 1.57, '#44ffaa'),
            ("NAV2 Course 180°", "Navigation.SelectedCourse2", 3.14, '#44ffaa'),
            ("XPDR 1200", "Communication.TransponderCode", 1200, '#ffaa44'),
            ("XPDR 7000", "Communication.TransponderCode", 7000, '#ffaa44')
        ])
        
        # === AUTOPILOT ===
        self.create_section(main_frame, "🤖 AUTOPILOT", 5, [
            ("AP MASTER ON", "Autopilot.Master", 1.0, '#ff44ff'),
            ("AP MASTER OFF", "Autopilot.Master", 0.0, '#ff44ff'),
            ("AP HEADING MODE", "Autopilot.Heading", 1.0, '#ff44ff'),
            ("AP VS MODE", "Autopilot.VerticalSpeed", 1.0, '#ff44ff'),
            ("AP Speed 30 m/s", "Autopilot.SelectedAirspeed", 30, '#ff66ff'),
            ("AP Speed 40 m/s", "Autopilot.SelectedAirspeed", 40, '#ff66ff'),
            ("AP Speed 50 m/s", "Autopilot.SelectedAirspeed", 50, '#ff66ff'),
            ("AP Speed 60 m/s", "Autopilot.SelectedAirspeed", 60, '#ff66ff'),
            ("AP Speed 80 m/s", "Autopilot.SelectedAirspeed", 80, '#ff66ff'),
            ("AP Speed 100 m/s", "Autopilot.SelectedAirspeed", 100, '#ff66ff'),
            ("AP Heading 000°", "Autopilot.SelectedHeading", 0.0, '#ff66ff'),
            ("AP Heading 090°", "Autopilot.SelectedHeading", 1.57, '#ff66ff'),
            ("AP Heading 180°", "Autopilot.SelectedHeading", 3.14, '#ff66ff'),
            ("AP Heading 270°", "Autopilot.SelectedHeading", 4.71, '#ff66ff'),
            ("AP Alt 300m", "Autopilot.SelectedAltitude", 300, '#ff88ff'),
            ("AP Alt 500m", "Autopilot.SelectedAltitude", 500, '#ff88ff'),
            ("AP Alt 1000m", "Autopilot.SelectedAltitude", 1000, '#ff88ff'),
            ("AP Alt 1500m", "Autopilot.SelectedAltitude", 1500, '#ff88ff'),
            ("AP Alt 3000m", "Autopilot.SelectedAltitude", 3000, '#ff88ff'),
            ("AP VS +10 m/s", "Autopilot.SelectedVerticalSpeed", 10, '#ffaaff'),
            ("AP VS +5 m/s", "Autopilot.SelectedVerticalSpeed", 5, '#ffaaff'),
            ("AP VS +2 m/s", "Autopilot.SelectedVerticalSpeed", 2, '#ffaaff'),
            ("AP VS 0 m/s", "Autopilot.SelectedVerticalSpeed", 0, '#ffaaff'),
            ("AP VS -2 m/s", "Autopilot.SelectedVerticalSpeed", -2, '#ffaaff'),
            ("AP VS -5 m/s", "Autopilot.SelectedVerticalSpeed", -5, '#ffaaff'),
            ("AP VS -10 m/s", "Autopilot.SelectedVerticalSpeed", -10, '#ffaaff'),
            ("AUTOTHROTTLE ON", "Autopilot.ThrottleEngaged", 1.0, '#dd44ff'),
            ("AUTOTHROTTLE OFF", "Autopilot.ThrottleEngaged", 0.0, '#dd44ff')
        ])
        
        # === ENGINES & FUEL ===
        self.create_section(main_frame, "🔧 ENGINES & FUEL", 6, [
            ("ENG1 MASTER ON", "Aircraft.EngineMaster1", 1.0, '#ffaa00'),
            ("ENG1 MASTER OFF", "Aircraft.EngineMaster1", 0.0, '#ffaa00'),
            ("ENG2 MASTER ON", "Aircraft.EngineMaster2", 1.0, '#ffaa00'),
            ("ENG2 MASTER OFF", "Aircraft.EngineMaster2", 0.0, '#ffaa00'),
            ("ENG3 MASTER ON", "Aircraft.EngineMaster3", 1.0, '#ffaa00'),
            ("ENG3 MASTER OFF", "Aircraft.EngineMaster3", 0.0, '#ffaa00'),
            ("ENG4 MASTER ON", "Aircraft.EngineMaster4", 1.0, '#ffaa00'),
            ("ENG4 MASTER OFF", "Aircraft.EngineMaster4", 0.0, '#ffaa00'),
            ("ENG1 STARTER", "Aircraft.Starter1", 1.0, '#ffcc00'),
            ("ENG2 STARTER", "Aircraft.Starter2", 1.0, '#ffcc00'),
            ("ENG3 STARTER", "Aircraft.Starter3", 1.0, '#ffcc00'),
            ("ENG4 STARTER", "Aircraft.Starter4", 1.0, '#ffcc00'),
            ("ENG1 IGNITION ON", "Aircraft.Ignition1", 1.0, '#ffdd00'),
            ("ENG1 IGNITION OFF", "Aircraft.Ignition1", 0.0, '#ffdd00'),
            ("ENG2 IGNITION ON", "Aircraft.Ignition2", 1.0, '#ffdd00'),
            ("ENG2 IGNITION OFF", "Aircraft.Ignition2", 0.0, '#ffdd00'),
            ("ENG3 IGNITION ON", "Aircraft.Ignition3", 1.0, '#ffdd00'),
            ("ENG3 IGNITION OFF", "Aircraft.Ignition3", 0.0, '#ffdd00'),
            ("ENG4 IGNITION ON", "Aircraft.Ignition4", 1.0, '#ffdd00'),
            ("ENG4 IGNITION OFF", "Aircraft.Ignition4", 0.0, '#ffdd00'),
            ("MIXTURE1 CUTOFF", "Controls.Mixture1", 0.0, '#aa00ff'),
            ("MIXTURE1 LEAN", "Controls.Mixture1", 0.5, '#aa00ff'),
            ("MIXTURE1 RICH", "Controls.Mixture1", 1.0, '#aa00ff'),
            ("MIXTURE2 CUTOFF", "Controls.Mixture2", 0.0, '#aa00ff'),
            ("MIXTURE2 LEAN", "Controls.Mixture2", 0.5, '#aa00ff'),
            ("MIXTURE2 RICH", "Controls.Mixture2", 1.0, '#aa00ff'),
            ("MIXTURE3 RICH", "Controls.Mixture3", 1.0, '#aa00ff'),
            ("MIXTURE4 RICH", "Controls.Mixture4", 1.0, '#aa00ff'),
            ("PROP1 LOW", "Controls.PropellerSpeed1", 0.5, '#0088ff'),
            ("PROP1 HIGH", "Controls.PropellerSpeed1", 1.0, '#0088ff'),
            ("PROP2 LOW", "Controls.PropellerSpeed2", 0.5, '#0088ff'),
            ("PROP2 HIGH", "Controls.PropellerSpeed2", 1.0, '#0088ff'),
            ("PROP3 HIGH", "Controls.PropellerSpeed3", 1.0, '#0088ff'),
            ("PROP4 HIGH", "Controls.PropellerSpeed4", 1.0, '#0088ff'),
            ("THRUST REV1 OFF", "Controls.ThrustReverse1", 0.0, '#ff8800'),
            ("THRUST REV1 ON", "Controls.ThrustReverse1", 1.0, '#ff8800'),
            ("THRUST REV2 OFF", "Controls.ThrustReverse2", 0.0, '#ff8800'),
            ("THRUST REV2 ON", "Controls.ThrustReverse2", 1.0, '#ff8800'),
            ("THRUST REV3 ON", "Controls.ThrustReverse3", 1.0, '#ff8800'),
            ("THRUST REV4 ON", "Controls.ThrustReverse4", 1.0, '#ff8800')
        ])
        
        # === HELICOPTER CONTROLS ===
        self.create_section(main_frame, "🚁 HELICOPTER", 7, [
            ("COLLECTIVE DOWN", "Controls.Collective", 0.2, '#ff0088'),
            ("COLLECTIVE LOW", "Controls.Collective", 0.3, '#ff0088'),
            ("COLLECTIVE MID", "Controls.Collective", 0.5, '#ff0088'),
            ("COLLECTIVE HIGH", "Controls.Collective", 0.7, '#ff0088'),
            ("COLLECTIVE UP", "Controls.Collective", 0.8, '#ff0088'),
            ("CYCLIC CENTER", "Controls.CyclicPitch", 0.0, '#ff0088'),
            ("CYCLIC FWD", "Controls.CyclicPitch", 0.3, '#ff0088'),
            ("CYCLIC AFT", "Controls.CyclicPitch", -0.3, '#ff0088'),
            ("CYCLIC NEUTRAL", "Controls.CyclicRoll", 0.0, '#ff0088'),
            ("CYCLIC LEFT", "Controls.CyclicRoll", -0.3, '#ff0088'),
            ("CYCLIC RIGHT", "Controls.CyclicRoll", 0.3, '#ff0088'),
            ("TAIL ROTOR CENTER", "Controls.TailRotor", 0.0, '#ff0088'),
            ("TAIL ROTOR LEFT", "Controls.TailRotor", -0.5, '#ff0088'),
            ("TAIL ROTOR RIGHT", "Controls.TailRotor", 0.5, '#ff0088'),
            ("ROTOR BRAKE OFF", "Controls.RotorBrake", 0.0, '#ff0088'),
            ("ROTOR BRAKE ON", "Controls.RotorBrake", 1.0, '#ff0088'),
            ("HELI THR1 50%", "Controls.HelicopterThrottle1", 0.5, '#ff4488'),
            ("HELI THR1 75%", "Controls.HelicopterThrottle1", 0.75, '#ff4488'),
            ("HELI THR2 50%", "Controls.HelicopterThrottle2", 0.5, '#ff4488'),
            ("HELI THR2 75%", "Controls.HelicopterThrottle2", 0.75, '#ff4488')
        ])
        
        # === TRIM CONTROLS ===
        self.create_section(main_frame, "✂️ TRIM & FLIGHT AIDS", 3, [
            ("AILERON TRIM L", "Controls.AileronTrim", -0.1, '#00ddff'),
            ("AILERON TRIM 0", "Controls.AileronTrim", 0.0, '#00ddff'),
            ("AILERON TRIM R", "Controls.AileronTrim", 0.1, '#00ddff'),
            ("RUDDER TRIM L", "Controls.RudderTrim", -0.1, '#00ddff'),
            ("RUDDER TRIM 0", "Controls.RudderTrim", 0.0, '#00ddff'),
            ("RUDDER TRIM R", "Controls.RudderTrim", 0.1, '#00ddff'),
            ("YAW DAMPER OFF", "Aircraft.YawDamperEnabled", 0.0, '#00ffdd'),
            ("YAW DAMPER ON", "Aircraft.YawDamperEnabled", 1.0, '#00ffdd'),
            ("AUTO PITCH TRIM", "Aircraft.AutoPitchTrim", 1.0, '#00ffdd'),
            ("CONNECT PEDALS", "Controls.PedalsDisconnect", 0.0, '#ffdd00'),
            ("DISCONNECT PEDALS", "Controls.PedalsDisconnect", 1.0, '#ffdd00')
        ])
        
        # === SIMULATION CONTROLS ===
        self.create_section(main_frame, "🎮 SIMULATION", 4, [
            ("PAUSE OFF", "Simulation.Pause", 0.0, '#888888'),
            ("PAUSE ON", "Simulation.Pause", 1.0, '#888888'),
            ("LIFT AIRCRAFT", "Simulation.LiftUp", 1.0, '#888888'),
            ("SOUND ON", "Simulation.Sound", 1.0, '#888888'),
            ("SOUND OFF", "Simulation.Sound", 0.0, '#888888'),
            ("FLIGHT INFO", "Simulation.FlightInformation", 1.0, '#888888'),
            ("MOVING MAP", "Simulation.MovingMap", 1.0, '#888888'),
            ("MOUSE CONTROL", "Simulation.UseMouseControl", 1.0, '#888888'),
            ("TIME CHANGE", "Simulation.TimeChange", 1.0, '#888888'),
            ("VISIBILITY MAX", "Simulation.Visibility", 1.0, '#888888'),
            ("VISIBILITY MIN", "Simulation.Visibility", 0.1, '#888888'),
            ("PLAYBACK START", "Simulation.PlaybackStart", 1.0, '#666666'),
            ("PLAYBACK STOP", "Simulation.PlaybackStop", 1.0, '#666666')
        ])
        
    def create_section(self, parent, title, index, buttons):
        """Create a section with title and buttons"""
        # Calculate position in 5-column grid
        row = index // 5
        col = index % 5
        
        # Section frame
        section_frame = tk.LabelFrame(parent, text=title, 
                                    font=('Arial', 11, 'bold'),
                                    fg='white', bg='#2b2b2b',
                                    relief='ridge', bd=2)
        section_frame.grid(row=row, column=col, sticky='nsew', padx=5, pady=5)
        
        # Configure grid weights for proper resizing
        parent.grid_rowconfigure(row, weight=1)
        
        # Add buttons (all single commands now)
        for i, (text, variable, value, color) in enumerate(buttons):
            btn = tk.Button(section_frame, text=text,
                          font=('Arial', 10, 'bold'),  # Increased font size
                          bg=color, fg='white',
                          relief='raised', bd=2,
                          height=2,  # Make buttons taller
                          command=lambda v=variable, val=value: self.send_command(v, val))
            
            btn.grid(row=i//2, column=i%2, sticky='nsew', padx=3, pady=3)  # Increased padding
            
        # Configure button grid
        for i in range((len(buttons) + 1) // 2):
            section_frame.grid_rowconfigure(i, weight=1)
        section_frame.grid_columnconfigure(0, weight=1)
        section_frame.grid_columnconfigure(1, weight=1)
    
    def send_command(self, variable, value):
        """Send single command to Aerofly (Compatible with DLL)"""
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
    
    def send_sequence_commands(self, commands, delay=0.1):
        """Send a sequence of commands with delay (Alternative to MULTI)"""
        for i, (variable, value) in enumerate(commands):
            if i > 0:
                time.sleep(delay)  # Small delay between commands
            self.send_command(variable, value)
    
    def run(self):
        """Start the application"""
        self.root.mainloop()

if __name__ == "__main__":
    print("=== AEROFLY FS4 SIMPLE CONTROLLER V3 - DLL COMPATIBLE ===")
    print("✅ NO MULTI COMMANDS - Individual commands only")
    print("✅ Compatible with Aerofly Bridge DLL")
    print("✅ 5-Column Layout for Optimal Button Size")
    print("Make sure Aerofly FS4 is running with the Bridge DLL loaded")
    print("Commands will be sent to localhost:12346")
    print("Window size: 1900x1000 for optimal display")
    print("=" * 60)
    
    app = SimpleAeroflyController()
    app.run()
