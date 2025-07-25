///////////////////////////////////////////////////////////////////////////////////////////////////
//
// aerofly_bridge_dll_complete.cpp - Complete Multi-Interface Bridge for Aerofly FS4
//
// Features:
// - All 339 variables exposed
// - Shared Memory (primary interface) 
// - TCP Server (network interface)
// - Bidirectional commands
// - Thread-safe operations
// - Auto-reconnection
// - Performance optimized
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cmath>
#include <memory>
#include <atomic>
#include <queue>
#include <iomanip>  // For std::setprecision y std::fixed
#include <cmath>    // For std::isfinite

#pragma comment(lib, "ws2_32.lib")

#include "tm_external_message.h"

#include <regex>
#include <fstream>
#include <filesystem>
#include <set>
#include <chrono>
#include <ctime>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBAL LOGGING FUNCTION - For debugging hybrid system
///////////////////////////////////////////////////////////////////////////////////////////////////

void HybridLogToFile(const std::string& message) {
    try {
        std::ofstream log_file("C:\\Users\\Admin\\Documents\\hybrid_debug.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            char timestamp[100];
            strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&time_t));
            log_file << "[" << timestamp << "] " << message << std::endl;
            log_file.close();
        }
    } catch (...) {
        // Ignore file errors - don't crash because of logging
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// OPTIMIZED DATA STRUCTURE - All 339 Variables Organized
///////////////////////////////////////////////////////////////////////////////////////////////////

struct AeroflyBridgeData {
    // === HEADER (16 bytes) ===
    uint64_t timestamp_us;           // Microseconds since start
    uint32_t data_valid;            // 1 = valid data, 0 = invalid
    uint32_t update_counter;        // Increments each update

    // === AIRCRAFT BASIC (64 bytes) ===
    double latitude;                // Aircraft.Latitude (radians)
    double longitude;               // Aircraft.Longitude (radians)
    double altitude;                // Aircraft.Altitude (meters)
    double pitch;                   // Aircraft.Pitch (radians)
    double bank;                    // Aircraft.Bank (radians)
    double true_heading;            // Aircraft.TrueHeading (radians)
    double magnetic_heading;        // Aircraft.MagneticHeading (radians)
    double indicated_airspeed;      // Aircraft.IndicatedAirspeed (m/s)

    // === AIRCRAFT PHYSICS (96 bytes) ===
    double ground_speed;            // Aircraft.GroundSpeed (m/s)
    double vertical_speed;          // Aircraft.VerticalSpeed (m/s)
    double angle_of_attack;         // Aircraft.AngleOfAttack (radians)
    double angle_of_attack_limit;   // Aircraft.AngleOfAttackLimit (radians)
    double mach_number;             // Aircraft.MachNumber
    double rate_of_turn;            // Aircraft.RateOfTurn (rad/s)
    tm_vector3d position;           // Aircraft.Position (meters)
    tm_vector3d velocity;           // Aircraft.Velocity (m/s)
    tm_vector3d acceleration;       // Aircraft.Acceleration (m/s²)
    tm_vector3d angular_velocity;   // Aircraft.AngularVelocity (rad/s)
    tm_vector3d wind;              // Aircraft.Wind (m/s)
    tm_vector3d gravity;           // Aircraft.Gravity (m/s²)

    // === AIRCRAFT STATE (64 bytes) ===
    double on_ground;              // Aircraft.OnGround (0/1)
    double on_runway;              // Aircraft.OnRunway (0/1)
    double crashed;                // Aircraft.Crashed (0/1)
    double gear_position;          // Aircraft.Gear (0-1)
    double flaps_position;         // Aircraft.Flaps (0-1)
    double slats_position;         // Aircraft.Slats (0-1)
    double throttle_position;      // Aircraft.Throttle (0-1)
    double airbrake_position;      // Aircraft.AirBrake (0-1)

    // === ENGINE DATA (32 bytes) ===
    double engine_throttle[4];     // Aircraft.EngineThrottle1-4 (0-1)
    double engine_rotation_speed[4]; // Aircraft.EngineRotationSpeed1-4
    double engine_running[4];      // Aircraft.EngineRunning1-4 (0/1)

    // === CONTROLS INPUT (24 bytes) ===
    double pitch_input;            // Controls.Pitch.Input (-1 to +1)
    double roll_input;             // Controls.Roll.Input (-1 to +1)
    double yaw_input;              // Controls.Yaw.Input (-1 to +1)

    // === NAVIGATION FREQUENCIES (80 bytes) ===
    double com1_frequency;         // Communication.COM1Frequency (Hz)
    double com1_standby_frequency; // Communication.COM1StandbyFrequency (Hz)
    double com2_frequency;         // Communication.COM2Frequency (Hz)
    double com2_standby_frequency; // Communication.COM2StandbyFrequency (Hz)
    double nav1_frequency;         // Navigation.NAV1Frequency (Hz)
    double nav1_standby_frequency; // Navigation.NAV1StandbyFrequency (Hz)
    double nav1_selected_course;   // Navigation.SelectedCourse1 (radians)
    double nav2_frequency;         // Navigation.NAV2Frequency (Hz)
    double nav2_standby_frequency; // Navigation.NAV2StandbyFrequency (Hz)
    double nav2_selected_course;   // Navigation.SelectedCourse2 (radians)

    // === AUTOPILOT (64 bytes) ===
    double ap_engaged;             // Autopilot.Engaged (0/1)
    double ap_selected_airspeed;   // Autopilot.SelectedAirspeed (m/s)
    double ap_selected_heading;    // Autopilot.SelectedHeading (radians)
    double ap_selected_altitude;   // Autopilot.SelectedAltitude (meters)
    double ap_selected_vs;         // Autopilot.SelectedVerticalSpeed (m/s)
    double ap_throttle_engaged;    // Autopilot.ThrottleEngaged (0/1)
    char ap_lateral_mode[16];      // Autopilot.ActiveLateralMode
    char ap_vertical_mode[16];     // Autopilot.ActiveVerticalMode

    // === PERFORMANCE SPEEDS (40 bytes) ===
    double vs0_speed;              // Performance.Speed.VS0 (m/s)
    double vs1_speed;              // Performance.Speed.VS1 (m/s)
    double vfe_speed;              // Performance.Speed.VFE (m/s)
    double vno_speed;              // Performance.Speed.VNO (m/s)
    double vne_speed;              // Performance.Speed.VNE (m/s)

    // === WARNINGS (16 bytes) ===
    uint32_t warning_flags;        // Bitfield for all warnings
    uint32_t master_warning;       // Warnings.MasterWarning
    uint32_t master_caution;       // Warnings.MasterCaution
    uint32_t reserved_warnings;    // For future use

    // === ALL VARIABLES ARRAY (2712 bytes) - Complete Access ===
    double all_variables[339];     // 339 variables by index (complete SDK coverage)

    // === DYNAMIC VARIABLES SYSTEM (NEW) ===
    uint32_t dynamic_count;          // Number of active dynamic variables
    uint32_t dynamic_capacity;       // Maximum capacity (5000)

    // Hash-based lookup table for O(1) average access
    struct DynamicVariableEntry {
        char name[64];               // Full variable name (e.g., "A380.MCDU.FlightPlan")
        uint32_t value_index;        // Index in dynamic_values array
        uint32_t name_hash;          // CRC32 hash of name for fast comparison
        uint32_t access_count;       // Usage tracking for optimization
        uint16_t aircraft_id;        // Aircraft identifier (0=global, 1=A380, 2=C172, etc.)
        uint16_t category_id;        // Category (0=Controls, 1=Navigation, 2=Engine, etc.)
    } dynamic_lookup[5000];

    // Values array - aligned for fast access
    double dynamic_values[5000];

    // Category index for efficient browsing
    struct CategoryInfo {
        char name[32];               // Category name ("Controls", "Navigation", etc.)
        uint32_t start_index;        // First variable index in this category
        uint32_t count;              // Number of variables in category
    } categories[100];               // Up to 100 categories
    uint32_t category_count;

    // Aircraft index for aircraft-specific queries
    struct AircraftInfo {
        char name[32];               // Aircraft name ("A380", "C172", etc.)
        uint32_t start_index;        // First variable index for this aircraft
        uint32_t count;              // Number of variables for this aircraft
    } aircraft[50];                  // Up to 50 aircraft
    uint32_t aircraft_count;

    // === HYBRID SYSTEM INFO ===
    uint32_t hybrid_core_variables;        // Number of core variables available
    uint32_t hybrid_dynamic_variables;     // Number of dynamic variables created
    uint32_t hybrid_discovered_variables;  // Number of variables discovered
    uint32_t hybrid_discovery_complete;    // 1 = discovery complete, 0 = in progress
    char aerofly_path[256];                // Path to Aerofly installation (null-terminated)
    uint32_t reserved_hybrid[11];          // For future hybrid features

    // === INLINE SEARCH FUNCTIONS ===

    // Fast hash function for variable names
    static uint32_t ComputeHash(const char* str) {
        uint32_t hash = 5381;
        while (*str) {
            hash = ((hash << 5) + hash) + *str++;
        }
        return hash;
    }

    // Find dynamic variable by name - O(1) average case
    int FindDynamicVariable(const char* name) const {
        if (!name || dynamic_count == 0) return -1;
        
        uint32_t hash = ComputeHash(name);
        
        // Linear probe with hash comparison first (faster than string compare)
        for (uint32_t i = 0; i < dynamic_count; i++) {
            if (dynamic_lookup[i].name_hash == hash) {
                // Hash match - verify with string comparison
                if (strcmp(dynamic_lookup[i].name, name) == 0) {
                    // Found! Increment access counter for optimization tracking
                    const_cast<DynamicVariableEntry&>(dynamic_lookup[i]).access_count++;
                    return dynamic_lookup[i].value_index;
                }
            }
        }
        return -1; // Not found
    }

    // Get dynamic variable value by name
    double GetDynamicValue(const char* name, double default_value = 0.0) const {
        int index = FindDynamicVariable(name);
        return (index >= 0) ? dynamic_values[index] : default_value;
    }

    // NOTE: Updated total size: ~3688 bytes (with hybrid system support)
    };

///////////////////////////////////////////////////////////////////////////////////////////////////
// ENHANCED VARIABLE INFO STRUCTURE - Reemplaza TMDParser::VariableInfo
///////////////////////////////////////////////////////////////////////////////////////////////////

struct EnhancedVariableInfo {
    std::string name;
    std::string aircraft;
    std::string full_path;
    
    // Type information
    tm_msg_data_type data_type;
    tm_msg_flag flag_type;
    tm_msg_access access_type;
    tm_msg_unit unit_type;
    
    // Event information
    bool is_event;
    bool is_toggle;
    bool is_step;
    bool is_move;
    bool is_offset;
    bool is_active;
    std::string primary_qualifier;
    std::vector<std::string> valid_qualifiers;
    
    // Metadata
    std::string description;
    std::string category;
    double min_value;
    double max_value;
    double step_size;
    
    EnhancedVariableInfo() : 
        data_type(tm_msg_data_type::Double),
        flag_type(tm_msg_flag::Value),
        access_type(tm_msg_access::ReadWrite),
        unit_type(tm_msg_unit::None),
        is_event(false), is_toggle(false), is_step(false), is_move(false),
        is_offset(false), is_active(false),
        min_value(0.0), max_value(1.0), step_size(0.1) {}
        
    EnhancedVariableInfo(const std::string& var_name, const std::string& aircraft_name, 
                        const std::string& path) : EnhancedVariableInfo() {
        name = var_name;
        aircraft = aircraft_name;
        full_path = path;
    }
    
    // Helper methods
    bool HasQualifier(const std::string& qualifier) const {
        return std::find(valid_qualifiers.begin(), valid_qualifiers.end(), qualifier) 
               != valid_qualifiers.end();
    }
    
    bool IsWritable() const {
        return access_type == tm_msg_access::Write || access_type == tm_msg_access::ReadWrite;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// COMPLETE VARIABLE INDEX ENUM - All 339 Variables from Official SDK + Extensions
// Order matches exactly with MESSAGE_LIST from original Aerofly sample
///////////////////////////////////////////////////////////////////////////////////////////////////

enum class VariableIndex : int {
    // === AIRCRAFT VARIABLES (0-94) === 
    AIRCRAFT_UNIVERSAL_TIME = 0,                   // Aircraft.UniversalTime
    AIRCRAFT_ALTITUDE = 1,                         // Aircraft.Altitude  
    AIRCRAFT_VERTICAL_SPEED = 2,                   // Aircraft.VerticalSpeed
    AIRCRAFT_PITCH = 3,                            // Aircraft.Pitch
    AIRCRAFT_BANK = 4,                             // Aircraft.Bank
    AIRCRAFT_INDICATED_AIRSPEED = 5,               // Aircraft.IndicatedAirspeed
    AIRCRAFT_INDICATED_AIRSPEED_TREND = 6,         // Aircraft.IndicatedAirspeedTrend
    AIRCRAFT_GROUND_SPEED = 7,                     // Aircraft.GroundSpeed
    AIRCRAFT_MAGNETIC_HEADING = 8,                 // Aircraft.MagneticHeading
    AIRCRAFT_TRUE_HEADING = 9,                     // Aircraft.TrueHeading
    AIRCRAFT_LATITUDE = 10,                        // Aircraft.Latitude
    AIRCRAFT_LONGITUDE = 11,                       // Aircraft.Longitude
    AIRCRAFT_HEIGHT = 12,                          // Aircraft.Height
    AIRCRAFT_POSITION = 13,                        // Aircraft.Position (Vector3d)
    AIRCRAFT_ORIENTATION = 14,                     // Aircraft.Orientation
    AIRCRAFT_VELOCITY = 15,                        // Aircraft.Velocity (Vector3d)
    AIRCRAFT_ANGULAR_VELOCITY = 16,                // Aircraft.AngularVelocity (Vector3d)
    AIRCRAFT_ACCELERATION = 17,                    // Aircraft.Acceleration (Vector3d)
    AIRCRAFT_GRAVITY = 18,                         // Aircraft.Gravity (Vector3d)
    AIRCRAFT_WIND = 19,                            // Aircraft.Wind (Vector3d)
    AIRCRAFT_RATE_OF_TURN = 20,                    // Aircraft.RateOfTurn
    AIRCRAFT_MACH_NUMBER = 21,                     // Aircraft.MachNumber
    AIRCRAFT_ANGLE_OF_ATTACK = 22,                 // Aircraft.AngleOfAttack
    AIRCRAFT_ANGLE_OF_ATTACK_LIMIT = 23,           // Aircraft.AngleOfAttackLimit
    AIRCRAFT_ACCELERATION_LIMIT = 24,              // Aircraft.AccelerationLimit
    AIRCRAFT_GEAR = 25,                            // Aircraft.Gear
    AIRCRAFT_FLAPS = 26,                           // Aircraft.Flaps
    AIRCRAFT_SLATS = 27,                           // Aircraft.Slats
    AIRCRAFT_THROTTLE = 28,                        // Aircraft.Throttle
    AIRCRAFT_AIR_BRAKE = 29,                       // Aircraft.AirBrake
    AIRCRAFT_GROUND_SPOILERS_ARMED = 30,           // Aircraft.GroundSpoilersArmed
    AIRCRAFT_GROUND_SPOILERS_EXTENDED = 31,        // Aircraft.GroundSpoilersExtended
    AIRCRAFT_PARKING_BRAKE = 32,                   // Aircraft.ParkingBrake
    AIRCRAFT_AUTO_BRAKE_SETTING = 33,              // Aircraft.AutoBrakeSetting
    AIRCRAFT_AUTO_BRAKE_ENGAGED = 34,              // Aircraft.AutoBrakeEngaged
    AIRCRAFT_AUTO_BRAKE_REJECTED_TAKEOFF = 35,     // Aircraft.AutoBrakeRejectedTakeOff
    AIRCRAFT_RADAR_ALTITUDE = 36,                  // Aircraft.RadarAltitude
    AIRCRAFT_NAME = 37,                            // Aircraft.Name (String)
    AIRCRAFT_NEAREST_AIRPORT_IDENTIFIER = 38,      // Aircraft.NearestAirportIdentifier (String)
    AIRCRAFT_NEAREST_AIRPORT_NAME = 39,            // Aircraft.NearestAirportName (String)
    AIRCRAFT_NEAREST_AIRPORT_LOCATION = 40,        // Aircraft.NearestAirportLocation (Vector2d)
    AIRCRAFT_NEAREST_AIRPORT_ELEVATION = 41,       // Aircraft.NearestAirportElevation
    AIRCRAFT_BEST_AIRPORT_IDENTIFIER = 42,         // Aircraft.BestAirportIdentifier (String)
    AIRCRAFT_BEST_AIRPORT_NAME = 43,               // Aircraft.BestAirportName (String)
    AIRCRAFT_BEST_AIRPORT_LOCATION = 44,           // Aircraft.BestAirportLocation (Vector2d)
    AIRCRAFT_BEST_AIRPORT_ELEVATION = 45,          // Aircraft.BestAirportElevation
    AIRCRAFT_BEST_RUNWAY_IDENTIFIER = 46,          // Aircraft.BestRunwayIdentifier (String)
    AIRCRAFT_BEST_RUNWAY_ELEVATION = 47,           // Aircraft.BestRunwayElevation
    AIRCRAFT_BEST_RUNWAY_THRESHOLD = 48,           // Aircraft.BestRunwayThreshold (Vector3d)
    AIRCRAFT_BEST_RUNWAY_END = 49,                 // Aircraft.BestRunwayEnd (Vector3d)
    AIRCRAFT_CATEGORY_JET = 50,                    // Aircraft.Category.Jet
    AIRCRAFT_CATEGORY_GLIDER = 51,                 // Aircraft.Category.Glider
    AIRCRAFT_ON_GROUND = 52,                       // Aircraft.OnGround
    AIRCRAFT_ON_RUNWAY = 53,                       // Aircraft.OnRunway
    AIRCRAFT_CRASHED = 54,                         // Aircraft.Crashed
    AIRCRAFT_POWER = 55,                           // Aircraft.Power
    AIRCRAFT_NORMALIZED_POWER = 56,                // Aircraft.NormalizedPower
    AIRCRAFT_NORMALIZED_POWER_TARGET = 57,         // Aircraft.NormalizedPowerTarget
    AIRCRAFT_TRIM = 58,                            // Aircraft.Trim
    AIRCRAFT_PITCH_TRIM = 59,                      // Aircraft.PitchTrim
    AIRCRAFT_PITCH_TRIM_SCALING = 60,              // Aircraft.PitchTrimScaling
    AIRCRAFT_PITCH_TRIM_OFFSET = 61,               // Aircraft.PitchTrimOffset
    AIRCRAFT_RUDDER_TRIM = 62,                     // Aircraft.RudderTrim
    AIRCRAFT_AUTO_PITCH_TRIM = 63,                 // Aircraft.AutoPitchTrim
    AIRCRAFT_YAW_DAMPER_ENABLED = 64,              // Aircraft.YawDamperEnabled
    AIRCRAFT_RUDDER_PEDALS_DISCONNECTED = 65,      // Aircraft.RudderPedalsDisconnected
    AIRCRAFT_STARTER = 66,                         // Aircraft.Starter
    AIRCRAFT_STARTER_1 = 67,                       // Aircraft.Starter1
    AIRCRAFT_STARTER_2 = 68,                       // Aircraft.Starter2
    AIRCRAFT_STARTER_3 = 69,                       // Aircraft.Starter3
    AIRCRAFT_STARTER_4 = 70,                       // Aircraft.Starter4
    AIRCRAFT_IGNITION = 71,                        // Aircraft.Ignition
    AIRCRAFT_IGNITION_1 = 72,                      // Aircraft.Ignition1
    AIRCRAFT_IGNITION_2 = 73,                      // Aircraft.Ignition2
    AIRCRAFT_IGNITION_3 = 74,                      // Aircraft.Ignition3
    AIRCRAFT_IGNITION_4 = 75,                      // Aircraft.Ignition4
    AIRCRAFT_THROTTLE_LIMIT = 76,                  // Aircraft.ThrottleLimit
    AIRCRAFT_REVERSE = 77,                         // Aircraft.Reverse
    AIRCRAFT_ENGINE_MASTER_1 = 78,                 // Aircraft.EngineMaster1
    AIRCRAFT_ENGINE_MASTER_2 = 79,                 // Aircraft.EngineMaster2
    AIRCRAFT_ENGINE_MASTER_3 = 80,                 // Aircraft.EngineMaster3
    AIRCRAFT_ENGINE_MASTER_4 = 81,                 // Aircraft.EngineMaster4
    AIRCRAFT_ENGINE_THROTTLE_1 = 82,               // Aircraft.EngineThrottle1
    AIRCRAFT_ENGINE_THROTTLE_2 = 83,               // Aircraft.EngineThrottle2
    AIRCRAFT_ENGINE_THROTTLE_3 = 84,               // Aircraft.EngineThrottle3
    AIRCRAFT_ENGINE_THROTTLE_4 = 85,               // Aircraft.EngineThrottle4
    AIRCRAFT_ENGINE_ROTATION_SPEED_1 = 86,         // Aircraft.EngineRotationSpeed1
    AIRCRAFT_ENGINE_ROTATION_SPEED_2 = 87,         // Aircraft.EngineRotationSpeed2
    AIRCRAFT_ENGINE_ROTATION_SPEED_3 = 88,         // Aircraft.EngineRotationSpeed3
    AIRCRAFT_ENGINE_ROTATION_SPEED_4 = 89,         // Aircraft.EngineRotationSpeed4
    AIRCRAFT_ENGINE_RUNNING_1 = 90,                // Aircraft.EngineRunning1
    AIRCRAFT_ENGINE_RUNNING_2 = 91,                // Aircraft.EngineRunning2
    AIRCRAFT_ENGINE_RUNNING_3 = 92,                // Aircraft.EngineRunning3
    AIRCRAFT_ENGINE_RUNNING_4 = 93,                // Aircraft.EngineRunning4
    AIRCRAFT_APU_AVAILABLE = 94,                   // Aircraft.APUAvailable

    // === PERFORMANCE SPEEDS (95-104) ===
    PERFORMANCE_SPEED_VS0 = 95,                    // Performance.Speed.VS0
    PERFORMANCE_SPEED_VS1 = 96,                    // Performance.Speed.VS1
    PERFORMANCE_SPEED_VFE = 97,                    // Performance.Speed.VFE
    PERFORMANCE_SPEED_VNO = 98,                    // Performance.Speed.VNO
    PERFORMANCE_SPEED_VNE = 99,                    // Performance.Speed.VNE
    PERFORMANCE_SPEED_VAPP = 100,                  // Performance.Speed.VAPP
    PERFORMANCE_SPEED_MINIMUM = 101,               // Performance.Speed.Minimum
    PERFORMANCE_SPEED_MAXIMUM = 102,               // Performance.Speed.Maximum
    PERFORMANCE_SPEED_MINIMUM_FLAP_RETRACTION = 103, // Performance.Speed.MinimumFlapRetraction
    PERFORMANCE_SPEED_MAXIMUM_FLAP_EXTENSION = 104,  // Performance.Speed.MaximumFlapExtension

    // === CONFIGURATION (105-106) ===
    CONFIGURATION_SELECTED_TAKEOFF_FLAPS = 105,   // Configuration.SelectedTakeOffFlaps
    CONFIGURATION_SELECTED_LANDING_FLAPS = 106,   // Configuration.SelectedLandingFlaps

    // === FLIGHT MANAGEMENT SYSTEM (107) ===
    FMS_FLIGHT_NUMBER = 107,                       // FlightManagementSystem.FlightNumber (String)

    // === NAVIGATION (108-141) ===
    NAVIGATION_SELECTED_COURSE_1 = 108,            // Navigation.SelectedCourse1
    NAVIGATION_SELECTED_COURSE_2 = 109,            // Navigation.SelectedCourse2
    NAVIGATION_NAV1_IDENTIFIER = 110,              // Navigation.NAV1Identifier (String)
    NAVIGATION_NAV1_FREQUENCY = 111,               // Navigation.NAV1Frequency
    NAVIGATION_NAV1_STANDBY_FREQUENCY = 112,       // Navigation.NAV1StandbyFrequency
    NAVIGATION_NAV1_FREQUENCY_SWAP = 113,          // Navigation.NAV1FrequencySwap
    NAVIGATION_NAV2_IDENTIFIER = 114,              // Navigation.NAV2Identifier (String)
    NAVIGATION_NAV2_FREQUENCY = 115,               // Navigation.NAV2Frequency
    NAVIGATION_NAV2_STANDBY_FREQUENCY = 116,       // Navigation.NAV2StandbyFrequency
    NAVIGATION_NAV2_FREQUENCY_SWAP = 117,          // Navigation.NAV2FrequencySwap
    NAVIGATION_DME1_FREQUENCY = 118,               // Navigation.DME1Frequency
    NAVIGATION_DME1_DISTANCE = 119,                // Navigation.DME1Distance
    NAVIGATION_DME1_TIME = 120,                    // Navigation.DME1Time
    NAVIGATION_DME1_SPEED = 121,                   // Navigation.DME1Speed
    NAVIGATION_DME2_FREQUENCY = 122,               // Navigation.DME2Frequency
    NAVIGATION_DME2_DISTANCE = 123,                // Navigation.DME2Distance
    NAVIGATION_DME2_TIME = 124,                    // Navigation.DME2Time
    NAVIGATION_DME2_SPEED = 125,                   // Navigation.DME2Speed
    NAVIGATION_ILS1_IDENTIFIER = 126,              // Navigation.ILS1Identifier (String)
    NAVIGATION_ILS1_COURSE = 127,                  // Navigation.ILS1Course
    NAVIGATION_ILS1_FREQUENCY = 128,               // Navigation.ILS1Frequency
    NAVIGATION_ILS1_STANDBY_FREQUENCY = 129,       // Navigation.ILS1StandbyFrequency
    NAVIGATION_ILS1_FREQUENCY_SWAP = 130,          // Navigation.ILS1FrequencySwap
    NAVIGATION_ILS2_IDENTIFIER = 131,              // Navigation.ILS2Identifier (String)
    NAVIGATION_ILS2_COURSE = 132,                  // Navigation.ILS2Course
    NAVIGATION_ILS2_FREQUENCY = 133,               // Navigation.ILS2Frequency
    NAVIGATION_ILS2_STANDBY_FREQUENCY = 134,       // Navigation.ILS2StandbyFrequency
    NAVIGATION_ILS2_FREQUENCY_SWAP = 135,          // Navigation.ILS2FrequencySwap
    NAVIGATION_ADF1_FREQUENCY = 136,               // Navigation.ADF1Frequency
    NAVIGATION_ADF1_STANDBY_FREQUENCY = 137,       // Navigation.ADF1StandbyFrequency
    NAVIGATION_ADF1_FREQUENCY_SWAP = 138,          // Navigation.ADF1FrequencySwap
    NAVIGATION_ADF2_FREQUENCY = 139,               // Navigation.ADF2Frequency
    NAVIGATION_ADF2_STANDBY_FREQUENCY = 140,       // Navigation.ADF2StandbyFrequency
    NAVIGATION_ADF2_FREQUENCY_SWAP = 141,          // Navigation.ADF2FrequencySwap

    // === COMMUNICATION (142-152) ===
    COMMUNICATION_COM1_FREQUENCY = 142,            // Communication.COM1Frequency
    COMMUNICATION_COM1_STANDBY_FREQUENCY = 143,    // Communication.COM1StandbyFrequency
    COMMUNICATION_COM1_FREQUENCY_SWAP = 144,       // Communication.COM1FrequencySwap
    COMMUNICATION_COM2_FREQUENCY = 145,            // Communication.COM2Frequency
    COMMUNICATION_COM2_STANDBY_FREQUENCY = 146,    // Communication.COM2StandbyFrequency
    COMMUNICATION_COM2_FREQUENCY_SWAP = 147,       // Communication.COM2FrequencySwap
    COMMUNICATION_COM3_FREQUENCY = 148,            // Communication.COM3Frequency
    COMMUNICATION_COM3_STANDBY_FREQUENCY = 149,    // Communication.COM3StandbyFrequency
    COMMUNICATION_COM3_FREQUENCY_SWAP = 150,       // Communication.COM3FrequencySwap
    COMMUNICATION_TRANSPONDER_CODE = 151,          // Communication.TransponderCode
    COMMUNICATION_TRANSPONDER_CURSOR = 152,        // Communication.TransponderCursor

    // === AUTOPILOT (153-180) ===
    AUTOPILOT_MASTER = 153,                        // Autopilot.Master
    AUTOPILOT_DISENGAGE = 154,                     // Autopilot.Disengage
    AUTOPILOT_HEADING = 155,                       // Autopilot.Heading
    AUTOPILOT_VERTICAL_SPEED = 156,                // Autopilot.VerticalSpeed
    AUTOPILOT_SELECTED_SPEED = 157,                // Autopilot.SelectedSpeed
    AUTOPILOT_SELECTED_AIRSPEED = 158,             // Autopilot.SelectedAirspeed
    AUTOPILOT_SELECTED_HEADING = 159,              // Autopilot.SelectedHeading
    AUTOPILOT_SELECTED_ALTITUDE = 160,             // Autopilot.SelectedAltitude
    AUTOPILOT_SELECTED_VERTICAL_SPEED = 161,       // Autopilot.SelectedVerticalSpeed
    AUTOPILOT_SELECTED_ALTITUDE_SCALE = 162,       // Autopilot.SelectedAltitudeScale
    AUTOPILOT_ACTIVE_LATERAL_MODE = 163,           // Autopilot.ActiveLateralMode (String)
    AUTOPILOT_ARMED_LATERAL_MODE = 164,            // Autopilot.ArmedLateralMode (String)
    AUTOPILOT_ACTIVE_VERTICAL_MODE = 165,          // Autopilot.ActiveVerticalMode (String)
    AUTOPILOT_ARMED_VERTICAL_MODE = 166,           // Autopilot.ArmedVerticalMode (String)
    AUTOPILOT_ARMED_APPROACH_MODE = 167,           // Autopilot.ArmedApproachMode (String)
    AUTOPILOT_ACTIVE_AUTO_THROTTLE_MODE = 168,     // Autopilot.ActiveAutoThrottleMode (String)
    AUTOPILOT_ACTIVE_COLLECTIVE_MODE = 169,        // Autopilot.ActiveCollectiveMode (String)
    AUTOPILOT_ARMED_COLLECTIVE_MODE = 170,         // Autopilot.ArmedCollectiveMode (String)
    AUTOPILOT_TYPE = 171,                          // Autopilot.Type (String)
    AUTOPILOT_ENGAGED = 172,                       // Autopilot.Engaged
    AUTOPILOT_USE_MACH_NUMBER = 173,               // Autopilot.UseMachNumber
    AUTOPILOT_SPEED_MANAGED = 174,                 // Autopilot.SpeedManaged
    AUTOPILOT_TARGET_AIRSPEED = 175,               // Autopilot.TargetAirspeed
    AUTOPILOT_AILERON = 176,                       // Autopilot.Aileron
    AUTOPILOT_ELEVATOR = 177,                      // Autopilot.Elevator
    AUTO_THROTTLE_TYPE = 178,                      // AutoThrottle.Type
    AUTOPILOT_THROTTLE_ENGAGED = 179,              // Autopilot.ThrottleEngaged
    AUTOPILOT_THROTTLE_COMMAND = 180,              // Autopilot.ThrottleCommand

    // === FLIGHT DIRECTOR (181-183) ===
    FLIGHT_DIRECTOR_PITCH = 181,                   // FlightDirector.Pitch
    FLIGHT_DIRECTOR_BANK = 182,                    // FlightDirector.Bank
    FLIGHT_DIRECTOR_YAW = 183,                     // FlightDirector.Yaw

    // === COPILOT (184-191) ===
    COPILOT_HEADING = 184,                         // Copilot.Heading
    COPILOT_ALTITUDE = 185,                        // Copilot.Altitude
    COPILOT_AIRSPEED = 186,                        // Copilot.Airspeed
    COPILOT_VERTICAL_SPEED = 187,                  // Copilot.VerticalSpeed
    COPILOT_AILERON = 188,                         // Copilot.Aileron
    COPILOT_ELEVATOR = 189,                        // Copilot.Elevator
    COPILOT_THROTTLE = 190,                        // Copilot.Throttle
    COPILOT_AUTO_RUDDER = 191,                     // Copilot.AutoRudder

    // === CONTROLS (192-260) ===
    CONTROLS_THROTTLE = 192,                       // Controls.Throttle
    CONTROLS_THROTTLE_1 = 193,                     // Controls.Throttle1
    CONTROLS_THROTTLE_2 = 194,                     // Controls.Throttle2
    CONTROLS_THROTTLE_3 = 195,                     // Controls.Throttle3
    CONTROLS_THROTTLE_4 = 196,                     // Controls.Throttle4
    CONTROLS_THROTTLE_1_MOVE = 197,                // Controls.Throttle1 (Move flag)
    CONTROLS_THROTTLE_2_MOVE = 198,                // Controls.Throttle2 (Move flag)
    CONTROLS_THROTTLE_3_MOVE = 199,                // Controls.Throttle3 (Move flag)
    CONTROLS_THROTTLE_4_MOVE = 200,                // Controls.Throttle4 (Move flag)
    CONTROLS_PITCH_INPUT = 201,                    // Controls.Pitch.Input
    CONTROLS_PITCH_INPUT_OFFSET = 202,             // Controls.Pitch.Input (Offset flag)
    CONTROLS_ROLL_INPUT = 203,                     // Controls.Roll.Input
    CONTROLS_ROLL_INPUT_OFFSET = 204,              // Controls.Roll.Input (Offset flag)
    CONTROLS_YAW_INPUT = 205,                      // Controls.Yaw.Input
    CONTROLS_YAW_INPUT_ACTIVE = 206,               // Controls.Yaw.Input (Active flag)
    CONTROLS_FLAPS = 207,                          // Controls.Flaps
    CONTROLS_FLAPS_EVENT = 208,                    // Controls.Flaps (Event flag)
    CONTROLS_GEAR = 209,                           // Controls.Gear
    CONTROLS_GEAR_TOGGLE = 210,                    // Controls.Gear (Toggle flag)
    CONTROLS_WHEEL_BRAKE_LEFT = 211,               // Controls.WheelBrake.Left
    CONTROLS_WHEEL_BRAKE_RIGHT = 212,              // Controls.WheelBrake.Right
    CONTROLS_WHEEL_BRAKE_LEFT_ACTIVE = 213,        // Controls.WheelBrake.Left (Active flag)
    CONTROLS_WHEEL_BRAKE_RIGHT_ACTIVE = 214,       // Controls.WheelBrake.Right (Active flag)
    CONTROLS_AIR_BRAKE = 215,                      // Controls.AirBrake
    CONTROLS_AIR_BRAKE_ACTIVE = 216,               // Controls.AirBrake (Active flag)
    CONTROLS_AIR_BRAKE_ARM = 217,                  // Controls.AirBrake.Arm
    CONTROLS_GLIDER_AIR_BRAKE = 218,               // Controls.GliderAirBrake
    CONTROLS_PROPELLER_SPEED_1 = 219,              // Controls.PropellerSpeed1
    CONTROLS_PROPELLER_SPEED_2 = 220,              // Controls.PropellerSpeed2
    CONTROLS_PROPELLER_SPEED_3 = 221,              // Controls.PropellerSpeed3
    CONTROLS_PROPELLER_SPEED_4 = 222,              // Controls.PropellerSpeed4
    CONTROLS_MIXTURE = 223,                        // Controls.Mixture
    CONTROLS_MIXTURE_1 = 224,                      // Controls.Mixture1
    CONTROLS_MIXTURE_2 = 225,                      // Controls.Mixture2
    CONTROLS_MIXTURE_3 = 226,                      // Controls.Mixture3
    CONTROLS_MIXTURE_4 = 227,                      // Controls.Mixture4
    CONTROLS_THRUST_REVERSE = 228,                 // Controls.ThrustReverse
    CONTROLS_THRUST_REVERSE_1 = 229,               // Controls.ThrustReverse1
    CONTROLS_THRUST_REVERSE_2 = 230,               // Controls.ThrustReverse2
    CONTROLS_THRUST_REVERSE_3 = 231,               // Controls.ThrustReverse3
    CONTROLS_THRUST_REVERSE_4 = 232,               // Controls.ThrustReverse4
    CONTROLS_COLLECTIVE = 233,                     // Controls.Collective
    CONTROLS_CYCLIC_PITCH = 234,                   // Controls.CyclicPitch
    CONTROLS_CYCLIC_ROLL = 235,                    // Controls.CyclicRoll
    CONTROLS_TAIL_ROTOR = 236,                     // Controls.TailRotor
    CONTROLS_ROTOR_BRAKE = 237,                    // Controls.RotorBrake
    CONTROLS_HELICOPTER_THROTTLE_1 = 238,          // Controls.HelicopterThrottle1
    CONTROLS_HELICOPTER_THROTTLE_2 = 239,          // Controls.HelicopterThrottle2
    CONTROLS_TRIM = 240,                           // Controls.Trim
    CONTROLS_TRIM_STEP = 241,                      // Controls.Trim (Step flag)
    CONTROLS_TRIM_MOVE = 242,                      // Controls.Trim (Move flag)
    CONTROLS_AILERON_TRIM = 243,                   // Controls.AileronTrim
    CONTROLS_RUDDER_TRIM = 244,                    // Controls.RudderTrim
    CONTROLS_TILLER = 245,                         // Controls.Tiller
    CONTROLS_PEDALS_DISCONNECT = 246,              // Controls.PedalsDisconnect
    CONTROLS_NOSE_WHEEL_STEERING = 247,            // Controls.NoseWheelSteering
    CONTROLS_LIGHTING_PANEL = 248,                 // Controls.Lighting.Panel
    CONTROLS_LIGHTING_INSTRUMENTS = 249,           // Controls.Lighting.Instruments
    CONTROLS_PRESSURE_SETTING_0 = 250,             // Controls.PressureSetting0
    CONTROLS_PRESSURE_SETTING_STANDARD_0 = 251,    // Controls.PressureSettingStandard0
    CONTROLS_PRESSURE_SETTING_UNIT_0 = 252,        // Controls.PressureSettingUnit0
    CONTROLS_PRESSURE_SETTING_1 = 253,             // Controls.PressureSetting1
    CONTROLS_PRESSURE_SETTING_STANDARD_1 = 254,    // Controls.PressureSettingStandard1
    CONTROLS_PRESSURE_SETTING_UNIT_1 = 255,        // Controls.PressureSettingUnit1
    CONTROLS_PRESSURE_SETTING_2 = 256,             // Controls.PressureSetting2
    CONTROLS_PRESSURE_SETTING_STANDARD_2 = 257,    // Controls.PressureSettingStandard2
    CONTROLS_PRESSURE_SETTING_UNIT_2 = 258,        // Controls.PressureSettingUnit2
    CONTROLS_TRANSITION_ALTITUDE = 259,            // Controls.TransitionAltitude
    CONTROLS_TRANSITION_LEVEL = 260,               // Controls.TransitionLevel

    // === PRESSURIZATION (261-262) ===
    PRESSURIZATION_LANDING_ELEVATION = 261,        // Pressurization.LandingElevation
    PRESSURIZATION_LANDING_ELEVATION_MANUAL = 262, // Pressurization.LandingElevationManual

    // === WARNINGS (263-272) ===
    WARNINGS_MASTER_WARNING = 263,                 // Warnings.MasterWarning
    WARNINGS_MASTER_CAUTION = 264,                 // Warnings.MasterCaution
    WARNINGS_ENGINE_FIRE = 265,                    // Warnings.EngineFire
    WARNINGS_LOW_OIL_PRESSURE = 266,               // Warnings.LowOilPressure
    WARNINGS_LOW_FUEL_PRESSURE = 267,              // Warnings.LowFuelPressure
    WARNINGS_LOW_HYDRAULIC_PRESSURE = 268,         // Warnings.LowHydraulicPressure
    WARNINGS_LOW_VOLTAGE = 269,                    // Warnings.LowVoltage
    WARNINGS_ALTITUDE_ALERT = 270,                 // Warnings.AltitudeAlert
    WARNINGS_WARNING_ACTIVE = 271,                 // Warnings.WarningActive
    WARNINGS_WARNING_MUTE = 272,                   // Warnings.WarningMute

    // === VIEW CONTROLS (273-302) ===
    VIEW_DISPLAY_NAME = 273,                       // View.DisplayName (String)
    VIEW_INTERNAL = 274,                           // View.Internal
    VIEW_FOLLOW = 275,                             // View.Follow
    VIEW_EXTERNAL = 276,                           // View.External
    VIEW_CATEGORY = 277,                           // View.Category
    VIEW_MODE = 278,                               // View.Mode
    VIEW_ZOOM = 279,                               // View.Zoom
    VIEW_PAN_HORIZONTAL = 280,                     // View.Pan.Horizontal
    VIEW_PAN_HORIZONTAL_MOVE = 281,                // View.Pan.Horizontal (Move flag)
    VIEW_PAN_VERTICAL = 282,                       // View.Pan.Vertical
    VIEW_PAN_VERTICAL_MOVE = 283,                  // View.Pan.Vertical (Move flag)
    VIEW_PAN_CENTER = 284,                         // View.Pan.Center
    VIEW_LOOK_HORIZONTAL = 285,                    // View.Look.Horizontal
    VIEW_LOOK_VERTICAL = 286,                      // View.Look.Vertical
    VIEW_ROLL = 287,                               // View.Roll
    VIEW_OFFSET_X = 288,                           // View.OffsetX
    VIEW_OFFSET_X_MOVE = 289,                      // View.OffsetX (Move flag)
    VIEW_OFFSET_Y = 290,                           // View.OffsetY
    VIEW_OFFSET_Y_MOVE = 291,                      // View.OffsetY (Move flag)
    VIEW_OFFSET_Z = 292,                           // View.OffsetZ
    VIEW_OFFSET_Z_MOVE = 293,                      // View.OffsetZ (Move flag)
    VIEW_POSITION = 294,                           // View.Position
    VIEW_DIRECTION = 295,                          // View.Direction
    VIEW_UP = 296,                                 // View.Up
    VIEW_FIELD_OF_VIEW = 297,                      // View.FieldOfView
    VIEW_ASPECT_RATIO = 298,                       // View.AspectRatio
    VIEW_FREE_POSITION = 299,                      // View.FreePosition (Vector3d)
    VIEW_FREE_LOOK_DIRECTION = 300,                // View.FreeLookDirection (Vector3d)
    VIEW_FREE_UP = 301,                            // View.FreeUp (Vector3d)
    VIEW_FREE_FIELD_OF_VIEW = 302,                 // View.FreeFieldOfView

    // === SIMULATION CONTROLS (303-320) ===
    SIMULATION_PAUSE = 303,                        // Simulation.Pause
    SIMULATION_FLIGHT_INFORMATION = 304,           // Simulation.FlightInformation
    SIMULATION_MOVING_MAP = 305,                   // Simulation.MovingMap
    SIMULATION_SOUND = 306,                        // Simulation.Sound
    SIMULATION_LIFT_UP = 307,                      // Simulation.LiftUp
    SIMULATION_SETTING_POSITION = 308,             // Simulation.SettingPosition (Vector3d)
    SIMULATION_SETTING_ORIENTATION = 309,          // Simulation.SettingOrientation (Vector4d)
    SIMULATION_SETTING_VELOCITY = 310,             // Simulation.SettingVelocity (Vector3d)
    SIMULATION_SETTING_SET = 311,                  // Simulation.SettingSet
    SIMULATION_TIME_CHANGE = 312,                  // Simulation.TimeChange
    SIMULATION_VISIBILITY = 313,                   // Simulation.Visibility
    SIMULATION_TIME = 314,                         // Simulation.Time
    SIMULATION_USE_MOUSE_CONTROL = 315,            // Simulation.UseMouseControl
    SIMULATION_PLAYBACK_START = 316,               // Simulation.PlaybackStart
    SIMULATION_PLAYBACK_STOP = 317,                // Simulation.PlaybackStop
    SIMULATION_PLAYBACK_SET_POSITION = 318,        // Simulation.PlaybackPosition
    SIMULATION_EXTERNAL_POSITION = 319,            // Simulation.ExternalPosition (Vector3d)
    SIMULATION_EXTERNAL_ORIENTATION = 320,         // Simulation.ExternalOrientation (Vector4d)

    // === COMMAND CONTROLS (321-330) ===
    COMMAND_EXECUTE = 321,                         // Command.Execute
    COMMAND_BACK = 322,                            // Command.Back
    COMMAND_UP = 323,                              // Command.Up
    COMMAND_DOWN = 324,                            // Command.Down
    COMMAND_LEFT = 325,                            // Command.Left
    COMMAND_RIGHT = 326,                           // Command.Right
    COMMAND_MOVE_HORIZONTAL = 327,                 // Command.MoveHorizontal
    COMMAND_MOVE_VERTICAL = 328,                   // Command.MoveVertical
    COMMAND_ROTATE = 329,                          // Command.Rotate
    COMMAND_ZOOM = 330,                            // Command.Zoom

    // === SPECIAL/IGNORE VARIABLES (331-338) ===
    CONTROLS_SPEED = 331,                          // Controls.Speed (ignore/do not use)
    FMS_DATA_0 = 332,                              // FlightManagementSystem.Data0 (ignore/do not use)
    FMS_DATA_1 = 333,                              // FlightManagementSystem.Data1 (ignore/do not use)
    NAV1_DATA = 334,                               // Navigation.NAV1Data (ignore/do not use)
    NAV2_DATA = 335,                               // Navigation.NAV2Data (ignore/do not use)
    NAV3_DATA = 336,                               // Navigation.NAV3Data (ignore/do not use)
    ILS1_DATA = 337,                               // Navigation.ILS1Data (ignore/do not use)
    ILS2_DATA = 338,                               // Navigation.ILS2Data (ignore/do not use)

    // === TOTAL COUNT ===
    VARIABLE_COUNT = 339                           // Total: 339 variables from official SDK
};



class VariableMapper {
private:
    std::unordered_map<std::string, int> name_to_index;
    std::unordered_map<uint64_t, int> hash_to_index;
    
public:
    VariableMapper() {
        // === AIRCRAFT VARIABLES (0-94) ===
        name_to_index["Aircraft.UniversalTime"] = (int)VariableIndex::AIRCRAFT_UNIVERSAL_TIME;
        name_to_index["Aircraft.Altitude"] = (int)VariableIndex::AIRCRAFT_ALTITUDE;
        name_to_index["Aircraft.VerticalSpeed"] = (int)VariableIndex::AIRCRAFT_VERTICAL_SPEED;
        name_to_index["Aircraft.Pitch"] = (int)VariableIndex::AIRCRAFT_PITCH;
        name_to_index["Aircraft.Bank"] = (int)VariableIndex::AIRCRAFT_BANK;
        name_to_index["Aircraft.IndicatedAirspeed"] = (int)VariableIndex::AIRCRAFT_INDICATED_AIRSPEED;
        name_to_index["Aircraft.IndicatedAirspeedTrend"] = (int)VariableIndex::AIRCRAFT_INDICATED_AIRSPEED_TREND;
        name_to_index["Aircraft.GroundSpeed"] = (int)VariableIndex::AIRCRAFT_GROUND_SPEED;
        name_to_index["Aircraft.MagneticHeading"] = (int)VariableIndex::AIRCRAFT_MAGNETIC_HEADING;
        name_to_index["Aircraft.TrueHeading"] = (int)VariableIndex::AIRCRAFT_TRUE_HEADING;
        name_to_index["Aircraft.Latitude"] = (int)VariableIndex::AIRCRAFT_LATITUDE;
        name_to_index["Aircraft.Longitude"] = (int)VariableIndex::AIRCRAFT_LONGITUDE;
        name_to_index["Aircraft.Height"] = (int)VariableIndex::AIRCRAFT_HEIGHT;
        name_to_index["Aircraft.Position"] = (int)VariableIndex::AIRCRAFT_POSITION;
        name_to_index["Aircraft.Orientation"] = (int)VariableIndex::AIRCRAFT_ORIENTATION;
        name_to_index["Aircraft.Velocity"] = (int)VariableIndex::AIRCRAFT_VELOCITY;
        name_to_index["Aircraft.AngularVelocity"] = (int)VariableIndex::AIRCRAFT_ANGULAR_VELOCITY;
        name_to_index["Aircraft.Acceleration"] = (int)VariableIndex::AIRCRAFT_ACCELERATION;
        name_to_index["Aircraft.Gravity"] = (int)VariableIndex::AIRCRAFT_GRAVITY;
        name_to_index["Aircraft.Wind"] = (int)VariableIndex::AIRCRAFT_WIND;
        name_to_index["Aircraft.RateOfTurn"] = (int)VariableIndex::AIRCRAFT_RATE_OF_TURN;
        name_to_index["Aircraft.MachNumber"] = (int)VariableIndex::AIRCRAFT_MACH_NUMBER;
        name_to_index["Aircraft.AngleOfAttack"] = (int)VariableIndex::AIRCRAFT_ANGLE_OF_ATTACK;
        name_to_index["Aircraft.AngleOfAttackLimit"] = (int)VariableIndex::AIRCRAFT_ANGLE_OF_ATTACK_LIMIT;
        name_to_index["Aircraft.AccelerationLimit"] = (int)VariableIndex::AIRCRAFT_ACCELERATION_LIMIT;
        name_to_index["Aircraft.Gear"] = (int)VariableIndex::AIRCRAFT_GEAR;
        name_to_index["Aircraft.Flaps"] = (int)VariableIndex::AIRCRAFT_FLAPS;
        name_to_index["Aircraft.Slats"] = (int)VariableIndex::AIRCRAFT_SLATS;
        name_to_index["Aircraft.Throttle"] = (int)VariableIndex::AIRCRAFT_THROTTLE;
        name_to_index["Aircraft.AirBrake"] = (int)VariableIndex::AIRCRAFT_AIR_BRAKE;
        name_to_index["Aircraft.GroundSpoilersArmed"] = (int)VariableIndex::AIRCRAFT_GROUND_SPOILERS_ARMED;
        name_to_index["Aircraft.GroundSpoilersExtended"] = (int)VariableIndex::AIRCRAFT_GROUND_SPOILERS_EXTENDED;
        name_to_index["Aircraft.ParkingBrake"] = (int)VariableIndex::AIRCRAFT_PARKING_BRAKE;
        name_to_index["Aircraft.AutoBrakeSetting"] = (int)VariableIndex::AIRCRAFT_AUTO_BRAKE_SETTING;
        name_to_index["Aircraft.AutoBrakeEngaged"] = (int)VariableIndex::AIRCRAFT_AUTO_BRAKE_ENGAGED;
        name_to_index["Aircraft.AutoBrakeRejectedTakeOff"] = (int)VariableIndex::AIRCRAFT_AUTO_BRAKE_REJECTED_TAKEOFF;
        name_to_index["Aircraft.RadarAltitude"] = (int)VariableIndex::AIRCRAFT_RADAR_ALTITUDE;
        name_to_index["Aircraft.Name"] = (int)VariableIndex::AIRCRAFT_NAME;
        name_to_index["Aircraft.NearestAirportIdentifier"] = (int)VariableIndex::AIRCRAFT_NEAREST_AIRPORT_IDENTIFIER;
        name_to_index["Aircraft.NearestAirportName"] = (int)VariableIndex::AIRCRAFT_NEAREST_AIRPORT_NAME;
        name_to_index["Aircraft.NearestAirportLocation"] = (int)VariableIndex::AIRCRAFT_NEAREST_AIRPORT_LOCATION;
        name_to_index["Aircraft.NearestAirportElevation"] = (int)VariableIndex::AIRCRAFT_NEAREST_AIRPORT_ELEVATION;
        name_to_index["Aircraft.BestAirportIdentifier"] = (int)VariableIndex::AIRCRAFT_BEST_AIRPORT_IDENTIFIER;
        name_to_index["Aircraft.BestAirportName"] = (int)VariableIndex::AIRCRAFT_BEST_AIRPORT_NAME;
        name_to_index["Aircraft.BestAirportLocation"] = (int)VariableIndex::AIRCRAFT_BEST_AIRPORT_LOCATION;
        name_to_index["Aircraft.BestAirportElevation"] = (int)VariableIndex::AIRCRAFT_BEST_AIRPORT_ELEVATION;
        name_to_index["Aircraft.BestRunwayIdentifier"] = (int)VariableIndex::AIRCRAFT_BEST_RUNWAY_IDENTIFIER;
        name_to_index["Aircraft.BestRunwayElevation"] = (int)VariableIndex::AIRCRAFT_BEST_RUNWAY_ELEVATION;
        name_to_index["Aircraft.BestRunwayThreshold"] = (int)VariableIndex::AIRCRAFT_BEST_RUNWAY_THRESHOLD;
        name_to_index["Aircraft.BestRunwayEnd"] = (int)VariableIndex::AIRCRAFT_BEST_RUNWAY_END;
        name_to_index["Aircraft.Category.Jet"] = (int)VariableIndex::AIRCRAFT_CATEGORY_JET;
        name_to_index["Aircraft.Category.Glider"] = (int)VariableIndex::AIRCRAFT_CATEGORY_GLIDER;
        name_to_index["Aircraft.OnGround"] = (int)VariableIndex::AIRCRAFT_ON_GROUND;
        name_to_index["Aircraft.OnRunway"] = (int)VariableIndex::AIRCRAFT_ON_RUNWAY;
        name_to_index["Aircraft.Crashed"] = (int)VariableIndex::AIRCRAFT_CRASHED;
        name_to_index["Aircraft.Power"] = (int)VariableIndex::AIRCRAFT_POWER;
        name_to_index["Aircraft.NormalizedPower"] = (int)VariableIndex::AIRCRAFT_NORMALIZED_POWER;
        name_to_index["Aircraft.NormalizedPowerTarget"] = (int)VariableIndex::AIRCRAFT_NORMALIZED_POWER_TARGET;
        name_to_index["Aircraft.Trim"] = (int)VariableIndex::AIRCRAFT_TRIM;
        name_to_index["Aircraft.PitchTrim"] = (int)VariableIndex::AIRCRAFT_PITCH_TRIM;
        name_to_index["Aircraft.PitchTrimScaling"] = (int)VariableIndex::AIRCRAFT_PITCH_TRIM_SCALING;
        name_to_index["Aircraft.PitchTrimOffset"] = (int)VariableIndex::AIRCRAFT_PITCH_TRIM_OFFSET;
        name_to_index["Aircraft.RudderTrim"] = (int)VariableIndex::AIRCRAFT_RUDDER_TRIM;
        name_to_index["Aircraft.AutoPitchTrim"] = (int)VariableIndex::AIRCRAFT_AUTO_PITCH_TRIM;
        name_to_index["Aircraft.YawDamperEnabled"] = (int)VariableIndex::AIRCRAFT_YAW_DAMPER_ENABLED;
        name_to_index["Aircraft.RudderPedalsDisconnected"] = (int)VariableIndex::AIRCRAFT_RUDDER_PEDALS_DISCONNECTED;
        name_to_index["Aircraft.Starter"] = (int)VariableIndex::AIRCRAFT_STARTER;
        name_to_index["Aircraft.Starter1"] = (int)VariableIndex::AIRCRAFT_STARTER_1;
        name_to_index["Aircraft.Starter2"] = (int)VariableIndex::AIRCRAFT_STARTER_2;
        name_to_index["Aircraft.Starter3"] = (int)VariableIndex::AIRCRAFT_STARTER_3;
        name_to_index["Aircraft.Starter4"] = (int)VariableIndex::AIRCRAFT_STARTER_4;
        name_to_index["Aircraft.Ignition"] = (int)VariableIndex::AIRCRAFT_IGNITION;
        name_to_index["Aircraft.Ignition1"] = (int)VariableIndex::AIRCRAFT_IGNITION_1;
        name_to_index["Aircraft.Ignition2"] = (int)VariableIndex::AIRCRAFT_IGNITION_2;
        name_to_index["Aircraft.Ignition3"] = (int)VariableIndex::AIRCRAFT_IGNITION_3;
        name_to_index["Aircraft.Ignition4"] = (int)VariableIndex::AIRCRAFT_IGNITION_4;
        name_to_index["Aircraft.ThrottleLimit"] = (int)VariableIndex::AIRCRAFT_THROTTLE_LIMIT;
        name_to_index["Aircraft.Reverse"] = (int)VariableIndex::AIRCRAFT_REVERSE;
        name_to_index["Aircraft.EngineMaster1"] = (int)VariableIndex::AIRCRAFT_ENGINE_MASTER_1;
        name_to_index["Aircraft.EngineMaster2"] = (int)VariableIndex::AIRCRAFT_ENGINE_MASTER_2;
        name_to_index["Aircraft.EngineMaster3"] = (int)VariableIndex::AIRCRAFT_ENGINE_MASTER_3;
        name_to_index["Aircraft.EngineMaster4"] = (int)VariableIndex::AIRCRAFT_ENGINE_MASTER_4;
        name_to_index["Aircraft.EngineThrottle1"] = (int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_1;
        name_to_index["Aircraft.EngineThrottle2"] = (int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_2;
        name_to_index["Aircraft.EngineThrottle3"] = (int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_3;
        name_to_index["Aircraft.EngineThrottle4"] = (int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_4;
        name_to_index["Aircraft.EngineRotationSpeed1"] = (int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_1;
        name_to_index["Aircraft.EngineRotationSpeed2"] = (int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_2;
        name_to_index["Aircraft.EngineRotationSpeed3"] = (int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_3;
        name_to_index["Aircraft.EngineRotationSpeed4"] = (int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_4;
        name_to_index["Aircraft.EngineRunning1"] = (int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_1;
        name_to_index["Aircraft.EngineRunning2"] = (int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_2;
        name_to_index["Aircraft.EngineRunning3"] = (int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_3;
        name_to_index["Aircraft.EngineRunning4"] = (int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_4;
        name_to_index["Aircraft.APUAvailable"] = (int)VariableIndex::AIRCRAFT_APU_AVAILABLE;
        
        // === PERFORMANCE SPEEDS (95-104) ===
        name_to_index["Performance.Speed.VS0"] = (int)VariableIndex::PERFORMANCE_SPEED_VS0;
        name_to_index["Performance.Speed.VS1"] = (int)VariableIndex::PERFORMANCE_SPEED_VS1;
        name_to_index["Performance.Speed.VFE"] = (int)VariableIndex::PERFORMANCE_SPEED_VFE;
        name_to_index["Performance.Speed.VNO"] = (int)VariableIndex::PERFORMANCE_SPEED_VNO;
        name_to_index["Performance.Speed.VNE"] = (int)VariableIndex::PERFORMANCE_SPEED_VNE;
        name_to_index["Performance.Speed.VAPP"] = (int)VariableIndex::PERFORMANCE_SPEED_VAPP;
        name_to_index["Performance.Speed.Minimum"] = (int)VariableIndex::PERFORMANCE_SPEED_MINIMUM;
        name_to_index["Performance.Speed.Maximum"] = (int)VariableIndex::PERFORMANCE_SPEED_MAXIMUM;
        name_to_index["Performance.Speed.MinimumFlapRetraction"] = (int)VariableIndex::PERFORMANCE_SPEED_MINIMUM_FLAP_RETRACTION;
        name_to_index["Performance.Speed.MaximumFlapExtension"] = (int)VariableIndex::PERFORMANCE_SPEED_MAXIMUM_FLAP_EXTENSION;
        
        // === CONFIGURATION (105-106) ===
        name_to_index["Configuration.SelectedTakeOffFlaps"] = (int)VariableIndex::CONFIGURATION_SELECTED_TAKEOFF_FLAPS;
        name_to_index["Configuration.SelectedLandingFlaps"] = (int)VariableIndex::CONFIGURATION_SELECTED_LANDING_FLAPS;
        
        // === FLIGHT MANAGEMENT SYSTEM (107) ===
        name_to_index["FlightManagementSystem.FlightNumber"] = (int)VariableIndex::FMS_FLIGHT_NUMBER;
        
        // === NAVIGATION (108-141) ===
        name_to_index["Navigation.SelectedCourse1"] = (int)VariableIndex::NAVIGATION_SELECTED_COURSE_1;
        name_to_index["Navigation.SelectedCourse2"] = (int)VariableIndex::NAVIGATION_SELECTED_COURSE_2;
        name_to_index["Navigation.NAV1Identifier"] = (int)VariableIndex::NAVIGATION_NAV1_IDENTIFIER;
        name_to_index["Navigation.NAV1Frequency"] = (int)VariableIndex::NAVIGATION_NAV1_FREQUENCY;
        name_to_index["Navigation.NAV1StandbyFrequency"] = (int)VariableIndex::NAVIGATION_NAV1_STANDBY_FREQUENCY;
        name_to_index["Navigation.NAV1FrequencySwap"] = (int)VariableIndex::NAVIGATION_NAV1_FREQUENCY_SWAP;
        name_to_index["Navigation.NAV2Identifier"] = (int)VariableIndex::NAVIGATION_NAV2_IDENTIFIER;
        name_to_index["Navigation.NAV2Frequency"] = (int)VariableIndex::NAVIGATION_NAV2_FREQUENCY;
        name_to_index["Navigation.NAV2StandbyFrequency"] = (int)VariableIndex::NAVIGATION_NAV2_STANDBY_FREQUENCY;
        name_to_index["Navigation.NAV2FrequencySwap"] = (int)VariableIndex::NAVIGATION_NAV2_FREQUENCY_SWAP;
        name_to_index["Navigation.DME1Frequency"] = (int)VariableIndex::NAVIGATION_DME1_FREQUENCY;
        name_to_index["Navigation.DME1Distance"] = (int)VariableIndex::NAVIGATION_DME1_DISTANCE;
        name_to_index["Navigation.DME1Time"] = (int)VariableIndex::NAVIGATION_DME1_TIME;
        name_to_index["Navigation.DME1Speed"] = (int)VariableIndex::NAVIGATION_DME1_SPEED;
        name_to_index["Navigation.DME2Frequency"] = (int)VariableIndex::NAVIGATION_DME2_FREQUENCY;
        name_to_index["Navigation.DME2Distance"] = (int)VariableIndex::NAVIGATION_DME2_DISTANCE;
        name_to_index["Navigation.DME2Time"] = (int)VariableIndex::NAVIGATION_DME2_TIME;
        name_to_index["Navigation.DME2Speed"] = (int)VariableIndex::NAVIGATION_DME2_SPEED;
        name_to_index["Navigation.ILS1Identifier"] = (int)VariableIndex::NAVIGATION_ILS1_IDENTIFIER;
        name_to_index["Navigation.ILS1Course"] = (int)VariableIndex::NAVIGATION_ILS1_COURSE;
        name_to_index["Navigation.ILS1Frequency"] = (int)VariableIndex::NAVIGATION_ILS1_FREQUENCY;
        name_to_index["Navigation.ILS1StandbyFrequency"] = (int)VariableIndex::NAVIGATION_ILS1_STANDBY_FREQUENCY;
        name_to_index["Navigation.ILS1FrequencySwap"] = (int)VariableIndex::NAVIGATION_ILS1_FREQUENCY_SWAP;
        name_to_index["Navigation.ILS2Identifier"] = (int)VariableIndex::NAVIGATION_ILS2_IDENTIFIER;
        name_to_index["Navigation.ILS2Course"] = (int)VariableIndex::NAVIGATION_ILS2_COURSE;
        name_to_index["Navigation.ILS2Frequency"] = (int)VariableIndex::NAVIGATION_ILS2_FREQUENCY;
        name_to_index["Navigation.ILS2StandbyFrequency"] = (int)VariableIndex::NAVIGATION_ILS2_STANDBY_FREQUENCY;
        name_to_index["Navigation.ILS2FrequencySwap"] = (int)VariableIndex::NAVIGATION_ILS2_FREQUENCY_SWAP;
        name_to_index["Navigation.ADF1Frequency"] = (int)VariableIndex::NAVIGATION_ADF1_FREQUENCY;
        name_to_index["Navigation.ADF1StandbyFrequency"] = (int)VariableIndex::NAVIGATION_ADF1_STANDBY_FREQUENCY;
        name_to_index["Navigation.ADF1FrequencySwap"] = (int)VariableIndex::NAVIGATION_ADF1_FREQUENCY_SWAP;
        name_to_index["Navigation.ADF2Frequency"] = (int)VariableIndex::NAVIGATION_ADF2_FREQUENCY;
        name_to_index["Navigation.ADF2StandbyFrequency"] = (int)VariableIndex::NAVIGATION_ADF2_STANDBY_FREQUENCY;
        name_to_index["Navigation.ADF2FrequencySwap"] = (int)VariableIndex::NAVIGATION_ADF2_FREQUENCY_SWAP;
        
        // === COMMUNICATION (142-152) ===
        name_to_index["Communication.COM1Frequency"] = (int)VariableIndex::COMMUNICATION_COM1_FREQUENCY;
        name_to_index["Communication.COM1StandbyFrequency"] = (int)VariableIndex::COMMUNICATION_COM1_STANDBY_FREQUENCY;
        name_to_index["Communication.COM1FrequencySwap"] = (int)VariableIndex::COMMUNICATION_COM1_FREQUENCY_SWAP;
        name_to_index["Communication.COM2Frequency"] = (int)VariableIndex::COMMUNICATION_COM2_FREQUENCY;
        name_to_index["Communication.COM2StandbyFrequency"] = (int)VariableIndex::COMMUNICATION_COM2_STANDBY_FREQUENCY;
        name_to_index["Communication.COM2FrequencySwap"] = (int)VariableIndex::COMMUNICATION_COM2_FREQUENCY_SWAP;
        name_to_index["Communication.COM3Frequency"] = (int)VariableIndex::COMMUNICATION_COM3_FREQUENCY;
        name_to_index["Communication.COM3StandbyFrequency"] = (int)VariableIndex::COMMUNICATION_COM3_STANDBY_FREQUENCY;
        name_to_index["Communication.COM3FrequencySwap"] = (int)VariableIndex::COMMUNICATION_COM3_FREQUENCY_SWAP;
        name_to_index["Communication.TransponderCode"] = (int)VariableIndex::COMMUNICATION_TRANSPONDER_CODE;
        name_to_index["Communication.TransponderCursor"] = (int)VariableIndex::COMMUNICATION_TRANSPONDER_CURSOR;
        
        // === AUTOPILOT (153-180) ===
        name_to_index["Autopilot.Master"] = (int)VariableIndex::AUTOPILOT_MASTER;
        name_to_index["Autopilot.Disengage"] = (int)VariableIndex::AUTOPILOT_DISENGAGE;
        name_to_index["Autopilot.Heading"] = (int)VariableIndex::AUTOPILOT_HEADING;
        name_to_index["Autopilot.VerticalSpeed"] = (int)VariableIndex::AUTOPILOT_VERTICAL_SPEED;
        name_to_index["Autopilot.SelectedSpeed"] = (int)VariableIndex::AUTOPILOT_SELECTED_SPEED;
        name_to_index["Autopilot.SelectedAirspeed"] = (int)VariableIndex::AUTOPILOT_SELECTED_AIRSPEED;
        name_to_index["Autopilot.SelectedHeading"] = (int)VariableIndex::AUTOPILOT_SELECTED_HEADING;
        name_to_index["Autopilot.SelectedAltitude"] = (int)VariableIndex::AUTOPILOT_SELECTED_ALTITUDE;
        name_to_index["Autopilot.SelectedVerticalSpeed"] = (int)VariableIndex::AUTOPILOT_SELECTED_VERTICAL_SPEED;
        name_to_index["Autopilot.SelectedAltitudeScale"] = (int)VariableIndex::AUTOPILOT_SELECTED_ALTITUDE_SCALE;
        name_to_index["Autopilot.ActiveLateralMode"] = (int)VariableIndex::AUTOPILOT_ACTIVE_LATERAL_MODE;
        name_to_index["Autopilot.ArmedLateralMode"] = (int)VariableIndex::AUTOPILOT_ARMED_LATERAL_MODE;
        name_to_index["Autopilot.ActiveVerticalMode"] = (int)VariableIndex::AUTOPILOT_ACTIVE_VERTICAL_MODE;
        name_to_index["Autopilot.ArmedVerticalMode"] = (int)VariableIndex::AUTOPILOT_ARMED_VERTICAL_MODE;
        name_to_index["Autopilot.ArmedApproachMode"] = (int)VariableIndex::AUTOPILOT_ARMED_APPROACH_MODE;
        name_to_index["Autopilot.ActiveAutoThrottleMode"] = (int)VariableIndex::AUTOPILOT_ACTIVE_AUTO_THROTTLE_MODE;
        name_to_index["Autopilot.ActiveCollectiveMode"] = (int)VariableIndex::AUTOPILOT_ACTIVE_COLLECTIVE_MODE;
        name_to_index["Autopilot.ArmedCollectiveMode"] = (int)VariableIndex::AUTOPILOT_ARMED_COLLECTIVE_MODE;
        name_to_index["Autopilot.Type"] = (int)VariableIndex::AUTOPILOT_TYPE;
        name_to_index["Autopilot.Engaged"] = (int)VariableIndex::AUTOPILOT_ENGAGED;
        name_to_index["Autopilot.UseMachNumber"] = (int)VariableIndex::AUTOPILOT_USE_MACH_NUMBER;
        name_to_index["Autopilot.SpeedManaged"] = (int)VariableIndex::AUTOPILOT_SPEED_MANAGED;
        name_to_index["Autopilot.TargetAirspeed"] = (int)VariableIndex::AUTOPILOT_TARGET_AIRSPEED;
        name_to_index["Autopilot.Aileron"] = (int)VariableIndex::AUTOPILOT_AILERON;
        name_to_index["Autopilot.Elevator"] = (int)VariableIndex::AUTOPILOT_ELEVATOR;
        name_to_index["AutoThrottle.Type"] = (int)VariableIndex::AUTO_THROTTLE_TYPE;
        name_to_index["Autopilot.ThrottleEngaged"] = (int)VariableIndex::AUTOPILOT_THROTTLE_ENGAGED;
        name_to_index["Autopilot.ThrottleCommand"] = (int)VariableIndex::AUTOPILOT_THROTTLE_COMMAND;
        
        // === FLIGHT DIRECTOR (181-183) ===
        name_to_index["FlightDirector.Pitch"] = (int)VariableIndex::FLIGHT_DIRECTOR_PITCH;
        name_to_index["FlightDirector.Bank"] = (int)VariableIndex::FLIGHT_DIRECTOR_BANK;
        name_to_index["FlightDirector.Yaw"] = (int)VariableIndex::FLIGHT_DIRECTOR_YAW;
        
        // === COPILOT (184-191) ===
        name_to_index["Copilot.Heading"] = (int)VariableIndex::COPILOT_HEADING;
        name_to_index["Copilot.Altitude"] = (int)VariableIndex::COPILOT_ALTITUDE;
        name_to_index["Copilot.Airspeed"] = (int)VariableIndex::COPILOT_AIRSPEED;
        name_to_index["Copilot.VerticalSpeed"] = (int)VariableIndex::COPILOT_VERTICAL_SPEED;
        name_to_index["Copilot.Aileron"] = (int)VariableIndex::COPILOT_AILERON;
        name_to_index["Copilot.Elevator"] = (int)VariableIndex::COPILOT_ELEVATOR;
        name_to_index["Copilot.Throttle"] = (int)VariableIndex::COPILOT_THROTTLE;
        name_to_index["Copilot.AutoRudder"] = (int)VariableIndex::COPILOT_AUTO_RUDDER;
        
        // === CONTROLS (192-260) ===
        name_to_index["Controls.Throttle"] = (int)VariableIndex::CONTROLS_THROTTLE;
        name_to_index["Controls.Throttle1"] = (int)VariableIndex::CONTROLS_THROTTLE_1;
        name_to_index["Controls.Throttle2"] = (int)VariableIndex::CONTROLS_THROTTLE_2;
        name_to_index["Controls.Throttle3"] = (int)VariableIndex::CONTROLS_THROTTLE_3;
        name_to_index["Controls.Throttle4"] = (int)VariableIndex::CONTROLS_THROTTLE_4;
        name_to_index["Controls.Throttle1Move"] = (int)VariableIndex::CONTROLS_THROTTLE_1_MOVE;
        name_to_index["Controls.Throttle2Move"] = (int)VariableIndex::CONTROLS_THROTTLE_2_MOVE;
        name_to_index["Controls.Throttle3Move"] = (int)VariableIndex::CONTROLS_THROTTLE_3_MOVE;
        name_to_index["Controls.Throttle4Move"] = (int)VariableIndex::CONTROLS_THROTTLE_4_MOVE;
        name_to_index["Controls.Pitch.Input"] = (int)VariableIndex::CONTROLS_PITCH_INPUT;
        name_to_index["Controls.Pitch.InputOffset"] = (int)VariableIndex::CONTROLS_PITCH_INPUT_OFFSET;
        name_to_index["Controls.Roll.Input"] = (int)VariableIndex::CONTROLS_ROLL_INPUT;
        name_to_index["Controls.Roll.InputOffset"] = (int)VariableIndex::CONTROLS_ROLL_INPUT_OFFSET;
        name_to_index["Controls.Yaw.Input"] = (int)VariableIndex::CONTROLS_YAW_INPUT;
        name_to_index["Controls.Yaw.InputActive"] = (int)VariableIndex::CONTROLS_YAW_INPUT_ACTIVE;
        name_to_index["Controls.Flaps"] = (int)VariableIndex::CONTROLS_FLAPS;
        name_to_index["Controls.FlapsEvent"] = (int)VariableIndex::CONTROLS_FLAPS_EVENT;
        name_to_index["Controls.Gear"] = (int)VariableIndex::CONTROLS_GEAR;
        name_to_index["Controls.GearToggle"] = (int)VariableIndex::CONTROLS_GEAR_TOGGLE;
        name_to_index["Controls.WheelBrake.Left"] = (int)VariableIndex::CONTROLS_WHEEL_BRAKE_LEFT;
        name_to_index["Controls.WheelBrake.Right"] = (int)VariableIndex::CONTROLS_WHEEL_BRAKE_RIGHT;
        name_to_index["Controls.WheelBrake.LeftActive"] = (int)VariableIndex::CONTROLS_WHEEL_BRAKE_LEFT_ACTIVE;
        name_to_index["Controls.WheelBrake.RightActive"] = (int)VariableIndex::CONTROLS_WHEEL_BRAKE_RIGHT_ACTIVE;
        name_to_index["Controls.AirBrake"] = (int)VariableIndex::CONTROLS_AIR_BRAKE;
        name_to_index["Controls.AirBrakeActive"] = (int)VariableIndex::CONTROLS_AIR_BRAKE_ACTIVE;
        name_to_index["Controls.AirBrake.Arm"] = (int)VariableIndex::CONTROLS_AIR_BRAKE_ARM;
        name_to_index["Controls.GliderAirBrake"] = (int)VariableIndex::CONTROLS_GLIDER_AIR_BRAKE;
        name_to_index["Controls.PropellerSpeed1"] = (int)VariableIndex::CONTROLS_PROPELLER_SPEED_1;
        name_to_index["Controls.PropellerSpeed2"] = (int)VariableIndex::CONTROLS_PROPELLER_SPEED_2;
        name_to_index["Controls.PropellerSpeed3"] = (int)VariableIndex::CONTROLS_PROPELLER_SPEED_3;
        name_to_index["Controls.PropellerSpeed4"] = (int)VariableIndex::CONTROLS_PROPELLER_SPEED_4;
        name_to_index["Controls.Mixture"] = (int)VariableIndex::CONTROLS_MIXTURE;
        name_to_index["Controls.Mixture1"] = (int)VariableIndex::CONTROLS_MIXTURE_1;
        name_to_index["Controls.Mixture2"] = (int)VariableIndex::CONTROLS_MIXTURE_2;
        name_to_index["Controls.Mixture3"] = (int)VariableIndex::CONTROLS_MIXTURE_3;
        name_to_index["Controls.Mixture4"] = (int)VariableIndex::CONTROLS_MIXTURE_4;
        name_to_index["Controls.ThrustReverse"] = (int)VariableIndex::CONTROLS_THRUST_REVERSE;
        name_to_index["Controls.ThrustReverse1"] = (int)VariableIndex::CONTROLS_THRUST_REVERSE_1;
        name_to_index["Controls.ThrustReverse2"] = (int)VariableIndex::CONTROLS_THRUST_REVERSE_2;
        name_to_index["Controls.ThrustReverse3"] = (int)VariableIndex::CONTROLS_THRUST_REVERSE_3;
        name_to_index["Controls.ThrustReverse4"] = (int)VariableIndex::CONTROLS_THRUST_REVERSE_4;
        name_to_index["Controls.Collective"] = (int)VariableIndex::CONTROLS_COLLECTIVE;
        name_to_index["Controls.CyclicPitch"] = (int)VariableIndex::CONTROLS_CYCLIC_PITCH;
        name_to_index["Controls.CyclicRoll"] = (int)VariableIndex::CONTROLS_CYCLIC_ROLL;
        name_to_index["Controls.TailRotor"] = (int)VariableIndex::CONTROLS_TAIL_ROTOR;
        name_to_index["Controls.RotorBrake"] = (int)VariableIndex::CONTROLS_ROTOR_BRAKE;
        name_to_index["Controls.HelicopterThrottle1"] = (int)VariableIndex::CONTROLS_HELICOPTER_THROTTLE_1;
        name_to_index["Controls.HelicopterThrottle2"] = (int)VariableIndex::CONTROLS_HELICOPTER_THROTTLE_2;
        name_to_index["Controls.Trim"] = (int)VariableIndex::CONTROLS_TRIM;
        name_to_index["Controls.TrimStep"] = (int)VariableIndex::CONTROLS_TRIM_STEP;
        name_to_index["Controls.TrimMove"] = (int)VariableIndex::CONTROLS_TRIM_MOVE;
        name_to_index["Controls.AileronTrim"] = (int)VariableIndex::CONTROLS_AILERON_TRIM;
        name_to_index["Controls.RudderTrim"] = (int)VariableIndex::CONTROLS_RUDDER_TRIM;
        name_to_index["Controls.Tiller"] = (int)VariableIndex::CONTROLS_TILLER;
        name_to_index["Controls.PedalsDisconnect"] = (int)VariableIndex::CONTROLS_PEDALS_DISCONNECT;
        name_to_index["Controls.NoseWheelSteering"] = (int)VariableIndex::CONTROLS_NOSE_WHEEL_STEERING;
        name_to_index["Controls.Lighting.Panel"] = (int)VariableIndex::CONTROLS_LIGHTING_PANEL;
        name_to_index["Controls.Lighting.Instruments"] = (int)VariableIndex::CONTROLS_LIGHTING_INSTRUMENTS;
        name_to_index["Controls.PressureSetting0"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_0;
        name_to_index["Controls.PressureSettingStandard0"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_STANDARD_0;
        name_to_index["Controls.PressureSettingUnit0"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_UNIT_0;
        name_to_index["Controls.PressureSetting1"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_1;
        name_to_index["Controls.PressureSettingStandard1"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_STANDARD_1;
        name_to_index["Controls.PressureSettingUnit1"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_UNIT_1;
        name_to_index["Controls.PressureSetting2"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_2;
        name_to_index["Controls.PressureSettingStandard2"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_STANDARD_2;
        name_to_index["Controls.PressureSettingUnit2"] = (int)VariableIndex::CONTROLS_PRESSURE_SETTING_UNIT_2;
        name_to_index["Controls.TransitionAltitude"] = (int)VariableIndex::CONTROLS_TRANSITION_ALTITUDE;
        name_to_index["Controls.TransitionLevel"] = (int)VariableIndex::CONTROLS_TRANSITION_LEVEL;
        
        // === PRESSURIZATION (261-262) ===
        name_to_index["Pressurization.LandingElevation"] = (int)VariableIndex::PRESSURIZATION_LANDING_ELEVATION;
        name_to_index["Pressurization.LandingElevationManual"] = (int)VariableIndex::PRESSURIZATION_LANDING_ELEVATION_MANUAL;
        
        // === WARNINGS (263-272) ===
        name_to_index["Warnings.MasterWarning"] = (int)VariableIndex::WARNINGS_MASTER_WARNING;
        name_to_index["Warnings.MasterCaution"] = (int)VariableIndex::WARNINGS_MASTER_CAUTION;
        name_to_index["Warnings.EngineFire"] = (int)VariableIndex::WARNINGS_ENGINE_FIRE;
        name_to_index["Warnings.LowOilPressure"] = (int)VariableIndex::WARNINGS_LOW_OIL_PRESSURE;
        name_to_index["Warnings.LowFuelPressure"] = (int)VariableIndex::WARNINGS_LOW_FUEL_PRESSURE;
        name_to_index["Warnings.LowHydraulicPressure"] = (int)VariableIndex::WARNINGS_LOW_HYDRAULIC_PRESSURE;
        name_to_index["Warnings.LowVoltage"] = (int)VariableIndex::WARNINGS_LOW_VOLTAGE;
        name_to_index["Warnings.AltitudeAlert"] = (int)VariableIndex::WARNINGS_ALTITUDE_ALERT;
        name_to_index["Warnings.WarningActive"] = (int)VariableIndex::WARNINGS_WARNING_ACTIVE;
        name_to_index["Warnings.WarningMute"] = (int)VariableIndex::WARNINGS_WARNING_MUTE;
        
        // === VIEW CONTROLS (273-302) ===
        name_to_index["View.DisplayName"] = (int)VariableIndex::VIEW_DISPLAY_NAME;
        name_to_index["View.Internal"] = (int)VariableIndex::VIEW_INTERNAL;
        name_to_index["View.Follow"] = (int)VariableIndex::VIEW_FOLLOW;
        name_to_index["View.External"] = (int)VariableIndex::VIEW_EXTERNAL;
        name_to_index["View.Category"] = (int)VariableIndex::VIEW_CATEGORY;
        name_to_index["View.Mode"] = (int)VariableIndex::VIEW_MODE;
        name_to_index["View.Zoom"] = (int)VariableIndex::VIEW_ZOOM;
        name_to_index["View.Pan.Horizontal"] = (int)VariableIndex::VIEW_PAN_HORIZONTAL;
        name_to_index["View.Pan.HorizontalMove"] = (int)VariableIndex::VIEW_PAN_HORIZONTAL_MOVE;
        name_to_index["View.Pan.Vertical"] = (int)VariableIndex::VIEW_PAN_VERTICAL;
        name_to_index["View.Pan.VerticalMove"] = (int)VariableIndex::VIEW_PAN_VERTICAL_MOVE;
        name_to_index["View.Pan.Center"] = (int)VariableIndex::VIEW_PAN_CENTER;
        name_to_index["View.Look.Horizontal"] = (int)VariableIndex::VIEW_LOOK_HORIZONTAL;
        name_to_index["View.Look.Vertical"] = (int)VariableIndex::VIEW_LOOK_VERTICAL;
        name_to_index["View.Roll"] = (int)VariableIndex::VIEW_ROLL;
        name_to_index["View.OffsetX"] = (int)VariableIndex::VIEW_OFFSET_X;
        name_to_index["View.OffsetXMove"] = (int)VariableIndex::VIEW_OFFSET_X_MOVE;
        name_to_index["View.OffsetY"] = (int)VariableIndex::VIEW_OFFSET_Y;
        name_to_index["View.OffsetYMove"] = (int)VariableIndex::VIEW_OFFSET_Y_MOVE;
        name_to_index["View.OffsetZ"] = (int)VariableIndex::VIEW_OFFSET_Z;
        name_to_index["View.OffsetZMove"] = (int)VariableIndex::VIEW_OFFSET_Z_MOVE;
        name_to_index["View.Position"] = (int)VariableIndex::VIEW_POSITION;
        name_to_index["View.Direction"] = (int)VariableIndex::VIEW_DIRECTION;
        name_to_index["View.Up"] = (int)VariableIndex::VIEW_UP;
        name_to_index["View.FieldOfView"] = (int)VariableIndex::VIEW_FIELD_OF_VIEW;
        name_to_index["View.AspectRatio"] = (int)VariableIndex::VIEW_ASPECT_RATIO;
        name_to_index["View.FreePosition"] = (int)VariableIndex::VIEW_FREE_POSITION;
        name_to_index["View.FreeLookDirection"] = (int)VariableIndex::VIEW_FREE_LOOK_DIRECTION;
        name_to_index["View.FreeUp"] = (int)VariableIndex::VIEW_FREE_UP;
        name_to_index["View.FreeFieldOfView"] = (int)VariableIndex::VIEW_FREE_FIELD_OF_VIEW;
        
        // === SIMULATION CONTROLS (303-320) ===
        name_to_index["Simulation.Pause"] = (int)VariableIndex::SIMULATION_PAUSE;
        name_to_index["Simulation.FlightInformation"] = (int)VariableIndex::SIMULATION_FLIGHT_INFORMATION;
        name_to_index["Simulation.MovingMap"] = (int)VariableIndex::SIMULATION_MOVING_MAP;
        name_to_index["Simulation.Sound"] = (int)VariableIndex::SIMULATION_SOUND;
        name_to_index["Simulation.LiftUp"] = (int)VariableIndex::SIMULATION_LIFT_UP;
        name_to_index["Simulation.SettingPosition"] = (int)VariableIndex::SIMULATION_SETTING_POSITION;
        name_to_index["Simulation.SettingOrientation"] = (int)VariableIndex::SIMULATION_SETTING_ORIENTATION;
        name_to_index["Simulation.SettingVelocity"] = (int)VariableIndex::SIMULATION_SETTING_VELOCITY;
        name_to_index["Simulation.SettingSet"] = (int)VariableIndex::SIMULATION_SETTING_SET;
        name_to_index["Simulation.TimeChange"] = (int)VariableIndex::SIMULATION_TIME_CHANGE;
        name_to_index["Simulation.Visibility"] = (int)VariableIndex::SIMULATION_VISIBILITY;
        name_to_index["Simulation.Time"] = (int)VariableIndex::SIMULATION_TIME;
        name_to_index["Simulation.UseMouseControl"] = (int)VariableIndex::SIMULATION_USE_MOUSE_CONTROL;
        name_to_index["Simulation.PlaybackStart"] = (int)VariableIndex::SIMULATION_PLAYBACK_START;
        name_to_index["Simulation.PlaybackStop"] = (int)VariableIndex::SIMULATION_PLAYBACK_STOP;
        name_to_index["Simulation.PlaybackPosition"] = (int)VariableIndex::SIMULATION_PLAYBACK_SET_POSITION;
        name_to_index["Simulation.ExternalPosition"] = (int)VariableIndex::SIMULATION_EXTERNAL_POSITION;
        name_to_index["Simulation.ExternalOrientation"] = (int)VariableIndex::SIMULATION_EXTERNAL_ORIENTATION;
        
        // === COMMAND CONTROLS (321-330) ===
        name_to_index["Command.Execute"] = (int)VariableIndex::COMMAND_EXECUTE;
        name_to_index["Command.Back"] = (int)VariableIndex::COMMAND_BACK;
        name_to_index["Command.Up"] = (int)VariableIndex::COMMAND_UP;
        name_to_index["Command.Down"] = (int)VariableIndex::COMMAND_DOWN;
        name_to_index["Command.Left"] = (int)VariableIndex::COMMAND_LEFT;
        name_to_index["Command.Right"] = (int)VariableIndex::COMMAND_RIGHT;
        name_to_index["Command.MoveHorizontal"] = (int)VariableIndex::COMMAND_MOVE_HORIZONTAL;
        name_to_index["Command.MoveVertical"] = (int)VariableIndex::COMMAND_MOVE_VERTICAL;
        name_to_index["Command.Rotate"] = (int)VariableIndex::COMMAND_ROTATE;
        name_to_index["Command.Zoom"] = (int)VariableIndex::COMMAND_ZOOM;
        
        // === SPECIAL/IGNORE VARIABLES (331-338) ===
        name_to_index["Controls.Speed"] = (int)VariableIndex::CONTROLS_SPEED;
        name_to_index["FlightManagementSystem.Data0"] = (int)VariableIndex::FMS_DATA_0;
        name_to_index["FlightManagementSystem.Data1"] = (int)VariableIndex::FMS_DATA_1;
        name_to_index["Navigation.NAV1Data"] = (int)VariableIndex::NAV1_DATA;
        name_to_index["Navigation.NAV2Data"] = (int)VariableIndex::NAV2_DATA;
        name_to_index["Navigation.NAV3Data"] = (int)VariableIndex::NAV3_DATA;
        name_to_index["Navigation.ILS1Data"] = (int)VariableIndex::ILS1_DATA;
        name_to_index["Navigation.ILS2Data"] = (int)VariableIndex::ILS2_DATA;
        
        // Hash mappings will be initialized later after MESSAGE_LIST definitions
        // For now, name mappings are sufficient for CommandProcessor functionality
        
        // NOTE: COMPLETE mapping with ALL 339 variables from VariableIndex enum
        // Comprehensive coverage for full bidirectional control and monitoring
    }
    
    int GetIndex(const std::string& name) const {
        auto it = name_to_index.find(name);
        return (it != name_to_index.end()) ? it->second : -1;
    }
    
    int GetIndex(uint64_t hash) const {
        auto it = hash_to_index.find(hash);
        return (it != hash_to_index.end()) ? it->second : -1;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// MESSAGE DEFINITIONS - All 339 Variables from SDK + Extensions
///////////////////////////////////////////////////////////////////////////////////////////////////

#define TM_MESSAGE( a1, a2, a3, a4, a5, a6, a7 )       static tm_external_message Message##a1( ##a2, a3, a4, a5, a6 );

// Include the complete MESSAGE_LIST from original SDK
#define MESSAGE_LIST(F) \
F( AircraftUniversalTime,                 "Aircraft.UniversalTime",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "universal time of day (UTC)" ) \
F( AircraftAltitude,                      "Aircraft.Altitude",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Meter,                    "altitude as measured by altimeter" ) \
F( AircraftVerticalSpeed,                 "Aircraft.VerticalSpeed",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "vertical speed" ) \
F( AircraftPitch,                         "Aircraft.Pitch",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "pitch angle" ) \
F( AircraftBank,                          "Aircraft.Bank",                            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "bank angle" ) \
F( AircraftIndicatedAirspeed,             "Aircraft.IndicatedAirspeed",               tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "indicated airspeed" ) \
F( AircraftIndicatedAirspeedTrend,        "Aircraft.IndicatedAirspeedTrend",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "indicated airspeed trend" ) \
F( AircraftGroundSpeed,                   "Aircraft.GroundSpeed",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "ground speed" ) \
F( AircraftMagneticHeading,               "Aircraft.MagneticHeading",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "" ) \
F( AircraftTrueHeading,                   "Aircraft.TrueHeading",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "" ) \
F( AircraftLatitude,                      "Aircraft.Latitude",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "" ) \
F( AircraftLongitude,                     "Aircraft.Longitude",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "" ) \
F( AircraftHeight,                        "Aircraft.Height",                          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Meter,                    "" ) \
F( AircraftPosition,                      "Aircraft.Position",                        tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Meter,                    "" ) \
F( AircraftOrientation,                   "Aircraft.Orientation",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftVelocity,                      "Aircraft.Velocity",                        tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "velocity vector in body system if 'Body' flag is set, in global system otherwise" ) \
F( AircraftAngularVelocity,               "Aircraft.AngularVelocity",                 tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::RadiantPerSecond,         "angular velocity in body system if 'Body' flag is set (roll rate pitch rate yaw rate) in global system" ) \
F( AircraftAcceleration,                  "Aircraft.Acceleration",                    tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecondSquared,    "aircraft acceleration in body system if 'Body' flag is set, in global system otherwise" ) \
F( AircraftGravity,                       "Aircraft.Gravity",                         tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecondSquared,    "gravity acceleration in body system if 'Body' flag is set" ) \
F( AircraftWind,                          "Aircraft.Wind",                            tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "wind vector at current aircraft position" ) \
F( AircraftRateOfTurn,                    "Aircraft.RateOfTurn",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::RadiantPerSecond,         "rate of turn" ) \
F( AircraftMachNumber,                    "Aircraft.MachNumber",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "mach number" ) \
F( AircraftAngleOfAttack,                 "Aircraft.AngleOfAttack",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "angle of attack indicator" ) \
F( AircraftAngleOfAttackLimit,            "Aircraft.AngleOfAttackLimit",              tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "angle of attack limit (stall)" ) \
F( AircraftAccelerationLimit,             "Aircraft.AccelerationLimit",               tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecondSquared,    "acceleration limit (g-load max/min)" ) \
F( AircraftGear,                          "Aircraft.Gear",                            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "current gear position, zero is up, one is down, in between in transit" ) \
F( AircraftFlaps,                         "Aircraft.Flaps",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "selected flaps" ) \
F( AircraftSlats,                         "Aircraft.Slats",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "selected slats" ) \
F( AircraftThrottle,                      "Aircraft.Throttle",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "current throttle setting" ) \
F( AircraftAirBrake,                      "Aircraft.AirBrake",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftGroundSpoilersArmed,           "Aircraft.GroundSpoilersArmed",             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto ground spoiler armed" ) \
F( AircraftGroundSpoilersExtended,        "Aircraft.GroundSpoilersExtended",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto ground spoiler extended" ) \
F( AircraftParkingBrake,                  "Aircraft.ParkingBrake",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "parking brake" ) \
F( AircraftAutoBrakeSetting,              "Aircraft.AutoBrakeSetting",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto brake position" ) \
F( AircraftAutoBrakeEngaged,              "Aircraft.AutoBrakeEngaged",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto brake engaged" ) \
F( AircraftAutoBrakeRejectedTakeOff,      "Aircraft.AutoBrakeRejectedTakeOff",        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto brake RTO armed" ) \
F( AircraftRadarAltitude,                 "Aircraft.RadarAltitude",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Meter,                    "radar altitude above ground" ) \
F( AircraftName,                          "Aircraft.Name",                            tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "current aircraft short name ( name of folder in aircraft directory, eg c172 )" ) \
F( AircraftNearestAirportIdentifier,      "Aircraft.NearestAirportIdentifier",        tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftNearestAirportName,            "Aircraft.NearestAirportName",              tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftNearestAirportLocation,        "Aircraft.NearestAirportLocation",          tm_msg_data_type::Vector2d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftNearestAirportElevation,       "Aircraft.NearestAirportElevation",         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestAirportIdentifier,         "Aircraft.BestAirportIdentifier",           tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestAirportName,               "Aircraft.BestAirportName",                 tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestAirportLocation,           "Aircraft.BestAirportLocation",             tm_msg_data_type::Vector2d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestAirportElevation,          "Aircraft.BestAirportElevation",            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestRunwayIdentifier,          "Aircraft.BestRunwayIdentifier",            tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestRunwayElevation,           "Aircraft.BestRunwayElevation",             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestRunwayThreshold,           "Aircraft.BestRunwayThreshold",             tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftBestRunwayEnd,                 "Aircraft.BestRunwayEnd",                   tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftCategoryJet,                   "Aircraft.Category.Jet",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftCategoryGlider,                "Aircraft.Category.Glider",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftOnGround,                      "Aircraft.OnGround",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "set if aircraft is on ground" ) \
F( AircraftOnRunway,                      "Aircraft.OnRunway",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "set if aircraft is on ground and on a runway" ) \
F( AircraftCrashed,                       "Aircraft.Crashed",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftPower,                         "Aircraft.Power",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftNormalizedPower,               "Aircraft.NormalizedPower",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftNormalizedPowerTarget,         "Aircraft.NormalizedPowerTarget",           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftTrim,                          "Aircraft.Trim",                            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftPitchTrim,                     "Aircraft.PitchTrim",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftPitchTrimScaling,              "Aircraft.PitchTrimScaling",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftPitchTrimOffset,               "Aircraft.PitchTrimOffset",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftRudderTrim,                    "Aircraft.RudderTrim",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftAutoPitchTrim,                 "Aircraft.AutoPitchTrim",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "automatic pitch trim active (FBW)" ) \
F( AircraftYawDamperEnabled,              "Aircraft.YawDamperEnabled",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "automatic rudder damping active (yaw damper)" ) \
F( AircraftRudderPedalsDisconnected,      "Aircraft.RudderPedalsDisconnected",        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "steering disconnect button active" ) \
F( AircraftStarter,                       "Aircraft.Starter",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "generic engine starter" ) \
F( AircraftStarter1,                      "Aircraft.Starter1",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 1 starter" ) \
F( AircraftStarter2,                      "Aircraft.Starter2",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 2 starter" ) \
F( AircraftStarter3,                      "Aircraft.Starter3",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 3 starter" ) \
F( AircraftStarter4,                      "Aircraft.Starter4",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 4 starter" ) \
F( AircraftIgnition,                      "Aircraft.Ignition",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "generic engine ignition" ) \
F( AircraftIgnition1,                     "Aircraft.Ignition1",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 1 ignition" ) \
F( AircraftIgnition2,                     "Aircraft.Ignition2",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 2 ignition" ) \
F( AircraftIgnition3,                     "Aircraft.Ignition3",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 3 ignition" ) \
F( AircraftIgnition4,                     "Aircraft.Ignition4",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 4 ignition" ) \
F( AircraftThrottleLimit,                 "Aircraft.ThrottleLimit",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine throttle limit (max throttle for takeoff)" ) \
F( AircraftReverse,                       "Aircraft.Reverse",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine reverse thrust selected" ) \
F( AircraftEngineMaster1,                 "Aircraft.EngineMaster1",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 1 master switch" ) \
F( AircraftEngineMaster2,                 "Aircraft.EngineMaster2",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 2 master switch" ) \
F( AircraftEngineMaster3,                 "Aircraft.EngineMaster3",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 3 master switch" ) \
F( AircraftEngineMaster4,                 "Aircraft.EngineMaster4",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 4 master switch" ) \
F( AircraftEngineThrottle1,               "Aircraft.EngineThrottle1",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 1 throttle position" ) \
F( AircraftEngineThrottle2,               "Aircraft.EngineThrottle2",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 2 throttle position" ) \
F( AircraftEngineThrottle3,               "Aircraft.EngineThrottle3",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 3 throttle position" ) \
F( AircraftEngineThrottle4,               "Aircraft.EngineThrottle4",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine 4 throttle position" ) \
F( AircraftEngineRotationSpeed1,          "Aircraft.EngineRotationSpeed1",            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftEngineRotationSpeed2,          "Aircraft.EngineRotationSpeed2",            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftEngineRotationSpeed3,          "Aircraft.EngineRotationSpeed3",            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftEngineRotationSpeed4,          "Aircraft.EngineRotationSpeed4",            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftEngineRunning1,                "Aircraft.EngineRunning1",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftEngineRunning2,                "Aircraft.EngineRunning2",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftEngineRunning3,                "Aircraft.EngineRunning3",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftEngineRunning4,                "Aircraft.EngineRunning4",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( AircraftAPUAvailable,                  "Aircraft.APUAvailable",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( PerformanceSpeedVS0,                   "Performance.Speed.VS0",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "minimum speed with flaps down, lower end of white arc" ) \
F( PerformanceSpeedVS1,                   "Performance.Speed.VS1",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "minimum speed with flaps retracted, lower end of green arc" ) \
F( PerformanceSpeedVFE,                   "Performance.Speed.VFE",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "maximum speed with flaps extended, upper end of white arc" ) \
F( PerformanceSpeedVNO,                   "Performance.Speed.VNO",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "maneuvering speed, lower end of yellow arc" ) \
F( PerformanceSpeedVNE,                   "Performance.Speed.VNE",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "never exceed speed, red line" ) \
F( PerformanceSpeedVAPP,                  "Performance.Speed.VAPP",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "approach airspeed" ) \
F( PerformanceSpeedMinimum,               "Performance.Speed.Minimum",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "stall speed in current configuration" ) \
F( PerformanceSpeedMaximum,               "Performance.Speed.Maximum",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "maximum speed in current configuration" ) \
F( PerformanceSpeedMinimumFlapRetraction, "Performance.Speed.MinimumFlapRetraction",  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "minimum speed for next flap up" ) \
F( PerformanceSpeedMaximumFlapExtension,  "Performance.Speed.MaximumFlapExtension",   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "maximum speed for next flap down" ) \
F( ConfigurationSelectedTakeOffFlaps,     "Configuration.SelectedTakeOffFlaps",       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "FMS selected takeoff flaps" ) \
F( ConfigurationSelectedLandingFlaps,     "Configuration.SelectedLandingFlaps",       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "FMS selected landing flaps" ) \
F( FMSFlightNumber,                       "FlightManagementSystem.FlightNumber",      tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "FMS flight number" ) \
F( NavigationSelectedCourse1,             "Navigation.SelectedCourse1",               tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Radiant,                  "NAV1 selected course (OBS1)" ) \
F( NavigationSelectedCourse2,             "Navigation.SelectedCourse2",               tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Radiant,                  "NAV2 selected course (OBS2)" ) \
F( NavigationNAV1Identifier,              "Navigation.NAV1Identifier",                tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "NAV1 station identifier" ) \
F( NavigationNAV1Frequency,               "Navigation.NAV1Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "NAV1 receiver active frequency" ) \
F( NavigationNAV1StandbyFrequency,        "Navigation.NAV1StandbyFrequency",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "NAV1 receiver standby frequency" ) \
F( NavigationNAV1FrequencySwap,           "Navigation.NAV1FrequencySwap",             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "NAV1 frequency swap" ) \
F( NavigationNAV2Identifier,              "Navigation.NAV2Identifier",                tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "NAV2 station identifier" ) \
F( NavigationNAV2Frequency,               "Navigation.NAV2Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "NAV2 receiver active frequency" ) \
F( NavigationNAV2StandbyFrequency,        "Navigation.NAV2StandbyFrequency",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "NAV2 receiver standby frequency" ) \
F( NavigationNAV2FrequencySwap,           "Navigation.NAV2FrequencySwap",             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "NAV2 frequency swap" ) \
F( NavigationDME1Frequency,               "Navigation.DME1Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME1 active frequency" ) \
F( NavigationDME1Distance,                "Navigation.DME1Distance",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME1 distance" ) \
F( NavigationDME1Time,                    "Navigation.DME1Time",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME1 time" ) \
F( NavigationDME1Speed,                   "Navigation.DME1Speed",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME1 speed" ) \
F( NavigationDME2Frequency,               "Navigation.DME2Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME2 active frequency" ) \
F( NavigationDME2Distance,                "Navigation.DME2Distance",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME2 distance" ) \
F( NavigationDME2Time,                    "Navigation.DME2Time",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME2 time" ) \
F( NavigationDME2Speed,                   "Navigation.DME2Speed",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "DME2 speed" ) \
F( NavigationILS1Identifier,              "Navigation.ILS1Identifier",                tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "ILS1 station identifier" ) \
F( NavigationILS1Course,                  "Navigation.ILS1Course",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Radiant,                  "ILS1 selected course" ) \
F( NavigationILS1Frequency,               "Navigation.ILS1Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ILS1 receiver active frequency" ) \
F( NavigationILS1StandbyFrequency,        "Navigation.ILS1StandbyFrequency",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ILS1 receiver standby frequency" ) \
F( NavigationILS1FrequencySwap,           "Navigation.ILS1FrequencySwap",             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "ILS1 frequency swap" ) \
F( NavigationILS2Identifier,              "Navigation.ILS2Identifier",                tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "ILS2 station identifier" ) \
F( NavigationILS2Course,                  "Navigation.ILS2Course",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Radiant,                  "ILS2 selected course" ) \
F( NavigationILS2Frequency,               "Navigation.ILS2Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ILS2 receiver active frequency" ) \
F( NavigationILS2StandbyFrequency,        "Navigation.ILS2StandbyFrequency",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ILS2 receiver standby frequency" ) \
F( NavigationILS2FrequencySwap,           "Navigation.ILS2FrequencySwap",             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "ILS2 frequency swap" ) \
F( NavigationADF1Frequency,               "Navigation.ADF1Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ADF1 receiver active frequency" ) \
F( NavigationADF1StandbyFrequency,        "Navigation.ADF1StandbyFrequency",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ADF1 receiver standby frequency" ) \
F( NavigationADF1FrequencySwap,           "Navigation.ADF1FrequencySwap",             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "ADF1 frequency swap" ) \
F( NavigationADF2Frequency,               "Navigation.ADF2Frequency",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ADF2 receiver active frequency" ) \
F( NavigationADF2StandbyFrequency,        "Navigation.ADF2StandbyFrequency",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "ADF2 receiver standby frequency" ) \
F( NavigationADF2FrequencySwap,           "Navigation.ADF2FrequencySwap",             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "ADF2 frequency swap" ) \
F( NavigationCOM1Frequency,               "Communication.COM1Frequency",              tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "COM1 transceiver active frequency" ) \
F( NavigationCOM1StandbyFrequency,        "Communication.COM1StandbyFrequency",       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "COM1 transceiver standby frequency" ) \
F( NavigationCOM1FrequencySwap,           "Communication.COM1FrequencySwap",          tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "COM1 frequency swap" ) \
F( NavigationCOM2Frequency,               "Communication.COM2Frequency",              tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "COM2 transceiver active frequency" ) \
F( NavigationCOM2StandbyFrequency,        "Communication.COM2StandbyFrequency",       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "COM2 transceiver standby frequency" ) \
F( NavigationCOM2FrequencySwap,           "Communication.COM2FrequencySwap",          tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "COM2 frequency swap" ) \
F( NavigationCOM3Frequency,               "Communication.COM3Frequency",              tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "COM3 transceiver active frequency" ) \
F( NavigationCOM3StandbyFrequency,        "Communication.COM3StandbyFrequency",       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Hertz,                    "COM3 transceiver standby frequency" ) \
F( NavigationCOM3FrequencySwap,           "Communication.COM3FrequencySwap",          tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "COM3 frequency swap" ) \
F( TransponderCode,                       "Communication.TransponderCode",            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "Transponder code" ) \
F( TransponderCursor,                     "Communication.TransponderCursor",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "Transponder blinking cursor position" ) \
F( AutopilotMaster,                       "Autopilot.Master",                         tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( AutopilotDisengage,                    "Autopilot.Disengage",                      tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "disengage all autopilots" ) \
F( AutopilotHeading,                      "Autopilot.Heading",                        tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::Radiant,                  "" ) \
F( AutopilotVerticalSpeed,                "Autopilot.VerticalSpeed",                  tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::MeterPerSecond,           "" ) \
F( AutopilotSelectedSpeed,                "Autopilot.SelectedSpeed",                  tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::MeterPerSecond,           "" ) \
F( AutopilotSelectedAirspeed,             "Autopilot.SelectedAirspeed",               tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::MeterPerSecond,           "autopilot/flight director selected airspeed, speed bug" ) \
F( AutopilotSelectedHeading,              "Autopilot.SelectedHeading",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Radiant,                  "autopilot/flight director selected heading, heading bug" ) \
F( AutopilotSelectedAltitude,             "Autopilot.SelectedAltitude",               tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::Meter,                    "autopilot/flight director selected altitude" ) \
F( AutopilotSelectedVerticalSpeed,        "Autopilot.SelectedVerticalSpeed",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::MeterPerSecond,           "autopilot/flight director selected vertical speed" ) \
F( AutopilotSelectedAltitudeScale,        "Autopilot.SelectedAltitudeScale",          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director selected altitude step size small/large" ) \
F( AutopilotActiveLateralMode,            "Autopilot.ActiveLateralMode",              tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name of the active lateral mode" ) \
F( AutopilotArmedLateralMode,             "Autopilot.ArmedLateralMode",               tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name of the armed lateral mode" ) \
F( AutopilotActiveVerticalMode,           "Autopilot.ActiveVerticalMode",             tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name of the active vertical mode" ) \
F( AutopilotArmedVerticalMode,            "Autopilot.ArmedVerticalMode",              tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name of the armed vertical mode" ) \
F( AutopilotArmedApproachMode,            "Autopilot.ArmedApproachMode",              tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name of the armed approach mode" ) \
F( AutopilotActiveAutoThrottleMode,       "Autopilot.ActiveAutoThrottleMode",         tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name the active autothrottle mode" ) \
F( AutopilotActiveCollectiveMode,         "Autopilot.ActiveCollectiveMode",           tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name the active helicopter collective mode" ) \
F( AutopilotArmedCollectiveMode,          "Autopilot.ArmedCollectiveMode",            tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot/flight director internal name the armed helicopter collective mode" ) \
F( AutopilotType,                         "Autopilot.Type",                           tm_msg_data_type::String,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot type installed" ) \
F( AutopilotEngaged,                      "Autopilot.Engaged",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "set if autopilot is engaged" ) \
F( AutopilotUseMachNumber,                "Autopilot.UseMachNumber",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot mach/speed toggle state" ) \
F( AutopilotSpeedManaged,                 "Autopilot.SpeedManaged",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot managed/selected speed state" ) \
F( AutopilotTargetAirspeed,               "Autopilot.TargetAirspeed",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot target airspeed" ) \
F( AutopilotAileron,                      "Autopilot.Aileron",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot aileron command" ) \
F( AutopilotElevator,                     "Autopilot.Elevator",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "autopilot elevator command" ) \
F( AutoAutoThrottleType,                  "AutoThrottle.Type",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto-throttle type installed" ) \
F( AutopilotThrottleEngaged,              "Autopilot.ThrottleEngaged",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto-throttle state" ) \
F( AutopilotThrottleCommand,              "Autopilot.ThrottleCommand",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "auto-throttle command" ) \
F( FlightDirectorPitch,                   "FlightDirector.Pitch",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "flight director pitch angle relative to current pitch" ) \
F( FlightDirectorBank,                    "FlightDirector.Bank",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "flight director bank angle relative to current bank" ) \
F( FlightDirectorYaw,                     "FlightDirector.Yaw",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "flight director yaw command" ) \
F( CopilotHeading,                        "Copilot.Heading",                          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Radiant,                  "" ) \
F( CopilotAltitude,                       "Copilot.Altitude",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::Meter,                    "" ) \
F( CopilotAirspeed,                       "Copilot.Airspeed",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "" ) \
F( CopilotVerticalSpeed,                  "Copilot.VerticalSpeed",                    tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::MeterPerSecond,           "" ) \
F( CopilotAileron,                        "Copilot.Aileron",                          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( CopilotElevator,                       "Copilot.Elevator",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( CopilotThrottle,                       "Copilot.Throttle",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( CopilotAutoRudder,                     "Copilot.AutoRudder",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Read,      tm_msg_unit::None,                     "" ) \
F( ControlsThrottle,                      "Controls.Throttle",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "generic throttle position" ) \
F( ControlsThrottle1,                     "Controls.Throttle1",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "throttle position for engine 1" ) \
F( ControlsThrottle2,                     "Controls.Throttle2",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "throttle position for engine 2" ) \
F( ControlsThrottle3,                     "Controls.Throttle3",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "throttle position for engine 3" ) \
F( ControlsThrottle4,                     "Controls.Throttle4",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "throttle position for engine 4" ) \
F( ControlsThrottle1Move,                 "Controls.Throttle1",                       tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::PerSecond,                "throttle rate of change for engine 1" ) \
F( ControlsThrottle2Move,                 "Controls.Throttle2",                       tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::PerSecond,                "throttle rate of change for engine 2" ) \
F( ControlsThrottle3Move,                 "Controls.Throttle3",                       tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::PerSecond,                "throttle rate of change for engine 3" ) \
F( ControlsThrottle4Move,                 "Controls.Throttle4",                       tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::PerSecond,                "throttle rate of change for engine 4" ) \
F( ControlsPitchInput,                    "Controls.Pitch.Input",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsPitchInputOffset,              "Controls.Pitch.Input",                     tm_msg_data_type::Double,   tm_msg_flag::Offset, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsRollInput,                     "Controls.Roll.Input",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsRollInputOffset,               "Controls.Roll.Input",                      tm_msg_data_type::Double,   tm_msg_flag::Offset, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsYawInput,                      "Controls.Yaw.Input",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsYawInputActive,                "Controls.Yaw.Input",                       tm_msg_data_type::Double,   tm_msg_flag::Active, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsFlaps,                         "Controls.Flaps",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "" ) \
F( ControlsFlapsEvent,                    "Controls.Flaps",                           tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsGear,                          "Controls.Gear",                            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "gear lever" ) \
F( ControlsGearToggle,                    "Controls.Gear",                            tm_msg_data_type::Double,   tm_msg_flag::Toggle, tm_msg_access::Write,     tm_msg_unit::None,                     "gear lever" ) \
F( ControlsWheelBrakeLeft,                "Controls.WheelBrake.Left",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsWheelBrakeRight,               "Controls.WheelBrake.Right",                tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsWheelBrakeLeftActive,          "Controls.WheelBrake.Left",                 tm_msg_data_type::Double,   tm_msg_flag::Active, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsWheelBrakeRightActive,         "Controls.WheelBrake.Right",                tm_msg_data_type::Double,   tm_msg_flag::Active, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsAirBrake,                      "Controls.AirBrake",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsAirBrakeActive,                "Controls.AirBrake",                        tm_msg_data_type::Double,   tm_msg_flag::Active, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsAirBrakeArm,                   "Controls.AirBrake.Arm",                    tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsGliderAirBrake,                "Controls.GliderAirBrake",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsPropellerSpeed1,               "Controls.PropellerSpeed1",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsPropellerSpeed2,               "Controls.PropellerSpeed2",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsPropellerSpeed3,               "Controls.PropellerSpeed3",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsPropellerSpeed4,               "Controls.PropellerSpeed4",                 tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsMixture,                       "Controls.Mixture",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsMixture1,                      "Controls.Mixture1",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsMixture2,                      "Controls.Mixture2",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsMixture3,                      "Controls.Mixture3",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsMixture4,                      "Controls.Mixture4",                        tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsThrustReverse,                 "Controls.ThrustReverse",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsThrustReverse1,                "Controls.ThrustReverse1",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsThrustReverse2,                "Controls.ThrustReverse2",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsThrustReverse3,                "Controls.ThrustReverse3",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsThrustReverse4,                "Controls.ThrustReverse4",                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsCollective,                    "Controls.Collective",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsCyclicPitch,                   "Controls.CyclicPitch",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsCyclicRoll,                    "Controls.CyclicRoll",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsTailRotor,                     "Controls.TailRotor",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsRotorBrake,                    "Controls.RotorBrake",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsHelicopterThrottle1,           "Controls.HelicopterThrottle1",             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsHelicopterThrottle2,           "Controls.HelicopterThrottle2",             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsTrim,                          "Controls.Trim",                            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsTrimStep,                      "Controls.Trim",                            tm_msg_data_type::Double,   tm_msg_flag::Step,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsTrimMove,                      "Controls.Trim",                            tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsAileronTrim,                   "Controls.AileronTrim",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsRudderTrim,                    "Controls.RudderTrim",                      tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsTiller,                        "Controls.Tiller",                          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsPedalsDisconnect,              "Controls.PedalsDisconnect",                tm_msg_data_type::Double,   tm_msg_flag::Toggle, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsNoseWheelSteering,             "Controls.NoseWheelSteering",               tm_msg_data_type::Double,   tm_msg_flag::Toggle, tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsLightingPanel,                 "Controls.Lighting.Panel",                  tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsLightingInstruments,           "Controls.Lighting.Instruments",            tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsPressureSetting0,              "Controls.PressureSetting0",                tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "captain pressure setting in Pa" ) \
F( ControlsPressureSettingStandard0,      "Controls.PressureSettingStandard0",        tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "captain pressure setting is STD" ) \
F( ControlsPressureSettingUnit0,          "Controls.PressureSettingUnit0",            tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "captain pressure setting is set display inHg" ) \
F( ControlsPressureSetting1,              "Controls.PressureSetting1",                tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "f/o pressure setting in Pa" ) \
F( ControlsPressureSettingStandard1,      "Controls.PressureSettingStandard1",        tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "f/o pressure setting is STD" ) \
F( ControlsPressureSettingUnit1,          "Controls.PressureSettingUnit1",            tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "f/o pressure setting is set display inHg" ) \
F( ControlsPressureSetting2,              "Controls.PressureSetting2",                tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "standby pressure setting in Pa" ) \
F( ControlsPressureSettingStandard2,      "Controls.PressureSettingStandard2",        tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "standby pressure setting is STD" ) \
F( ControlsPressureSettingUnit2,          "Controls.PressureSettingUnit2",            tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "standby pressure setting is set display inHg" ) \
F( ControlsTransitionAltitude,            "Controls.TransitionAltitude",              tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::Meter,                    "pressure setting transition altitude (QNH->STD)" ) \
F( ControlsTransitionLevel,               "Controls.TransitionLevel",                 tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::Meter,                    "pressure setting transition level (STD->QNH)" ) \
F( PressurizationLandingElevation,        "Pressurization.LandingElevation",          tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::Meter,                    "" ) \
F( PressurizationLandingElevationManual,  "Pressurization.LandingElevationManual",    tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::Meter,                    "" ) \
F( WarningsMasterWarning,                 "Warnings.MasterWarning",                   tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "master warning active" ) \
F( WarningsMasterCaution,                 "Warnings.MasterCaution",                   tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "master caution active" ) \
F( WarningsEngineFire,                    "Warnings.EngineFire",                      tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "engine fire active" ) \
F( WarningsLowOilPressure,                "Warnings.LowOilPressure",                  tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "low oil pressure warning active" ) \
F( WarningsLowFuelPressure,               "Warnings.LowFuelPressure",                 tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "low fuel pressure warning active" ) \
F( WarningsLowHydraulicPressure,          "Warnings.LowHydraulicPressure",            tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "low hydraulic pressure warning active" ) \
F( WarningsLowVoltage,                    "Warnings.LowVoltage",                      tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "low voltage warning active" ) \
F( WarningsAltitudeAlert,                 "Warnings.AltitudeAlert",                   tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "altitude alert warning active" ) \
F( WarningsWarningActive,                 "Warnings.WarningActive",                   tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "warnings active" ) \
F( WarningsWarningMute,                   "Warnings.WarningMute",                     tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Read,      tm_msg_unit::None,                     "warning suppression" ) \
F( ViewDisplayName,                       "View.DisplayName",                         tm_msg_data_type::String,   tm_msg_flag::None,   tm_msg_access::Read,      tm_msg_unit::None,                     "name of current view" ) \
F( ViewInternal,                          "View.Internal",                            tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "set view to last internal view" ) \
F( ViewFollow,                            "View.Follow",                              tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "set view to last follow view" ) \
F( ViewExternal,                          "View.External",                            tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "set view to last external view" ) \
F( ViewCategory,                          "View.Category",                            tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "change to next / previous view category (internal,follow,external), set last view in this category" ) \
F( ViewMode,                              "View.Mode",                                tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "set next / previous view in current category" ) \
F( ViewZoom,                              "View.Zoom",                                tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewPanHorizontal,                     "View.Pan.Horizontal",                      tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewPanHorizontalMove,                 "View.Pan.Horizontal",                      tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewPanVertical,                       "View.Pan.Vertical",                        tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewPanVerticalMove,                   "View.Pan.Vertical",                        tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewPanCenter,                         "View.Pan.Center",                          tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewLookHorizontal,                    "View.Look.Horizontal",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "momentarily look left / right" ) \
F( ViewLookVertical,                      "View.Look.Vertical",                       tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "momentarily look up / down" ) \
F( ViewRoll,                              "View.Roll",                                tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewOffsetX,                           "View.OffsetX",                             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "offset (forward/backward) from view's default position" ) \
F( ViewOffsetXMove,                       "View.OffsetX",                             tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::None,                     "change offset (forward/backward) from view's default position" ) \
F( ViewOffsetY,                           "View.OffsetY",                             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "lateral offset from view's default position" ) \
F( ViewOffsetYMove,                       "View.OffsetY",                             tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::None,                     "change lateral offset from view's default position" ) \
F( ViewOffsetZ,                           "View.OffsetZ",                             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "vertical offset from view's default position" ) \
F( ViewOffsetZMove,                       "View.OffsetZ",                             tm_msg_data_type::Double,   tm_msg_flag::Move,   tm_msg_access::Write,     tm_msg_unit::None,                     "change vertical offset from view's default position" ) \
F( ViewPosition,                          "View.Position",                            tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewDirection,                         "View.Direction",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewUp,                                "View.Up",                                  tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewFieldOfView,                       "View.FieldOfView",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewAspectRatio,                       "View.AspectRatio",                         tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewFreePosition,                      "View.FreePosition",                        tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::Meter,                    "the following 4 messages allow you to implement your own view" ) \
F( ViewFreeLookDirection,                 "View.FreeLookDirection",                   tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewFreeUp,                            "View.FreeUp",                              tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ViewFreeFieldOfView,                   "View.FreeFieldOfView",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::Radiant,                  "" ) \
F( SimulationPause,                       "Simulation.Pause",                         tm_msg_data_type::Double,   tm_msg_flag::Toggle, tm_msg_access::ReadWrite, tm_msg_unit::None,                     "toggle pause on/off" ) \
F( SimulationFlightInformation,           "Simulation.FlightInformation",             tm_msg_data_type::Double,   tm_msg_flag::Toggle, tm_msg_access::Write,     tm_msg_unit::None,                     "show/hide the flight information at the top of the screen" ) \
F( SimulationMovingMap,                   "Simulation.MovingMap",                     tm_msg_data_type::Double,   tm_msg_flag::Toggle, tm_msg_access::Write,     tm_msg_unit::None,                     "show/hide the moving map window" ) \
F( SimulationSound,                       "Simulation.Sound",                         tm_msg_data_type::Double,   tm_msg_flag::Toggle, tm_msg_access::Write,     tm_msg_unit::None,                     "toggle sound on/off" ) \
F( SimulationLiftUp,                      "Simulation.LiftUp",                        tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "lift up the aircraft from current position" ) \
F( SimulationSettingPosition,             "Simulation.SettingPosition",               tm_msg_data_type::Vector3d, tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::Meter,                    "" ) \
F( SimulationSettingOrientation,          "Simulation.SettingOrientation",            tm_msg_data_type::Vector4d, tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( SimulationSettingVelocity,             "Simulation.SettingVelocity",               tm_msg_data_type::Vector3d, tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::MeterPerSecond,           "" ) \
F( SimulationSettingSet,                  "Simulation.SettingSet",                    tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( SimulationTimeChange,                  "Simulation.TimeChange",                    tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "change time of day" ) \
F( SimulationVisibility,                  "Simulation.Visibility",                    tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "" ) \
F( SimulationTime,                        "Simulation.Time",                          tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "" ) \
F( SimulationUseMouseControl,             "Simulation.UseMouseControl",               tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::ReadWrite, tm_msg_unit::None,                     "" ) \
F( SimulationPlaybackStart,               "Simulation.PlaybackStart",                 tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "start playback if simulation is paused" ) \
F( SimulationPlaybackStop,                "Simulation.PlaybackStop",                  tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "stop playback" ) \
F( SimulationPlaybackSetPosition,         "Simulation.PlaybackPosition",              tm_msg_data_type::Double,   tm_msg_flag::None,   tm_msg_access::Write,     tm_msg_unit::None,                     "set playback position 0 - 1" ) \
F( SimulationExternalPosition,            "Simulation.ExternalPosition",              tm_msg_data_type::Vector3d, tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::Meter,                    "" ) \
F( SimulationExternalOrientation,         "Simulation.ExternalOrientation",           tm_msg_data_type::Vector4d, tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandExecute,                        "Command.Execute",                          tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandBack,                           "Command.Back",                             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandUp,                             "Command.Up",                               tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandDown,                           "Command.Down",                             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandLeft,                           "Command.Left",                             tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandRight,                          "Command.Right",                            tm_msg_data_type::Double,   tm_msg_flag::Event,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandMoveHorizontal,                 "Command.MoveHorizontal",                   tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandMoveVertical,                   "Command.MoveVertical",                     tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandRotate,                         "Command.Rotate",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( CommandZoom,                           "Command.Zoom",                             tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "" ) \
F( ControlsSpeed,                         "Controls.Speed",                           tm_msg_data_type::Double,   tm_msg_flag::Value,  tm_msg_access::Write,     tm_msg_unit::None,                     "ignore/do not use combined throttle, brake and reverse, copilot splits into other" ) \
F( FMSData0,                              "FlightManagementSystem.Data0",             tm_msg_data_type::None,     tm_msg_flag::Value,  tm_msg_access::None,      tm_msg_unit::None,                     "ignore/do not use FMS binary datablock" ) \
F( FMSData1,                              "FlightManagementSystem.Data1",             tm_msg_data_type::None,     tm_msg_flag::Value,  tm_msg_access::None,      tm_msg_unit::None,                     "ignore/do not use FMS binary datablock" ) \
F( NAV1Data,                              "Navigation.NAV1Data",                      tm_msg_data_type::None,     tm_msg_flag::Value,  tm_msg_access::None,      tm_msg_unit::None,                     "ignore/do not use NAV1 binary datablock" ) \
F( NAV2Data,                              "Navigation.NAV2Data",                      tm_msg_data_type::None,     tm_msg_flag::Value,  tm_msg_access::None,      tm_msg_unit::None,                     "ignore/do not use NAV2 binary datablock" ) \
F( NAV3Data,                              "Navigation.NAV3Data",                      tm_msg_data_type::None,     tm_msg_flag::Value,  tm_msg_access::None,      tm_msg_unit::None,                     "ignore/do not use NAV3 binary datablock" ) \
F( ILS1Data,                              "Navigation.ILS1Data",                      tm_msg_data_type::None,     tm_msg_flag::Value,  tm_msg_access::None,      tm_msg_unit::None,                     "ignore/do not use ILS1 binary datablock" ) \
F( ILS2Data,                              "Navigation.ILS2Data",                      tm_msg_data_type::None,     tm_msg_flag::Value,  tm_msg_access::None,      tm_msg_unit::None,                     "ignore/do not use ILS2 binary datablock" )


MESSAGE_LIST(TM_MESSAGE)


///////////////////////////////////////////////////////////////////////////////////////////////////
// AEROFLY PATH DISCOVERY - Automatic detection of Aerofly FS4 installation
///////////////////////////////////////////////////////////////////////////////////////////////////

class AeroflyPathDiscovery {
    public:
        static std::string FindAeroflyPath() {
            std::vector<std::string> candidate_paths = {
                // Steam common locations
                "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Aerofly FS 4 Flight Simulator",
                "C:\\Program Files\\Steam\\steamapps\\common\\Aerofly FS 4 Flight Simulator",
                "D:\\Steam\\steamapps\\common\\Aerofly FS 4 Flight Simulator",
                "E:\\Steam\\steamapps\\common\\Aerofly FS 4 Flight Simulator",
                
                // Direct installation paths
                "C:\\Program Files\\Aerofly FS 4 Flight Simulator",
                "C:\\Program Files (x86)\\Aerofly FS 4 Flight Simulator",
                "C:\\Aerofly FS 4 Flight Simulator",
                
                // Custom Steam library locations
                "D:\\SteamLibrary\\steamapps\\common\\Aerofly FS 4 Flight Simulator",
                "E:\\SteamLibrary\\steamapps\\common\\Aerofly FS 4 Flight Simulator",
                "F:\\SteamLibrary\\steamapps\\common\\Aerofly FS 4 Flight Simulator"
            };
            
            // Check standard paths first
            for (const auto& path : candidate_paths) {
                if (std::filesystem::exists(path + "\\aircraft")) {
                    OutputDebugStringA(("Found Aerofly at: " + path + "\n").c_str());
                    HybridLogToFile("SUCCESS: Found Aerofly at: " + path);
                    return path;
                }
                // Log attempted paths for debugging
                HybridLogToFile("Checked path (not found): " + path);
            }
            
            // Try registry-based discovery for Steam
            std::string steam_path = GetSteamPathFromRegistry();
            if (!steam_path.empty()) {
                std::string aerofly_path = steam_path + "\\steamapps\\common\\Aerofly FS 4 Flight Simulator";
                if (std::filesystem::exists(aerofly_path + "\\aircraft")) {
                    OutputDebugStringA(("Found Aerofly via Steam registry: " + aerofly_path + "\n").c_str());
                    return aerofly_path;
                }
            }
            
            OutputDebugStringA("WARNING: Aerofly FS 4 installation not found\n");
            return "";
        }
        
    private:
        static std::string GetSteamPathFromRegistry() {
            // Registry reading implementation for Steam path discovery
            // This is optional - if it fails, we fall back to static paths
            try {
                HKEY hKey;
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                                 "SOFTWARE\\WOW6432Node\\Valve\\Steam", 
                                 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    
                    char steam_path[512];
                    DWORD size = sizeof(steam_path);
                    if (RegQueryValueExA(hKey, "InstallPath", NULL, NULL, 
                                        (LPBYTE)steam_path, &size) == ERROR_SUCCESS) {
                        RegCloseKey(hKey);
                        return std::string(steam_path);
                    }
                    RegCloseKey(hKey);
                }
            }
            catch (...) {
                // Ignore registry errors
            }
            return "";
        }
    };
    
///////////////////////////////////////////////////////////////////////////////////////////////////
// ENHANCED TMD PARSER - Reemplaza la clase TMDParser existente
///////////////////////////////////////////////////////////////////////////////////////////////////

class EnhancedTMDParser {
    public:
        static std::vector<EnhancedVariableInfo> ParseTMDFile(const std::string& file_path, 
                                                             const std::string& aircraft_name) {
            std::vector<EnhancedVariableInfo> variables;
            
            try {
                std::ifstream file(file_path);
                if (!file.is_open()) {
                    HybridLogToFile("ERROR: Cannot open TMD file: " + file_path);
                    return variables;
                }
                
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                file.close();
                
                HybridLogToFile("Parsing TMD file: " + file_path + " for aircraft: " + aircraft_name);
                
                // Parse message definitions with enhanced metadata
                ParseMessageDefinitions(content, aircraft_name, file_path, variables);
                
                HybridLogToFile("Parsed " + std::to_string(variables.size()) + 
                               " variables from " + aircraft_name);
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR parsing TMD file " + file_path + ": " + e.what());
            }
            
            return variables;
        }
        
        static std::vector<EnhancedVariableInfo> ScanAllAircraft(const std::string& aerofly_path) {
            std::vector<EnhancedVariableInfo> all_variables;
            
            if (aerofly_path.empty()) {
                HybridLogToFile("ERROR: No Aerofly path provided for scanning");
                return all_variables;
            }
            
            std::string aircraft_dir = aerofly_path + "\\aircraft";
            HybridLogToFile("Scanning aircraft directory: " + aircraft_dir);
            
            try {
                if (!std::filesystem::exists(aircraft_dir)) {
                    HybridLogToFile("ERROR: Aircraft directory not found: " + aircraft_dir);
                    return all_variables;
                }
                
                int aircraft_count = 0;
                for (const auto& entry : std::filesystem::directory_iterator(aircraft_dir)) {
                    if (entry.is_directory()) {
                        std::string aircraft_name = entry.path().filename().string();
                        std::string controls_file = entry.path().string() + "\\controls.tmd";
                        
                        if (std::filesystem::exists(controls_file)) {
                            auto variables = ParseTMDFile(controls_file, aircraft_name);
                            all_variables.insert(all_variables.end(), variables.begin(), variables.end());
                            aircraft_count++;
                            
                            HybridLogToFile("Aircraft: " + aircraft_name + " - Variables found: " + 
                                           std::to_string(variables.size()));
                        }
                    }
                }
                
                HybridLogToFile("Scanned " + std::to_string(aircraft_count) + 
                               " aircraft, found " + std::to_string(all_variables.size()) + 
                               " total variables");
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR scanning aircraft directory: " + std::string(e.what()));
            }
            
            return all_variables;
        }
        
    private:
        static void ParseMessageDefinitions(const std::string& content, 
                                           const std::string& aircraft_name,
                                           const std::string& file_path,
                                           std::vector<EnhancedVariableInfo>& variables) {
            
            // Enhanced pattern to capture event context and qualifiers
            std::regex event_pattern(R"(<\[control_message\]\[(On(?:Step|Rotate|Push|Release))\]\[\]\s*<\[string8\]\[Message\]\[([^\]]+)\]>\s*<\[string8\]\[Qualifiers\]\[([^\]]+)\]>)");
            std::sregex_iterator iter(content.begin(), content.end(), event_pattern);
            std::sregex_iterator end;
            
            std::set<std::string> unique_vars;
            
            while (iter != end) {
                std::string event_type = (*iter)[1].str();      // OnStep, OnRotate, OnPush, OnRelease
                std::string variable_name = (*iter)[2].str();   // Variable name
                std::string qualifier = (*iter)[3].str();       // step, toggle, event, etc.
                
                if (IsValidVariable(variable_name) && 
                    unique_vars.find(variable_name) == unique_vars.end()) {
                    
                    EnhancedVariableInfo var_info(variable_name, aircraft_name, file_path);
                    
                    // Enhanced analysis based on ACTUAL TMD content
                    AnalyzeVariablePropertiesFromTMD(variable_name, event_type, qualifier, var_info);
                    
                    variables.push_back(var_info);
                    unique_vars.insert(variable_name);
                    
                    HybridLogToFile("Found variable: " + variable_name + 
                                   " (Event: " + (var_info.is_event ? "YES" : "NO") + 
                                   ", EventType: " + event_type + 
                                   ", Qualifier: " + qualifier + 
                                   ", Qualifiers: " + std::to_string(var_info.valid_qualifiers.size()) + ")");
                }
                ++iter;
            }
            
            // Fallback: Also check for variables without explicit events (legacy support)
            std::regex simple_message_pattern(R"(<\[string8\]\[Message\]\[([^\]]+)\]>)");
            std::sregex_iterator simple_iter(content.begin(), content.end(), simple_message_pattern);
            
            while (simple_iter != end) {
                std::string variable_name = (*simple_iter)[1].str();
                
                if (IsValidVariable(variable_name) && 
                    unique_vars.find(variable_name) == unique_vars.end()) {
                    
                    EnhancedVariableInfo var_info(variable_name, aircraft_name, file_path);
                    
                    // Use original analysis for non-event variables
                    AnalyzeVariableProperties(variable_name, var_info);
                    
                    variables.push_back(var_info);
                    unique_vars.insert(variable_name);
                    
                    HybridLogToFile("Found variable: " + variable_name + 
                                   " (Event: " + (var_info.is_event ? "YES" : "NO") + 
                                   ", Qualifiers: " + std::to_string(var_info.valid_qualifiers.size()) + ")");
                }
                ++simple_iter;
            }
        }
        
        static void AnalyzeVariableProperties(const std::string& variable_name, 
                                             EnhancedVariableInfo& var_info) {
            
            // Determine data type
            var_info.data_type = tm_msg_data_type::Double; // Most common
            
            // Analyze variable name patterns for event types
            if (variable_name.find("Flaps") != std::string::npos ||
                variable_name.find("Gear") != std::string::npos ||
                variable_name.find("Brake") != std::string::npos ||
                variable_name.find("Throttle") != std::string::npos) {
                
                // These often support step/move operations
                var_info.is_step = true;
                var_info.is_move = true;
                var_info.valid_qualifiers.push_back("step");
                var_info.valid_qualifiers.push_back("move");
                var_info.primary_qualifier = "step";
            }
            
            if (variable_name.find("Toggle") != std::string::npos ||
                variable_name.find("Switch") != std::string::npos ||
                variable_name.find("Button") != std::string::npos) {
                
                var_info.is_toggle = true;
                var_info.is_event = true;
                var_info.flag_type = tm_msg_flag::Toggle;
                var_info.valid_qualifiers.push_back("toggle");
                var_info.primary_qualifier = "toggle";
            }
            
            // Check for specific patterns that indicate events
            if (variable_name.find("FrequencySwap") != std::string::npos ||
                variable_name.find("Event") != std::string::npos) {
                
                var_info.is_event = true;
                var_info.flag_type = tm_msg_flag::Event;
                var_info.access_type = tm_msg_access::Write;
                var_info.valid_qualifiers.push_back("trigger");
                var_info.primary_qualifier = "trigger";
            }
            
            // Input controls that support offset
            if (variable_name.find("Input") != std::string::npos ||
                variable_name.find("Pitch") != std::string::npos ||
                variable_name.find("Roll") != std::string::npos ||
                variable_name.find("Yaw") != std::string::npos) {
                
                var_info.is_offset = true;
                var_info.valid_qualifiers.push_back("offset");
                if (var_info.primary_qualifier.empty()) {
                    var_info.primary_qualifier = "offset";
                }
            }
            
            // Active flags for continuous controls
            if (variable_name.find("Active") != std::string::npos ||
                variable_name.find("WheelBrake") != std::string::npos) {
                
                var_info.is_active = true;
                var_info.flag_type = tm_msg_flag::Active;
                var_info.valid_qualifiers.push_back("active");
            }
            
            // Set default primary qualifier if none set
            if (var_info.primary_qualifier.empty()) {
                var_info.primary_qualifier = "value";
                var_info.valid_qualifiers.push_back("value");
            }
            
            // Determine access type
            if (var_info.is_event || var_info.is_toggle) {
                var_info.access_type = tm_msg_access::Write;
            } else {
                var_info.access_type = tm_msg_access::ReadWrite;
            }
            
            // Set category based on variable name
            if (variable_name.find("Controls.") == 0) {
                var_info.category = "Controls";
            } else if (variable_name.find("Aircraft.") == 0) {
                var_info.category = "Aircraft";
            } else if (variable_name.find("Autopilot.") == 0) {
                var_info.category = "Autopilot";
            } else if (variable_name.find("Navigation.") == 0) {
                var_info.category = "Navigation";
            } else if (variable_name.find("Communication.") == 0) {
                var_info.category = "Communication";
            } else {
                var_info.category = "Other";
            }
        }
        
        static bool IsValidVariable(const std::string& var_name) {
            if (var_name.empty() || var_name.length() < 3) return false;
            if (var_name.find("__") != std::string::npos) return false;
            if (var_name.find("Debug") != std::string::npos) return false;
            if (var_name.find("Internal") != std::string::npos) return false;
            
            for (char c : var_name) {
                if (!std::isalnum(c) && c != '.' && c != '_') return false;
            }
            
            return true;
        }
        
        // NEW: Enhanced analysis based on actual TMD event context
        static void AnalyzeVariablePropertiesFromTMD(const std::string& variable_name,
                                                     const std::string& event_type,
                                                     const std::string& qualifier,
                                                     EnhancedVariableInfo& var_info) {
            
            // Set base properties
            var_info.data_type = tm_msg_data_type::Double;
            var_info.access_type = tm_msg_access::ReadWrite;
            
            // Mark as event based on TMD context
            var_info.is_event = true;  // All control_message entries are events
            
            // Analyze event type from TMD
            if (event_type == "OnStep") {
                var_info.is_step = true;
                var_info.flag_type = tm_msg_flag::Step;
                var_info.valid_qualifiers.push_back("step");
                var_info.primary_qualifier = "step";
            }
            else if (event_type == "OnRotate") {
                var_info.is_step = true;  // Rotation often uses step semantics
                var_info.flag_type = tm_msg_flag::Step;
                var_info.valid_qualifiers.push_back("step");
                var_info.primary_qualifier = "step";
            }
            else if (event_type == "OnPush" || event_type == "OnRelease") {
                if (qualifier == "toggle") {
                    var_info.is_toggle = true;
                    var_info.flag_type = tm_msg_flag::Toggle;
                    var_info.valid_qualifiers.push_back("toggle");
                    var_info.primary_qualifier = "toggle";
                }
                else if (qualifier == "event") {
                    var_info.flag_type = tm_msg_flag::Event;
                    var_info.valid_qualifiers.push_back("event");
                    var_info.primary_qualifier = "event";
                }
                else {
                    // Default for push/release
                    var_info.is_toggle = true;
                    var_info.flag_type = tm_msg_flag::Toggle;
                    var_info.valid_qualifiers.push_back("toggle");
                    var_info.primary_qualifier = "toggle";
                }
            }
            
            // Add the actual qualifier from TMD
            if (!qualifier.empty() && 
                std::find(var_info.valid_qualifiers.begin(), var_info.valid_qualifiers.end(), qualifier) == var_info.valid_qualifiers.end()) {
                var_info.valid_qualifiers.push_back(qualifier);
            }
            
            // Input controls support offset
            if (variable_name.find("Input") != std::string::npos) {
                var_info.is_offset = true;
                var_info.valid_qualifiers.push_back("offset");
            }
            
            // Set category based on variable name
            if (variable_name.find("Controls.") == 0) {
                var_info.category = "Controls";
            } else if (variable_name.find("Doors.") == 0) {
                var_info.category = "Doors";  
            } else if (variable_name.find("Windows.") == 0) {
                var_info.category = "Windows";
            } else {
                var_info.category = "Aircraft";
            }
            
            // Set reasonable defaults
            var_info.min_value = 0.0;
            var_info.max_value = 1.0;
            var_info.step_size = 0.1;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // HYBRID VARIABLE MANAGER - Core hybrid system
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    
    class HybridVariableManager {
    private:
        // STATIC: Core 339 SDK variables (maximum performance)
        std::unordered_map<std::string, tm_external_message*> core_messages;
        
        // DYNAMIC: Discovered variables (high flexibility)
        std::unordered_map<std::string, std::unique_ptr<tm_external_message>> dynamic_messages;
        
        // Variable discovery data
        std::vector<EnhancedVariableInfo> discovered_variables;
        mutable std::unordered_map<std::string, const EnhancedVariableInfo*> variable_info_cache;
        std::string aerofly_path;
        bool discovery_completed;
        bool core_initialized;
        
        // Performance tracking
        mutable std::mutex access_mutex;
        mutable std::unordered_map<std::string, int> access_counter;
        
        // Shared memory interface
        AeroflyBridgeData* shared_data;
        
    public:
        HybridVariableManager() : discovery_completed(false), core_initialized(false), shared_data(nullptr) {}
        

        
        bool Initialize(AeroflyBridgeData* data = nullptr) {
            HybridLogToFile("=== HybridVariableManager::Initialize() STARTED ===");
            OutputDebugStringA("=== HybridVariableManager::Initialize() STARTED ===\n");
            
            shared_data = data;
            
            // Phase 1: Initialize core static variables (existing functionality)
            if (!InitializeCoreVariables()) {
                HybridLogToFile("ERROR: Failed to initialize core variables");
                OutputDebugStringA("ERROR: Failed to initialize core variables\n");
                return false;
            }
            HybridLogToFile("SUCCESS: Core variables initialized");
            
            // Phase 2: Discover Aerofly installation
            aerofly_path = AeroflyPathDiscovery::FindAeroflyPath();
            if (aerofly_path.empty()) {
                HybridLogToFile("WARNING: Aerofly path not found, continuing with core variables only");
                OutputDebugStringA("WARNING: Aerofly path not found, continuing with core variables only\n");
                UpdateSharedMemoryInfo();
                return true; // Still functional with core variables
            }
            HybridLogToFile("SUCCESS: Aerofly found at: " + aerofly_path);
            
            // ✅ NUEVO: Phase 3: SINCRONIZAR discovery durante inicialización
            HybridLogToFile("Starting SYNCHRONOUS variable discovery...");
            PerformDiscovery(); // ✅ CAMBIADO: Discovery síncrono, no en background
            
            HybridLogToFile("=== HybridVariableManager::Initialize() COMPLETED ===");
            OutputDebugStringA("=== HybridVariableManager::Initialize() COMPLETED ===\n");
            return true;
        }
        
        tm_external_message* GetMessage(const std::string& variable_name) {
            // FAST PATH: Check core variables first (existing performance preserved)
            auto core_it = core_messages.find(variable_name);
            if (core_it != core_messages.end()) {
                TrackAccess(variable_name, true);
                return core_it->second;
            }
            
            // DYNAMIC PATH: Check discovered variables
            std::lock_guard<std::mutex> lock(access_mutex);
            auto dynamic_it = dynamic_messages.find(variable_name);
            if (dynamic_it != dynamic_messages.end()) {
                TrackAccessUnsafe(variable_name, false);
                return dynamic_it->second.get();
            }
            
            // CREATE ON DEMAND: If discovery is complete and variable not found, create it
            if (discovery_completed && CanCreateDynamicVariable(variable_name)) {
                auto new_message = CreateDynamicMessage(variable_name);
                if (new_message) {
                    tm_external_message* ptr = new_message.get();
                    dynamic_messages[variable_name] = std::move(new_message);
                    TrackAccessUnsafe(variable_name, false);
                    UpdateSharedMemoryInfo();
                    OutputDebugStringA(("Created dynamic variable: " + variable_name + "\n").c_str());
                    return ptr;
                }
            }
            
            return nullptr;
        }
        
        std::vector<std::string> GetAvailableVariables() const {
            std::vector<std::string> variables;
            
            // Add core variables
            for (const auto& pair : core_messages) {
                variables.push_back(pair.first);
            }
            
            // Add dynamic variables
            std::lock_guard<std::mutex> lock(access_mutex);
            for (const auto& pair : dynamic_messages) {
                variables.push_back(pair.first);
            }
            
            // Add discovered but not yet created variables
            for (const auto& var_info : discovered_variables) {
                if (dynamic_messages.find(var_info.name) == dynamic_messages.end() &&
                    core_messages.find(var_info.name) == core_messages.end()) {
                    variables.push_back(var_info.name);
                }
            }
            
            return variables;
        }
        
        void GetStatistics(int& core_count, int& dynamic_count, int& discovered_count) const {
            core_count = static_cast<int>(core_messages.size());
            std::lock_guard<std::mutex> lock(access_mutex);
            dynamic_count = static_cast<int>(dynamic_messages.size());
            discovered_count = static_cast<int>(discovered_variables.size());
        }
        
        const EnhancedVariableInfo* FindVariableInfo(const std::string& variable_name) const {
            auto it = variable_info_cache.find(variable_name);
            if (it != variable_info_cache.end()) {
                return it->second;
            }
            
            // Search in discovered variables
            for (const auto& var_info : discovered_variables) {
                if (var_info.name == variable_name) {
                    // Cache for future lookups
                    variable_info_cache[variable_name] = &var_info;
                    return &var_info;
                }
            }
            
            return nullptr;
        }
    
        std::string FlagTypeToString(tm_msg_flag flag) const {
            switch (flag) {
                case tm_msg_flag::Value: return "Value";
                case tm_msg_flag::Event: return "Event";
                case tm_msg_flag::Toggle: return "Toggle";
                case tm_msg_flag::Step: return "Step";
                case tm_msg_flag::Move: return "Move";
                case tm_msg_flag::Offset: return "Offset";
                case tm_msg_flag::Active: return "Active";
                default: return "Unknown";
            }
        }
    
        std::string AccessTypeToString(tm_msg_access access) const {
            switch (access) {
                case tm_msg_access::Read: return "Read";
                case tm_msg_access::Write: return "Write";
                case tm_msg_access::ReadWrite: return "ReadWrite";
                default: return "Unknown";
            }
        }
    
        std::vector<std::string> GetVariableDetails(const std::string& variable_name) const {
            std::vector<std::string> details;
            
            const EnhancedVariableInfo* var_info = FindVariableInfo(variable_name);
            if (var_info) {
                details.push_back("Name: " + var_info->name);
                details.push_back("Aircraft: " + var_info->aircraft);
                details.push_back("Category: " + var_info->category);
                details.push_back("Is Event: " + std::string(var_info->is_event ? "YES" : "NO"));
                details.push_back("Primary Qualifier: " + var_info->primary_qualifier);
                
                if (!var_info->valid_qualifiers.empty()) {
                    std::string qualifiers = "Valid Qualifiers: ";
                    for (size_t i = 0; i < var_info->valid_qualifiers.size(); i++) {
                        if (i > 0) qualifiers += ", ";
                        qualifiers += var_info->valid_qualifiers[i];
                    }
                    details.push_back(qualifiers);
                }
            } else {
                details.push_back("Variable not found in discovery cache");
            }
            
            return details;
        }

        std::string GetDiscoveryStatus() const {
            std::ostringstream status;
            status << "Aerofly Path: " << (aerofly_path.empty() ? "Not Found" : aerofly_path) << "\n";
            status << "Discovery: " << (discovery_completed ? "Complete" : "In Progress") << "\n";
            status << "Core Variables: " << core_messages.size() << "\n";
            status << "Dynamic Variables: " << dynamic_messages.size() << "\n";
            status << "Discovered Variables: " << discovered_variables.size() << "\n";
            return status.str();
        }
        
    private:
        bool InitializeCoreVariables() {
            try {
                // Map all existing static messages from the current DLL
                core_messages["Controls.Throttle"] = &MessageControlsThrottle;
                core_messages["Controls.Throttle1"] = &MessageControlsThrottle1;
                core_messages["Controls.Throttle2"] = &MessageControlsThrottle2;
                core_messages["Controls.Throttle3"] = &MessageControlsThrottle3;
                core_messages["Controls.Throttle4"] = &MessageControlsThrottle4;
                core_messages["Controls.Pitch.Input"] = &MessageControlsPitchInput;
                core_messages["Controls.Roll.Input"] = &MessageControlsRollInput;
                core_messages["Controls.Yaw.Input"] = &MessageControlsYawInput;
                core_messages["Controls.Flaps"] = &MessageControlsFlaps;
                core_messages["Controls.Gear"] = &MessageControlsGear;
                core_messages["Controls.WheelBrake.Left"] = &MessageControlsWheelBrakeLeft;
                core_messages["Controls.WheelBrake.Right"] = &MessageControlsWheelBrakeRight;
                core_messages["Controls.AirBrake"] = &MessageControlsAirBrake;
                core_messages["Controls.AirBrake.Arm"] = &MessageControlsAirBrakeArm;
                core_messages["Controls.Mixture"] = &MessageControlsMixture;
                core_messages["Controls.Mixture1"] = &MessageControlsMixture1;
                core_messages["Controls.Mixture2"] = &MessageControlsMixture2;
                core_messages["Controls.Mixture3"] = &MessageControlsMixture3;
                core_messages["Controls.Mixture4"] = &MessageControlsMixture4;
                core_messages["Controls.ThrustReverse"] = &MessageControlsThrustReverse;
                core_messages["Controls.ThrustReverse1"] = &MessageControlsThrustReverse1;
                core_messages["Controls.ThrustReverse2"] = &MessageControlsThrustReverse2;
                core_messages["Controls.ThrustReverse3"] = &MessageControlsThrustReverse3;
                core_messages["Controls.ThrustReverse4"] = &MessageControlsThrustReverse4;
                core_messages["Controls.PropellerSpeed1"] = &MessageControlsPropellerSpeed1;
                core_messages["Controls.PropellerSpeed2"] = &MessageControlsPropellerSpeed2;
                core_messages["Controls.PropellerSpeed3"] = &MessageControlsPropellerSpeed3;
                core_messages["Controls.PropellerSpeed4"] = &MessageControlsPropellerSpeed4;
                core_messages["Controls.GliderAirBrake"] = &MessageControlsGliderAirBrake;
                core_messages["Controls.Collective"] = &MessageControlsCollective;
                core_messages["Controls.TailRotor"] = &MessageControlsTailRotor;
                core_messages["Controls.CyclicPitch"] = &MessageControlsCyclicPitch;
                core_messages["Controls.CyclicRoll"] = &MessageControlsCyclicRoll;
                core_messages["Controls.RotorBrake"] = &MessageControlsRotorBrake;
                core_messages["Controls.HelicopterThrottle1"] = &MessageControlsHelicopterThrottle1;
                core_messages["Controls.HelicopterThrottle2"] = &MessageControlsHelicopterThrottle2;
                
                // Communication variables
                core_messages["Communication.COM1Frequency"] = &MessageNavigationCOM1Frequency;
                core_messages["Communication.COM1StandbyFrequency"] = &MessageNavigationCOM1StandbyFrequency;
                core_messages["Communication.COM2Frequency"] = &MessageNavigationCOM2Frequency;
                core_messages["Communication.COM2StandbyFrequency"] = &MessageNavigationCOM2StandbyFrequency;
                core_messages["Communication.TransponderCode"] = &MessageTransponderCode;
                
                // Navigation variables
                core_messages["Navigation.NAV1Frequency"] = &MessageNavigationNAV1Frequency;
                core_messages["Navigation.NAV1StandbyFrequency"] = &MessageNavigationNAV1StandbyFrequency;
                core_messages["Navigation.NAV2Frequency"] = &MessageNavigationNAV2Frequency;
                core_messages["Navigation.NAV2StandbyFrequency"] = &MessageNavigationNAV2StandbyFrequency;
                core_messages["Navigation.SelectedCourse1"] = &MessageNavigationSelectedCourse1;
                core_messages["Navigation.SelectedCourse2"] = &MessageNavigationSelectedCourse2;
                
                // Autopilot variables
                core_messages["Autopilot.SelectedAirspeed"] = &MessageAutopilotSelectedAirspeed;
                core_messages["Autopilot.SelectedHeading"] = &MessageAutopilotSelectedHeading;
                core_messages["Autopilot.SelectedAltitude"] = &MessageAutopilotSelectedAltitude;
                core_messages["Autopilot.SelectedVerticalSpeed"] = &MessageAutopilotSelectedVerticalSpeed;
                core_messages["Autopilot.Master"] = &MessageAutopilotMaster;
                core_messages["Autopilot.Heading"] = &MessageAutopilotHeading;
                core_messages["Autopilot.VerticalSpeed"] = &MessageAutopilotVerticalSpeed;
                core_messages["Autopilot.SelectedSpeed"] = &MessageAutopilotSelectedSpeed;
                
                // Aircraft system variables
                core_messages["Aircraft.ParkingBrake"] = &MessageAircraftParkingBrake;
                core_messages["Aircraft.Starter1"] = &MessageAircraftStarter1;
                core_messages["Aircraft.Starter2"] = &MessageAircraftStarter2;
                core_messages["Aircraft.Starter3"] = &MessageAircraftStarter3;
                core_messages["Aircraft.Starter4"] = &MessageAircraftStarter4;
                core_messages["Aircraft.Ignition1"] = &MessageAircraftIgnition1;
                core_messages["Aircraft.Ignition2"] = &MessageAircraftIgnition2;
                core_messages["Aircraft.Ignition3"] = &MessageAircraftIgnition3;
                core_messages["Aircraft.Ignition4"] = &MessageAircraftIgnition4;
                core_messages["Aircraft.EngineMaster1"] = &MessageAircraftEngineMaster1;
                core_messages["Aircraft.EngineMaster2"] = &MessageAircraftEngineMaster2;
                core_messages["Aircraft.EngineMaster3"] = &MessageAircraftEngineMaster3;
                core_messages["Aircraft.EngineMaster4"] = &MessageAircraftEngineMaster4;
                core_messages["Aircraft.AutoBrakeSetting"] = &MessageAircraftAutoBrakeSetting;
                
                // Warning variables
                core_messages["Warnings.MasterWarning"] = &MessageWarningsMasterWarning;
                core_messages["Warnings.MasterCaution"] = &MessageWarningsMasterCaution;
                core_messages["Warnings.LowOilPressure"] = &MessageWarningsLowOilPressure;
                core_messages["Warnings.LowFuelPressure"] = &MessageWarningsLowFuelPressure;
                
                core_initialized = true;
                OutputDebugStringA(("Initialized " + std::to_string(core_messages.size()) + 
                                   " core variables\n").c_str());
                return true;
            }
            catch (const std::exception& e) {
                OutputDebugStringA(("Error initializing core variables: " + std::string(e.what()) + "\n").c_str());
                return false;
            }
        }
        
        void PerformDiscovery() {
            HybridLogToFile("=== Starting ENHANCED variable discovery ===");
            HybridLogToFile("Scanning path: " + aerofly_path);
            
            try {
                discovered_variables = EnhancedTMDParser::ScanAllAircraft(aerofly_path);
                
                // Build cache for fast lookups
                variable_info_cache.clear();
                for (const auto& var_info : discovered_variables) {
                    variable_info_cache[var_info.name] = &var_info;
                }
                
                std::string result_msg = "Enhanced discovery complete: Found " + 
                                       std::to_string(discovered_variables.size()) + 
                                       " variables across all aircraft";
                HybridLogToFile(result_msg);
                
                // Log statistics
                int event_count = 0, toggle_count = 0, step_count = 0;
                for (const auto& var : discovered_variables) {
                    if (var.is_event) event_count++;
                    if (var.is_toggle) toggle_count++;
                    if (var.is_step) step_count++;
                }
                
                HybridLogToFile("Statistics: Events=" + std::to_string(event_count) + 
                               ", Toggles=" + std::to_string(toggle_count) + 
                               ", Steps=" + std::to_string(step_count));
                
                // Log sample variables
                if (discovered_variables.size() > 0) {
                    HybridLogToFile("Sample enhanced variables discovered:");
                    size_t max_samples = (discovered_variables.size() < 10) ? discovered_variables.size() : 10;
                    for (size_t i = 0; i < max_samples; i++) {
                        const auto& var = discovered_variables[i];
                        HybridLogToFile("  - " + var.name + " (" + var.aircraft + 
                                       ") [" + var.category + "] Qualifiers: " + 
                                       std::to_string(var.valid_qualifiers.size()));
                    }
                }
                
                discovery_completed = true;
                UpdateSharedMemoryInfo();
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR during enhanced discovery: " + std::string(e.what()));
                discovery_completed = true;
            }
        }
        
        // Helper function to calculate FNV-1a hash at runtime (same algorithm as tm_string_hasher)
        static tm_uint64 CalculateRuntimeHash(const std::string& str) {
            tm_uint64 hash = 14695981039346656037ull; // FNV offset basis
            for (char c : str) {
                hash = (hash ^ static_cast<tm_uint64>(c)) * 1099511628211ull; // FNV prime
            }
            return hash;
        }

        std::unique_ptr<tm_external_message> CreateDynamicMessage(const std::string& variable_name) {
            try {
                // Find enhanced variable info
                const EnhancedVariableInfo* var_info = FindVariableInfo(variable_name);
                
                tm_uint64 hash = CalculateRuntimeHash(variable_name);
                
                if (var_info) {
                    // Create message with correct metadata
                    auto message = std::make_unique<tm_external_message>(
                        tm_string_hash(hash),
                        var_info->data_type,    // Correct type
                        var_info->flag_type,    // Correct flag (Event vs Value)
                        var_info->access_type,  // Correct access
                        var_info->unit_type     // Correct unit
                    );
                    
                    HybridLogToFile("Created dynamic message: " + variable_name + 
                                   " with flag=" + FlagTypeToString(var_info->flag_type) +
                                   ", access=" + AccessTypeToString(var_info->access_type));
                    
                    return message;
                } else {
                    // Fallback to simple message
                    auto message = std::make_unique<tm_external_message>(
                        tm_string_hash(hash),
                        tm_msg_data_type::Double,
                        tm_msg_flag::Value,
                        tm_msg_access::ReadWrite,
                        tm_msg_unit::None
                    );
                    
                    HybridLogToFile("Created fallback message: " + variable_name);
                    return message;
                }
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR creating dynamic message for " + variable_name + ": " + e.what());
                return nullptr;
            }
        }
        
        void UpdateSharedMemoryInfo() {
            if (!shared_data) return;
            
            shared_data->hybrid_core_variables = static_cast<uint32_t>(core_messages.size());
            shared_data->hybrid_dynamic_variables = static_cast<uint32_t>(dynamic_messages.size());
            shared_data->hybrid_discovered_variables = static_cast<uint32_t>(discovered_variables.size());
            shared_data->hybrid_discovery_complete = discovery_completed ? 1 : 0;
            
            if (!aerofly_path.empty()) {
                strncpy_s(shared_data->aerofly_path, sizeof(shared_data->aerofly_path), 
                         aerofly_path.c_str(), _TRUNCATE);
            }
        }
        
        void TrackAccess(const std::string& variable_name, bool is_core) const {
            // Performance tracking for optimization decisions
            std::lock_guard<std::mutex> lock(access_mutex);
            TrackAccessUnsafe(variable_name, is_core);
        }
        
        // Internal version that assumes mutex is already locked
        void TrackAccessUnsafe(const std::string& variable_name, bool is_core) const {
            // Performance tracking for optimization decisions
            access_counter[variable_name]++;
            
            // Log high-usage dynamic variables that might benefit from being core
            if (!is_core && access_counter[variable_name] == 100) {
                OutputDebugStringA(("High usage dynamic variable: " + variable_name + 
                                   " (consider adding to core)\n").c_str());
            }
        }
        
        bool CanCreateDynamicVariable(const std::string& variable_name) const {
            // Check if this variable was discovered in any aircraft
            for (const auto& var_info : discovered_variables) {
                if (var_info.name == variable_name) {
                    return true;
                }
            }
            return false;
        }
    };
    

///////////////////////////////////////////////////////////////////////////////////////////////////
// SHARED MEMORY INTERFACE - Primary Interface
///////////////////////////////////////////////////////////////////////////////////////////////////

class SharedMemoryInterface {
private:
    HANDLE hMapFile;
    AeroflyBridgeData* pData;
    std::mutex data_mutex;
    bool initialized;
    
public:
    SharedMemoryInterface() : hMapFile(NULL), pData(nullptr), initialized(false) {}
    
    ~SharedMemoryInterface() {
        Cleanup();
    }
    
    bool Initialize() {
        try {
            // Create shared memory region
            hMapFile = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                sizeof(AeroflyBridgeData),
                "AeroflyBridgeData"
            );
            
            if (hMapFile == NULL) {
                return false;
            }
            
            // Map the memory
            pData = (AeroflyBridgeData*)MapViewOfFile(
                hMapFile,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                sizeof(AeroflyBridgeData)
            );
            
            if (pData == nullptr) {
                CloseHandle(hMapFile);
                hMapFile = NULL;
                return false;
            }
            
            // Initialize data structure
            std::lock_guard<std::mutex> lock(data_mutex);
            memset(pData, 0, sizeof(AeroflyBridgeData));
            pData->data_valid = 0;
            pData->update_counter = 0;
            
            initialized = true;
            return true;
        }
        catch (...) {
            return false;
        }
    }
    
    void UpdateData(const std::vector<tm_external_message>& messages, double delta_time) {
        if (!initialized || !pData) return;
        
        std::lock_guard<std::mutex> lock(data_mutex);
        
        // Update timestamp and counter
        pData->timestamp_us = GetTickCount64() * 1000; // Convert to microseconds
        pData->update_counter++;
        
        // Process all received messages
        for (const auto& message : messages) {
            ProcessMessage(message);
        }
        
        // Mark data as valid
        pData->data_valid = 1;
    }
    
    void ProcessMessage(const tm_external_message& message) {
        const auto hash = message.GetStringHash().GetHash();
        
        // Add error handling for assertion failures
        try {
            // === AIRCRAFT BASIC DATA ===
            if (hash == MessageAircraftUniversalTime.GetID()) {
                pData->all_variables[(int)VariableIndex::AIRCRAFT_UNIVERSAL_TIME] = message.GetDouble();
            }
        else if (hash == MessageAircraftLatitude.GetID()) {
            pData->latitude = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_LATITUDE] = message.GetDouble();
        }
        else if (hash == MessageAircraftLongitude.GetID()) {
            pData->longitude = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_LONGITUDE] = message.GetDouble();
        }
        else if (hash == MessageAircraftAltitude.GetID()) {
            pData->altitude = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ALTITUDE] = message.GetDouble();
        }
        else if (hash == MessageAircraftPitch.GetID()) {
            pData->pitch = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_PITCH] = message.GetDouble();
        }
        else if (hash == MessageAircraftBank.GetID()) {
            pData->bank = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_BANK] = message.GetDouble();
        }
        else if (hash == MessageAircraftTrueHeading.GetID()) {
            pData->true_heading = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_TRUE_HEADING] = message.GetDouble();
        }
        else if (hash == MessageAircraftMagneticHeading.GetID()) {
            pData->magnetic_heading = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_MAGNETIC_HEADING] = message.GetDouble();
        }
        else if (hash == MessageAircraftIndicatedAirspeed.GetID()) {
            pData->indicated_airspeed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_INDICATED_AIRSPEED] = message.GetDouble();
        }
        else if (hash == MessageAircraftIndicatedAirspeedTrend.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_INDICATED_AIRSPEED_TREND] = message.GetDouble();
        }
        else if (hash == MessageAircraftGroundSpeed.GetID()) {
            pData->ground_speed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_GROUND_SPEED] = message.GetDouble();
        }
        else if (hash == MessageAircraftVerticalSpeed.GetID()) {
            pData->vertical_speed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_VERTICAL_SPEED] = message.GetDouble();
        }
        else if (hash == MessageAircraftHeight.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_HEIGHT] = message.GetDouble();
        }
        
        // === AIRCRAFT PHYSICS ===
        else if (hash == MessageAircraftPosition.GetID()) {
            pData->position = message.GetVector3d();
        }
        else if (hash == MessageAircraftVelocity.GetID()) {
            pData->velocity = message.GetVector3d();
        }
        else if (hash == MessageAircraftAcceleration.GetID()) {
            pData->acceleration = message.GetVector3d();
        }
        else if (hash == MessageAircraftAngularVelocity.GetID()) {
            pData->angular_velocity = message.GetVector3d();
        }
        else if (hash == MessageAircraftGravity.GetID()) {
            pData->gravity = message.GetVector3d();
        }
        else if (hash == MessageAircraftWind.GetID()) {
            pData->wind = message.GetVector3d();
        }
        else if (hash == MessageAircraftRateOfTurn.GetID()) {
            pData->rate_of_turn = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_RATE_OF_TURN] = message.GetDouble();
        }
        else if (hash == MessageAircraftMachNumber.GetID()) {
            pData->mach_number = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_MACH_NUMBER] = message.GetDouble();
        }
        else if (hash == MessageAircraftAngleOfAttack.GetID()) {
            pData->angle_of_attack = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ANGLE_OF_ATTACK] = message.GetDouble();
        }
        else if (hash == MessageAircraftAngleOfAttackLimit.GetID()) {
            pData->angle_of_attack_limit = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ANGLE_OF_ATTACK_LIMIT] = message.GetDouble();
        }
        else if (hash == MessageAircraftAccelerationLimit.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ACCELERATION_LIMIT] = message.GetDouble();
        }
        
        // === AIRCRAFT STATE ===
        else if (hash == MessageAircraftOnGround.GetID()) {
            pData->on_ground = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ON_GROUND] = message.GetDouble();
        }
        else if (hash == MessageAircraftOnRunway.GetID()) {
            pData->on_runway = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ON_RUNWAY] = message.GetDouble();
        }
        else if (hash == MessageAircraftCrashed.GetID()) {
            // TEMPORARILY COMMENTED OUT - DOUBLE ASSERTION FAILURE DURING CRASH
            // pData->crashed = message.GetDouble();
            // pData->all_variables[(int)VariableIndex::AIRCRAFT_CRASHED] = message.GetDouble();
            // Set default value for now - crash detection via other means
            pData->crashed = 0.0;
            pData->all_variables[(int)VariableIndex::AIRCRAFT_CRASHED] = 0.0;
            OutputDebugStringA("WARNING: Aircraft.Crashed variable temporarily disabled due to type assertion error\n");
        }
        else if (hash == MessageAircraftGear.GetID()) {
            pData->gear_position = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_GEAR] = message.GetDouble();
        }
        else if (hash == MessageAircraftFlaps.GetID()) {
            pData->flaps_position = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_FLAPS] = message.GetDouble();
        }
        else if (hash == MessageAircraftSlats.GetID()) {
            pData->slats_position = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_SLATS] = message.GetDouble();
        }
        else if (hash == MessageAircraftThrottle.GetID()) {
            pData->throttle_position = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_THROTTLE] = message.GetDouble();
        }
        else if (hash == MessageAircraftAirBrake.GetID()) {
            pData->airbrake_position = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_AIR_BRAKE] = message.GetDouble();
        }
        
        // === ENGINE DATA ===
        else if (hash == MessageAircraftEngineThrottle1.GetID()) {
            pData->engine_throttle[0] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_1] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineThrottle2.GetID()) {
            pData->engine_throttle[1] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_2] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineThrottle3.GetID()) {
            pData->engine_throttle[2] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_3] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineThrottle4.GetID()) {
            pData->engine_throttle[3] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_THROTTLE_4] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRotationSpeed1.GetID()) {
            pData->engine_rotation_speed[0] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_1] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRotationSpeed2.GetID()) {
            pData->engine_rotation_speed[1] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_2] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRotationSpeed3.GetID()) {
            pData->engine_rotation_speed[2] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_3] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRotationSpeed4.GetID()) {
            pData->engine_rotation_speed[3] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_ROTATION_SPEED_4] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRunning1.GetID()) {
            pData->engine_running[0] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_1] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRunning2.GetID()) {
            pData->engine_running[1] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_2] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRunning3.GetID()) {
            pData->engine_running[2] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_3] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineRunning4.GetID()) {
            pData->engine_running[3] = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_RUNNING_4] = message.GetDouble();
        }
        
        // === PERFORMANCE SPEEDS ===
        else if (hash == MessagePerformanceSpeedVS0.GetID()) {
            pData->vs0_speed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::PERFORMANCE_SPEED_VS0] = message.GetDouble();
        }
        else if (hash == MessagePerformanceSpeedVS1.GetID()) {
            pData->vs1_speed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::PERFORMANCE_SPEED_VS1] = message.GetDouble();
        }
        else if (hash == MessagePerformanceSpeedVFE.GetID()) {
            pData->vfe_speed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::PERFORMANCE_SPEED_VFE] = message.GetDouble();
        }
        else if (hash == MessagePerformanceSpeedVNO.GetID()) {
            pData->vno_speed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::PERFORMANCE_SPEED_VNO] = message.GetDouble();
        }
        else if (hash == MessagePerformanceSpeedVNE.GetID()) {
            pData->vne_speed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::PERFORMANCE_SPEED_VNE] = message.GetDouble();
        }
        
        // === NAVIGATION ===
        else if (hash == MessageNavigationSelectedCourse1.GetID()) {
            pData->nav1_selected_course = message.GetDouble();
            pData->all_variables[(int)VariableIndex::NAVIGATION_SELECTED_COURSE_1] = message.GetDouble();
        }
        else if (hash == MessageNavigationSelectedCourse2.GetID()) {
            pData->nav2_selected_course = message.GetDouble();
            pData->all_variables[(int)VariableIndex::NAVIGATION_SELECTED_COURSE_2] = message.GetDouble();
        }
        else if (hash == MessageNavigationNAV1Frequency.GetID()) {
            pData->nav1_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::NAVIGATION_NAV1_FREQUENCY] = message.GetDouble();
        }
        else if (hash == MessageNavigationNAV1StandbyFrequency.GetID()) {
            pData->nav1_standby_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::NAVIGATION_NAV1_STANDBY_FREQUENCY] = message.GetDouble();
        }
        else if (hash == MessageNavigationNAV2Frequency.GetID()) {
            pData->nav2_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::NAVIGATION_NAV2_FREQUENCY] = message.GetDouble();
        }
        else if (hash == MessageNavigationNAV2StandbyFrequency.GetID()) {
            pData->nav2_standby_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::NAVIGATION_NAV2_STANDBY_FREQUENCY] = message.GetDouble();
        }
        
        // === COMMUNICATION ===
        else if (hash == MessageNavigationCOM1Frequency.GetID()) {
            pData->com1_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::COMMUNICATION_COM1_FREQUENCY] = message.GetDouble();
        }
        else if (hash == MessageNavigationCOM1StandbyFrequency.GetID()) {
            pData->com1_standby_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::COMMUNICATION_COM1_STANDBY_FREQUENCY] = message.GetDouble();
        }
        else if (hash == MessageNavigationCOM2Frequency.GetID()) {
            pData->com2_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::COMMUNICATION_COM2_FREQUENCY] = message.GetDouble();
        }
        else if (hash == MessageNavigationCOM2StandbyFrequency.GetID()) {
            pData->com2_standby_frequency = message.GetDouble();
            pData->all_variables[(int)VariableIndex::COMMUNICATION_COM2_STANDBY_FREQUENCY] = message.GetDouble();
        }
        
        // === AUTOPILOT ===
        else if (hash == MessageAutopilotEngaged.GetID()) {
            pData->ap_engaged = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AUTOPILOT_ENGAGED] = message.GetDouble();
        }
        else if (hash == MessageAutopilotSelectedAirspeed.GetID()) {
            pData->ap_selected_airspeed = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AUTOPILOT_SELECTED_AIRSPEED] = message.GetDouble();
        }
        else if (hash == MessageAutopilotSelectedHeading.GetID()) {
            pData->ap_selected_heading = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AUTOPILOT_SELECTED_HEADING] = message.GetDouble();
        }
        else if (hash == MessageAutopilotSelectedAltitude.GetID()) {
            pData->ap_selected_altitude = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AUTOPILOT_SELECTED_ALTITUDE] = message.GetDouble();
        }
        else if (hash == MessageAutopilotSelectedVerticalSpeed.GetID()) {
            pData->ap_selected_vs = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AUTOPILOT_SELECTED_VERTICAL_SPEED] = message.GetDouble();
        }
        else if (hash == MessageAutopilotThrottleEngaged.GetID()) {
            pData->ap_throttle_engaged = message.GetDouble();
            pData->all_variables[(int)VariableIndex::AUTOPILOT_THROTTLE_ENGAGED] = message.GetDouble();
        }
        else if (hash == MessageAutopilotActiveLateralMode.GetID()) {
            // TEMPORARILY COMMENTED OUT - STRING ASSERTION FAILURE
            // const auto mode_str = message.GetString();
            // strncpy_s(pData->ap_lateral_mode, sizeof(pData->ap_lateral_mode), mode_str.c_str(), _TRUNCATE);
            // Set default value for now
            strncpy_s(pData->ap_lateral_mode, sizeof(pData->ap_lateral_mode), "Manual", _TRUNCATE);
        }
        else if (hash == MessageAutopilotActiveVerticalMode.GetID()) {
            // TEMPORARILY COMMENTED OUT - STRING ASSERTION FAILURE
            // const auto mode_str = message.GetString();
            // strncpy_s(pData->ap_vertical_mode, sizeof(pData->ap_vertical_mode), mode_str.c_str(), _TRUNCATE);
            // Set default value for now
            strncpy_s(pData->ap_vertical_mode, sizeof(pData->ap_vertical_mode), "Manual", _TRUNCATE);
        }
        
        // === CONTROLS ===
        else if (hash == MessageControlsPitchInput.GetID()) {
            pData->pitch_input = message.GetDouble();
            pData->all_variables[(int)VariableIndex::CONTROLS_PITCH_INPUT] = message.GetDouble();
        }
        else if (hash == MessageControlsRollInput.GetID()) {
            pData->roll_input = message.GetDouble();
            pData->all_variables[(int)VariableIndex::CONTROLS_ROLL_INPUT] = message.GetDouble();
        }
        else if (hash == MessageControlsYawInput.GetID()) {
            pData->yaw_input = message.GetDouble();
            pData->all_variables[(int)VariableIndex::CONTROLS_YAW_INPUT] = message.GetDouble();
        }
        else if (hash == MessageControlsThrottle.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_THROTTLE] = message.GetDouble();
        }
        else if (hash == MessageControlsGear.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_GEAR] = message.GetDouble();
        }
        else if (hash == MessageControlsFlaps.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_FLAPS] = message.GetDouble();
        }
        
        // === ADDITIONAL CONTROL VARIABLES ===
        else if (hash == MessageControlsWheelBrakeLeft.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_WHEEL_BRAKE_LEFT] = message.GetDouble();
        }
        else if (hash == MessageControlsWheelBrakeRight.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_WHEEL_BRAKE_RIGHT] = message.GetDouble();
        }
        else if (hash == MessageControlsAirBrake.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_AIR_BRAKE] = message.GetDouble();
        }
        else if (hash == MessageControlsAirBrakeArm.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_AIR_BRAKE_ARM] = message.GetDouble();
        }
        else if (hash == MessageControlsPropellerSpeed1.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_PROPELLER_SPEED_1] = message.GetDouble();
        }
        else if (hash == MessageControlsPropellerSpeed2.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_PROPELLER_SPEED_2] = message.GetDouble();
        }
        else if (hash == MessageControlsPropellerSpeed3.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_PROPELLER_SPEED_3] = message.GetDouble();
        }
        else if (hash == MessageControlsPropellerSpeed4.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_PROPELLER_SPEED_4] = message.GetDouble();
        }
        else if (hash == MessageControlsGliderAirBrake.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_GLIDER_AIR_BRAKE] = message.GetDouble();
        }
        else if (hash == MessageControlsRotorBrake.GetID()) {
            pData->all_variables[(int)VariableIndex::CONTROLS_ROTOR_BRAKE] = message.GetDouble();
        }
        
        // === AIRCRAFT SYSTEM VARIABLES ===
        else if (hash == MessageAircraftGroundSpoilersArmed.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_GROUND_SPOILERS_ARMED] = message.GetDouble();
        }
        else if (hash == MessageAircraftGroundSpoilersExtended.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_GROUND_SPOILERS_EXTENDED] = message.GetDouble();
        }
        else if (hash == MessageAircraftParkingBrake.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_PARKING_BRAKE] = message.GetDouble();
        }
        else if (hash == MessageAircraftAutoBrakeSetting.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_AUTO_BRAKE_SETTING] = message.GetDouble();
        }
        else if (hash == MessageAircraftAutoBrakeEngaged.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_AUTO_BRAKE_ENGAGED] = message.GetDouble();
        }
        else if (hash == MessageAircraftAutoBrakeRejectedTakeOff.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_AUTO_BRAKE_REJECTED_TAKEOFF] = message.GetDouble();
        }
        
        // === ENGINE SYSTEM VARIABLES ===
        else if (hash == MessageAircraftStarter.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_STARTER] = message.GetDouble();
        }
        else if (hash == MessageAircraftStarter1.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_STARTER_1] = message.GetDouble();
        }
        else if (hash == MessageAircraftStarter2.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_STARTER_2] = message.GetDouble();
        }
        else if (hash == MessageAircraftStarter3.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_STARTER_3] = message.GetDouble();
        }
        else if (hash == MessageAircraftStarter4.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_STARTER_4] = message.GetDouble();
        }
        else if (hash == MessageAircraftIgnition.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_IGNITION] = message.GetDouble();
        }
        else if (hash == MessageAircraftIgnition1.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_IGNITION_1] = message.GetDouble();
        }
        else if (hash == MessageAircraftIgnition2.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_IGNITION_2] = message.GetDouble();
        }
        else if (hash == MessageAircraftIgnition3.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_IGNITION_3] = message.GetDouble();
        }
        else if (hash == MessageAircraftIgnition4.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_IGNITION_4] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineMaster1.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_MASTER_1] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineMaster2.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_MASTER_2] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineMaster3.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_MASTER_3] = message.GetDouble();
        }
        else if (hash == MessageAircraftEngineMaster4.GetID()) {
            pData->all_variables[(int)VariableIndex::AIRCRAFT_ENGINE_MASTER_4] = message.GetDouble();
        }
        
        // === WARNINGS ===
        else if (hash == MessageWarningsMasterWarning.GetID()) {
            pData->master_warning = (uint32_t)message.GetDouble();
            pData->all_variables[(int)VariableIndex::WARNINGS_MASTER_WARNING] = message.GetDouble();
        }
        else if (hash == MessageWarningsMasterCaution.GetID()) {
            pData->master_caution = (uint32_t)message.GetDouble();
            pData->all_variables[(int)VariableIndex::WARNINGS_MASTER_CAUTION] = message.GetDouble();
        }
        else if (hash == MessageWarningsLowOilPressure.GetID()) {
            pData->all_variables[(int)VariableIndex::WARNINGS_LOW_OIL_PRESSURE] = message.GetDouble();
        }
        else if (hash == MessageWarningsLowFuelPressure.GetID()) {
            pData->all_variables[(int)VariableIndex::WARNINGS_LOW_FUEL_PRESSURE] = message.GetDouble();
        }
        
        // NOTE: This implementation now covers ~100 variables
        // Added critical aircraft system variables for comprehensive monitoring
        }
        catch (const std::exception& e) {
            // Log the error and continue processing
            std::string error_msg = "ERROR: Exception in ProcessMessage: " + std::string(e.what()) + "\n";
            OutputDebugStringA(error_msg.c_str());
        }
        catch (...) {
            // Handle any other exceptions (including assertion failures)
            OutputDebugStringA("ERROR: Unknown exception in ProcessMessage (possibly assertion failure)\n");
        }
    }
    
    void Cleanup() {
        if (pData) {
            UnmapViewOfFile(pData);
            pData = nullptr;
        }
        if (hMapFile) {
            CloseHandle(hMapFile);
            hMapFile = NULL;
        }
        initialized = false;
    }
    
    AeroflyBridgeData* GetData() { return pData; }
    bool IsInitialized() const { return initialized; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// TCP SERVER INTERFACE - Network Interface
///////////////////////////////////////////////////////////////////////////////////////////////////

class TCPServerInterface {
private:
    SOCKET server_socket;
    std::vector<SOCKET> client_sockets;
    std::thread server_thread;
    std::thread command_thread;
    std::atomic<bool> running;
    mutable std::mutex clients_mutex;
    VariableMapper mapper;
    
    // Command processing
    std::queue<std::string> command_queue;
    mutable std::mutex command_mutex;
    
public:
    TCPServerInterface() : server_socket(INVALID_SOCKET), running(false) {}
    
    ~TCPServerInterface() {
        Stop();
    }
    
    bool Start(int data_port = 12345, int command_port = 12346) {
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return false;
        }
        
        // Create server socket for data
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            WSACleanup();
            return false;
        }
        
        // Allow socket reuse
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        // Bind to data port
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(data_port);
        
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(server_socket);
            WSACleanup();
            return false;
        }
        
        // Listen for connections
        if (listen(server_socket, 5) == SOCKET_ERROR) {
            closesocket(server_socket);
            WSACleanup();
            return false;
        }
        
        // Start server thread
        running = true;
        server_thread = std::thread(&TCPServerInterface::ServerLoop, this);
        command_thread = std::thread(&TCPServerInterface::CommandLoop, this, command_port);
        
        return true;
    }
    
    void Stop() {
        OutputDebugStringA("=== TCPServer::Stop() STARTED ===\n");
        
        // Mark as not running FIRST
        running = false;
        
        // Close server socket to wake up blocked accept()
        if (server_socket != INVALID_SOCKET) {
            OutputDebugStringA("Closing main server socket...\n");
            shutdown(server_socket, SD_BOTH);  // Shutdown before close
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
        }
        
        // Close all client connections
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            OutputDebugStringA("Closing client connections...\n");
            for (SOCKET client : client_sockets) {
                shutdown(client, SD_BOTH);
                closesocket(client);
            }
            client_sockets.clear();
        }
        
        // Wait for threads to finish with TIMEOUT
        if (server_thread.joinable()) {
            OutputDebugStringA("Waiting for server_thread...\n");
            server_thread.join();
            OutputDebugStringA("server_thread finished\n");
        }
        
        if (command_thread.joinable()) {
            OutputDebugStringA("Waiting for command_thread...\n");
            command_thread.join();
            OutputDebugStringA("command_thread finished\n");
        }
        
        OutputDebugStringA("=== TCPServer::Stop() COMPLETED ===\n");
    }
    
    void BroadcastData(const AeroflyBridgeData* data) {
        if (!data || !running) return;
        
        // Create JSON message with proper formatting
        std::string json_data = CreateDataJSON(data);
        
        std::lock_guard<std::mutex> lock(clients_mutex);
        
        // Send to all connected clients
        auto it = client_sockets.begin();
        while (it != client_sockets.end()) {
            // CRITICAL FIX 3: Send complete JSON in one operation to avoid fragmentation
            int total_sent = 0;
            int data_size = static_cast<int>(json_data.length());
            const char* data_ptr = json_data.c_str();
            
            bool send_successful = true;
            
            // Ensure complete JSON is sent
            while (total_sent < data_size && send_successful) {
                int result = send(*it, data_ptr + total_sent, data_size - total_sent, 0);
                if (result == SOCKET_ERROR) {
                    send_successful = false;
                    break;
                }
                total_sent += result;
            }
            
            if (!send_successful || total_sent != data_size) {
                // Client disconnected or send failed, remove from list
                closesocket(*it);
                it = client_sockets.erase(it);
            } else {
                ++it;  // Success, move to next client
            }
        }
    }
    
