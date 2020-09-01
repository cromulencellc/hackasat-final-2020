#include <stdint.h>

#if MY_TEAM_ID == 0
const uint32_t radio_read_key[4] = { 0x20D540B4, 0xB7158648, 0xD69C7139, 0x183E714A };
const uint32_t radio_write_key[4] = { 0xEC6A7F90, 0xEFABA446, 0xBE0B1B0A, 0x3950C575 };
const uint32_t radio_random_key[4] = { 0xA0114167, 0xD065713E, 0xB4EF7B00, 0x2858947C };
#elif MY_TEAM_ID == 1
const uint32_t radio_read_key[4] = { 0x1A1A9108, 0xD18CA2FF, 0x58F507D6, 0x592647C5 };
const uint32_t radio_write_key[4] = { 0xC9C3EA1C, 0x34AD9D64, 0x0823B0BC, 0x8DA8A5B1 };
const uint32_t radio_random_key[4] = { 0xC8CDD332, 0x86B2332A, 0x9C0E992B, 0x6FE38B24 };
#elif MY_TEAM_ID == 2
const uint32_t radio_read_key[4] = { 0xDC60482D, 0x10163EA5, 0xD4A4FE35, 0x59B08CD7 };
const uint32_t radio_write_key[4] = { 0xBE50CE79, 0x7836DE50, 0x0B2B7DF1, 0x683D90E8 };
const uint32_t radio_random_key[4] = { 0xA7A3D6E3, 0x28929D95, 0xE3FC9E1C, 0xAE2014D1 };
#elif MY_TEAM_ID == 3
const uint32_t radio_read_key[4] = { 0xD7C0FCB0, 0xC7EFEC85, 0x2577B1D2, 0x87707B55 };
const uint32_t radio_write_key[4] = { 0x2C15CBF8, 0x3B4EA81D, 0x9CEB00E7, 0x57AF381F };
const uint32_t radio_random_key[4] = { 0x5754AFB6, 0xFDA3E0BA, 0xF7932A7A, 0x5C8738D6 };
#elif MY_TEAM_ID == 4
const uint32_t radio_read_key[4] = { 0x3A908691, 0xFC8340E6, 0x53E68E8D, 0xE91D1C91 };
const uint32_t radio_write_key[4] = { 0xD0FE490C, 0x1931F782, 0x9F8B15DF, 0x38DE1AB9 };
const uint32_t radio_random_key[4] = { 0x2D422B40, 0x0333EB60, 0xB9DCC93E, 0x7FD7250E };
#elif MY_TEAM_ID == 5
const uint32_t radio_read_key[4] = { 0x61095344, 0x9185F168, 0x008B4BAD, 0xB5E40E3E };
const uint32_t radio_write_key[4] = { 0x5A3C5E63, 0x13AD147A, 0x4AEF17CC, 0xF9D2A0C1 };
const uint32_t radio_random_key[4] = { 0x9BC01FBB, 0xDBD8BE06, 0x501F4F88, 0xDADA050C };
#elif MY_TEAM_ID == 6
const uint32_t radio_read_key[4] = { 0xB6C1FAA9, 0x5F7D7BD0, 0xE1EB98A6, 0xE669A775 };
const uint32_t radio_write_key[4] = { 0x91AABAA9, 0x5EE80924, 0x64B8F4DA, 0xE3C30189 };
const uint32_t radio_random_key[4] = { 0xDBF85F2B, 0x3E6F5C71, 0x5662672A, 0x3FF34DCB };
#elif MY_TEAM_ID == 7
const uint32_t radio_read_key[4] = { 0xE69F539A, 0x0D3D3102, 0x0550D4E5, 0x2A10E104 };
const uint32_t radio_write_key[4] = { 0xAFBA6AE2, 0xD7DE8593, 0x44B03B47, 0xD24528C0 };
const uint32_t radio_random_key[4] = { 0x188E500F, 0x56301E6D, 0x2CD07AED, 0x8B82830B };
#elif MY_TEAM_ID == 8
const uint32_t radio_read_key[4] = { 0xB152FC69, 0x66170DEC, 0x96A3D796, 0x0F9F9029 };
const uint32_t radio_write_key[4] = { 0xD523AB13, 0xD7978A30, 0xE0ECFC4F, 0x02DADF25 };
const uint32_t radio_random_key[4] = { 0x493D3378, 0x5FC3D8F4, 0x6795F39D, 0xA9BA1264 };
#elif MY_TEAM_ID == 9
const uint32_t radio_read_key[4] = { 0x2C0C7D31, 0x24EFE354, 0x1E33FA43, 0x476B5624 };
const uint32_t radio_write_key[4] = { 0x2BEB56B5, 0x9D738E95, 0x525493B5, 0xD17BB890 };
const uint32_t radio_random_key[4] = { 0xC2B3E90D, 0xA6592EB1, 0xACFA5A62, 0x5729F27A };
#elif MY_TEAM_ID == 10
const uint32_t radio_read_key[4] = { 0x2C8D9DE5, 0x313759D5, 0xB6F2314E, 0x0935D478 };
const uint32_t radio_write_key[4] = { 0x0FC67DF5, 0x8FDBA3F4, 0x2EFDE3D4, 0x57588D3D };
const uint32_t radio_random_key[4] = { 0x8CCF7C0C, 0x6DE55A65, 0x9B949F59, 0xF0DD54D9 };
#elif MY_TEAM_ID == 11
const uint32_t radio_read_key[4] = { 0xFCC0F0E5, 0xD0EF7093, 0x809BEA77, 0x2FD55C52 };
const uint32_t radio_write_key[4] = { 0x1A8CB327, 0xC3F1A539, 0x19F95D63, 0x7F9840D9 };
const uint32_t radio_random_key[4] = { 0x841BA61C, 0xD7B99B37, 0xD820E2F8, 0x8F64EC0F };
#elif MY_TEAM_ID == 12
const uint32_t radio_read_key[4] = { 0x5E0E392C, 0x4A917B1B, 0xC4F23B63, 0x6516BF84 };
const uint32_t radio_write_key[4] = { 0xF890EC54, 0x4BBFAB95, 0x0C510853, 0x305D0A78 };
const uint32_t radio_random_key[4] = { 0x00364095, 0x2641E6D9, 0x1D631C80, 0x1A6CCA3E };
#elif MY_TEAM_ID == 13
const uint32_t radio_read_key[4] = { 0xDE675D63, 0xED3AF0EB, 0x9B99424A, 0x831AC84F };
const uint32_t radio_write_key[4] = { 0x96F78F47, 0x8C4FCBAC, 0x62E3A905, 0xC8857E61 };
const uint32_t radio_random_key[4] = { 0x83F25571, 0x2394AEEE, 0x9F473688, 0x5C1EDF4A };
#elif MY_TEAM_ID == 14
const uint32_t radio_read_key[4] = { 0xB6776DC7, 0x4831D13B, 0x1E37AC0D, 0x5C3EB6E2 };
const uint32_t radio_write_key[4] = { 0x29DC9DDC, 0x2CE083B0, 0x42C9103F, 0x42D81A6F };
const uint32_t radio_random_key[4] = { 0xDEF4284E, 0x49EC9DAC, 0xCD4BC5D0, 0x4274DABD };
#elif MY_TEAM_ID == 15
const uint32_t radio_read_key[4] = { 0x7B39B74F, 0xE0F493B6, 0x881F1107, 0x82B17C76 };
const uint32_t radio_write_key[4] = { 0xDD885A88, 0xE20AE915, 0x2FB8EC5C, 0xBD7E145B };
const uint32_t radio_random_key[4] = { 0x6C29890B, 0x6AA6797E, 0xDA4A8DC8, 0x946961D1 };
#elif MY_TEAM_ID == 16
const uint32_t radio_read_key[4] = { 0x61852CAC, 0x31DCBB5F, 0x1447AC80, 0x30B6424F };
const uint32_t radio_write_key[4] = { 0xAA674633, 0x0E114CE8, 0x6D06BBD9, 0x4E466839 };
const uint32_t radio_random_key[4] = { 0xCCF092C0, 0xBB8F2AAE, 0xD39E72D8, 0x025BBA21 };
#elif MY_TEAM_ID == 17
const uint32_t radio_read_key[4] = { 0xBAD2776F, 0x46AA25D3, 0x69274EE6, 0x000A4816 };
const uint32_t radio_write_key[4] = { 0x98F7E633, 0x2A0A27EA, 0x86CA89D1, 0xC04521B9 };
const uint32_t radio_random_key[4] = { 0xC33052E0, 0xCD7EA948, 0x1D86067A, 0x87B6BCD7 };
#elif MY_TEAM_ID == 18
const uint32_t radio_read_key[4] = { 0xBE74690C, 0x848B0009, 0xF0BFBBF3, 0xCC9B7066 };
const uint32_t radio_write_key[4] = { 0xE9AED09F, 0x42EF5760, 0x47D82997, 0xC6061D35 };
const uint32_t radio_random_key[4] = { 0xF54A79F2, 0xCDC22592, 0x29D95F3B, 0x3229AA20 };
#elif MY_TEAM_ID == 19
const uint32_t radio_read_key[4] = { 0xD2B6DA6A, 0xA0CE6DD5, 0x3674DFB0, 0xD686F0C0 };
const uint32_t radio_write_key[4] = { 0xF6AA054E, 0x126FD4EF, 0x7E7918F0, 0x76B215C9 };
const uint32_t radio_random_key[4] = { 0x87593A41, 0x68C06DC7, 0x6C71C6EB, 0x88E33142 };
#elif MY_TEAM_ID == 20
const uint32_t radio_read_key[4] = { 0xCC1D6D0B, 0x54342A6B, 0x75B8379D, 0x45D41A35 };
const uint32_t radio_write_key[4] = { 0xB0571ED7, 0x77DC25B9, 0xB29019DF, 0xB592A751 };
const uint32_t radio_random_key[4] = { 0xCCF38CC7, 0xF59A3748, 0xA6661459, 0xACC66C4B };
#elif MY_TEAM_ID == 21
const uint32_t radio_read_key[4] = { 0x32387065, 0xFF624DF0, 0x63214B03, 0x8AB4008B };
const uint32_t radio_write_key[4] = { 0x5E9EB6CF, 0x1537023F, 0xF75FCD34, 0xC125D8E5 };
const uint32_t radio_random_key[4] = { 0xCEADB690, 0x2E8E9A3A, 0xDD6D4C42, 0x86413636 };
#endif
