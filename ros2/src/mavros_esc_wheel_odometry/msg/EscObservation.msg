# Internal normalized ESC report. No wheel odometry is transported here.
# header.stamp is the most recent genuine RPM/voltage/current observation.
# metadata_stamp independently dates temperature/failure/error information.
std_msgs/Header header
builtin_interfaces/Time metadata_stamp
uint8 SOURCE_COMMON=1
uint8 SOURCE_ARDUPILOT_LEGACY=2
uint8 source
uint8 esc_index
bool valid
int32 rpm
bool rpm_valid
# Legacy magnitude is usable for blades, never signed wheel direction.
bool rpm_direction_valid
float32 voltage
bool voltage_valid
float32 current
bool current_valid
float32 temperature
bool temperature_valid
uint16 failure_flags
bool failure_flags_valid
uint32 error_count
bool error_count_valid
# Retained only for existing bridge diagnostics; unavailable for COMMON.
float32 totalcurrent
bool totalcurrent_valid
uint16 count
bool count_valid
