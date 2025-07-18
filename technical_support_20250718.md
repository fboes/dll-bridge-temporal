Subject: External DLL SDK - Issues with Assertion Errors and Variable Processing

Hello IPACS Development Team,

After extensive development and testing against your official sample code, I've encountered some critical issues that require your expertise. I would greatly appreciate your guidance on these technical matters:

## 🚨 ISSUES

### 1. Aircraft.Crashed Assertion Error (BLOCKING)
**Problem**: The Aircraft.Crashed variable is causing runtime assertion failures when the aircraft crashes:
- **Error**: "Header.DataType == tm_msg_data_type::Double" (line 849 in tm_external_message.h)
- **Context**: Occurs specifically during aircraft crash events
- **SDK Definition**: MESSAGE_LIST shows Aircraft.Crashed as tm_msg_data_type::Double
- **Comparison**: Your sample code doesn't handle this variable, so the issue wasn't apparent
- **Current Workaround**: Temporarily disabled with try-catch blocks

**Code**:
```cpp
F( AircraftCrashed, "Aircraft.Crashed", tm_msg_data_type::Double, tm_msg_flag::Value, tm_msg_access::Read, tm_msg_unit::None, "" )

// In ProcessMessage():
else if (hash == MessageAircraftCrashed.GetID()) {
    pData->crashed = message.GetDouble();  // ← ASSERTION FAILURE HERE
}
```

**Questions**:
- What is the correct data type for Aircraft.Crashed during crash events?
- Are there special conditions or timing requirements for reading this variable?
- Does the data type change temporarily during crash state transitions?
- Is there a safer way to detect aircraft crash status?

### 2. Autopilot String Variables Assertion Error (BLOCKING)
**Problem**: Autopilot mode string variables cause assertion failures:
- **Variables**: Autopilot.ActiveLateralMode, Autopilot.ActiveVerticalMode
- **Error**: "Header.DataType == tm_msg_data_type::String" (line 877 in tm_external_message.h)
- **Context**: Occurs during autopilot mode changes
- **Pattern**: Your sample doesn't read these variables, avoiding the issue
- **Current Workaround**: Temporarily disabled with default "Manual" values

**Code**:
```cpp
F( AutopilotActiveLateralMode, "Autopilot.ActiveLateralMode", tm_msg_data_type::String, tm_msg_flag::Value, tm_msg_access::Read, tm_msg_unit::None, "" )

// In ProcessMessage():
const auto mode_str = message.GetString();  // ← ASSERTION FAILURE HERE
strncpy_s(pData->ap_lateral_mode, sizeof(pData->ap_lateral_mode), mode_str.c_str(), _TRUNCATE);
```

**Questions**:
- What is the correct method to read autopilot mode strings?
- Are these variables only available during specific autopilot states?
- Do string variables have different data types during mode transitions?
- Is there a timing dependency for string variable availability?

### 3. Variables Not Updating (FUNCTIONAL)
**Problem**: Several important variables never change from their default values:
- **Aircraft.OnRunway**: Always returns 0.0 even when aircraft is clearly on runway
- **Aircraft.ParkingBrake**: Doesn't reflect actual parking brake state
- **Engine system variables**: Many engine-related variables remain static
- **Navigation variables**: Some COM/NAV frequencies don't update
- **Comparison**: Your sample only reads basic position/altitude, so these issues weren't visible

**Questions**:
- Do some variables require specific initialization or aircraft states?
- Are certain variables only functional with specific aircraft types?
- Are there dependencies between variables that affect their update behavior?

### 4. Update Rate and Performance Optimization

**Problem**: Need guidance on optimal performance parameters.
**Observation**: Your sample doesn't address performance considerations for production DLLs.

**Questions**:
- What is the recommended frequency for the DLL Update callback?
- Are there performance implications of processing 100+ variables per frame?
- Should we implement variable filtering/subscription mechanisms?
- What is the maximum safe message_list_sent size per update?

### 5. Message Flags and Access Patterns
**Problem**: Understanding proper usage of tm_msg_flag types.
**Observation**: Your sample uses constexpr false to disable examples, but doesn't explain flag usage.

