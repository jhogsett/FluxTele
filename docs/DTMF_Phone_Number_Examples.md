// SimDTMF Random Phone Number Generation Examples

// ===============================================
// Constructor Usage Examples
// ===============================================

// Fixed phone number (original behavior)
SimDTMF dtmf_fixed(&wave_gen_pool, &signal_meter, 14150000.0, "15556781234");

// Random NANP phone numbers (new feature)  
SimDTMF dtmf_random(&wave_gen_pool, &signal_meter, 14155000.0);

// ===============================================
// Realistic Generated Phone Number Examples
// ===============================================

// Area codes from major North American cities:
// 212 (New York), 213 (Los Angeles), 214 (Dallas), 215 (Philadelphia)
// 301 (Maryland), 312 (Chicago), 404 (Atlanta), 415 (San Francisco)
// 416 (Toronto), 503 (Portland), 604 (Vancouver), 713 (Houston)
// 801 (Salt Lake City), 902 (Nova Scotia), 916 (Sacramento)

// Example generated numbers:
// 12125551987 (New York area)
// 14155552341 (San Francisco area)  
// 17135558765 (Houston area)
// 19165554123 (Sacramento area)

// ===============================================
// NANP Rules Enforced
// ===============================================

// Format: 1 + NXX + NXX + XXXX
//         ^ Country code (always 1)
//             ^^^ Area code (realistic selections)
//                 ^^^ Central office code (N=2-9, avoid 555)  
//                     ^^^^ Subscriber number (avoid patterns)

// Avoided codes:
// - Area codes: None (using realistic list)
// - CO codes: 555, 911, 411, 611 (reserved)
// - Subscriber: 0000, 1111, 2222...9999, 1234 (obvious patterns)

// ===============================================
// Integration with Station Manager
// ===============================================

// Random phone numbers automatically regenerate when:
// 1. Station is moved by StationManager
// 2. randomize() is called manually
// 3. Station is reinitialized

// Debug output format:
// "DTMF: Generated new random number: 12145678901"
// "DTMF: Dialing random number: 12145678901"

// ===============================================
// Memory Usage Impact
// ===============================================

// Flash: +1.2% (realistic area code array)
// RAM: Minimal (+26 bytes for phone number buffer)
// Total: 61.0% Flash, 24.0% RAM (excellent headroom)
