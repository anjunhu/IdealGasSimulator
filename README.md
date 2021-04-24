# ECSE444 Final Project: Interactive IdealGasSimulator
## Table of contents
* [General info](#general-info)
* [Features](#features)
## General info
The aim of this project is to design and implement an Interactive Ideal Gas Simulator that demonstrates the interplay of work and internal energy in thermodynamic systems, involving tools such as multiple I2C sensors, ADC, DAC and UART.
## Features
* ADC (analog to digital converter)\
- In this project, we will use the ADC to calculate the internal energy (Particle Kinetic Energy) of a box of gas from its internal temperature in Kelvins.
* I2C Sensor\
- We will use the I2C sensors to judge whether the user is doing 'work' on the system and further change its energy balance. To obtain ambient temperature, pressure and the external work, we utilize the Inter-Integrated Circuit interface moduels which provides us with a variety of useful sensor. In this project, we configured I2C2 with PB10 and PB11 to get the required measurements. In particular, we will use a L4S51 gyrometer, a LPS22HB pressure sensor and a HTS221 temperature.
* DAC output
* Kalman Filter
* GUI with UART
