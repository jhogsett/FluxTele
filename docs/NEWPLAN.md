1) duplicate SimPager to SimPager2 verbatim
2) comfirm it works 
3) wrap it's wave generator code in #ifdefs so that it can be switched in our out easily
4) duplicate all of it for use with a second wave generator, also wrapped in #ifdefs
5) both independent uses of the wave generator will be switched on SOLO and tested on the device to confirm that are both identical to Sim Pager.
6) They will be switched on together and tested

***** IT IS AT THIS STAGE THAT IT ALWAYS FAILS TO WORK *****

IMPORTANT NOTE: when SimPager2 is running with both the primary and second wave generator logic running, they must each use different frequencies or they won't be distinguishable.