## Sim Exchange

- Basic Telco station that can play simple Telco sounds
    - ringing tone: 440 Hz + 480 Hz, 2s on + 4s off repeating
    - busy tone: 480 Hz + 620 Hz, 0.5s on + 0.5s off repeating
    - reorder signal: 480 Hz + 620 Hz, 0.25s on + 0.25s off repeating
    - dial tone: 350 Hz + 440 Hz, 30s on + 2s off
        - this is my own timing, 
        - real dial tone would time out to the off hook tone that requires four wave generators to reproduce
    - error tone: 
        - 913.8 for 380ms
        - 1428.5 for 276ms
        - 1776.7 for 380ms
        - off for 2s
            - this is my own timing
            - a real error tone would be followed by a spoken recording of some kind

