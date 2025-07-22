# stress_tester_individual.py - Probar límites SIN modificar el DLL
import tkinter as tk
from tkinter import ttk
import socket
import json
import threading
import time
import random

class IndividualStressTester:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("🧪 Aerofly Stress Tester - Individual Commands")
        self.root.geometry("1000x700")
        self.root.configure(bg='#1a1a1a')
        
        # Test results
        self.test_results = []
        self.monitoring = False
        self.last_data = None
        self.testing = False
        
        self.setup_gui()
        self.start_monitoring()
        
    def setup_gui(self):
        """Setup GUI for stress testing"""
        
        # Header
        header = tk.Frame(self.root, bg='#1a1a1a')
        header.pack(fill='x', padx=10, pady=5)
        
        title = tk.Label(header, text="🧪 AEROFLY STRESS TESTER (Current DLL)", 
                        font=('Arial', 16, 'bold'), fg='cyan', bg='#1a1a1a')
        title.pack()
        
        subtitle = tk.Label(header, text="Testing individual commands rapidly - NO DLL modification needed", 
                           font=('Arial', 10), fg='gray', bg='#1a1a1a')
        subtitle.pack()
        
        # Status
        self.status_label = tk.Label(header, text="🔄 Initializing...", 
                                   font=('Arial', 12), fg='yellow', bg='#1a1a1a')
        self.status_label.pack(pady=5)
        
        # Control panel
        control_frame = tk.LabelFrame(self.root, text="🎮 Test Controls", 
                                    font=('Arial', 12, 'bold'), fg='white', bg='#1a1a1a')
        control_frame.pack(fill='x', padx=10, pady=5)
        
        # Test buttons
        test_buttons_frame = tk.Frame(control_frame, bg='#1a1a1a')
        test_buttons_frame.pack(pady=10)
        
        # Different stress tests
        tests = [
            ("🔥 2-Command Burst", self.test_2_commands, '#4444ff'),
            ("⚡ 5-Command Burst", self.test_5_commands, '#ff4444'),
            ("🚀 10-Command Burst", self.test_10_commands, '#ff8800'),
            ("💥 20-Command Flood", self.test_20_commands, '#ff0000'),
            ("🌪️ Rapid Fire (5x)", self.test_rapid_fire, '#ff00ff'),
            ("🎯 Precision Test", self.test_precision, '#00ff88'),
            ("⏱️ Timing Test", self.test_timing, '#ffff00'),
            ("🛑 Stop & Reset", self.stop_and_reset, '#666666')
        ]
        
        for i, (text, command, color) in enumerate(tests):
            btn = tk.Button(test_buttons_frame, text=text, 
                          font=('Arial', 10, 'bold'),
                          bg=color, fg='white', 
                          relief='raised', bd=3,
                          command=command)
            btn.grid(row=i//4, column=i%4, padx=5, pady=5, sticky='ew')
        
        # Configure grid
        for i in range(4):
            test_buttons_frame.grid_columnconfigure(i, weight=1)
        
        # Configuration frame
        config_frame = tk.LabelFrame(self.root, text="⚙️ Test Configuration", 
                                   font=('Arial', 12, 'bold'), fg='white', bg='#1a1a1a')
        config_frame.pack(fill='x', padx=10, pady=5)
        
        config_inner = tk.Frame(config_frame, bg='#1a1a1a')
        config_inner.pack(pady=5)
        
        tk.Label(config_inner, text="Delay between commands (ms):", 
                font=('Arial', 10), fg='white', bg='#1a1a1a').pack(side='left')
        
        self.delay_var = tk.StringVar(value="50")
        delay_spinbox = tk.Spinbox(config_inner, from_=0, to=1000, increment=10, 
                                 textvariable=self.delay_var, width=8)
        delay_spinbox.pack(side='left', padx=5)
        
        tk.Label(config_inner, text="ms", 
                font=('Arial', 10), fg='white', bg='#1a1a1a').pack(side='left')
        
        # Live data monitor
        data_frame = tk.LabelFrame(self.root, text="📊 Live Aircraft Data", 
                                 font=('Arial', 12, 'bold'), fg='white', bg='#1a1a1a')
        data_frame.pack(fill='x', padx=10, pady=5)
        
        self.data_display = tk.Label(data_frame, text="Waiting for data...", 
                                   font=('Consolas', 10), fg='lime', bg='#000000')
        self.data_display.pack(fill='x', padx=5, pady=5)
        
        # Results area
        results_frame = tk.LabelFrame(self.root, text="📋 Test Results & Performance", 
                                    font=('Arial', 12, 'bold'), fg='white', bg='#1a1a1a')
        results_frame.pack(fill='both', expand=True, padx=10, pady=5)
        
        # Results text with scrollbar
        text_frame = tk.Frame(results_frame, bg='#1a1a1a')
        text_frame.pack(fill='both', expand=True, padx=5, pady=5)
        
        self.results_text = tk.Text(text_frame, font=('Consolas', 9), 
                                  bg='#000000', fg='white')
        scrollbar = tk.Scrollbar(text_frame, orient='vertical', command=self.results_text.yview)
        self.results_text.configure(yscrollcommand=scrollbar.set)
        
        self.results_text.pack(side='left', fill='both', expand=True)
        scrollbar.pack(side='right', fill='y')
        
        self.log("🚀 Individual command stress tester initialized")
        self.log("📋 Tests send individual commands rapidly to test Aerofly's limits")
        self.log("⚙️ Adjust delay between commands in configuration panel")
        
    def start_monitoring(self):
        """Monitor live data from Aerofly"""
        def monitor():
            self.monitoring = True
            while self.monitoring:
                try:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(2.0)
                    sock.connect(('localhost', 12345))
                    
                    self.status_label.config(text="🟢 Connected - Monitoring...", fg='lime')
                    
                    while self.monitoring:
                        data = sock.recv(2048).decode('utf-8', errors='replace')
                        if not data:
                            break
                        
                        # Parse latest JSON
                        lines = data.split('\n')
                        for line in lines:
                            line = line.strip()
                            if line.startswith('{') and line.endswith('}'):
                                try:
                                    parsed = json.loads(line)
                                    self.last_data = parsed
                                    self.update_data_display()
                                    break
                                except:
                                    pass
                    
                    sock.close()
                    
                except Exception as e:
                    self.status_label.config(text=f"🔴 Connection error", fg='red')
                    time.sleep(3)
        
        threading.Thread(target=monitor, daemon=True).start()
    
    def update_data_display(self):
        """Update live data display"""
        if not self.last_data:
            return
        
        try:
            aircraft = self.last_data.get('aircraft', {})
            controls = self.last_data.get('controls', {})
            
            info = (
                f"ALT: {aircraft.get('altitude', 0):.1f}m | "
                f"SPD: {aircraft.get('airspeed', 0):.1f}m/s | "
                f"THR: {controls.get('throttle', 0)*100:.0f}% | "
                f"PITCH: {controls.get('pitch_input', 0):.3f} | "
                f"ROLL: {controls.get('roll_input', 0):.3f} | "
                f"GEAR: {'DN' if controls.get('gear', 0) > 0.5 else 'UP'} | "
                f"FLAPS: {controls.get('flaps', 0)*100:.0f}% | "
                f"#{self.last_data.get('update_counter', 'N/A')}"
            )
            
            self.data_display.config(text=info)
            
        except Exception as e:
            self.log(f"❌ Data display error: {e}")
    
    def send_individual_command(self, variable, value):
        """Send single command and measure response time"""
        try:
            start_time = time.time()
            
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(3.0)
            sock.connect(('localhost', 12346))
            
            command = {
                "variable": variable,
                "value": value
            }
            
            cmd_data = json.dumps(command).encode('utf-8')
            sock.send(cmd_data)
            sock.close()
            
            elapsed = (time.time() - start_time) * 1000
            return True, elapsed
            
        except Exception as e:
            return False, 0
    
    def send_command_burst(self, commands, test_name):
        """Send multiple individual commands rapidly"""
        delay_ms = int(self.delay_var.get())
        delay_sec = delay_ms / 1000.0
        
        start_time = time.time()
        successful = 0
        failed = 0
        total_response_time = 0
        
        self.log(f"\n🚀 === {test_name.upper()} ===")
        self.log(f"📊 Sending {len(commands)} commands with {delay_ms}ms delay")
        
        for i, (variable, value) in enumerate(commands):
            success, response_time = self.send_individual_command(variable, value)
            
            if success:
                successful += 1
                total_response_time += response_time
                status = "✅"
            else:
                failed += 1
                status = "❌"
            
            self.log(f"  {status} {i+1:2d}/{len(commands)}: {variable} = {value} ({response_time:.1f}ms)")
            
            # Delay between commands (except last one)
            if i < len(commands) - 1:
                time.sleep(delay_sec)
        
        # Calculate statistics
        total_time = (time.time() - start_time) * 1000
        avg_response = total_response_time / max(successful, 1)
        success_rate = successful / len(commands) * 100
        
        self.log(f"📈 RESULTS:")
        self.log(f"   ✅ Successful: {successful}/{len(commands)} ({success_rate:.1f}%)")
        self.log(f"   ❌ Failed: {failed}")
        self.log(f"   ⏱️ Total time: {total_time:.1f}ms")
        self.log(f"   📊 Avg response: {avg_response:.1f}ms")
        self.log(f"   🚀 Commands/sec: {len(commands)/(total_time/1000):.1f}")
        
        return success_rate > 90, avg_response
    
    def test_2_commands(self):
        """Test 2 commands in quick succession"""
        commands = [
            ("Controls.Throttle", 0.5),
            ("Controls.Gear", 1.0)
        ]
        
        success, avg_time = self.send_command_burst(commands, "2-Command Burst")
        
        if success and avg_time < 200:
            self.log("   🎯 RESULT: ✅ EXCELLENT - Fast and reliable")
        elif success:
            self.log("   🎯 RESULT: ✅ GOOD - Reliable but slower")
        else:
            self.log("   🎯 RESULT: ⚠️ ISSUES - Some commands failed")
    
    def test_5_commands(self):
        """Test 5 commands in quick succession"""
        commands = [
            ("Controls.Throttle", 0.7),
            ("Controls.Gear", 0.0),
            ("Controls.Flaps", 0.5),
            ("Controls.Pitch.Input", 0.1),
            ("Controls.Roll.Input", -0.1)
        ]
        
        success, avg_time = self.send_command_burst(commands, "5-Command Burst")
        
        if success and avg_time < 300:
            self.log("   🎯 RESULT: ✅ EXCELLENT - Handles multiple commands well")
        elif success:
            self.log("   🎯 RESULT: ✅ GOOD - Multiple commands work")
        else:
            self.log("   🎯 RESULT: ⚠️ STRAIN - System showing stress")
    
    def test_10_commands(self):
        """Test 10 commands - higher load"""
        commands = [
            ("Controls.Throttle", 0.3),
            ("Controls.Gear", 1.0),
            ("Controls.Flaps", 1.0),
            ("Controls.Pitch.Input", 0.0),
            ("Controls.Roll.Input", 0.0),
            ("Controls.Yaw.Input", 0.0),
            ("Controls.WheelBrake.Left", 0.5),
            ("Controls.WheelBrake.Right", 0.5),
            ("Autopilot.SelectedAirspeed", 60),
            ("Communication.COM1Frequency", 122800000)
        ]
        
        success, avg_time = self.send_command_burst(commands, "10-Command Burst")
        
        if success and avg_time < 400:
            self.log("   🎯 RESULT: ✅ ROBUST - Handles high load well")
        elif success:
            self.log("   🎯 RESULT: ✅ STABLE - Can handle load with delays")
        else:
            self.log("   🎯 RESULT: ⚠️ OVERLOAD - Approaching limits")
    
    def test_20_commands(self):
        """Test 20 commands - stress test"""
        commands = []
        
        # Generate 20 varied commands
        base_commands = [
            ("Controls.Throttle", random.uniform(0.1, 0.9)),
            ("Controls.Pitch.Input", random.uniform(-0.2, 0.2)),
            ("Controls.Roll.Input", random.uniform(-0.2, 0.2)),
            ("Controls.Yaw.Input", random.uniform(-0.1, 0.1)),
            ("Controls.Flaps", random.choice([0.0, 0.25, 0.5, 0.75, 1.0])),
            ("Autopilot.SelectedAirspeed", random.uniform(40, 80)),
            ("Communication.COM1Frequency", random.choice([121500000, 122800000, 124200000])),
            ("Navigation.NAV1Frequency", random.choice([108000000, 110500000, 112000000]))
        ]
        
        # Repeat and randomize to get 20 commands
        for i in range(20):
            cmd = base_commands[i % len(base_commands)]
            if "random" in str(cmd[1]):
                # Re-randomize for variety
                if "Throttle" in cmd[0]:
                    commands.append((cmd[0], random.uniform(0.1, 0.9)))
                elif "Input" in cmd[0]:
                    commands.append((cmd[0], random.uniform(-0.2, 0.2)))
                else:
                    commands.append(cmd)
            else:
                commands.append(cmd)
        
        success, avg_time = self.send_command_burst(commands, "20-Command FLOOD")
        
        if success and avg_time < 500:
            self.log("   🎯 RESULT: 🚀 AMAZING - System handles flood perfectly")
        elif success:
            self.log("   🎯 RESULT: ✅ STRONG - Can handle heavy load")
        else:
            self.log("   🎯 RESULT: 💥 SATURATED - System overwhelmed")
    
    def test_rapid_fire(self):
        """Test rapid fire - 5 bursts of commands"""
        self.log(f"\n🌪️ === RAPID FIRE TEST ===")
        self.log("📊 Sending 5 rapid bursts of 3 commands each")
        
        def rapid_fire():
            for burst in range(5):
                commands = [
                    ("Controls.Pitch.Input", random.uniform(-0.1, 0.1)),
                    ("Controls.Roll.Input", random.uniform(-0.1, 0.1)),
                    ("Controls.Throttle", 0.5 + 0.2 * random.random())
                ]
                
                self.log(f"\n🔥 Burst {burst + 1}/5:")
                success, avg_time = self.send_command_burst(commands, f"Burst-{burst+1}")
                
                time.sleep(0.5)  # 500ms between bursts
            
            self.log("\n🎯 RAPID FIRE COMPLETED")
        
        threading.Thread(target=rapid_fire, daemon=True).start()
    
    def test_precision(self):
        """Test precision - very specific values"""
        commands = [
            ("Controls.Throttle", 0.123456),
            ("Controls.Pitch.Input", 0.05),
            ("Controls.Roll.Input", -0.03),
            ("Autopilot.SelectedAirspeed", 67.89)
        ]
        
        success, avg_time = self.send_command_burst(commands, "Precision Test")
        
        # Wait for data to update and check precision
        time.sleep(2)
        
        if self.last_data:
            controls = self.last_data.get('controls', {})
            throttle_actual = controls.get('throttle', 0)
            pitch_actual = controls.get('pitch_input', 0)
            
            throttle_error = abs(throttle_actual - 0.123456)
            pitch_error = abs(pitch_actual - 0.05)
            
            self.log(f"🔍 PRECISION ANALYSIS:")
            self.log(f"   📏 Throttle: expected 0.123456, got {throttle_actual:.6f} (error: {throttle_error:.6f})")
            self.log(f"   📏 Pitch: expected 0.05000, got {pitch_actual:.6f} (error: {pitch_error:.6f})")
            
            if throttle_error < 0.001 and pitch_error < 0.001:
                self.log("   🎯 RESULT: ✅ HIGH PRECISION maintained")
            else:
                self.log("   🎯 RESULT: ⚠️ PRECISION LOSS detected")
    
    def test_timing(self):
        """Test timing consistency"""
        commands = [("Controls.Throttle", 0.5)] * 10  # Same command 10 times
        
        self.log(f"\n⏱️ === TIMING CONSISTENCY TEST ===")
        self.log("📊 Sending same command 10 times to measure timing variance")
        
        delays = []
        for i in range(10):
            success, response_time = self.send_individual_command("Controls.Throttle", 0.5 + i * 0.01)
            if success:
                delays.append(response_time)
                self.log(f"  ⏱️ Command {i+1:2d}: {response_time:.1f}ms")
            
            time.sleep(int(self.delay_var.get()) / 1000.0)
        
        if delays:
            avg_delay = sum(delays) / len(delays)
            min_delay = min(delays)
            max_delay = max(delays)
            variance = max_delay - min_delay
            
            self.log(f"📊 TIMING ANALYSIS:")
            self.log(f"   📈 Average: {avg_delay:.1f}ms")
            self.log(f"   ⚡ Fastest: {min_delay:.1f}ms")
            self.log(f"   🐌 Slowest: {max_delay:.1f}ms")
            self.log(f"   📊 Variance: {variance:.1f}ms")
            
            if variance < 50:
                self.log("   🎯 RESULT: ✅ CONSISTENT TIMING")
            elif variance < 100:
                self.log("   🎯 RESULT: ✅ ACCEPTABLE VARIANCE")
            else:
                self.log("   🎯 RESULT: ⚠️ HIGH VARIANCE - Unstable timing")
    
    def stop_and_reset(self):
        """Stop tests and reset aircraft"""
        self.log(f"\n🛑 === STOP & RESET ===")
        
        reset_commands = [
            ("Controls.Throttle", 0.0),
            ("Controls.Pitch.Input", 0.0),
            ("Controls.Roll.Input", 0.0),
            ("Controls.Yaw.Input", 0.0),
            ("Controls.WheelBrake.Left", 0.0),
            ("Controls.WheelBrake.Right", 0.0)
        ]
        
        self.send_command_burst(reset_commands, "RESET")
        self.log("🔄 Aircraft controls reset to neutral")
    
    def log(self, message):
        """Add message to results log"""
        timestamp = time.strftime("%H:%M:%S")
        self.results_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.results_text.see(tk.END)
        print(message)  # Also print to console
    
    def run(self):
        """Start the application"""
        self.root.mainloop()

if __name__ == "__main__":
    print("🧪 AEROFLY INDIVIDUAL COMMAND STRESS TESTER")
    print("Testing rapid individual commands with current DLL")
    print("No DLL modification required!")
    print("="*60)
    
    app = IndividualStressTester()
    app.run()