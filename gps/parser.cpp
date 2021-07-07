#include "parser.hpp"

// simple HEX to DEC conversion
uint8_t hexToDec(std::string hex)
{
    uint8_t receivedCs = 0;
    if (hex[0] > '9')
        receivedCs = (hex[0] - 'A' + 10) * 16;
    else
        receivedCs = (hex[0] - '0') * 16;
    if (hex[1] > '9')
        receivedCs += hex[1] - 'A' + 10;
    else
        receivedCs += hex[1] - '0';
    return receivedCs;
}

bool Parser::parse_RMC(std::string raw_line, sbt_RMC_msg &msg)
{
    bool result = false;
    std::vector<std::string> raw_msg(g_RMC_field_count + 1, "");
    std::string tmp;
    size_t  pos;
    uint8_t cs = 0;

    if (raw_line.find("GPRMC") == std::string::npos &&
        raw_line.find("GNRMC") == std::string::npos)
        return false;

    for (char c: raw_line)
    {
        if (pos != RMCFieldID::cs && c != '*')
            cs ^= c;

        switch (c)
        {
        case '$':
            pos = RMCFieldID::msgType;   // pos = 0
            break;
        case ',':
            raw_msg[pos] = tmp;
            tmp.clear();
            pos++;
            if (pos > RMCFieldID::cs)
            {
                printf("Exceeded raw_msg buffer\n");
                return false;
            }
            break;
        case '*':
            raw_msg[pos] = tmp;
            tmp.clear();
            pos = RMCFieldID::cs;
            break;
        case '\r':
            raw_msg[pos] = tmp;
            break;
        case '\n':
            break;
        default:
            tmp += c;
        }

    }
    // log frame
    // printf("[%s]", raw_line.c_str());
    //for (auto a: raw_msg) printf("[%s]", a.c_str()); puts("");

    if (raw_msg[RMCFieldID::msgType] != "GPRMC" &&
        raw_msg[RMCFieldID::msgType] != "GNRMC")
    {
        // throw "Unknow msg type - " + raw_msg[RMCFieldID::msgType];
        printf("Recived other msg: [%s]\n", raw_msg[RMCFieldID::msgType].c_str());
        return false;
    }
    sbt_RMC_msg tmp_msg;
    result = parse_raw_RMC_msg(raw_msg, cs, tmp_msg);
    if (result == true)
        msg = tmp_msg;
    return result;
}

bool Parser::parse_raw_RMC_msg(const std::vector<std::string> &raw_msg, int cs, sbt_RMC_msg &msg)
{
    // catch stoi() exception
    try{
        // get time
        if (raw_msg[RMCFieldID::time].size() == 9)
        {
            msg.UTC_time_t.h  = std::stoi(raw_msg[RMCFieldID::time].substr(0,2));
            msg.UTC_time_t.m  = std::stoi(raw_msg[RMCFieldID::time].substr(2,2));
            msg.UTC_time_t.s  = std::stoi(raw_msg[RMCFieldID::time].substr(4,2));
            msg.UTC_time_t.ms = std::stoi(raw_msg[RMCFieldID::time].substr(7,2));
        }

        uint8_t receivedCs = hexToDec(raw_msg[RMCFieldID::cs]);

        //if (receivedCs != cs)
        //    return false;

        if (raw_msg[RMCFieldID::status] == "A")
        {
            msg.status = RMCFieldID::Status::Valid;
            msg.Date_t.d = std::stoi(raw_msg[RMCFieldID::date].substr(0,2));
            msg.Date_t.m = std::stoi(raw_msg[RMCFieldID::date].substr(2,2));
            msg.Date_t.y = std::stoi(raw_msg[RMCFieldID::date].substr(4,2));

            try{
                msg.Position_t.lat = position_to_deg(raw_msg[RMCFieldID::lat]);
                msg.Position_t.lon = position_to_deg(raw_msg[RMCFieldID::lon]);

                if (raw_msg[RMCFieldID::NS] == "S")
                    msg.Position_t.lat *= -1;
                if (raw_msg[RMCFieldID::EW] == "W")
                    msg.Position_t.lon *= -1;
            }
            catch (const char *what)
            {
                // invalid format
                msg.Position_t.lat = 0.0f;
                msg.Position_t.lon = 0.0f;
                msg.status = RMCFieldID::Status::Invalid;
                printf("INV pos %s - %d\n", __FILE__, __LINE__);
                return false;
            }

            if (!raw_msg[RMCFieldID::spd].empty())
            {
                msg.speed = std::stod(raw_msg[RMCFieldID::spd]);
                msg.speed = msg.speed * 1.852001;                   // knts -> km_h
            }
            else
                msg.speed = -1;

            if (!raw_msg[RMCFieldID::cog].empty())
                msg.cog  = std::stod(raw_msg[RMCFieldID::cog]);
            else
                msg.cog = -1;

            return true;
        }
        else
        {
            printf("Invalid gps data");
            msg.status = RMCFieldID::Status::Invalid;
            msg.Date_t.d = 0;
            msg.Date_t.m = 0;
            msg.Date_t.y = 0;

            msg.Position_t.lat = 0.0f;
            msg.Position_t.lon = 0.0f;
            msg.speed          = 0.0f;
            msg.cog            = 0.0f;
        }
        return false;
    }
    catch (...)
    {
        msg.status = RMCFieldID::Status::Invalid;
        printf("Stoi %s - %d\n", __FILE__, __LINE__);
        return false;
    }
}

double Parser::position_to_deg(const std::string &raw_pos)
{
    double pos_deg;

    size_t dot_pos = raw_pos.find('.');
    if (dot_pos == std::string::npos)
        throw "Invalid position format!";

    size_t dm_sep = dot_pos - 2;                       // deg and min separation point
    pos_deg  = std::stod(raw_pos.substr(0, dm_sep));   // pure deg
    pos_deg += std::stod(raw_pos.substr(dm_sep)) / 60; // conversion: min -> deg

    return pos_deg;
}