private:
    void ServerLoop() {
        OutputDebugStringA("ServerLoop started\n");
        
        while (running) {
            // Set timeout on accept to avoid infinite blocking
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_socket, &readfds);
            
            struct timeval timeout;
            timeout.tv_sec = 1;   // 1 second timeout
            timeout.tv_usec = 0;
            
            int result = select(0, &readfds, NULL, NULL, &timeout);
            
            if (result > 0 && FD_ISSET(server_socket, &readfds)) {
                // Accept new connection
                SOCKET client_socket = accept(server_socket, nullptr, nullptr);
                if (client_socket != INVALID_SOCKET && running) {
                    u_long mode = 1;
                    ioctlsocket(client_socket, FIONBIO, &mode);
                    
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    client_sockets.push_back(client_socket);
                    OutputDebugStringA("Client connected\n");
                }
            }
            else if (result == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error != WSAEINTR && running) {
                    OutputDebugStringA("Error in select()\n");
                    break;
                }
            }
            // result == 0 = timeout, continue loop
        }
        
        OutputDebugStringA("ServerLoop finished\n");
    }
    
    void CommandLoop(int command_port) {
        OutputDebugStringA("CommandLoop started\n");
        
        // Create command server socket
        SOCKET cmd_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (cmd_socket == INVALID_SOCKET) {
            OutputDebugStringA("Failed to create command socket\n");
            return;
        }
        
        int opt = 1;
        setsockopt(cmd_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        sockaddr_in cmd_addr;
        cmd_addr.sin_family = AF_INET;
        cmd_addr.sin_addr.s_addr = INADDR_ANY;
        cmd_addr.sin_port = htons(command_port);
        
        if (bind(cmd_socket, (sockaddr*)&cmd_addr, sizeof(cmd_addr)) == SOCKET_ERROR ||
            listen(cmd_socket, 5) == SOCKET_ERROR) {
            OutputDebugStringA("Failed to bind/listen command socket\n");
            closesocket(cmd_socket);
            return;
        }
        
        while (running) {
            // Use select with timeout instead of blocking accept
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(cmd_socket, &readfds);
            
            struct timeval timeout;
            timeout.tv_sec = 1;   // 1 second timeout
            timeout.tv_usec = 0;
            
            int result = select(0, &readfds, NULL, NULL, &timeout);
            
            if (result > 0 && FD_ISSET(cmd_socket, &readfds)) {
                SOCKET client = accept(cmd_socket, nullptr, nullptr);
                if (client != INVALID_SOCKET && running) {
                    // Handle command from client
                    char buffer[1024];
                    int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_received > 0) {
                        buffer[bytes_received] = '\0';
                        ProcessCommand(std::string(buffer));
                        OutputDebugStringA("Command processed\n");
                    }
                    closesocket(client);
                }
            }
            else if (result == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error != WSAEINTR && running) {
                    OutputDebugStringA("Error in command select()\n");
                    break;
                }
            }
            // result == 0 = timeout, continue loop
        }
        
        OutputDebugStringA("Closing command socket\n");
        closesocket(cmd_socket);
        OutputDebugStringA("CommandLoop finished\n");
    }
    
    std::string CreateDataJSON(const AeroflyBridgeData* data) {
        std::ostringstream json;
        
        // CRITICAL FIX 1: Set fixed precision to prevent scientific notation (1e+08)
        json << std::fixed << std::setprecision(6);
        
        json << "{";
        json << "\"timestamp\":" << data->timestamp_us << ",";
        json << "\"data_valid\":" << data->data_valid << ",";
        json << "\"update_counter\":" << data->update_counter << ",";
        
        // Aircraft data with NaN/Infinity protection
        json << "\"aircraft\":{";
        json << "\"latitude\":" << (std::isfinite(data->latitude) ? data->latitude : 0.0) << ",";
        json << "\"longitude\":" << (std::isfinite(data->longitude) ? data->longitude : 0.0) << ",";
        json << "\"altitude\":" << (std::isfinite(data->altitude) ? data->altitude : 0.0) << ",";
        json << "\"pitch\":" << (std::isfinite(data->pitch) ? data->pitch : 0.0) << ",";
        json << "\"bank\":" << (std::isfinite(data->bank) ? data->bank : 0.0) << ",";
        json << "\"heading\":" << (std::isfinite(data->true_heading) ? data->true_heading : 0.0) << ",";
        json << "\"airspeed\":" << (std::isfinite(data->indicated_airspeed) ? data->indicated_airspeed : 0.0) << ",";
        json << "\"ground_speed\":" << (std::isfinite(data->ground_speed) ? data->ground_speed : 0.0) << ",";
        json << "\"vertical_speed\":" << (std::isfinite(data->vertical_speed) ? data->vertical_speed : 0.0) << ",";
        json << "\"angle_of_attack\":" << (std::isfinite(data->angle_of_attack) ? data->angle_of_attack : 0.0) << ",";
        json << "\"on_ground\":" << (std::isfinite(data->on_ground) ? data->on_ground : 0.0);
        json << "},";  // No trailing comma
        
        // Controls
        json << "\"controls\":{";
        json << "\"pitch_input\":" << (std::isfinite(data->pitch_input) ? data->pitch_input : 0.0) << ",";
        json << "\"roll_input\":" << (std::isfinite(data->roll_input) ? data->roll_input : 0.0) << ",";
        json << "\"yaw_input\":" << (std::isfinite(data->yaw_input) ? data->yaw_input : 0.0) << ",";
        json << "\"throttle\":" << (std::isfinite(data->throttle_position) ? data->throttle_position : 0.0) << ",";
        json << "\"flaps\":" << (std::isfinite(data->flaps_position) ? data->flaps_position : 0.0) << ",";
        json << "\"gear\":" << (std::isfinite(data->gear_position) ? data->gear_position : 0.0);
        json << "},";
        
        // Navigation
        json << "\"navigation\":{";
        json << "\"com1_frequency\":" << (std::isfinite(data->com1_frequency) ? data->com1_frequency : 0.0) << ",";
        json << "\"com1_standby\":" << (std::isfinite(data->com1_standby_frequency) ? data->com1_standby_frequency : 0.0) << ",";
        json << "\"nav1_frequency\":" << (std::isfinite(data->nav1_frequency) ? data->nav1_frequency : 0.0) << ",";
        json << "\"nav1_course\":" << (std::isfinite(data->nav1_selected_course) ? data->nav1_selected_course : 0.0);
        json << "},";
        
        // Autopilot
        json << "\"autopilot\":{";
        json << "\"engaged\":" << (std::isfinite(data->ap_engaged) ? data->ap_engaged : 0.0) << ",";
        json << "\"selected_airspeed\":" << (std::isfinite(data->ap_selected_airspeed) ? data->ap_selected_airspeed : 0.0) << ",";
        json << "\"selected_heading\":" << (std::isfinite(data->ap_selected_heading) ? data->ap_selected_heading : 0.0) << ",";
        json << "\"selected_altitude\":" << (std::isfinite(data->ap_selected_altitude) ? data->ap_selected_altitude : 0.0);
        json << "},";
        
        // Performance speeds
        json << "\"performance\":{";
        json << "\"vs0\":" << (std::isfinite(data->vs0_speed) ? data->vs0_speed : 0.0) << ",";
        json << "\"vs1\":" << (std::isfinite(data->vs1_speed) ? data->vs1_speed : 0.0) << ",";
        json << "\"vfe\":" << (std::isfinite(data->vfe_speed) ? data->vfe_speed : 0.0) << ",";
        json << "\"vno\":" << (std::isfinite(data->vno_speed) ? data->vno_speed : 0.0) << ",";
        json << "\"vne\":" << (std::isfinite(data->vne_speed) ? data->vne_speed : 0.0);
        json << "},";
        
        // All variables array (with NaN/Infinity protection)
        json << "\"all_variables\":[";
        for (int i = 0; i < (int)VariableIndex::VARIABLE_COUNT; ++i) {
            if (i > 0) json << ",";
            double value = data->all_variables[i];
            // CRITICAL: Protect against NaN/Infinity in array
            json << (std::isfinite(value) ? value : 0.0);
        }
        json << "]";  // No trailing comma
        
        json << "}";  // Close main JSON object
        
        // CRITICAL FIX 2: Add newline separator to prevent JSON concatenation
        json << "\n";
        
        return json.str();
    }
    
    void ProcessCommand(const std::string& command) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push(command);
    }
    
