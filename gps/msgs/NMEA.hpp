#ifndef NMEAMESSAGES_HPP
#define NMEAMESSAGES_HPP

// C++ standard headers

#include <string>
#include <cstdint> // uint8_t

// NMEA messages
// https://www.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_%28UBX-13003221%29.pdf

// xxRMC - page 157
// RMC - recomended mimnimum data
namespace RMCFieldID 
{
    enum RMCFieldID : uint8_t
    {
        msgType,   // string     - Message ID - should be xxRMC
        time,      // hhmmss.ss  - UTC time
        status,    // character  - Data validity status, A - valid | V - invalid
        lat,       // ddmm.mmmmm - Latitude (degrees and minutes)
        NS,        // character  - North/South indicator
        lon,       // ddmm.mmmmm - Longitude (degrees and minutes)
        EW,        // character  - East/West indicator
        spd,       // numeric    - Speed over ground (km/s = spd * 1.852001)
        cog,       // numeric    - Course over ground
        date,      // ddmmyy     - UTC date
        mv,        // numeric    - Magnetic var. val.
        mvEW,      // character  - Magnetic variation
        posMode,   // character  - Mode Indicator
        navStatus, // character  - Navigational status indicator
        cs         // hex        - Checksum
    };
    // Data validity status, A - valid | V - invalid
	enum Status
    {
		Invalid = 0,	
		Valid 	= 1
	};
    // NMEA Positioning System Mode Indicator
    enum PSMI
    {
        A,          // Autonomous
        D,          // Differential
        E,          // Estimated (dead reckoning) mode
        M,          // Manual input
        N           // Data not valid
    };
}
constexpr uint8_t g_RMC_field_count = static_cast<uint8_t>(RMCFieldID::cs); //last field - <CR,LF>

struct sbt_RMC_msg
{
    bool status;    // Data validity status

    struct
    {
        uint8_t h;
        uint8_t m;
        uint8_t s;
        uint8_t ms;
    } UTC_time_t;

    struct
    {
        uint8_t d;
        uint8_t m;
        uint8_t y;
    } Date_t;

    struct
    {
        double lat; // [deg], positive value means its towards North otherwise South
        double lon; // [deg], positive value means its towards East  otherwise West
    } Position_t;

    double speed;  // [km/h]
	double cog;    // [deg]
};

#endif // NMEAMESSAGES_HPP