## Known Bugs

- Station Manger crashes on relocating stations in this case:
    - MAX_STATIONS = 1
    - Have tuned up and station got moved
    - have tuned back down below 55.0 MHz causing another move
    - crash happens at this point and Serial.print() is cut off masking the exact location

