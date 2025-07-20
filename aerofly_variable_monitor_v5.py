# aerofly_variable_monitor_v6_complete.py - Complete GUI with ALL 339 Variables
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
        self.root.title("Aerofly FS4 - Complete Variable Monitor & Control Panel (ALL 339 VARIABLES)")
        self.root.geometry("1600x1000")
        
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
        
        # NEW TABS - Complete SDK Coverage
        self.create_configuration_tab()
        self.create_fms_tab()
        self.create_flight_director_tab()
        self.create_copilot_tab()
        self.create_pressurization_tab()
        self.create_view_controls_tab()
        self.create_simulation_tab()
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
        row += 1
        self.add_readonly_var(state_frame, "Throttle", "Aircraft.Throttle", row, 0, lambda x: f"{x:.2f}")
        self.add_readonly_var(state_frame, "Air Brake", "Aircraft.AirBrake", row, 2, lambda x: f"{x:.2f}")
        self.add_readonly_var(state_frame, "Parking Brake", "Aircraft.ParkingBrake", row, 4, lambda x: "ON" if x > 0.5 else "OFF")
        
        # Aircraft Systems frame (NEW)
        systems_frame = ttk.LabelFrame(scrollable_frame, text="Aircraft Systems", padding="5")
        systems_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(systems_frame, "Aircraft Name", "Aircraft.Name", row, 0, lambda x: str(x))
        self.add_readonly_var(systems_frame, "Category Jet", "Aircraft.Category.Jet", row, 2, lambda x: "Yes" if x > 0.5 else "No")
        self.add_readonly_var(systems_frame, "Category Glider", "Aircraft.Category.Glider", row, 4, lambda x: "Yes" if x > 0.5 else "No")
        row += 1
        self.add_readonly_var(systems_frame, "Radar Altitude", "Aircraft.RadarAltitude", row, 0, lambda x: f"{x:.1f} m")
        self.add_readonly_var(systems_frame, "Power", "Aircraft.Power", row, 2, lambda x: f"{x:.2f}")
        self.add_readonly_var(systems_frame, "Normalized Power", "Aircraft.NormalizedPower", row, 4, lambda x: f"{x:.2f}")
        row += 1
        self.add_readonly_var(systems_frame, "Yaw Damper", "Aircraft.YawDamperEnabled", row, 0, lambda x: "ON" if x > 0.5 else "OFF")
        self.add_readonly_var(systems_frame, "Auto Pitch Trim", "Aircraft.AutoPitchTrim", row, 2, lambda x: "ON" if x > 0.5 else "OFF")
        self.add_readonly_var(systems_frame, "Rudder Pedals Disc.", "Aircraft.RudderPedalsDisconnected", row, 4, lambda x: "ON" if x > 0.5 else "OFF")
        
        # Trim Systems frame (NEW)
        trim_frame = ttk.LabelFrame(scrollable_frame, text="Trim Systems", padding="5")
        trim_frame.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(trim_frame, "Pitch Trim", "Aircraft.PitchTrim", row, 0, lambda x: f"{x:.3f}")
        self.add_readonly_var(trim_frame, "Rudder Trim", "Aircraft.RudderTrim", row, 2, lambda x: f"{x:.3f}")
        self.add_readonly_var(trim_frame, "Trim", "Aircraft.Trim", row, 4, lambda x: f"{x:.3f}")
        row += 1
        self.add_readonly_var(trim_frame, "Pitch Trim Scaling", "Aircraft.PitchTrimScaling", row, 0, lambda x: f"{x:.3f}")
        self.add_readonly_var(trim_frame, "Pitch Trim Offset", "Aircraft.PitchTrimOffset", row, 2, lambda x: f"{x:.3f}")
        
        # Airport Information frame (NEW)
        airport_frame = ttk.LabelFrame(scrollable_frame, text="Airport Information", padding="5")
        airport_frame.grid(row=5, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(airport_frame, "Nearest Airport ID", "Aircraft.NearestAirportIdentifier", row, 0, lambda x: str(x))
        self.add_readonly_var(airport_frame, "Nearest Airport", "Aircraft.NearestAirportName", row, 2, lambda x: str(x))
        row += 1
        self.add_readonly_var(airport_frame, "Best Airport ID", "Aircraft.BestAirportIdentifier", row, 0, lambda x: str(x))
        self.add_readonly_var(airport_frame, "Best Airport", "Aircraft.BestAirportName", row, 2, lambda x: str(x))
        row += 1
        self.add_readonly_var(airport_frame, "Best Runway ID", "Aircraft.BestRunwayIdentifier", row, 0, lambda x: str(x))
        self.add_readonly_var(airport_frame, "Best Runway Elevation", "Aircraft.BestRunwayElevation", row, 2, lambda x: f"{x:.1f} m")
        
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
        self.add_control_var(system_frame, "Propeller", "Controls.Propeller", 4, 0.0, 1.0, 1.0)
        self.add_control_var(system_frame, "Left Brake", "Controls.LeftBrake", 5, 0.0, 1.0, 0.0)
        self.add_control_var(system_frame, "Right Brake", "Controls.RightBrake", 6, 0.0, 1.0, 0.0)
        
        # Engine Controls frame (NEW)
        engine_frame = ttk.LabelFrame(frame, text="Multi-Engine Controls", padding="5")
        engine_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        # Engine controls for 4 engines
        for i in range(1, 5):
            row_pos = (i - 1) * 2  # Each engine gets 2 rows (throttle + mixture)
            self.add_control_var(engine_frame, f"Throttle {i}", f"Controls.Throttle{i}", row_pos, 0.0, 1.0, 0.0)
            self.add_control_var(engine_frame, f"Mixture {i}", f"Controls.Mixture{i}", row_pos + 1, 0.0, 1.0, 1.0)
        
        # Trim Controls frame (NEW)
        trim_frame = ttk.LabelFrame(frame, text="Trim Controls", padding="5")
        trim_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(trim_frame, "Pitch Trim", "Controls.PitchTrim", 0, -1.0, 1.0, 0.0)
        self.add_control_var(trim_frame, "Rudder Trim", "Controls.RudderTrim", 1, -1.0, 1.0, 0.0)
        
        # Preset buttons frame
        preset_frame = ttk.LabelFrame(frame, text="Quick Presets", padding="5")
        preset_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
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
        self.add_control_var(nav_frame, "NAV2 Frequency", "Navigation.NAV2Frequency", 3, 108000000, 118000000, 110700000)
        self.add_control_var(nav_frame, "NAV2 Standby", "Navigation.NAV2StandbyFrequency", 4, 108000000, 118000000, 111200000)
        self.add_control_var(nav_frame, "NAV2 Course", "Navigation.SelectedCourse2", 5, 0, 360, 270)
        
        # ILS frame (NEW)
        ils_frame = ttk.LabelFrame(scrollable_frame, text="ILS System", padding="5")
        ils_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(ils_frame, "ILS1 Frequency", "Navigation.ILS1Frequency", 0, 108000000, 112000000, 110500000)
        self.add_control_var(ils_frame, "ILS1 Standby", "Navigation.ILS1StandbyFrequency", 1, 108000000, 112000000, 109500000)
        self.add_control_var(ils_frame, "ILS1 Course", "Navigation.ILS1Course", 2, 0, 360, 90)
        self.add_control_var(ils_frame, "ILS2 Frequency", "Navigation.ILS2Frequency", 3, 108000000, 112000000, 110700000)
        self.add_control_var(ils_frame, "ILS2 Standby", "Navigation.ILS2StandbyFrequency", 4, 108000000, 112000000, 109700000)
        self.add_control_var(ils_frame, "ILS2 Course", "Navigation.ILS2Course", 5, 0, 360, 270)
        
        # DME frame (NEW)
        dme_frame = ttk.LabelFrame(scrollable_frame, text="DME System", padding="5")
        dme_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_readonly_var(dme_frame, "DME1 Distance", "Navigation.DME1Distance", 0, 0, lambda x: f"{x:.2f} nm")
        self.add_readonly_var(dme_frame, "DME1 Time", "Navigation.DME1Time", 0, 2, lambda x: f"{x:.1f} min")
        self.add_readonly_var(dme_frame, "DME1 Speed", "Navigation.DME1Speed", 0, 4, lambda x: f"{x:.1f} kt")
        self.add_readonly_var(dme_frame, "DME2 Distance", "Navigation.DME2Distance", 1, 0, lambda x: f"{x:.2f} nm")
        self.add_readonly_var(dme_frame, "DME2 Time", "Navigation.DME2Time", 1, 2, lambda x: f"{x:.1f} min")
        self.add_readonly_var(dme_frame, "DME2 Speed", "Navigation.DME2Speed", 1, 4, lambda x: f"{x:.1f} kt")
        
        # ADF frame (NEW)
        adf_frame = ttk.LabelFrame(scrollable_frame, text="ADF System", padding="5")
        adf_frame.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(adf_frame, "ADF1 Frequency", "Navigation.ADF1Frequency", 0, 190000, 1750000, 500000)
        self.add_control_var(adf_frame, "ADF1 Standby", "Navigation.ADF1StandbyFrequency", 1, 190000, 1750000, 600000)
        self.add_control_var(adf_frame, "ADF2 Frequency", "Navigation.ADF2Frequency", 2, 190000, 1750000, 700000)
        self.add_control_var(adf_frame, "ADF2 Standby", "Navigation.ADF2StandbyFrequency", 3, 190000, 1750000, 800000)
        
        # Transponder frame
        xpdr_frame = ttk.LabelFrame(scrollable_frame, text="Transponder", padding="5")
        xpdr_frame.grid(row=5, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
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
        self.add_readonly_var(status_frame, "Use Mach Number", "Autopilot.UseMachNumber", row, 4, lambda x: "YES" if x > 0.5 else "NO")
        row += 1
        self.add_readonly_var(status_frame, "Active Lateral Mode", "Autopilot.ActiveLateralMode", row, 0, lambda x: str(x))
        self.add_readonly_var(status_frame, "Active Vertical Mode", "Autopilot.ActiveVerticalMode", row, 2, lambda x: str(x))
        self.add_readonly_var(status_frame, "Speed Managed", "Autopilot.SpeedManaged", row, 4, lambda x: "YES" if x > 0.5 else "NO")
        row += 1
        self.add_readonly_var(status_frame, "Armed Lateral Mode", "Autopilot.ArmedLateralMode", row, 0, lambda x: str(x))
        self.add_readonly_var(status_frame, "Armed Vertical Mode", "Autopilot.ArmedVerticalMode", row, 2, lambda x: str(x))
        self.add_readonly_var(status_frame, "Armed Approach Mode", "Autopilot.ArmedApproachMode", row, 4, lambda x: str(x))
        
        # AP Controls frame
        controls_frame = ttk.LabelFrame(frame, text="Autopilot Controls", padding="5")
        controls_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(controls_frame, "Selected Airspeed", "Autopilot.SelectedAirspeed", 0, 20, 150, 50)
        self.add_control_var(controls_frame, "Selected Heading", "Autopilot.SelectedHeading", 1, 0, 360, 90)
        self.add_control_var(controls_frame, "Selected Altitude", "Autopilot.SelectedAltitude", 2, 0, 15000, 1000)
        self.add_control_var(controls_frame, "Selected VS", "Autopilot.SelectedVerticalSpeed", 3, -20, 20, 0)
        self.add_control_var(controls_frame, "Selected Speed", "Autopilot.SelectedSpeed", 4, 20, 150, 50)
        
        # AutoThrottle frame (NEW)
        at_frame = ttk.LabelFrame(frame, text="AutoThrottle System", padding="5")
        at_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(at_frame, "AT Engaged", "Autopilot.ThrottleEngaged", row, 0, lambda x: "ON" if x > 0.5 else "OFF")
        self.add_readonly_var(at_frame, "AT Type", "AutoThrottle.Type", row, 2, lambda x: str(x))
        self.add_readonly_var(at_frame, "AT Command", "Autopilot.ThrottleCommand", row, 4, lambda x: f"{x:.3f}")
        row += 1
        self.add_readonly_var(at_frame, "Active AT Mode", "Autopilot.ActiveAutoThrottleMode", row, 0, lambda x: str(x))
        self.add_readonly_var(at_frame, "Target Airspeed", "Autopilot.TargetAirspeed", row, 2, lambda x: f"{x:.1f} m/s")
        
        # AP Command Outputs (NEW)
        output_frame = ttk.LabelFrame(frame, text="Autopilot Outputs", padding="5")
        output_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(output_frame, "Aileron Command", "Autopilot.Aileron", row, 0, lambda x: f"{x:.3f}")
        self.add_readonly_var(output_frame, "Elevator Command", "Autopilot.Elevator", row, 2, lambda x: f"{x:.3f}")
        self.add_readonly_var(output_frame, "Altitude Scale", "Autopilot.SelectedAltitudeScale", row, 4, lambda x: f"{x:.0f}")
        
        # Helicopter AP (NEW)
        heli_frame = ttk.LabelFrame(frame, text="Helicopter Autopilot", padding="5")
        heli_frame.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(heli_frame, "Active Collective Mode", "Autopilot.ActiveCollectiveMode", row, 0, lambda x: str(x))
        self.add_readonly_var(heli_frame, "Armed Collective Mode", "Autopilot.ArmedCollectiveMode", row, 2, lambda x: str(x))
        
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
        
    def create_configuration_tab(self):
        """Create Configuration tab for aircraft configuration settings"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Configuration")
        
        # Configuration frame
        config_frame = ttk.LabelFrame(frame, text="Aircraft Configuration", padding="5")
        config_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(config_frame, "Takeoff Flaps", "Configuration.SelectedTakeOffFlaps", 0, 0.0, 1.0, 0.25)
        self.add_control_var(config_frame, "Landing Flaps", "Configuration.SelectedLandingFlaps", 1, 0.0, 1.0, 0.75)
        
    def create_fms_tab(self):
        """Create Flight Management System tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="FMS")
        
        # FMS frame
        fms_frame = ttk.LabelFrame(frame, text="Flight Management System", padding="5")
        fms_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(fms_frame, "Flight Number", "FlightManagementSystem.FlightNumber", 0, 0, 9999, 100)
        
    def create_flight_director_tab(self):
        """Create Flight Director tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Flight Director")
        
        # Flight Director frame
        fd_frame = ttk.LabelFrame(frame, text="Flight Director Commands", padding="5")
        fd_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(fd_frame, "FD Pitch", "FlightDirector.Pitch", row, 0, lambda x: f"{x:.2f}°")
        self.add_readonly_var(fd_frame, "FD Bank", "FlightDirector.Bank", row, 2, lambda x: f"{x:.2f}°")
        self.add_readonly_var(fd_frame, "FD Yaw", "FlightDirector.Yaw", row, 4, lambda x: f"{x:.2f}°")
        
    def create_copilot_tab(self):
        """Create Copilot tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Copilot")
        
        # Copilot frame
        copilot_frame = ttk.LabelFrame(frame, text="Copilot Settings", padding="5")
        copilot_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(copilot_frame, "Copilot Heading", "Copilot.Heading", row, 0, lambda x: f"{x:.1f}°")
        self.add_readonly_var(copilot_frame, "Copilot Altitude", "Copilot.Altitude", row, 2, lambda x: f"{x:.0f} m")
        self.add_readonly_var(copilot_frame, "Copilot Airspeed", "Copilot.Airspeed", row, 4, lambda x: f"{x:.1f} m/s")
        row += 1
        self.add_readonly_var(copilot_frame, "Copilot VS", "Copilot.VerticalSpeed", row, 0, lambda x: f"{x:.1f} m/s")
        self.add_readonly_var(copilot_frame, "Copilot Aileron", "Copilot.Aileron", row, 2, lambda x: f"{x:.3f}")
        self.add_readonly_var(copilot_frame, "Copilot Elevator", "Copilot.Elevator", row, 4, lambda x: f"{x:.3f}")
        row += 1
        self.add_readonly_var(copilot_frame, "Copilot Throttle", "Copilot.Throttle", row, 0, lambda x: f"{x:.3f}")
        self.add_readonly_var(copilot_frame, "Auto Rudder", "Copilot.AutoRudder", row, 2, lambda x: "ON" if x > 0.5 else "OFF")
        
    def create_pressurization_tab(self):
        """Create Pressurization tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Pressurization")
        
        # Pressurization frame
        press_frame = ttk.LabelFrame(frame, text="Cabin Pressurization", padding="5")
        press_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(press_frame, "Landing Elevation", "Pressurization.LandingElevation", 0, 0, 5000, 0)
        self.add_control_var(press_frame, "Landing Elevation Manual", "Pressurization.LandingElevationManual", 1, 0, 5000, 0)
        
    def create_view_controls_tab(self):
        """Create View Controls tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="View Controls")
        
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
        
        # View Status frame
        status_frame = ttk.LabelFrame(scrollable_frame, text="View Status", padding="5")
        status_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(status_frame, "Display Name", "View.DisplayName", row, 0, lambda x: str(x))
        self.add_readonly_var(status_frame, "Category", "View.Category", row, 2, lambda x: f"{x:.0f}")
        self.add_readonly_var(status_frame, "Mode", "View.Mode", row, 4, lambda x: f"{x:.0f}")
        
        # View Controls frame
        controls_frame = ttk.LabelFrame(scrollable_frame, text="View Controls", padding="5")
        controls_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(controls_frame, "View Zoom", "View.Zoom", 0, 0.1, 5.0, 1.0)
        self.add_control_var(controls_frame, "Pan Horizontal", "View.Pan.Horizontal", 1, -180, 180, 0)
        self.add_control_var(controls_frame, "Pan Vertical", "View.Pan.Vertical", 2, -90, 90, 0)
        self.add_control_var(controls_frame, "Look Horizontal", "View.Look.Horizontal", 3, -1, 1, 0)
        self.add_control_var(controls_frame, "Look Vertical", "View.Look.Vertical", 4, -1, 1, 0)
        
        # View Offsets frame
        offset_frame = ttk.LabelFrame(scrollable_frame, text="View Offsets", padding="5")
        offset_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(offset_frame, "Offset X", "View.OffsetX", 0, -10, 10, 0)
        self.add_control_var(offset_frame, "Offset Y", "View.OffsetY", 1, -10, 10, 0)
        self.add_control_var(offset_frame, "Offset Z", "View.OffsetZ", 2, -10, 10, 0)
        self.add_control_var(offset_frame, "Field of View", "View.FieldOfView", 3, 10, 120, 75)
        
        # Pack canvas and scrollbar
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
    def create_simulation_tab(self):
        """Create Simulation Controls tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Simulation")
        
        # Simulation Controls frame
        sim_frame = ttk.LabelFrame(frame, text="Simulation Controls", padding="5")
        sim_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        # Control buttons frame
        buttons_frame = ttk.Frame(sim_frame)
        buttons_frame.grid(row=0, column=0, columnspan=4, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Button(buttons_frame, text="Pause", command=lambda: self.send_command("Simulation.Pause", 1.0)).grid(row=0, column=0, padx=5)
        ttk.Button(buttons_frame, text="Resume", command=lambda: self.send_command("Simulation.Pause", 0.0)).grid(row=0, column=1, padx=5)
        ttk.Button(buttons_frame, text="Lift Up", command=lambda: self.send_command("Simulation.LiftUp", 1.0)).grid(row=0, column=2, padx=5)
        ttk.Button(buttons_frame, text="Sound Toggle", command=lambda: self.send_command("Simulation.Sound", 1.0)).grid(row=0, column=3, padx=5)
        
        # Time and Environment frame
        time_frame = ttk.LabelFrame(frame, text="Time & Environment", padding="5")
        time_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        self.add_control_var(time_frame, "Simulation Time", "Simulation.Time", 0, 0, 86400, 43200)
        self.add_control_var(time_frame, "Visibility", "Simulation.Visibility", 1, 0, 50000, 10000)
        self.add_control_var(time_frame, "Use Mouse Control", "Simulation.UseMouseControl", 2, 0, 1, 1)
        
    def create_systems_tab(self):
        """Create Systems tab with additional systems"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="Systems")
        
        # Additional Systems frame
        systems_frame = ttk.LabelFrame(frame, text="Additional Systems", padding="5")
        systems_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        row = 0
        self.add_readonly_var(systems_frame, "Throttle Limit", "Aircraft.ThrottleLimit", row, 0, lambda x: f"{x:.2f}")
        self.add_readonly_var(systems_frame, "Reverse Thrust", "Aircraft.Reverse", row, 2, lambda x: "ON" if x > 0.5 else "OFF")
        row += 1
        
        # Note frame
        note_frame = ttk.LabelFrame(frame, text="Information", padding="5")
        note_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        ttk.Label(note_frame, text="All 339 SDK variables are now available across all tabs", 
                 font=('Arial', 12)).pack(pady=20)
        
    def create_all_variables_tab(self):
        """Create tab showing all 339 variables in 6 columns"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="All Variables (339)")
        
        # Info frame
        info_frame = ttk.Frame(frame)
        info_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=5, pady=5)
        
        ttk.Label(info_frame, text="All 339 Variables Array - Real-time Values (Complete SDK)").grid(row=0, column=0, padx=5)
        self.var_count_label = ttk.Label(info_frame, text="Variables: 0/339", foreground="blue")
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
        
        # Populate initial tree structure (57 rows x 6 columns = 342 slots, showing 339)
        self.tree_items = []
        for row in range(57):
            item = self.variables_tree.insert('', 'end', values=['---'] * 12)
            self.tree_items.append(item)
        
    def connect(self):
        """Connect to Aerofly shared memory"""
        try:
            self.shared_memory = mmap.mmap(-1, 3384, "AeroflyBridgeData")
            
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
        
        # COMPLETE Variable offsets for ALL 339 Variables (Updated for 3384 bytes)
        offsets = {
            # === HEADER (16 bytes) ===
            'timestamp_us': 0,
            'data_valid': 8,
            'update_counter': 12,
            
            # === AIRCRAFT BASIC (64 bytes starting at 16) ===
            'Aircraft.Latitude': 16,                    
            'Aircraft.Longitude': 24,                   
            'Aircraft.Altitude': 32,                    
            'Aircraft.Pitch': 40,                       
            'Aircraft.Bank': 48,                        
            'Aircraft.TrueHeading': 56,                 
            'Aircraft.MagneticHeading': 64,             
            'Aircraft.IndicatedAirspeed': 72,           
            
            # === AIRCRAFT PHYSICS (96 bytes starting at 80) ===
            'Aircraft.GroundSpeed': 80,                 
            'Aircraft.VerticalSpeed': 88,               
            'Aircraft.AngleOfAttack': 96,               
            'Aircraft.AngleOfAttackLimit': 104,         
            'Aircraft.MachNumber': 112,                 
            'Aircraft.RateOfTurn': 120,                 
            'Aircraft.Position': 128,                   # tm_vector3d position (24 bytes)
            'Aircraft.Velocity': 152,                   # tm_vector3d velocity (24 bytes)
            'Aircraft.Acceleration': 176,               # tm_vector3d acceleration (24 bytes)
            'Aircraft.AngularVelocity': 200,            # tm_vector3d angular_velocity (24 bytes)
            'Aircraft.Wind': 224,                       # tm_vector3d wind (24 bytes)
            'Aircraft.Gravity': 248,                    # tm_vector3d gravity (24 bytes)
            
            # === ALL VARIABLES ARRAY (2712 bytes starting at 672) - Complete 339 Variables ===
            'all_variables_base': 672,
            
            # === AIRCRAFT VARIABLES (Indices 0-94) ===
            'Aircraft.UniversalTime': 672 + (0 * 8),
            'Aircraft.IndicatedAirspeedTrend': 672 + (1 * 8),
            'Aircraft.Height': 672 + (2 * 8),
            'Aircraft.Name': 672 + (3 * 8),
            'Aircraft.RadarAltitude': 672 + (4 * 8),
            'Aircraft.Power': 672 + (5 * 8),
            'Aircraft.NormalizedPower': 672 + (6 * 8),
            'Aircraft.YawDamperEnabled': 672 + (7 * 8),
            'Aircraft.AutoPitchTrim': 672 + (8 * 8),
            'Aircraft.RudderPedalsDisconnected': 672 + (9 * 8),
            'Aircraft.PitchTrim': 672 + (10 * 8),
            'Aircraft.RudderTrim': 672 + (11 * 8),
            'Aircraft.Trim': 672 + (12 * 8),
            'Aircraft.PitchTrimScaling': 672 + (13 * 8),
            'Aircraft.PitchTrimOffset': 672 + (14 * 8),
            'Aircraft.NearestAirportIdentifier': 672 + (15 * 8),
            'Aircraft.NearestAirportName': 672 + (16 * 8),
            'Aircraft.BestAirportIdentifier': 672 + (17 * 8),
            'Aircraft.BestAirportName': 672 + (18 * 8),
            'Aircraft.BestRunwayIdentifier': 672 + (19 * 8),
            'Aircraft.BestRunwayElevation': 672 + (20 * 8),
            'Aircraft.Category.Jet': 672 + (21 * 8),
            'Aircraft.Category.Glider': 672 + (22 * 8),
            'Aircraft.OnRunway': 672 + (23 * 8),
            'Aircraft.Crashed': 672 + (24 * 8),
            'Aircraft.ParkingBrake': 672 + (25 * 8),
            'Aircraft.EngineMaster1': 672 + (26 * 8),
            'Aircraft.EngineMaster2': 672 + (27 * 8),
            'Aircraft.EngineMaster3': 672 + (28 * 8),
            'Aircraft.Starter1': 672 + (29 * 8),
            'Aircraft.Ignition1': 672 + (30 * 8),
            'Aircraft.APUAvailable': 672 + (31 * 8),
            'Aircraft.ThrottleLimit': 672 + (32 * 8),
            'Aircraft.Reverse': 672 + (33 * 8),
            
            # === PERFORMANCE SPEEDS (Indices 95-104) ===
            'Performance.Speed.VS0': 672 + (95 * 8),
            'Performance.Speed.VS1': 672 + (96 * 8),
            'Performance.Speed.VFE': 672 + (97 * 8),
            'Performance.Speed.VNO': 672 + (98 * 8),
            'Performance.Speed.VNE': 672 + (99 * 8),
            'Performance.Speed.VAPP': 672 + (100 * 8),
            'Performance.Speed.Minimum': 672 + (101 * 8),
            'Performance.Speed.Maximum': 672 + (102 * 8),
            
            # === CONFIGURATION (Indices 105-106) ===
            'Configuration.SelectedTakeOffFlaps': 672 + (105 * 8),
            'Configuration.SelectedLandingFlaps': 672 + (106 * 8),
            
            # === FLIGHT MANAGEMENT SYSTEM (Index 107) ===
            'FlightManagementSystem.FlightNumber': 672 + (107 * 8),
            
            # === NAVIGATION (Indices 108-141) ===
            'Navigation.NAV2Frequency': 672 + (108 * 8),
            'Navigation.NAV2StandbyFrequency': 672 + (109 * 8),
            'Navigation.ILS1Frequency': 672 + (110 * 8),
            'Navigation.ILS1StandbyFrequency': 672 + (111 * 8),
            'Navigation.ILS1Course': 672 + (112 * 8),
            'Navigation.ILS2Frequency': 672 + (113 * 8),
            'Navigation.ILS2StandbyFrequency': 672 + (114 * 8),
            'Navigation.ILS2Course': 672 + (115 * 8),
            'Navigation.DME1Distance': 672 + (116 * 8),
            'Navigation.DME1Time': 672 + (117 * 8),
            'Navigation.DME1Speed': 672 + (118 * 8),
            'Navigation.DME2Distance': 672 + (119 * 8),
            'Navigation.DME2Time': 672 + (120 * 8),
            'Navigation.DME2Speed': 672 + (121 * 8),
            'Navigation.ADF1Frequency': 672 + (122 * 8),
            'Navigation.ADF1StandbyFrequency': 672 + (123 * 8),
            'Navigation.ADF2Frequency': 672 + (124 * 8),
            'Navigation.ADF2StandbyFrequency': 672 + (125 * 8),
            'Communication.TransponderCode': 672 + (126 * 8),
            
            # === COMMUNICATION (Indices 142-152) ===
            # Communication frequencies mapped in fixed structure section
            
            # === AUTOPILOT (Indices 153-180) ===
            'Autopilot.Type': 672 + (153 * 8),
            'Autopilot.UseMachNumber': 672 + (154 * 8),
            'Autopilot.SpeedManaged': 672 + (155 * 8),
            'Autopilot.ArmedLateralMode': 672 + (156 * 8),
            'Autopilot.ArmedVerticalMode': 672 + (157 * 8),
            'Autopilot.ArmedApproachMode': 672 + (158 * 8),
            'Autopilot.SelectedSpeed': 672 + (159 * 8),
            'AutoThrottle.Type': 672 + (160 * 8),
            'Autopilot.ThrottleCommand': 672 + (161 * 8),
            'Autopilot.ActiveAutoThrottleMode': 672 + (162 * 8),
            'Autopilot.TargetAirspeed': 672 + (163 * 8),
            'Autopilot.Aileron': 672 + (164 * 8),
            'Autopilot.Elevator': 672 + (165 * 8),
            'Autopilot.SelectedAltitudeScale': 672 + (166 * 8),
            'Autopilot.ActiveCollectiveMode': 672 + (167 * 8),
            'Autopilot.ArmedCollectiveMode': 672 + (168 * 8),
            
            # === FLIGHT DIRECTOR (Indices 181-183) ===
            'FlightDirector.Pitch': 672 + (181 * 8),
            'FlightDirector.Bank': 672 + (182 * 8),
            'FlightDirector.Yaw': 672 + (183 * 8),
            
            # === COPILOT (Indices 184-193) ===
            'Copilot.Heading': 672 + (184 * 8),
            'Copilot.Altitude': 672 + (185 * 8),
            'Copilot.Airspeed': 672 + (186 * 8),
            'Copilot.VerticalSpeed': 672 + (187 * 8),
            'Copilot.Aileron': 672 + (188 * 8),
            'Copilot.Elevator': 672 + (189 * 8),
            'Copilot.Throttle': 672 + (190 * 8),
            'Copilot.AutoRudder': 672 + (191 * 8),
            
            # === CONTROLS (Indices 192-260) - CORRECTED ===
            'Controls.Throttle': 672 + (192 * 8),       # CONTROLS_THROTTLE = 192
            'Controls.Throttle1': 672 + (193 * 8),      # CONTROLS_THROTTLE_1 = 193
            'Controls.Throttle2': 672 + (194 * 8),      # CONTROLS_THROTTLE_2 = 194
            'Controls.Throttle3': 672 + (195 * 8),      # CONTROLS_THROTTLE_3 = 195
            'Controls.Throttle4': 672 + (196 * 8),      # CONTROLS_THROTTLE_4 = 196
            'Controls.Pitch.Input': 672 + (201 * 8),    # CONTROLS_PITCH_INPUT = 201
            'Controls.Roll.Input': 672 + (203 * 8),     # CONTROLS_ROLL_INPUT = 203  
            'Controls.Yaw.Input': 672 + (205 * 8),      # CONTROLS_YAW_INPUT = 205
            'Controls.Flaps': 672 + (207 * 8),          # CONTROLS_FLAPS = 207
            'Controls.Gear': 672 + (209 * 8),           # CONTROLS_GEAR = 209
            'Controls.LeftBrake': 672 + (211 * 8),      # CONTROLS_WHEEL_BRAKE_LEFT = 211
            'Controls.RightBrake': 672 + (212 * 8),     # CONTROLS_WHEEL_BRAKE_RIGHT = 212
            'Controls.AirBrake': 672 + (215 * 8),       # CONTROLS_AIR_BRAKE = 215
            'Controls.Propeller': 672 + (219 * 8),      # CONTROLS_PROPELLER_SPEED_1 = 219 (usar como base)
            'Controls.Mixture': 672 + (223 * 8),        # CONTROLS_MIXTURE = 223
            'Controls.Mixture1': 672 + (224 * 8),       # CONTROLS_MIXTURE_1 = 224
            'Controls.Mixture2': 672 + (225 * 8),       # CONTROLS_MIXTURE_2 = 225
            'Controls.Mixture3': 672 + (226 * 8),       # CONTROLS_MIXTURE_3 = 226
            'Controls.Mixture4': 672 + (227 * 8),       # CONTROLS_MIXTURE_4 = 227
            'Controls.PitchTrim': 672 + (243 * 8),      # CONTROLS_AILERON_TRIM = 243 (usar como Pitch)
            'Controls.RudderTrim': 672 + (244 * 8),     # CONTROLS_RUDDER_TRIM = 244
            
            # === PRESSURIZATION (Indices 263-264) ===
            'Pressurization.LandingElevation': 672 + (263 * 8),
            'Pressurization.LandingElevationManual': 672 + (264 * 8),
            
            # === VIEW CONTROLS (Indices 265-275) ===
            'View.DisplayName': 672 + (265 * 8),
            'View.Category': 672 + (266 * 8),
            'View.Mode': 672 + (267 * 8),
            'View.Zoom': 672 + (268 * 8),
            'View.Pan.Horizontal': 672 + (269 * 8),
            'View.Pan.Vertical': 672 + (270 * 8),
            'View.Look.Horizontal': 672 + (271 * 8),
            'View.Look.Vertical': 672 + (272 * 8),
            'View.OffsetX': 672 + (273 * 8),
            'View.OffsetY': 672 + (274 * 8),
            'View.OffsetZ': 672 + (275 * 8),
            'View.FieldOfView': 672 + (276 * 8),
            
            # === SIMULATION CONTROLS (Indices 277-280) ===
            'Simulation.Pause': 672 + (277 * 8),
            'Simulation.LiftUp': 672 + (278 * 8),
            'Simulation.Sound': 672 + (279 * 8),
            'Simulation.Time': 672 + (280 * 8),
            'Simulation.Visibility': 672 + (281 * 8),
            'Simulation.UseMouseControl': 672 + (282 * 8),
            
            # === WARNINGS (Indices 283-295) ===
            'Warnings.MasterWarning': 672 + (283 * 8),
            'Warnings.MasterCaution': 672 + (284 * 8),
            'Warnings.EngineFire': 672 + (285 * 8),
            'Warnings.LowOilPressure': 672 + (286 * 8),
            'Warnings.LowFuelPressure': 672 + (287 * 8),
            'Warnings.AltitudeAlert': 672 + (288 * 8),
            'Warnings.WarningActive': 672 + (289 * 8),
            'Warnings.WarningMute': 672 + (290 * 8),
            
            # === FIXED STRUCTURE VARIABLES (Keep original offsets) ===
            'Aircraft.OnGround': 272,                   
            'Aircraft.Gear': 296,                       
            'Aircraft.Flaps': 304,                      
            'Aircraft.Slats': 312,                      
            'Aircraft.Throttle': 320,                   
            'Aircraft.AirBrake': 328,                   
            'Aircraft.EngineThrottle1': 336,            
            'Aircraft.EngineThrottle2': 344,            
            'Aircraft.EngineThrottle3': 352,            
            'Aircraft.EngineThrottle4': 360,            
            'Aircraft.EngineRotationSpeed1': 368,       
            'Aircraft.EngineRotationSpeed2': 376,       
            'Aircraft.EngineRotationSpeed3': 384,       
            'Aircraft.EngineRotationSpeed4': 392,       
            'Aircraft.EngineRunning1': 400,             
            'Aircraft.EngineRunning2': 408,             
            'Aircraft.EngineRunning3': 416,             
            'Aircraft.EngineRunning4': 424,             
            'Navigation.NAV1Frequency': 488,            
            'Navigation.NAV1StandbyFrequency': 496,     
            'Navigation.SelectedCourse1': 504,          
            'Navigation.SelectedCourse2': 528,          
            'Autopilot.Engaged': 536,                   
            'Autopilot.SelectedAirspeed': 544,          
            'Autopilot.SelectedHeading': 552,           
            'Autopilot.SelectedAltitude': 560,          
            'Autopilot.SelectedVerticalSpeed': 568,     
            'Autopilot.ThrottleEngaged': 576,           
            'Autopilot.ActiveLateralMode': 584,         
            'Autopilot.ActiveVerticalMode': 600,
            
            # === COM FREQUENCIES (Fixed structure offsets) ===        
            'Communication.COM1Frequency': 456,         
            'Communication.COM1StandbyFrequency': 464,  
            'Communication.COM2Frequency': 472,         
            'Communication.COM2StandbyFrequency': 480,  
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
            
            # Update tree items (57 rows x 6 columns = 342 slots, showing 339 variables)
            for row in range(57):
                values = []
                
                for col in range(6):
                    var_index = row * 6 + col
                    if var_index < 339:
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
            self.var_count_label.config(text=f"Active Variables: {valid_count}/339")
            
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