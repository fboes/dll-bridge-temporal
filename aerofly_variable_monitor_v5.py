# aerofly_variable_monitor_corrected.py - Complete GUI with CORRECTED offsets
import tkinter as tk
from tkinter import ttk, scrolledtext
import mmap
import struct
import socket
import json
import threading
import time

class AeroflyVariableMonitor:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Aerofly FS4 - Variable Monitor & Control Panel (CORRECTED)")
        self.root.geometry("1400x900")
        
        # Connection objects
        self.shared_memory = None
        self.connected = False
        self.update_thread = None
        self.running = False
        
        # Variable storage
        self.variables = {}
        self.controls = {}
        
        # Setup GUI
        self.setup_gui()
        self.setup_variables()

        # Configure button command after all methods are defined
        self.connect_btn.config(command=self.connect)
        
    def setup_gui(self):
        """Setup the GUI layout"""
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Connection frame
        conn_frame = ttk.LabelFrame(main_frame, text="Connection", padding="5")
        conn_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)
        
        self.connect_btn = ttk.Button(conn_frame, text="Connect to Aerofly")
        self.connect_btn.grid(row=0, column=0, padx=5)

        self.status_label = ttk.Label(conn_frame, text="Disconnected", foreground="red")
        self.status_label.grid(row=0, column=1, padx=10)

        self.update_rate_label = ttk.Label(conn_frame, text="Update Rate: 0 Hz")
        self.update_rate_label.grid(row=0, column=2, padx=10)
        
        # Create notebook for tabs
        self.notebook = ttk.Notebook(main_frame)
        self.notebook.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        
        # Create tabs for each category
        self.create_aircraft_tab()
        self.create_controls_tab()
        self.create_navigation_tab()
        self.create_autopilot_tab()
        self.create_performance_tab()
        self.create_engine_tab()
        self.create_environment_tab()
        self.create_warnings_tab()
        self.create_systems_tab()
        
        # All Variables tab (keep at the end)
        self.create_all_variables_tab()
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)

    def add_readonly_var(self, parent, label, var_name, row, col, format_func=None):
        """Add a read-only variable display"""
        ttk.Label(parent, text=f"{label}:").grid(row=row, column=col, sticky=tk.W, padx=5, pady=2)
        
        var_label = ttk.Label(parent, text="---", foreground="blue", font=('Courier', 10))
        var_label.grid(row=row, column=col+1, sticky=tk.W, padx=5, pady=2)
        
        self.variables[var_name] = {
            'label': var_label,
            'format_func': format_func or (lambda x: f"{x:.3f}"),
            'type': 'readonly'
        }
        
    def add_control_var(self, parent, label, var_name, row, min_val, max_val, default_val):
        """Add a controllable variable with slider and entry"""
        ttk.Label(parent, text=f"{label}:").grid(row=row, column=0, sticky=tk.W, padx=5, pady=2)
        
        # Current value display
        value_label = ttk.Label(parent, text="---", foreground="green", font=('Courier', 10))
        value_label.grid(row=row, column=1, padx=5, pady=2)
        
        # Slider
        var = tk.DoubleVar(value=default_val)
        scale = ttk.Scale(parent, from_=min_val, to=max_val, variable=var, orient=tk.HORIZONTAL, length=200)
        scale.grid(row=row, column=2, padx=5, pady=2)
        
        # Entry for precise values
        entry = ttk.Entry(parent, width=10)
        entry.grid(row=row, column=3, padx=5, pady=2)
        entry.insert(0, str(default_val))
        
        # Send button
        send_btn = ttk.Button(parent, text="Send", 
                                command=lambda: self.send_from_control(var_name, var, entry))
        send_btn.grid(row=row, column=4, padx=5, pady=2)
        
        self.variables[var_name] = {
            'label': value_label,
            'format_func': lambda x: f"{x:.3f}",
            'type': 'control'
        }
        
        self.controls[var_name] = {
            'var': var,
            'entry': entry,
            'scale': scale
        }
        
    def send_from_control(self, var_name, var, entry):
        """Send command from control"""
        try:
            # Try to get value from entry first, then from slider
            value = float(entry.get())
            var.set(value)  # Update slider
        except ValueError:
            value = var.get()  # Use slider value
            entry.delete(0, tk.END)
            entry.insert(0, f"{value:.3f}")  # Update entry
            
        self.send_command(var_name, value)
        
    def send_command(self, variable, value):
        """Send command to Aerofly"""
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
            
            print(f"Command sent: {variable} = {value}")
            
        except Exception as e:
            print(f"Error sending command: {e}")
            
    def setup_variables(self):
        """Initialize all variable structures"""
        # This will be populated with all 285 variables
        pass
        
    def create_aircraft_tab(self):
        """Create Aircraft data tab with ALL aircraft variables"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Aircraft")
        
        # Create scrollable frame
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # Position & Navigation frame
        pos_frame = ttk.LabelFrame(scrollable_frame, text="Position & Navigation", padding="5")
        pos_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(pos_frame, "Latitude", "Aircraft.Latitude", row, 0, lambda x: f"{x:.6f}°")
        self.add_readonly_var(pos_frame, "Longitude", "Aircraft.Longitude", row, 2, lambda x: f"{x:.6f}°")
        self.add_readonly_var(pos_frame, "Altitude", "Aircraft.Altitude", row, 4, lambda x: f"{x:.1f} m")
        row += 1
        self.add_readonly_var(pos_frame, "True Heading", "Aircraft.TrueHeading", row, 0, lambda x: f"{x:.1f}°")
        self.add_readonly_var(pos_frame, "Magnetic Heading", "Aircraft.MagneticHeading", row, 2, lambda x: f"{x:.1f}°")
        self.add_readonly_var(pos_frame, "Height", "Aircraft.Height", row, 4, lambda x: f"{x:.1f} m")
        row += 2
        self.add_readonly_var(pos_frame, "Pitch", "Aircraft.Pitch", row, 0, lambda x: f"{x:.2f}°")
        self.add_readonly_var(pos_frame, "Bank", "Aircraft.Bank", row, 2, lambda x: f"{x:.2f}°")
        self.add_readonly_var(pos_frame, "Rate of Turn", "Aircraft.RateOfTurn", row, 4, lambda x: f"{x:.3f} rad/s")
        
        # Speed frame
        speed_frame = ttk.LabelFrame(scrollable_frame, text="Speeds", padding="5")
        speed_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(speed_frame, "IAS", "Aircraft.IndicatedAirspeed", row, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speed_frame, "IAS Trend", "Aircraft.IndicatedAirspeedTrend", row, 2, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speed_frame, "Ground Speed", "Aircraft.GroundSpeed", row, 4, lambda x: f"{x:.1f} m/s")
        row += 1
        self.add_readonly_var(speed_frame, "Vertical Speed", "Aircraft.VerticalSpeed", row, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speed_frame, "Mach Number", "Aircraft.MachNumber", row, 2, lambda x: f"{x:.3f}")
        self.add_readonly_var(speed_frame, "Angle of Attack", "Aircraft.AngleOfAttack", row, 4, lambda x: f"{x:.2f}°")
        
        # Aircraft State frame
        state_frame = ttk.LabelFrame(scrollable_frame, text="Aircraft State", padding="5")
        state_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(state_frame, "On Ground", "Aircraft.OnGround", row, 0, lambda x: "Yes" if x > 0.5 else "No")
        self.add_readonly_var(state_frame, "On Runway", "Aircraft.OnRunway", row, 2, lambda x: "Yes" if x > 0.5 else "No")
        self.add_readonly_var(state_frame, "Crashed", "Aircraft.Crashed", row, 4, lambda x: "Yes" if x > 0.5 else "No")
        row += 1
        self.add_readonly_var(state_frame, "Gear Position", "Aircraft.Gear", row, 0, lambda x: f"{x:.2f}")
        self.add_readonly_var(state_frame, "Flaps Position", "Aircraft.Flaps", row, 2, lambda x: f"{x*100:.1f}%")
        self.add_readonly_var(state_frame, "Slats Position", "Aircraft.Slats", row, 4, lambda x: f"{x:.2f}")
        row += 2
        self.add_readonly_var(state_frame, "Throttle", "Aircraft.Throttle", row, 0, lambda x: f"{x:.2f}")
        self.add_readonly_var(state_frame, "Air Brake", "Aircraft.AirBrake", row, 2, lambda x: f"{x:.2f}")
        self.add_readonly_var(state_frame, "Parking Brake", "Aircraft.ParkingBrake", row, 4, lambda x: "ON" if x > 0.5 else "OFF")
        
        # Pack canvas and scrollbar
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

    def create_controls_tab(self):
        """Create Controls tab with ALL control variables"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Controls")
        
        # Flight controls frame
        flight_frame = ttk.LabelFrame(frame, text="Flight Controls", padding="5")
        flight_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        self.add_control_var(flight_frame, "Pitch Input", "Controls.Pitch.Input", 0, -1.0, 1.0, 0.0)
        self.add_control_var(flight_frame, "Roll Input", "Controls.Roll.Input", 1, -1.0, 1.0, 0.0)
        self.add_control_var(flight_frame, "Yaw Input", "Controls.Yaw.Input", 2, -1.0, 1.0, 0.0)
        self.add_control_var(flight_frame, "Throttle", "Controls.Throttle", 3, 0.0, 1.0, 0.0)
        
        # System controls frame
        system_frame = ttk.LabelFrame(frame, text="System Controls", padding="5")
        system_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        self.add_control_var(system_frame, "Flaps", "Controls.Flaps", 0, 0.0, 1.0, 0.0)
        self.add_control_var(system_frame, "Gear", "Controls.Gear", 1, 0.0, 1.0, 0.0)
        self.add_control_var(system_frame, "Air Brake", "Controls.AirBrake", 2, 0.0, 1.0, 0.0)
        self.add_control_var(system_frame, "Mixture", "Controls.Mixture", 3, 0.0, 1.0, 1.0)
        
        # Preset buttons frame
        preset_frame = ttk.LabelFrame(frame, text="Quick Presets", padding="5")
        preset_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        # Flaps presets
        ttk.Label(preset_frame, text="Flaps:").grid(row=0, column=0, padx=5)
        ttk.Button(preset_frame, text="Up", command=lambda: self.send_command("Controls.Flaps", 0.0)).grid(row=0, column=1, padx=2)
        ttk.Button(preset_frame, text="1", command=lambda: self.send_command("Controls.Flaps", 0.25)).grid(row=0, column=2, padx=2)
        ttk.Button(preset_frame, text="2", command=lambda: self.send_command("Controls.Flaps", 0.5)).grid(row=0, column=3, padx=2)
        ttk.Button(preset_frame, text="3", command=lambda: self.send_command("Controls.Flaps", 0.75)).grid(row=0, column=4, padx=2)
        ttk.Button(preset_frame, text="Full", command=lambda: self.send_command("Controls.Flaps", 1.0)).grid(row=0, column=5, padx=2)
        
        # Gear presets
        ttk.Label(preset_frame, text="Gear:").grid(row=1, column=0, padx=5, pady=5)
        ttk.Button(preset_frame, text="Up", command=lambda: self.send_command("Controls.Gear", 0.0)).grid(row=1, column=1, padx=2)
        ttk.Button(preset_frame, text="Down", command=lambda: self.send_command("Controls.Gear", 1.0)).grid(row=1, column=2, padx=2)
        
    def create_navigation_tab(self):
        """Create Navigation/Communication tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Navigation")
        
        # Create scrollable frame
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # COM frame
        com_frame = ttk.LabelFrame(scrollable_frame, text="Communication", padding="5")
        com_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(com_frame, "COM1 Frequency", "Communication.COM1Frequency", 0, 118000000, 137000000, 122800000)
        self.add_control_var(com_frame, "COM1 Standby", "Communication.COM1StandbyFrequency", 1, 118000000, 137000000, 121500000)
        self.add_control_var(com_frame, "COM2 Frequency", "Communication.COM2Frequency", 2, 118000000, 137000000, 122900000)
        self.add_control_var(com_frame, "COM2 Standby", "Communication.COM2StandbyFrequency", 3, 118000000, 137000000, 121600000)
        
        # NAV frame
        nav_frame = ttk.LabelFrame(scrollable_frame, text="Navigation", padding="5")
        nav_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(nav_frame, "NAV1 Frequency", "Navigation.NAV1Frequency", 0, 108000000, 118000000, 110500000)
        self.add_control_var(nav_frame, "NAV1 Standby", "Navigation.NAV1StandbyFrequency", 1, 108000000, 118000000, 111000000)
        self.add_control_var(nav_frame, "NAV1 Course", "Navigation.SelectedCourse1", 2, 0, 360, 90)
        self.add_control_var(nav_frame, "NAV2 Course", "Navigation.SelectedCourse2", 3, 0, 360, 270)
        
        # Transponder frame
        xpdr_frame = ttk.LabelFrame(scrollable_frame, text="Transponder", padding="5")
        xpdr_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(xpdr_frame, "Transponder Code", "Communication.TransponderCode", 0, 0, 7777, 1200)
        
        # Pack canvas and scrollbar
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
    def create_autopilot_tab(self):
        """Create Autopilot tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Autopilot")
        
        # AP Status frame
        status_frame = ttk.LabelFrame(frame, text="Autopilot Status", padding="5")
        status_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(status_frame, "AP Engaged", "Autopilot.Engaged", row, 0, lambda x: "ON" if x > 0.5 else "OFF")
        self.add_readonly_var(status_frame, "AP Type", "Autopilot.Type", row, 2, lambda x: str(x))
        row += 1
        self.add_readonly_var(status_frame, "Active Lateral Mode", "Autopilot.ActiveLateralMode", row, 0, lambda x: str(x))
        self.add_readonly_var(status_frame, "Active Vertical Mode", "Autopilot.ActiveVerticalMode", row, 2, lambda x: str(x))
        
        # AP Controls frame
        controls_frame = ttk.LabelFrame(frame, text="Autopilot Controls", padding="5")
        controls_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(controls_frame, "Selected Airspeed", "Autopilot.SelectedAirspeed", 0, 20, 150, 50)
        self.add_control_var(controls_frame, "Selected Heading", "Autopilot.SelectedHeading", 1, 0, 360, 90)
        self.add_control_var(controls_frame, "Selected Altitude", "Autopilot.SelectedAltitude", 2, 0, 15000, 1000)
        self.add_control_var(controls_frame, "Selected VS", "Autopilot.SelectedVerticalSpeed", 3, -20, 20, 0)
        
    def create_performance_tab(self):
        """Create Performance speeds tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Performance")
        
        speeds_frame = ttk.LabelFrame(frame, text="V-Speeds", padding="5")
        speeds_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        self.add_readonly_var(speeds_frame, "VS0 (Stall flaps down)", "Performance.Speed.VS0", 0, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speeds_frame, "VS1 (Stall clean)", "Performance.Speed.VS1", 1, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speeds_frame, "VFE (Max flaps)", "Performance.Speed.VFE", 2, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speeds_frame, "VNO (Maneuvering)", "Performance.Speed.VNO", 3, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speeds_frame, "VNE (Never exceed)", "Performance.Speed.VNE", 4, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speeds_frame, "VAPP (Approach)", "Performance.Speed.VAPP", 5, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speeds_frame, "Minimum (Current)", "Performance.Speed.Minimum", 6, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(speeds_frame, "Maximum (Current)", "Performance.Speed.Maximum", 7, 0, lambda x: f"{x:.1f} m/s")
        
    def create_engine_tab(self):
        """Create Engine tab with ALL engine variables"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Engine")
        
        # Create scrollable frame
        canvas = tk.Canvas(frame)
        scrollbar = ttk.Scrollbar(frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # Engine Status frame (Multi-engine support)
        status_frame = ttk.LabelFrame(scrollable_frame, text="Engine Status (Engines 1-4)", padding="5")
        status_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        # Engine 1-4 Status
        for i in range(1, 5):
            row = i - 1
            self.add_readonly_var(status_frame, f"Engine {i} Running", f"Aircraft.EngineRunning{i}", row, 0, 
                                lambda x: "Yes" if x > 0.5 else "No")
            self.add_readonly_var(status_frame, f"Engine {i} RPM", f"Aircraft.EngineRotationSpeed{i}", row, 2, 
                                lambda x: f"{x:.0f} RPM")
            self.add_readonly_var(status_frame, f"Engine {i} Throttle", f"Aircraft.EngineThrottle{i}", row, 4, 
                                lambda x: f"{x:.2f}")
        
        # Engine Systems frame
        systems_frame = ttk.LabelFrame(scrollable_frame, text="Engine Systems", padding="5")
        systems_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(systems_frame, "Engine Master 1", "Aircraft.EngineMaster1", row, 0, 
                            lambda x: "ON" if x > 0.5 else "OFF")
        self.add_readonly_var(systems_frame, "Engine Master 2", "Aircraft.EngineMaster2", row, 2, 
                            lambda x: "ON" if x > 0.5 else "OFF")
        self.add_readonly_var(systems_frame, "Engine Master 3", "Aircraft.EngineMaster3", row, 4, 
                            lambda x: "ON" if x > 0.5 else "OFF")
        row += 1
        self.add_readonly_var(systems_frame, "Starter 1", "Aircraft.Starter1", row, 0, 
                            lambda x: "ENGAGED" if x > 0.5 else "OFF")
        self.add_readonly_var(systems_frame, "Ignition 1", "Aircraft.Ignition1", row, 2, 
                            lambda x: "ON" if x > 0.5 else "OFF")
        self.add_readonly_var(systems_frame, "APU Available", "Aircraft.APUAvailable", row, 4, 
                            lambda x: "Available" if x > 0.5 else "N/A")
        
        # Engine Controls frame
        controls_frame = ttk.LabelFrame(scrollable_frame, text="Engine Controls", padding="5")
        controls_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(controls_frame, "Throttle 1", "Controls.Throttle1", 0, 0.0, 1.0, 0.0)
        self.add_control_var(controls_frame, "Throttle 2", "Controls.Throttle2", 1, 0.0, 1.0, 0.0)
        self.add_control_var(controls_frame, "Mixture 1", "Controls.Mixture1", 2, 0.0, 1.0, 1.0)
        self.add_control_var(controls_frame, "Mixture 2", "Controls.Mixture2", 3, 0.0, 1.0, 1.0)
        
        # Pack canvas and scrollbar
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
    def create_environment_tab(self):
        """Create Environment tab - using Wind data from SDK"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Environment")
        
        # Wind & Environment frame
        wind_frame = ttk.LabelFrame(frame, text="Wind & Environment", padding="5")
        wind_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(wind_frame, "Wind Vector", "Aircraft.Wind", row, 0, lambda x: f"Vector3D")
        row += 1
        self.add_readonly_var(wind_frame, "Universal Time", "Aircraft.UniversalTime", row, 0, lambda x: f"{x:.2f} h")
        self.add_readonly_var(wind_frame, "Gravity Vector", "Aircraft.Gravity", row, 2, lambda x: f"Vector3D")
        
        # Note frame
        note_frame = ttk.LabelFrame(frame, text="Note", padding="5")
        note_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        ttk.Label(note_frame, text="Environment variables available: Wind, Gravity, Time", 
                 font=('Arial', 10)).pack(pady=10)
        
    def create_warnings_tab(self):
        """Create Warnings tab with available warning variables"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Warnings")
        
        # Master Warnings frame
        master_frame = ttk.LabelFrame(frame, text="Master Warnings", padding="5")
        master_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(master_frame, "Master Warning", "Warnings.MasterWarning", row, 0, 
                            lambda x: "🔴 ACTIVE" if x > 0.5 else "🟢 Normal")
        self.add_readonly_var(master_frame, "Master Caution", "Warnings.MasterCaution", row, 2, 
                            lambda x: "🟡 ACTIVE" if x > 0.5 else "🟢 Normal")
        row += 1
        self.add_readonly_var(master_frame, "Engine Fire", "Warnings.EngineFire", row, 0, 
                            lambda x: "🔥 FIRE" if x > 0.5 else "Normal")
        self.add_readonly_var(master_frame, "Low Oil Pressure", "Warnings.LowOilPressure", row, 2, 
                            lambda x: "⬇️ LOW" if x > 0.5 else "Normal")
        row += 2
        self.add_readonly_var(master_frame, "Low Fuel Pressure", "Warnings.LowFuelPressure", row, 0, 
                            lambda x: "⛽ LOW" if x > 0.5 else "Normal")
        self.add_readonly_var(master_frame, "Altitude Alert", "Warnings.AltitudeAlert", row, 2, 
                            lambda x: "📏 ALERT" if x > 0.5 else "Normal")
        
        # Warning Status frame
        status_frame = ttk.LabelFrame(frame, text="Warning Status", padding="5")
        status_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(status_frame, "Warning Active", "Warnings.WarningActive", row, 0, 
                            lambda x: "⚠️ ACTIVE" if x > 0.5 else "Normal")
        self.add_readonly_var(status_frame, "Warning Mute", "Warnings.WarningMute", row, 2, 
                            lambda x: "🔇 MUTED" if x > 0.5 else "Normal")
        
    def create_systems_tab(self):
        """Create Systems tab - placeholder for future implementation"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Systems")
        
        # Note frame
        note_frame = ttk.LabelFrame(frame, text="Systems Information", padding="5")
        note_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        ttk.Label(note_frame, text="System variables will be added as they become available in the SDK", 
                 font=('Arial', 12)).pack(pady=20)
        
    def create_all_variables_tab(self):
        """Create tab showing all 285 variables in 6 columns"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="All Variables")
        
        # Info frame
        info_frame = ttk.Frame(frame)
        info_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        ttk.Label(info_frame, text="All 266 Variables Array - Real-time Values").grid(row=0, column=0, padx=5)
        self.var_count_label = ttk.Label(info_frame, text="Variables: 0/266", foreground="blue")
        self.var_count_label.grid(row=0, column=1, padx=20)
        
        # Create Treeview with 6 columns
        self.tree_frame = ttk.Frame(frame)
        self.tree_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        columns = ('var1', 'val1', 'var2', 'val2', 'var3', 'val3', 'var4', 'val4', 'var5', 'val5', 'var6', 'val6')
        self.variables_tree = ttk.Treeview(self.tree_frame, columns=columns, show='headings', height=25)
        
        # Configure column headers and widths
        for i in range(6):
            var_col = f'var{i+1}'
            val_col = f'val{i+1}'
            self.variables_tree.heading(var_col, text=f'Var {i*48}+')
            self.variables_tree.heading(val_col, text='Value')
            self.variables_tree.column(var_col, width=70, anchor='center')
            self.variables_tree.column(val_col, width=90, anchor='e')
        
        # Add scrollbar
        tree_scrollbar = ttk.Scrollbar(self.tree_frame, orient=tk.VERTICAL, command=self.variables_tree.yview)
        self.variables_tree.configure(yscrollcommand=tree_scrollbar.set)
        
        # Grid the treeview and scrollbar
        self.variables_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        tree_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Configure grid weights
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(1, weight=1)
        self.tree_frame.columnconfigure(0, weight=1)
        self.tree_frame.rowconfigure(0, weight=1)
        
        # Populate initial tree structure (45 rows x 6 columns = 270 variables, showing 266)
        self.tree_items = []
        for row in range(45):
            item = self.variables_tree.insert('', 'end', values=['---'] * 12)
            self.tree_items.append(item)
        
    def connect(self):
        """Connect to Aerofly shared memory"""
        try:
            self.shared_memory = mmap.mmap(-1, 2800, "AeroflyBridgeData")
            
            # Test connection
            self.shared_memory.seek(8)  # data_valid offset
            valid = struct.unpack('I', self.shared_memory.read(4))[0]
            
            if valid == 1:
                self.connected = True
                self.status_label.config(text="Connected", foreground="green")
                self.connect_btn.config(text="Disconnect", command=self.disconnect)
                
                # Start update thread
                self.running = True
                self.update_thread = threading.Thread(target=self.update_loop, daemon=True)
                self.update_thread.start()
                
                print("Connected to Aerofly Bridge")
            else:
                raise Exception("Invalid data - make sure Aerofly is running")
                
        except Exception as e:
            print(f"Connection failed: {e}")
            self.status_label.config(text=f"Error: {e}", foreground="red")
            
    def disconnect(self):
        """Disconnect from Aerofly"""
        self.running = False
        self.connected = False
        
        if self.shared_memory:
            self.shared_memory.close()
            self.shared_memory = None
            
        self.status_label.config(text="Disconnected", foreground="red")
        self.connect_btn.config(text="Connect to Aerofly", command=self.connect)
        
    def read_double(self, offset):
        """Read double from shared memory"""
        if not self.shared_memory:
            return 0.0
        try:
            self.shared_memory.seek(offset)
            data = self.shared_memory.read(8)
            return struct.unpack('d', data)[0]
        except:
            return 0.0
            
    def read_vector3d(self, offset):
        """Read Vector3D from shared memory"""
        if not self.shared_memory:
            return (0.0, 0.0, 0.0)
        try:
            self.shared_memory.seek(offset)
            data = self.shared_memory.read(24)  # 3 doubles = 24 bytes
            return struct.unpack('ddd', data)
        except:
            return (0.0, 0.0, 0.0)
            
    def update_loop(self):
        """Main update loop"""
        last_update = time.time()
        update_count = 0
        
        # CORRECTED Variable offsets based on AeroflyBridgeData structure
        offsets = {
            # === HEADER (16 bytes) ===
            'timestamp_us': 0,
            'data_valid': 8,
            'update_counter': 12,
            
            # === AIRCRAFT BASIC (64 bytes starting at 16) ===
            'Aircraft.Latitude': 16,                    # double latitude
            'Aircraft.Longitude': 24,                   # double longitude
            'Aircraft.Altitude': 32,                    # double altitude
            'Aircraft.Pitch': 40,                       # double pitch
            'Aircraft.Bank': 48,                        # double bank
            'Aircraft.TrueHeading': 56,                 # double true_heading
            'Aircraft.MagneticHeading': 64,             # double magnetic_heading
            'Aircraft.IndicatedAirspeed': 72,           # double indicated_airspeed
            
            # === AIRCRAFT PHYSICS (96 bytes starting at 80) ===
            'Aircraft.GroundSpeed': 80,                 # double ground_speed
            'Aircraft.VerticalSpeed': 88,               # double vertical_speed
            'Aircraft.AngleOfAttack': 96,               # double angle_of_attack
            'Aircraft.AngleOfAttackLimit': 104,         # double angle_of_attack_limit
            'Aircraft.MachNumber': 112,                 # double mach_number
            'Aircraft.RateOfTurn': 120,                 # double rate_of_turn
            'Aircraft.Position': 128,                   # tm_vector3d position (24 bytes)
            'Aircraft.Velocity': 152,                   # tm_vector3d velocity (24 bytes)
            'Aircraft.Acceleration': 176,               # tm_vector3d acceleration (24 bytes)
            'Aircraft.AngularVelocity': 200,            # tm_vector3d angular_velocity (24 bytes)
            'Aircraft.Wind': 224,                       # tm_vector3d wind (24 bytes)
            'Aircraft.Gravity': 248,                    # tm_vector3d gravity (24 bytes)
            
            # === AIRCRAFT STATE (64 bytes starting at 272) ===
            'Aircraft.OnGround': 272,                   # double on_ground
            'Aircraft.OnRunway': 280,                   # double on_runway
            'Aircraft.Crashed': 288,                    # double crashed
            'Aircraft.Gear': 296,                       # double gear_position
            'Aircraft.Flaps': 304,                      # double flaps_position
            'Aircraft.Slats': 312,                      # double slats_position
            'Aircraft.Throttle': 320,                   # double throttle_position
            'Aircraft.AirBrake': 328,                   # double airbrake_position
            
            # === ENGINE DATA (32 bytes starting at 336) ===
            'Aircraft.EngineThrottle1': 336,            # double engine_throttle[0]
            'Aircraft.EngineThrottle2': 344,            # double engine_throttle[1]
            'Aircraft.EngineThrottle3': 352,            # double engine_throttle[2]
            'Aircraft.EngineThrottle4': 360,            # double engine_throttle[3]
            'Aircraft.EngineRotationSpeed1': 368,       # double engine_rotation_speed[0]
            'Aircraft.EngineRotationSpeed2': 376,       # double engine_rotation_speed[1]
            'Aircraft.EngineRotationSpeed3': 384,       # double engine_rotation_speed[2]
            'Aircraft.EngineRotationSpeed4': 392,       # double engine_rotation_speed[3]
            'Aircraft.EngineRunning1': 400,             # double engine_running[0]
            'Aircraft.EngineRunning2': 408,             # double engine_running[1]
            'Aircraft.EngineRunning3': 416,             # double engine_running[2]
            'Aircraft.EngineRunning4': 424,             # double engine_running[3]
            
            # === CONTROLS INPUT (24 bytes starting at 432) ===
            'Controls.Pitch.Input': 432,                # double pitch_input
            'Controls.Roll.Input': 440,                 # double roll_input
            'Controls.Yaw.Input': 448,                  # double yaw_input
            
            # === NAVIGATION FREQUENCIES (80 bytes starting at 456) ===
            'Communication.COM1Frequency': 456,         # double com1_frequency
            'Communication.COM1StandbyFrequency': 464,  # double com1_standby_frequency
            'Communication.COM2Frequency': 472,         # double com2_frequency
            'Communication.COM2StandbyFrequency': 480,  # double com2_standby_frequency
            'Navigation.NAV1Frequency': 488,            # double nav1_frequency
            'Navigation.NAV1StandbyFrequency': 496,     # double nav1_standby_frequency
            'Navigation.SelectedCourse1': 504,          # double nav1_selected_course
            'Navigation.NAV2Frequency': 512,            # double nav2_frequency
            'Navigation.NAV2StandbyFrequency': 520,     # double nav2_standby_frequency
            'Navigation.SelectedCourse2': 528,          # double nav2_selected_course
            
            # === AUTOPILOT (64 bytes starting at 536) ===
            'Autopilot.Engaged': 536,                   # double ap_engaged
            'Autopilot.SelectedAirspeed': 544,          # double ap_selected_airspeed
            'Autopilot.SelectedHeading': 552,           # double ap_selected_heading
            'Autopilot.SelectedAltitude': 560,          # double ap_selected_altitude
            'Autopilot.SelectedVerticalSpeed': 568,     # double ap_selected_vs
            'Autopilot.ThrottleEngaged': 576,           # double ap_throttle_engaged
            'Autopilot.ActiveLateralMode': 584,         # char ap_lateral_mode[16]
            'Autopilot.ActiveVerticalMode': 600,        # char ap_vertical_mode[16]
            
            # === PERFORMANCE SPEEDS (40 bytes starting at 616) ===
            'Performance.Speed.VS0': 616,               # double vs0_speed
            'Performance.Speed.VS1': 624,               # double vs1_speed
            'Performance.Speed.VFE': 632,               # double vfe_speed
            'Performance.Speed.VNO': 640,               # double vno_speed
            'Performance.Speed.VNE': 648,               # double vne_speed
            
            # === WARNINGS (16 bytes starting at 656) ===
            'Warnings.MasterWarning': 660,              # uint32_t master_warning
            'Warnings.MasterCaution': 664,              # uint32_t master_caution
            
            # === ALL VARIABLES ARRAY (2280 bytes starting at 672) ===
            'all_variables_base': 672,                  # double all_variables[285]
            
            # Additional SDK variables (based on corrected MESSAGE_LIST and VariableIndex)
            'Aircraft.UniversalTime': 672,              # all_variables[0]
            'Aircraft.IndicatedAirspeedTrend': 680,     # all_variables[1]
            'Aircraft.Height': 688,                     # all_variables[2]
            
            # === AIRCRAFT STATE VARIABLES (corrected offsets) ===
            'Aircraft.OnRunway': 672 + (53 * 8),       # all_variables[53] = 1096
            'Aircraft.Crashed': 672 + (54 * 8),        # all_variables[54] = 1104
            'Aircraft.ParkingBrake': 672 + (32 * 8),   # all_variables[32] = 928
            
            # === ENGINE SYSTEM VARIABLES ===
            'Aircraft.EngineMaster1': 672 + (78 * 8),  # all_variables[78] = 1296
            'Aircraft.EngineMaster2': 672 + (79 * 8),  # all_variables[79] = 1304
            'Aircraft.EngineMaster3': 672 + (80 * 8),  # all_variables[80] = 1312
            'Aircraft.Starter1': 672 + (67 * 8),       # all_variables[67] = 1208
            'Aircraft.Ignition1': 672 + (72 * 8),      # all_variables[72] = 1248
            'Aircraft.APUAvailable': 672 + (94 * 8),   # all_variables[94] = 1424
            'Performance.Speed.VAPP': 752,              # all_variables[10]
            'Performance.Speed.Minimum': 760,           # all_variables[11]
            'Performance.Speed.Maximum': 768,           # all_variables[12]
            'Communication.TransponderCode': 776,       # all_variables[13]
            'Autopilot.Type': 784,                      # all_variables[14]
            'Warnings.EngineFire': 792,                 # all_variables[15]
            'Warnings.LowOilPressure': 800,             # all_variables[16]
            'Warnings.LowFuelPressure': 808,            # all_variables[17]
            'Warnings.AltitudeAlert': 816,              # all_variables[18]
            'Warnings.WarningActive': 824,              # all_variables[19]
            'Warnings.WarningMute': 832,                # all_variables[20]
        }
        
        while self.running:
            try:
                # Update all variables
                for var_name, var_info in self.variables.items():
                    if var_name in offsets:
                        offset = offsets[var_name]
                        
                        # Handle Vector3D variables
                        if var_name in ['Aircraft.Position', 'Aircraft.Velocity', 'Aircraft.Acceleration', 
                                       'Aircraft.AngularVelocity', 'Aircraft.Wind', 'Aircraft.Gravity']:
                            vector = self.read_vector3d(offset)
                            formatted_value = f"({vector[0]:.2f}, {vector[1]:.2f}, {vector[2]:.2f})"
                        # Handle string variables (like Autopilot modes)
                        elif var_name in ['Autopilot.ActiveLateralMode', 'Autopilot.ActiveVerticalMode', 'Autopilot.Type']:
                            # For now, treat as double until we implement string reading
                            value = self.read_double(offset)
                            formatted_value = f"Mode: {value:.0f}"
                        else:
                            # Handle normal double variables
                            value = self.read_double(offset)
                            formatted_value = var_info['format_func'](value)
                        
                        var_info['label'].config(text=formatted_value)
                
                # Update rate calculation
                update_count += 1
                current_time = time.time()
                if current_time - last_update >= 1.0:
                    rate = update_count / (current_time - last_update)
                    self.update_rate_label.config(text=f"Update Rate: {rate:.1f} Hz")
                    last_update = current_time
                    update_count = 0
                
                # Update all variables display
                self.update_all_variables_display()
                
            except Exception as e:
                print(f"Update error: {e}")
                break
                
            time.sleep(0.05)  # 20 Hz update rate
            
    def update_all_variables_display(self):
        """Update the all variables tree display with 6 columns"""
        if not hasattr(self, 'variables_tree'):
            return
            
        try:
            base_offset = 672  # Start of all_variables array
            valid_count = 0
            
            # Update tree items (45 rows x 6 columns = 270 slots, showing 266 variables)
            for row in range(45):
                values = []
                
                for col in range(6):
                    var_index = row * 6 + col
                    if var_index < 266:
                        try:
                            value = self.read_double(base_offset + var_index * 8)
                            if abs(value) > 1e-10 or value == 0.0:
                                valid_count += 1
                            values.extend([f"[{var_index:03d}]", f"{value:9.5f}"])
                        except:
                            values.extend([f"[{var_index:03d}]", "ERROR"])
                    else:
                        values.extend(["", ""])
                
                # Update this row in the tree
                if row < len(self.tree_items):
                    self.variables_tree.item(self.tree_items[row], values=values)
            
            # Update count label
            self.var_count_label.config(text=f"Active Variables: {valid_count}/266")
            
        except Exception as e:
            print(f"Error updating variables tree: {e}")
            
    def run(self):
        """Start the GUI"""
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.root.mainloop()
        
    def on_closing(self):
        """Handle window closing"""
        self.running = False
        if self.connected:
            self.disconnect()
        self.root.destroy()

if __name__ == "__main__":
    app = AeroflyVariableMonitor()
    app.run()