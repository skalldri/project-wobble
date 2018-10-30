# High Level
The DRV8825 Stepper Driver chip accepts one form of command: a STEP signal that is pulsed for ~20uS to indicate that it needs to step the motor.

To rotate the stepper motor continuously, the host MCU must generate a signal consisting of a single STEP pulse for ~20 uS followed by a variable length delay, depending on the desired steps per second.

## Potential Control Mechanisms

### PWM
PWM is a potential control method for driving the stepper motors, since it is designed to output pulses of a particularly defined width. However, PWM is not suitable for this application since it cannot alter the frequency of the PWM signal with enough fidelity to vary the motor speed to any desired RPM. On the NRF52, it is limited to a small handful of frequencies.

### SPI
SPI could be used to implement a variety of frequencies on the STEP pin, by connecting STEP to the MOSI pin of the SPI peripheral. By varying the size of the data buffer, and injecting enough 1's into the buffer to create the STEP pulse, variable STEPs per second can be generated.

On the NRF52, the SPI peripheral does not support scatter-gather. This means that the entire SPI data buffer must be allocated contiguously in RAM and popualted at all times. This consumes large amounts of RAM for the mostly-empty data buffers needed to implement this control scheme.

### TIMER design
The NRF52 TIMER can be used to generate a variable STEP signal by toggling a GPIO inside a TIMER interupt.

There are several ways to do this.

#### Method 1: Single Timer, Constant Update Rate
Configure a timer to update at a constant rate. The period of the timer should be equal to the width of the STEP pulse.

Inside the timer ISR, count the number of timer ticks since the last STEP pulse. Use this to time the interval between STEP pulses. Use different variables for both DRV8825 chips to allow different RPMs for the two motors.

#### Method 2: Multiple Timers, Dynamic Update Rate
Configure a timer for each DRV8825. The timer is configured with two different timeouts, depending on what it is doing. One timeout sets the time between STEP pulses. This timeout can be varied by the application. The other timoue is constant, and defines the width of the STEP pulse. 

The ISR alternates the timer timeout between these two different intervals, and toggles the GPIO each time. This produces the variable-spaced STEP pulses needed to control the motor, while reducing the ISR load on the system.

Each DRV8825 ISR operates this algorithm independently.

#### Method 3: Single Timer, Dynamic Update Rate
Building on Method 2, this method uses a single ISR with a highly dynamic update rate. Instead of just two different internals, the timer ISR can now wait for a variable ammount of time, determined by a simple scheduler inside the ISR.

When executed, the ISR executes an operation of some kind based on the current system state. It then looks at the state of both DRV8825s, and determines what the next operation needs to be: servicing a STEP pulse rising edge, or falling edge, on either DRV8825. It then sets the timeout of the timer to the next operation that must be performed. 

When the ISR is next activated, the ISR examines the system state and performs the action on the correct STEP pin, and then repeats the algorithm to compute the next event.


## Selection

For the purposes of this robot, TIMER Method 2 will be implemented, as it provides a good tradeoff between complexity of design and ISR load on the system.

Since this ISR is providing motor / balance control, it must be executed with a higher priority than other long-running ISRs. In particular, this ISR must execute at a higher priority than the RADIO ISR and the IMU ISR.