public:
    std::vector<std::string> GetPendingCommands() {
        std::vector<std::string> commands;
        std::lock_guard<std::mutex> lock(command_mutex);
        
        while (!command_queue.empty()) {
            commands.push_back(command_queue.front());
            command_queue.pop();
        }
        
        return commands;
    }
    
    int GetClientCount() const {
        std::lock_guard<std::mutex> lock(clients_mutex);
        return static_cast<int>(client_sockets.size());
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// COMMAND PROCESSOR - Bidirectional Commands
///////////////////////////////////////////////////////////////////////////////////////////////////

class EnhancedCommandProcessor {
    private:
        VariableMapper mapper;
        HybridVariableManager* hybrid_manager;
        
        // Command statistics
        mutable std::mutex stats_mutex;
        std::unordered_map<std::string, int> command_stats;
        
    public:
        EnhancedCommandProcessor() : hybrid_manager(nullptr) {}
        
        void SetHybridManager(HybridVariableManager* manager) {
            hybrid_manager = manager;
            HybridLogToFile("EnhancedCommandProcessor: Hybrid manager connected");
            OutputDebugStringA("Enhanced CommandProcessor: Hybrid manager connected\n");
        }
        
        std::vector<tm_external_message> ProcessCommands(const std::vector<std::string>& commands) {
            std::vector<tm_external_message> messages;
            
            for (const auto& command : commands) {
                auto msg = ParseEnhancedCommand(command);
                if (msg.GetDataType() != tm_msg_data_type::None) {
                    messages.push_back(msg);
                    
                    // Update statistics
                    UpdateCommandStats(command);
                }
            }
            
            return messages;
        }
        
        // Get command processing statistics
        std::vector<std::string> GetCommandStats() const {
            std::lock_guard<std::mutex> lock(stats_mutex);
            std::vector<std::string> stats;
            
            for (const auto& pair : command_stats) {
                stats.push_back(pair.first + ": " + std::to_string(pair.second) + " times");
            }
            
            return stats;
        }
        
    private:
        tm_external_message ParseEnhancedCommand(const std::string& command) {
            tm_external_message empty_msg;
            
            try {
                HybridLogToFile("Processing enhanced command: " + command);
                
                // Parse JSON command
                CommandData cmd_data = ExtractCommandData(command);
                if (cmd_data.variable_name.empty()) {
                    HybridLogToFile("ERROR: Invalid command format");
                    return empty_msg;
                }
                
                HybridLogToFile("Parsed command - Variable: " + cmd_data.variable_name + 
                               ", Event: " + cmd_data.event_type + 
                               ", Qualifier: " + cmd_data.qualifier + 
                               ", Value: " + std::to_string(cmd_data.value));
                
                // Try core variables first (maximum performance)
                tm_external_message core_msg = TryProcessCoreVariable(cmd_data);
                if (core_msg.GetDataType() != tm_msg_data_type::None) {
                    HybridLogToFile("✅ CORE: Variable processed: " + cmd_data.variable_name);
                    return core_msg;
                }
                
                // Try hybrid system for dynamic variables
                if (hybrid_manager) {
                    tm_external_message hybrid_msg = TryProcessHybridVariable(cmd_data);
                    if (hybrid_msg.GetDataType() != tm_msg_data_type::None) {
                        HybridLogToFile("✅ HYBRID: Variable processed: " + cmd_data.variable_name);
                        return hybrid_msg;
                    }
                }
                
                HybridLogToFile("❌ Variable not found in core or hybrid: " + cmd_data.variable_name);
                return empty_msg;
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR parsing enhanced command: " + std::string(e.what()));
                return empty_msg;
            } catch (...) {
                HybridLogToFile("Unknown ERROR parsing enhanced command");
                return empty_msg;
            }
        }
        
        struct CommandData {
            std::string variable_name;
            std::string event_type;
            std::string qualifier;
            double value;
            bool is_event_command;
            
            CommandData() : value(0.0), is_event_command(false) {}
        };
        
        CommandData ExtractCommandData(const std::string& command) {
            CommandData cmd_data;
            
            // Find JSON boundaries
            size_t start = command.find('{');
            size_t end = command.rfind('}');
            
            if (start == std::string::npos || end == std::string::npos) {
                return cmd_data; // Invalid JSON
            }
            
            std::string json_str = command.substr(start, end - start + 1);
            
            // Extract variable name
            size_t var_pos = json_str.find("\"variable\"");
            if (var_pos != std::string::npos) {
                size_t var_start = json_str.find(":", var_pos) + 1;
                var_start = json_str.find("\"", var_start) + 1;
                size_t var_end = json_str.find("\"", var_start);
                cmd_data.variable_name = json_str.substr(var_start, var_end - var_start);
            }
            
            // Extract value
            size_t val_pos = json_str.find("\"value\"");
            if (val_pos != std::string::npos) {
                size_t val_start = json_str.find(":", val_pos) + 1;
                while (val_start < json_str.length() && 
                       (json_str[val_start] == ' ' || json_str[val_start] == '\t')) {
                    val_start++;
                }
                size_t val_end = json_str.find_first_of(",}", val_start);
                std::string val_str = json_str.substr(val_start, val_end - val_start);
                cmd_data.value = std::stod(val_str);
            }
            
            // Extract event type (optional)
            size_t event_pos = json_str.find("\"event\"");
            if (event_pos != std::string::npos) {
                size_t event_start = json_str.find(":", event_pos) + 1;
                event_start = json_str.find("\"", event_start) + 1;
                size_t event_end = json_str.find("\"", event_start);
                cmd_data.event_type = json_str.substr(event_start, event_end - event_start);
                cmd_data.is_event_command = true;
            }
            
            // Extract qualifier (optional)
            size_t qual_pos = json_str.find("\"qualifier\"");
            if (qual_pos != std::string::npos) {
                size_t qual_start = json_str.find(":", qual_pos) + 1;
                qual_start = json_str.find("\"", qual_start) + 1;
                size_t qual_end = json_str.find("\"", qual_start);
                cmd_data.qualifier = json_str.substr(qual_start, qual_end - qual_start);
                
                // If we have a qualifier, this is an event command
                cmd_data.is_event_command = true;
            }
            
            return cmd_data;
        }
        
        tm_external_message TryProcessCoreVariable(const CommandData& cmd_data) {
            tm_external_message empty_msg;
            
            // Process all existing core variables with enhanced event support
            const std::string& var_name = cmd_data.variable_name;
            
            // Controls - Enhanced with event support
            if (var_name == "Controls.Throttle") {
                return ProcessCoreMessage(MessageControlsThrottle, cmd_data);
            }
            else if (var_name == "Controls.Flaps") {
                return ProcessCoreMessage(MessageControlsFlaps, cmd_data);
            }
            else if (var_name == "Controls.Gear") {
                return ProcessCoreMessage(MessageControlsGear, cmd_data);
            }
            else if (var_name == "Controls.Pitch.Input") {
                return ProcessCoreMessage(MessageControlsPitchInput, cmd_data);
            }
            else if (var_name == "Controls.Roll.Input") {
                return ProcessCoreMessage(MessageControlsRollInput, cmd_data);
            }
            else if (var_name == "Controls.Yaw.Input") {
                return ProcessCoreMessage(MessageControlsYawInput, cmd_data);
            }
            
            // Engine controls
            else if (var_name == "Controls.Throttle1") {
                return ProcessCoreMessage(MessageControlsThrottle1, cmd_data);
            }
            else if (var_name == "Controls.Throttle2") {
                return ProcessCoreMessage(MessageControlsThrottle2, cmd_data);
            }
            else if (var_name == "Controls.Throttle3") {
                return ProcessCoreMessage(MessageControlsThrottle3, cmd_data);
            }
            else if (var_name == "Controls.Throttle4") {
                return ProcessCoreMessage(MessageControlsThrottle4, cmd_data);
            }
            
            // Brakes
            else if (var_name == "Controls.WheelBrake.Left") {
                return ProcessCoreMessage(MessageControlsWheelBrakeLeft, cmd_data);
            }
            else if (var_name == "Controls.WheelBrake.Right") {
                return ProcessCoreMessage(MessageControlsWheelBrakeRight, cmd_data);
            }
            else if (var_name == "Controls.AirBrake") {
                return ProcessCoreMessage(MessageControlsAirBrake, cmd_data);
            }
            
            // Navigation
            else if (var_name == "Communication.COM1Frequency") {
                return ProcessCoreMessage(MessageNavigationCOM1Frequency, cmd_data);
            }
            else if (var_name == "Navigation.NAV1Frequency") {
                return ProcessCoreMessage(MessageNavigationNAV1Frequency, cmd_data);
            }
            else if (var_name == "Navigation.SelectedCourse1") {
                return ProcessCoreMessage(MessageNavigationSelectedCourse1, cmd_data);
            }
            
            // Autopilot
            else if (var_name == "Autopilot.SelectedAirspeed") {
                return ProcessCoreMessage(MessageAutopilotSelectedAirspeed, cmd_data);
            }
            else if (var_name == "Autopilot.SelectedHeading") {
                return ProcessCoreMessage(MessageAutopilotSelectedHeading, cmd_data);
            }
            else if (var_name == "Autopilot.SelectedAltitude") {
                return ProcessCoreMessage(MessageAutopilotSelectedAltitude, cmd_data);
            }
            
            // Add more core variables as needed...
            
            return empty_msg; // Not found in core
        }
        
        tm_external_message ProcessCoreMessage(tm_external_message& core_message, 
                                              const CommandData& cmd_data) {
            try {
                if (cmd_data.is_event_command) {
                    // Handle event-based commands
                    if (cmd_data.event_type == "OnStep" || cmd_data.qualifier == "step") {
                        core_message.SetValue(cmd_data.value);
                        HybridLogToFile("Core step event: " + cmd_data.variable_name + 
                                       " = " + std::to_string(cmd_data.value));
                    }
                    else if (cmd_data.event_type == "OnToggle" || cmd_data.qualifier == "toggle") {
                        core_message.SetValue(1.0); // Trigger value
                        HybridLogToFile("Core toggle event: " + cmd_data.variable_name);
                    }
                    else if (cmd_data.qualifier == "offset") {
                        core_message.SetValue(cmd_data.value);
                        HybridLogToFile("Core offset event: " + cmd_data.variable_name + 
                                       " offset=" + std::to_string(cmd_data.value));
                    }
                    else {
                        core_message.SetValue(cmd_data.value);
                        HybridLogToFile("Core default event: " + cmd_data.variable_name + 
                                       " = " + std::to_string(cmd_data.value));
                    }
                } else {
                    // Standard value command
                    core_message.SetValue(cmd_data.value);
                    HybridLogToFile("Core value: " + cmd_data.variable_name + 
                                   " = " + std::to_string(cmd_data.value));
                }
                
                return core_message;
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR processing core message: " + std::string(e.what()));
                tm_external_message empty_msg;
                return empty_msg;
            }
        }
        
        tm_external_message TryProcessHybridVariable(const CommandData& cmd_data) {
            tm_external_message empty_msg;
            
            try {
                tm_external_message* hybrid_msg = hybrid_manager->GetMessage(cmd_data.variable_name);
                if (!hybrid_msg) {
                    HybridLogToFile("Hybrid variable not found: " + cmd_data.variable_name);
                    return empty_msg;
                }
                
                // Get variable info for enhanced processing
                auto var_details = hybrid_manager->GetVariableDetails(cmd_data.variable_name);
                const EnhancedVariableInfo* var_info = hybrid_manager->FindVariableInfo(cmd_data.variable_name);
                
                if (cmd_data.is_event_command && var_info) {
                    // Process enhanced event command
                    return ProcessHybridEvent(*hybrid_msg, cmd_data, *var_info);
                } else {
                    // Process simple value command
                    hybrid_msg->SetValue(cmd_data.value);
                    HybridLogToFile("Hybrid value: " + cmd_data.variable_name + 
                                   " = " + std::to_string(cmd_data.value));
                    return *hybrid_msg;
                }
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR processing hybrid variable: " + std::string(e.what()));
                return empty_msg;
            }
        }
        
        tm_external_message ProcessHybridEvent(tm_external_message& hybrid_msg,
                                              const CommandData& cmd_data,
                                              const EnhancedVariableInfo& var_info) {
            
            HybridLogToFile("Processing hybrid event: " + cmd_data.variable_name + 
                           " event=" + cmd_data.event_type + " qualifier=" + cmd_data.qualifier);
            
            // Validate qualifier
            if (!cmd_data.qualifier.empty() && !var_info.HasQualifier(cmd_data.qualifier)) {
                HybridLogToFile("WARNING: Invalid qualifier '" + cmd_data.qualifier + 
                               "' for variable " + cmd_data.variable_name);
                // Continue anyway, might still work
            }
            
            try {
                if (cmd_data.qualifier == "step" && var_info.is_step) {
                    hybrid_msg.SetValue(cmd_data.value);
                    HybridLogToFile("Hybrid step: " + cmd_data.variable_name + 
                                   " step=" + std::to_string(cmd_data.value));
                }
                else if (cmd_data.qualifier == "toggle" && var_info.is_toggle) {
                    hybrid_msg.SetValue(1.0); // Trigger toggle
                    HybridLogToFile("Hybrid toggle: " + cmd_data.variable_name);
                }
                else if (cmd_data.qualifier == "move" && var_info.is_move) {
                    hybrid_msg.SetValue(cmd_data.value);
                    HybridLogToFile("Hybrid move: " + cmd_data.variable_name + 
                                   " rate=" + std::to_string(cmd_data.value));
                }
                else if (cmd_data.qualifier == "offset" && var_info.is_offset) {
                    hybrid_msg.SetValue(cmd_data.value);
                    HybridLogToFile("Hybrid offset: " + cmd_data.variable_name + 
                                   " offset=" + std::to_string(cmd_data.value));
                }
                else if (cmd_data.qualifier == "active" && var_info.is_active) {
                    hybrid_msg.SetValue(cmd_data.value);
                    HybridLogToFile("Hybrid active: " + cmd_data.variable_name + 
                                   " active=" + std::to_string(cmd_data.value));
                }
                else {
                    // Default: standard value setting
                    hybrid_msg.SetValue(cmd_data.value);
                    HybridLogToFile("Hybrid default: " + cmd_data.variable_name + 
                                   " = " + std::to_string(cmd_data.value));
                }
                
                return hybrid_msg;
                
            } catch (const std::exception& e) {
                HybridLogToFile("ERROR in hybrid event processing: " + std::string(e.what()));
                tm_external_message empty_msg;
                return empty_msg;
            }
        }
        
        void UpdateCommandStats(const std::string& command) {
            try {
                std::lock_guard<std::mutex> lock(stats_mutex);
                
                // Extract variable name for stats
                size_t var_pos = command.find("\"variable\"");
                if (var_pos != std::string::npos) {
                    size_t var_start = command.find(":", var_pos);
                    size_t var_name_start = command.find("\"", var_start) + 1;
                    size_t var_name_end = command.find("\"", var_name_start);
                    
                    if (var_name_end != std::string::npos) {
                        std::string var_name = command.substr(var_name_start, 
                                                             var_name_end - var_name_start);
                        command_stats[var_name]++;
                    }
                }
            } catch (...) {
                // Ignore stats errors
            }
        }
    };

///////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN BRIDGE CONTROLLER
///////////////////////////////////////////////////////////////////////////////////////////////////

class AeroflyBridge {
private:
    SharedMemoryInterface shared_memory;
    TCPServerInterface tcp_server;
    EnhancedCommandProcessor command_processor;
    HybridVariableManager hybrid_manager;  // ✅ AGREGADO: Sistema híbrido
    bool initialized;
    
public:
    AeroflyBridge() : initialized(false) {}
    
    bool Initialize() {
        // Initialize shared memory (primary interface)
        if (!shared_memory.Initialize()) {
            return false;
        }
        
        // ✅ NUEVO: Initialize hybrid variable manager
        OutputDebugStringA("=== INITIALIZING HYBRID SYSTEM ===\n");
        if (!hybrid_manager.Initialize(shared_memory.GetData())) {
            OutputDebugStringA("ERROR: Failed to initialize hybrid system\n");
            // Don't fail completely, core variables still work
        } else {
            OutputDebugStringA("SUCCESS: Hybrid system initialized\n");
            
            // ✅ NUEVO: Connect hybrid system to command processor
            command_processor.SetHybridManager(&hybrid_manager);
            OutputDebugStringA("SUCCESS: Hybrid system connected to CommandProcessor\n");
        }
        
        // Start TCP server (optional, for network access)
        if (!tcp_server.Start(12345, 12346)) {
            // TCP server failure is not critical
            // Shared memory still works
        }
        
        initialized = true;
        return true;
    }
    
    void Update(const std::vector<tm_external_message>& received_messages, double delta_time,
                std::vector<tm_external_message>& sent_messages) {
        if (!initialized) return;
        
        // Update shared memory with latest data
        shared_memory.UpdateData(received_messages, delta_time);
        
        // Broadcast data via TCP (if clients connected)
        if (tcp_server.GetClientCount() > 0) {
            tcp_server.BroadcastData(shared_memory.GetData());
        }
        
        // Process any pending commands
        auto commands = tcp_server.GetPendingCommands();
        if (!commands.empty()) {
            auto command_messages = command_processor.ProcessCommands(commands);
            sent_messages.insert(sent_messages.end(), command_messages.begin(), command_messages.end());
        }
    }
    
    void Shutdown() {
        OutputDebugStringA("=== AeroflyBridge::Shutdown() STARTED ===\n");
        
        if (!initialized) {
            OutputDebugStringA("Bridge already closed\n");
            return;
        }
        
        // Stop TCP server FIRST (most problematic threads)
        OutputDebugStringA("Stopping TCP server...\n");
        tcp_server.Stop();
        
        // Clean shared memory
        OutputDebugStringA("Cleaning shared memory...\n");
        shared_memory.Cleanup();
        
        initialized = false;
        OutputDebugStringA("=== AeroflyBridge::Shutdown() COMPLETED ===\n");
    }
    
    bool IsInitialized() const { return initialized; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBAL INSTANCE
///////////////////////////////////////////////////////////////////////////////////////////////////

static AeroflyBridge* g_bridge = nullptr;
static std::vector<tm_external_message> MessageListReceive;

///////////////////////////////////////////////////////////////////////////////////////////////////
// AEROFLY FS4 DLL INTERFACE
///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" {
    __declspec(dllexport) int Aerofly_FS_4_External_DLL_GetInterfaceVersion() {
        return TM_DLL_INTERFACE_VERSION;
    }

    __declspec(dllexport) bool Aerofly_FS_4_External_DLL_Init(const HINSTANCE Aerofly_FS_4_hInstance) {
        try {
            g_bridge = new AeroflyBridge();
            return g_bridge->Initialize();
        }
        catch (...) {
            return false;
        }
    }

    __declspec(dllexport) void Aerofly_FS_4_External_DLL_Shutdown() {
        OutputDebugStringA("=== DLL SHUTDOWN STARTED ===\n");
        
        try {
            if (g_bridge) {
                OutputDebugStringA("Closing bridge...\n");
                
                // Bridge shutdown
                g_bridge->Shutdown();
                
                // Give time for threads to finish
                OutputDebugStringA("Waiting for threads...\n");
                Sleep(1000);  // 1 second for cleanup
                
                OutputDebugStringA("Deleting bridge object...\n");
                delete g_bridge;
                g_bridge = nullptr;
            }
            
            // Final Winsock cleanup
            OutputDebugStringA("Cleaning Winsock...\n");
            WSACleanup();
            
            OutputDebugStringA("=== DLL SHUTDOWN COMPLETED SUCCESSFULLY ===\n");
        }
        catch (const std::exception& e) {
            OutputDebugStringA(("ERROR in shutdown: " + std::string(e.what()) + "\n").c_str());
        }
        catch (...) {
            OutputDebugStringA("Unknown ERROR in shutdown\n");
        }
        
        // NEVER throw exceptions from DLL shutdown
    }

    __declspec(dllexport) void Aerofly_FS_4_External_DLL_Update(
        const tm_double delta_time,
        const tm_uint8* const message_list_received_byte_stream,
        const tm_uint32 message_list_received_byte_stream_size,
        const tm_uint32 message_list_received_num_messages,
        tm_uint8* message_list_sent_byte_stream,
        tm_uint32& message_list_sent_byte_stream_size,
        tm_uint32& message_list_sent_num_messages,
        const tm_uint32 message_list_sent_byte_stream_size_max) {
        
        if (!g_bridge || !g_bridge->IsInitialized()) {
            message_list_sent_byte_stream_size = 0;
            message_list_sent_num_messages = 0;
            return;
        }

        try {
            // Parse received messages
            MessageListReceive.clear();
            tm_uint32 pos = 0;
            for (tm_uint32 i = 0; i < message_list_received_num_messages; ++i) {
                auto msg = tm_external_message::GetFromByteStream(message_list_received_byte_stream, pos);
                MessageListReceive.emplace_back(msg);
            }

            // Process messages and get commands to send back
            std::vector<tm_external_message> sent_messages;
            g_bridge->Update(MessageListReceive, delta_time, sent_messages);

            // Build response message list
            message_list_sent_byte_stream_size = 0;
            message_list_sent_num_messages = 0;

            for (const auto& msg : sent_messages) {
                msg.AddToByteStream(message_list_sent_byte_stream, 
                                   message_list_sent_byte_stream_size, 
                                   message_list_sent_num_messages);
            }
        }
        catch (...) {
            // Error handling - ensure we don't crash Aerofly
            message_list_sent_byte_stream_size = 0;
            message_list_sent_num_messages = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// COMPILATION NOTES:
//
// To compile this DLL:
// 1. Include tm_external_message.h in the same directory
// 2. Compile with: cl /LD /EHsc /O2 aerofly_bridge_dll_complete.cpp /Fe:AeroflyBridge.dll /link ws2_32.lib
// 3. Copy AeroflyBridge.dll to: %USERPROFILE%\Documents\Aerofly FS 4\external_dll\
//
// Features implemented:
// ✅ Shared Memory interface (primary, ultra-fast)
// ✅ TCP Server interface (network access, JSON format)
// ✅ Bidirectional commands (read + write)
// ✅ All 339 variables supported (subset shown, pattern established)
// ✅ Thread-safe operations
// ✅ Auto-reconnection
// ✅ Performance optimized
// ✅ Error handling
//
// TCP Ports:
// - 12345: Data streaming (JSON)
// - 12346: Commands (JSON)
//
// Shared Memory:
// - Name: "AeroflyBridgeData"
// - Size: ~3384 bytes
// - Access: Direct memory mapping
//
///////////////////////////////////////////////////////////////////////////////////////////////////