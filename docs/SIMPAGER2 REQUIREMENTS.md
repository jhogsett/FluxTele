## SimPager2 Requirements

- works like the SimPager, but plays two simultaneous tones, not one tone
- acquires two, not one, wave generator resource, tracking their acquisition and freeing
- if two cannot be acquired, it frees any so-far allocated one, and retries forever until two are successfully acquired
- Uses the same tone organizatin and timing as SimPager just with two simultaneous tones
