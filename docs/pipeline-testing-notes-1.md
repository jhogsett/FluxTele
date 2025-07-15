
## must haves
- newly relocated stations need to be placed within the radio spectrum _asymmetrically_ when tuning up versus when tuning down to add import realism
    - (Good) the tuning up direction currently works well:
        - while tuning up, the new stations appear, and because of the BFO offset, they are not heard immediately, but are naturally _dialed in_ as continue turning the tuning knob
    - (Not Good) the tuning down direction sounds unnatural
        - while tuning down, the new stations **pop** into place, because they are placed so that their audible frequency is near the top of the max audible frequency 5000, which can still be easily heard
        - It would be better if they are placed a bit further down the dial allowing them to be more naturally dialed into. The BFO Offset might be the right amount. 
- newly relocated stations should be newly-randomimzed so they appear to be a whole brand new station
    - new callsign
    - WPM drift

## look into
- while tuning down quickly a long way, so it pulled all the stations down, they all popped into place at once, sounding unnatural

## nice to haves
- have a flag that indicates whether or not a station is _allowed_ to be automatically moved (though currently it's handled well by type of station)
- when randomizing a station's properties like WPM also random Fist Quality
- when stations drift, if they started within the limmits of a ham band, they are restricted from moving outside of the legal band limits
- assign a probability to by _station kind_ for it to be chosen for relocation (for example, CW station high, numbers stations low, pager station zero)
