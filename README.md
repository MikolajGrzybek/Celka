# Celka

This repository is a summary of my activity as an embedded software developer in the AGH Solarboat student research group.
The code for each systems that I have developed is stored on individual branches

## Branch Celka-cooling_algorithm
This is an implementation of the algorithm for hysteresis control of the pump for cooling the boat engines. The control is based on reading the temperature of both motors, their analysis and switching the pump on and off at the right moment. The algorithm additionally analyzes the values of the two flow meters. The following errors are checked for: leakage of the cooling system, blockage or damage to the pump, overheating of the motors, error in temperature reading and communication error on the FreeRTOS system. 

## Branch Celka-bilge_pump_algorithm
Implementation of the bilge pump control algorithm. Measurement of the presence of water is performed by cyclically switching the pump on and testing the pump current value. Work in progress...

## License
[CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/)
