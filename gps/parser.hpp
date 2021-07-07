#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

#include "msgs/NMEA.hpp"

class Parser  
{
	public:
		Parser() = delete;

	static bool parse_RMC(std::string raw_line, sbt_RMC_msg &msg);

	private:
	static bool   parse_raw_RMC_msg(const std::vector<std::string> &rawMsg, int cs, sbt_RMC_msg &msg);
	static double position_to_deg(const std::string &raw_pos); 
};
#endif