**Questions**:
- When should we use tm_msg_flag::Value vs tm_msg_flag::Event?
- How do tm_msg_flag::Move, ::Toggle, ::Step flags work in practice?
- Are there timing requirements for Event-type messages?
- Can we mix different flag types in a single update cycle?

**Example Confusion**:
```cpp
// These use different flags - why?
F( ControlsGear, "Controls.Gear", tm_msg_flag::Value, tm_msg_access::ReadWrite )
F( ControlsGearToggle, "Controls.Gear", tm_msg_flag::Toggle, tm_msg_access::Write )
```

### 6. Fuel System Variables
**Problem**: Cannot locate fuel-related variables in the SDK.
**Observation**: Your sample and MESSAGE_LIST don't include fuel variables.

**Questions**:
- Are there built-in fuel quantity/flow variables in the SDK?
- Variables like Aircraft.FuelQuantity, Aircraft.FuelFlow, Aircraft.FuelRemaining?
- If not available, what is the recommended approach for fuel monitoring?
- Are there differences between fuel types (Jet-A vs AvGas) in the SDK?

### 7. Vector Data Types (Vector3d/Vector4d) Handling
**Problem**: Need clarification on complex data types.
**Observation**: Your sample shows basic Vector3d usage but lacks detailed explanation.

**Code Example**:
```cpp
// Works fine in your sample:
aircraft_position = message.GetVector3d();

// But what about individual components?
// Are these accessible: position.x, position.y, position.z?
```

**Questions**:
- What is the exact structure/layout of Vector3d and Vector4d?
- How to access individual components safely?
- Are there coordinate system conventions (NED vs ENU)?
- Any endianness considerations for cross-platform compatibility?

## 📋 TECHNICAL CONTEXT

**Development Environment**:
- Visual Studio 2022 Community
- Windows 10/11 x64
- Using official tm_external_message.h from SDK
- Based on your official sample architecture
- Shared memory: 2800 bytes, processing ~100 variables
- TCP server: Ports 12345 (data) + 12346 (commands)

**Current Architecture**:
- Extends your sample with production-grade error handling
- Thread-safe shared memory access (unlike the sample)
- Real-time variable processing in ProcessMessage()
- Bidirectional command processing (beyond sample scope)
- Professional logging and debugging capabilities


## 🔧 ERROR PATTERNS AND DEBUGGING INFO

### Assertion Error Pattern Analysis
**Common Pattern**: All assertion errors occur at specific lines:
- Line 849: `assert(Header.DataType == tm_msg_data_type::Double);`
- Line 877: `assert(Header.DataType == tm_msg_data_type::String);`

**Timing Pattern**: Errors seem to occur during:
- State transitions (normal flight → crashed)
- Mode changes (manual → autopilot)
- System activations (engines starting, autopilot engaging)

**Hypothesis**: Variables might temporarily change data types or become unavailable during transitions.

**Debugging Approach Used**:
```cpp
// Added to ProcessMessage for debugging:
try {
    if (hash == MessageAircraftCrashed.GetID()) {
        OutputDebugStringA(("Crash variable - DataType: " + 
                           std::to_string((int)message.GetDataType()) + "\n").c_str());
        if (message.GetDataType() == tm_msg_data_type::Double) {
            pData->crashed = message.GetDouble();
        } else {
            // Log unexpected type but continue processing
            pData->crashed = 0.0;  // Safe default
        }
    }
} catch (...) {
    OutputDebugStringA("Exception in crash variable processing\n");
}
```

## 🎯 IMMEDIATE PRIORITIES

**CRITICAL (Production Blocking)**:
1. Aircraft.Crashed assertion error → Alternative crash detection method
2. Autopilot string variables → Proper string handling pattern
3. Data type verification → Safe variable reading methodology

**HIGH PRIORITY (Functionality)**:
4. Non-updating variables → Activation/dependency requirements
5. Vector data access → Proper Vector3d/Vector4d usage
6. Message flags → Correct Event/Value/Toggle patterns

**MEDIUM PRIORITY (Enhancement)**:
7. Fuel system variables → Complete fuel monitoring solution
8. Performance optimization → Update rate and filtering guidance
9. Official sample enhancement → Production-ready patterns


Thank you for your time and for creating such a powerful simulation platform. I look forward to your insights on these technical challenges.

Best regards,

Juan Luis G.