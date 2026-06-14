OrbitOS: Adaptive Spacecraft Communication System

Project Overview

OrbitOS is a FreeRTOS-based simulation of a spacecraft orbital mission communication system developed as part of an Operating Systems course project. The system demonstrates how real-time operating system concepts can be applied to mission-critical environments by adapting system behavior across different spacecraft mission phases.

The simulation models spacecraft operations during Launch, Ascent, and Orbit phases while supporting telemetry communication, dynamic task scheduling, fault detection, emergency handling, and system monitoring.

⸻

Features

* Multi-tasking using FreeRTOS
* Mission phase simulation (Launch, Ascent, Orbit)
* Dynamic priority switching based on mission phase
* Queue-based inter-task communication
* Telemetry generation with phase-specific sensors
* Logging and message tracking
* Console-based system monitoring dashboard
* Fault detection using predefined sensor thresholds
* Emergency task activation and handling
* Runtime statistics including heap monitoring

⸻

System Architecture

The system consists of four primary FreeRTOS tasks:

* Telemetry Task
    * Generates telemetry data from spacecraft sensors.
    * Adapts telemetry frequency and priority according to mission phase.
* Logging Task
    * Receives telemetry messages from the queue.
    * Records communication events and message statistics.
* Phase Control Task
    * Controls transitions between Launch, Ascent, and Orbit phases.
    * Updates scheduling parameters dynamically.
* Emergency Task
    * Detects critical sensor conditions.
    * Activates emergency handling procedures and records emergency events.

⸻

Mission Phases and Telemetry

Launch Phase

Example telemetry:

* Engine Temperature
* Fuel Pressure
* Thrust
* Vibration

Ascent Phase

Example telemetry:

* Altitude
* Velocity
* Fuel Remaining
* External Temperature

Orbit Phase

Example telemetry:

* Battery Level
* Signal Strength
* Solar Panel Output
* Internal Temperature

⸻

Technologies Used

* C Programming Language
* FreeRTOS
* Microsoft Visual Studio
* Windows Environment

⸻

Demonstrated Results

* Successful queue-based communication
* Dynamic mission phase transitions
* Adaptive telemetry scheduling
* 100% message delivery under normal conditions
* Fault detection and emergency activation
* Real-time dashboard monitoring

⸻

Future Improvements

* Watchdog implementation
* Command processing task
* Hardware integration with embedded platforms
* Advanced fault prediction techniques

⸻

Team

OrbitOS

Operating Systems Final Project
