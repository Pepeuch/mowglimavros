# Direct, lossless projection of MAVLink BATTERY_STATUS for consumers which
# need to distinguish MAVLink's unavailable sentinels from measured zeroes.
std_msgs/Header header
uint8 id
float32 voltage
bool voltage_available
float32 current
bool current_available
float32 percentage
bool percentage_available
uint8 charge_state
