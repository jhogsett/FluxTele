1. restart with clean code from git
2. create duplicates of SimTransmitter and SimStation as SimTransmitter2 and SimStation2. These will be used as probe classes so that we can later duplicate their wave generator handling code into "A" and "B" #ifdef guarded cases and dianose why things break down when we try to work with two generators together. But for now they should be identical copies and function exactly the same as the originals with only the class names renamed (no variable or code changes yet).
3. create a single station configuration with the duplicated SimStation and test on the device to prove it works identically
4. identify the variables and methods in both new classes that touch the wave generators, either directly or indirectly
5. add #ifdef guards around all of those cases, with the guard enabled
6. test with a single station config using the new station code and see that it works properly, both untuned on start up and tuned after being relocated by StationManager
7. make a verbatim copy of all the just-guarded code segments with different named guard so they can be switched interchangeably
8. in the duplicated copy, modify the variable names to not clash with the original versions (so that later, if we enable both guarded cases, there is not a name clash)
9. make the copied guarded code the new enabled code, disabling the original guard code
10. test on the device. It should work identically to #5 above
11. if not, when we need to examine where the duplication process did not succeed.
