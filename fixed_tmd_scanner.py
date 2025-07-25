#!/usr/bin/env python3
"""
Fixed TMD Scanner
================

Corrected version that properly extracts control_message information.
Based on debug results showing the TMD structure works correctly.
"""

import os
import re
import csv
import json
from collections import defaultdict, Counter
from pathlib import Path
import argparse
from dataclasses import dataclass, asdict
from typing import List, Dict, Set, Optional

@dataclass
class ControlMessage:
    """Represents a complete control message from TMD"""
    variable_name: str          # Message field
    event_type: str            # OnStep, OnRotate, OnPush, etc.
    qualifier: str             # step, toggle, event, value
    value: float               # Numeric value
    aircraft: str              # Aircraft name
    control_name: str          # Control object name
    control_type: str          # knob, button, lever toggle, stick
    file_path: str             # Source file

class FixedTMDScanner:
    def __init__(self):
        self.control_messages: List[ControlMessage] = []
        self.variables_by_aircraft: Dict[str, Set[str]] = defaultdict(set)
        self.aircraft_count = 0
        self.tmd_files_found = 0
        
    def find_aerofly_path(self):
        """Find Aerofly FS4 installation directory"""
        # Use the exact path provided by user
        path = r"C:\Program Files (x86)\Steam\steamapps\common\Aerofly FS 4 Flight Simulator"
        aircraft_path = os.path.join(path, "aircraft")
        if os.path.exists(aircraft_path):
            print(f"✅ Found Aerofly FS4 at: {path}")
            return aircraft_path
        else:
            print(f"❌ Aircraft directory not found: {aircraft_path}")
            return None
    
    def extract_control_messages_from_tmd(self, file_path: str, aircraft_name: str) -> List[ControlMessage]:
        """Extract ALL control_message structures from a TMD file - SIMPLIFIED APPROACH"""
        messages = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
                content = file.read()
                
            print(f"      📄 File size: {len(content)} chars, scanning for control_messages...")
            
            # DIRECT APPROACH: Find all control_message blocks in the content
            # Pattern matches the structure we confirmed works
            message_pattern = r'<\[control_message\]\[([^\]]+)\]\[\](.*?)(?=<\[control_message\]|<\[float64\]\[Radius\]|<\[tmvector3d\]\[Dimensions\]|<\[control_|>\s*<\[control_|\Z)'
            
            matches = re.finditer(message_pattern, content, re.DOTALL)
            
            messages_found = 0
            for match in matches:
                event_type = match.group(1)
                message_content = match.group(2)
                
                # Extract variable name (Message)
                var_match = re.search(r'<\[string8\]\[Message\]\[([^\]]+)\]>', message_content)
                if not var_match:
                    continue
                variable_name = var_match.group(1)
                
                # Extract Qualifiers
                qual_match = re.search(r'<\[string8\]\[Qualifiers\]\[([^\]]*)\]>', message_content)
                qualifier = qual_match.group(1) if qual_match else ""
                
                # Extract Value
                val_match = re.search(r'<\[float64\]\[Value\]\[([^\]]+)\]>', message_content)
                if not val_match:
                    continue
                    
                try:
                    value = float(val_match.group(1))
                except ValueError:
                    continue
                
                # Try to find the control this message belongs to
                # Look backwards from message position to find the parent control
                control_name, control_type = self.find_parent_control(content, match.start())
                
                # Create ControlMessage object
                msg = ControlMessage(
                    variable_name=variable_name,
                    event_type=event_type,
                    qualifier=qualifier,
                    value=value,
                    aircraft=aircraft_name,
                    control_name=control_name,
                    control_type=control_type,
                    file_path=file_path
                )
                
                messages.append(msg)
                messages_found += 1
                
            print(f"      ✅ Extracted {messages_found} control messages")
                
        except Exception as e:
            print(f"      ⚠️  Error reading {file_path}: {e}")
            
        return messages
    
    def find_parent_control(self, content: str, message_pos: int) -> tuple[str, str]:
        """Find the parent control object for a control_message"""
        # Look backwards from message position to find the most recent control definition
        before_message = content[:message_pos]
        
        # Find the last control object before this message
        control_pattern = r'<\[control_([^\]]+)\]\[([^\]]*)\]\[\]'
        control_matches = list(re.finditer(control_pattern, before_message))
        
        if control_matches:
            last_control = control_matches[-1]
            control_type = last_control.group(1)
            control_name = last_control.group(2)
            
            # Skip 'message' controls and input/rotation controls, find the actual interactive control
            if control_type in ['message', 'input', 'rotation', 'translation', 'product']:
                # Look for the previous interactive control
                interactive_controls = ['cylinder', 'box', 'sphere']
                for match in reversed(control_matches[:-1]):
                    if match.group(1) in interactive_controls:
                        return match.group(2), match.group(1)
            
            return control_name, control_type
        
        return "unknown", "unknown"
    
    def extract_control_type_from_content(self, content: str, control_start: int) -> str:
        """Extract the Type field from control content"""
        # Look for Type field after control start
        control_section = content[control_start:control_start + 1000]  # Limited search
        type_match = re.search(r'<\[string8\]\[Type\]\[([^\]]+)\]>', control_section)
        if type_match:
            return type_match.group(1)
        return "unknown"
    
    def scan_aircraft_folder(self, aircraft_path: str):
        """Scan all aircraft folders for controls.tmd files"""
        print(f"🔍 Scanning aircraft directory: {aircraft_path}")
        
        if not os.path.exists(aircraft_path):
            print(f"❌ Aircraft directory not found: {aircraft_path}")
            return
            
        for aircraft_name in os.listdir(aircraft_path):
            aircraft_dir = os.path.join(aircraft_path, aircraft_name)
            
            if not os.path.isdir(aircraft_dir):
                continue
                
            print(f"📁 Scanning aircraft: {aircraft_name}")
            
            # Look for controls.tmd files in multiple locations
            tmd_locations = [
                os.path.join(aircraft_dir, "controls.tmd"),
                os.path.join(aircraft_dir, "base", "controls.tmd"),
                os.path.join(aircraft_dir, "cockpit", "controls.tmd"),
            ]
            
            aircraft_messages = []
            for tmd_path in tmd_locations:
                if os.path.exists(tmd_path):
                    print(f"    📄 Found TMD: {os.path.relpath(tmd_path, aircraft_path)}")
                    messages = self.extract_control_messages_from_tmd(tmd_path, aircraft_name)
                    aircraft_messages.extend(messages)
                    self.tmd_files_found += 1
                    
            if aircraft_messages:
                self.control_messages.extend(aircraft_messages)
                
                # Track variables by aircraft
                for msg in aircraft_messages:
                    self.variables_by_aircraft[aircraft_name].add(msg.variable_name)
                
                self.aircraft_count += 1
                print(f"    ✅ {aircraft_name}: {len(aircraft_messages)} control messages found")
                print(f"       Unique variables: {len(self.variables_by_aircraft[aircraft_name])}")
                
                # Show sample of found variables
                sample_vars = list(self.variables_by_aircraft[aircraft_name])[:5]
                print(f"       Sample variables: {', '.join(sample_vars)}")
            else:
                print(f"    ⚠️  {aircraft_name}: No control messages found")
    
    def analyze_variable_consistency(self) -> Dict[str, Dict]:
        """Analyze consistency of events across aircraft for each variable"""
        variable_data = defaultdict(lambda: {
            'aircraft': set(),
            'event_types': set(),
            'qualifiers': set(),
            'values': set(),
            'control_types': set(),
            'messages': []
        })
        
        # Collect all data for each variable
        for msg in self.control_messages:
            var_data = variable_data[msg.variable_name]
            var_data['aircraft'].add(msg.aircraft)
            var_data['event_types'].add(msg.event_type)
            var_data['qualifiers'].add(msg.qualifier)
            var_data['values'].add(msg.value)
            var_data['control_types'].add(msg.control_type)
            var_data['messages'].append(msg)
        
        return variable_data
    
    def generate_reports(self, output_dir="./fixed_tmd_results"):
        """Generate comprehensive reports"""
        os.makedirs(output_dir, exist_ok=True)
        
        print(f"\n📊 Generating reports in: {output_dir}")
        
        if len(self.control_messages) == 0:
            print("❌ No control messages found - cannot generate meaningful reports")
            return
        
        # Analyze variable consistency
        variable_data = self.analyze_variable_consistency()
        
        # 1. Complete control messages CSV
        self.generate_complete_messages_csv(output_dir)
        
        # 2. Variable summary
        self.generate_variable_summary(output_dir, variable_data)
        
        # 3. Event patterns for DLL implementation
        self.generate_dll_implementation_guide(output_dir, variable_data)
        
        # 4. Aircraft comparison
        self.generate_aircraft_comparison(output_dir)
        
    def generate_complete_messages_csv(self, output_dir: str):
        """Generate CSV with ALL control messages"""
        csv_path = os.path.join(output_dir, "control_messages_complete.csv")
        
        with open(csv_path, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = [
                'variable_name', 'event_type', 'qualifier', 'value',
                'aircraft', 'control_name', 'control_type', 'file_path'
            ]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            
            for msg in sorted(self.control_messages, key=lambda x: (x.variable_name, x.aircraft)):
                writer.writerow(asdict(msg))
                
        print(f"✅ Complete control messages CSV: {csv_path}")
        print(f"   Total control messages: {len(self.control_messages)}")
    
    def generate_variable_summary(self, output_dir: str, variable_data: Dict):
        """Generate variable summary report"""
        summary_path = os.path.join(output_dir, "variable_summary.txt")
        
        with open(summary_path, 'w', encoding='utf-8') as f:
            f.write("VARIABLE SUMMARY REPORT\n")
            f.write("=" * 30 + "\n\n")
            
            f.write(f"📊 SUMMARY STATISTICS\n")
            f.write(f"Aircraft scanned: {self.aircraft_count}\n")
            f.write(f"TMD files found: {self.tmd_files_found}\n")
            f.write(f"Total control messages: {len(self.control_messages)}\n")
            f.write(f"Unique variables: {len(variable_data)}\n\n")
            
            # Event type distribution
            event_counter = Counter()
            for msg in self.control_messages:
                event_counter[msg.event_type] += 1
                
            f.write("🎯 EVENT TYPE DISTRIBUTION\n")
            for event_type, count in event_counter.most_common():
                percentage = (count / len(self.control_messages)) * 100
                f.write(f"{event_type:<15} {count:4d} messages ({percentage:5.1f}%)\n")
            f.write("\n")
            
            # Most common variables
            var_counter = Counter()
            for msg in self.control_messages:
                var_counter[msg.variable_name] += 1
                
            f.write("🔥 MOST COMMON VARIABLES\n")
            for var_name, count in var_counter.most_common(20):
                aircraft_count = len(variable_data[var_name]['aircraft'])
                f.write(f"{var_name:<45} {count:3d} msgs, {aircraft_count:2d} aircraft\n")
            
        print(f"✅ Variable summary: {summary_path}")
    
    def generate_dll_implementation_guide(self, output_dir: str, variable_data: Dict):
        """Generate implementation guide for DLL"""
        guide_path = os.path.join(output_dir, "dll_implementation_guide.txt")
        
        with open(guide_path, 'w', encoding='utf-8') as f:
            f.write("DLL IMPLEMENTATION GUIDE\n")
            f.write("=" * 30 + "\n\n")
            
            f.write("🔧 READY-TO-IMPLEMENT C++ MAPPING\n")
            f.write("-" * 40 + "\n\n")
            
            f.write("```cpp\n")
            f.write("// Auto-generated hybrid variable mapping\n")
            f.write("std::unordered_map<std::string, HybridVariableInfo> hybrid_mapping = {\n")
            
            # Generate C++ mapping for consistent variables
            for var_name, data in sorted(variable_data.items()):
                if len(data['event_types']) == 1:  # Consistent event type
                    event_type = list(data['event_types'])[0]
                    qualifier = list(data['qualifiers'])[0] if data['qualifiers'] else ""
                    
                    # Use most common value
                    value_counter = Counter()
                    for msg in data['messages']:
                        value_counter[msg.value] += 1
                    most_common_value = value_counter.most_common(1)[0][0]
                    
                    aircraft_count = len(data['aircraft'])
                    f.write(f'    {{"{var_name}", HybridVariableInfo("{event_type}", "{qualifier}", {most_common_value})}}, // {aircraft_count} aircraft\n')
                    
            f.write("};\n")
            f.write("```\n\n")
            
            f.write("🧪 TEST CASES FOR VALIDATION\n")
            f.write("-" * 30 + "\n")
            
            # Generate test cases for most common variables
            var_counter = Counter()
            for msg in self.control_messages:
                var_counter[msg.variable_name] += 1
                
            f.write("Recommended test sequence:\n\n")
            for i, (var_name, count) in enumerate(var_counter.most_common(10), 1):
                data = variable_data[var_name]
                event_type = list(data['event_types'])[0] if len(data['event_types']) == 1 else "mixed"
                aircraft_list = ', '.join(list(data['aircraft'])[:3])
                if len(data['aircraft']) > 3:
                    aircraft_list += f" (+{len(data['aircraft'])-3} more)"
                    
                f.write(f'{i:2d}. {{"variable": "{var_name}", "value": 1.0}}\n')
                f.write(f'    Event: {event_type}, Aircraft: {aircraft_list}\n\n')
                
        print(f"✅ DLL implementation guide: {guide_path}")
    
    def generate_aircraft_comparison(self, output_dir: str):
        """Generate aircraft comparison report"""
        comparison_path = os.path.join(output_dir, "aircraft_comparison.txt")
        
        with open(comparison_path, 'w', encoding='utf-8') as f:
            f.write("AIRCRAFT COMPARISON REPORT\n")
            f.write("=" * 35 + "\n\n")
            
            f.write("📊 VARIABLES BY AIRCRAFT\n")
            f.write("-" * 25 + "\n")
            
            aircraft_stats = []
            for aircraft_name, variables in self.variables_by_aircraft.items():
                msg_count = len([msg for msg in self.control_messages if msg.aircraft == aircraft_name])
                aircraft_stats.append((aircraft_name, len(variables), msg_count))
            
            # Sort by variable count
            aircraft_stats.sort(key=lambda x: x[1], reverse=True)
            
            for aircraft, var_count, msg_count in aircraft_stats:
                f.write(f"{aircraft:<20} {var_count:3d} variables, {msg_count:3d} messages\n")
                
        print(f"✅ Aircraft comparison: {comparison_path}")

def main():
    parser = argparse.ArgumentParser(description="Fixed TMD Scanner - Extract control messages")
    parser.add_argument("--output", default="./fixed_tmd_results", help="Output directory")
    
    args = parser.parse_args()
    
    scanner = FixedTMDScanner()
    
    # Find aircraft directory
    aircraft_path = scanner.find_aerofly_path()
    
    if not aircraft_path:
        print("❌ Cannot find aircraft directory.")
        return
        
    # Scan all aircraft
    scanner.scan_aircraft_folder(aircraft_path)
    
    # Generate reports
    scanner.generate_reports(args.output)
    
    print(f"\n🎉 SCAN COMPLETE!")
    print(f"📊 Results: {len(scanner.control_messages)} control messages")
    print(f"📊 Unique variables: {len(set(msg.variable_name for msg in scanner.control_messages))}")
    print(f"📁 Reports saved to: {args.output}")

if __name__ == "__main__":
    main()
