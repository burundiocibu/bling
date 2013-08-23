#ifndef _ENSEMBLE_HPP
#define _ENSEMBLE_HPP

// Details about the entire ensemble of slaves
namespace ensemble
{
   const unsigned default_channel = 2;
   const size_t addr_len = 4;
   const size_t num_slaves = 300;
   const uint8_t master_addr[addr_len] = {0xA1, 0xA3, 0xA5, 0xA6};
   const uint8_t slave_addr[num_slaves][addr_len]=
   {
      { 0x3f ,  0x5b ,  0x74 ,  0x38 }, // slave 0 (broadcast)
      { 0xcd ,  0x80 ,  0x27 ,  0x40 }, // slave 1
      { 0xe4 ,  0x18 ,  0xdd ,  0x0a }, // slave 2
      { 0x87 ,  0xb5 ,  0xcf ,  0x93 }, // slave 3
      { 0x59 ,  0x2c ,  0x53 ,  0x3f }, // slave 4
      { 0xd3 ,  0xad ,  0xa8 ,  0xff }, // slave 5
      { 0x3c ,  0x66 ,  0xbd ,  0x1e }, // slave 6
      { 0x69 ,  0x27 ,  0x01 ,  0xbd }, // slave 7
      { 0xe9 ,  0xa7 ,  0xb3 ,  0x14 }, // slave 8
      { 0x84 ,  0x10 ,  0xbb ,  0x11 }, // slave 9
      { 0x68 ,  0x3e ,  0xd0 ,  0x34 }, // slave 10
      { 0x53 ,  0x81 ,  0xda ,  0x96 }, // slave 11
      { 0xa2 ,  0xf4 ,  0xfc ,  0xa8 }, // slave 12
      { 0x02 ,  0x51 ,  0x2d ,  0xb0 }, // slave 13
      { 0x18 ,  0x38 ,  0x74 ,  0xe6 }, // slave 14
      { 0x15 ,  0x94 ,  0xec ,  0xa0 }, // slave 15
      { 0x6d ,  0xb8 ,  0xf6 ,  0xf0 }, // slave 16
      { 0xdd ,  0xe0 ,  0x5b ,  0x51 }, // slave 17
      { 0xa8 ,  0xd3 ,  0x26 ,  0x5d }, // slave 18
      { 0x35 ,  0xd7 ,  0x8b ,  0x2c }, // slave 19
      { 0xa0 ,  0xf1 ,  0x3a ,  0x9e }, // slave 20
      { 0x77 ,  0xa3 ,  0xf8 ,  0x14 }, // slave 21
      { 0x19 ,  0xe0 ,  0xc4 ,  0x39 }, // slave 22
      { 0x96 ,  0xb9 ,  0x6c ,  0xfa }, // slave 23
      { 0x2b ,  0x8b ,  0x5c ,  0x7d }, // slave 24
      { 0x59 ,  0x6b ,  0x25 ,  0x8d }, // slave 25
      { 0x3c ,  0xbf ,  0xc2 ,  0xa6 }, // slave 26
      { 0xd7 ,  0xe1 ,  0x3f ,  0xef }, // slave 27
      { 0xc3 ,  0x71 ,  0x05 ,  0xb6 }, // slave 28
      { 0x6a ,  0x6a ,  0xbf ,  0xdd }, // slave 29
      { 0x18 ,  0x2c ,  0x8b ,  0x79 }, // slave 30
      { 0x36 ,  0x28 ,  0x9d ,  0x40 }, // slave 31
      { 0x5e ,  0x3e ,  0x49 ,  0x1b }, // slave 32
      { 0x00 ,  0xef ,  0xe5 ,  0x73 }, // slave 33
      { 0x21 ,  0x0a ,  0xa0 ,  0x55 }, // slave 34
      { 0xad ,  0x3a ,  0x2f ,  0x74 }, // slave 35
      { 0xb8 ,  0x78 ,  0xd3 ,  0xad }, // slave 36
      { 0x08 ,  0x37 ,  0x42 ,  0x5c }, // slave 37
      { 0x82 ,  0x48 ,  0xcc ,  0x2b }, // slave 38
      { 0x05 ,  0x2b ,  0x6d ,  0x8b }, // slave 39
      { 0xe2 ,  0x71 ,  0x18 ,  0x52 }, // slave 40
      { 0xdd ,  0x0f ,  0x9e ,  0x5a }, // slave 41
      { 0x2a ,  0x92 ,  0xb7 ,  0x1f }, // slave 42
      { 0x0f ,  0x5f ,  0x85 ,  0xe1 }, // slave 43
      { 0x57 ,  0x71 ,  0xcc ,  0x6c }, // slave 44
      { 0xcc ,  0x0a ,  0xbf ,  0xef }, // slave 45
      { 0xb0 ,  0xba ,  0x73 ,  0x43 }, // slave 46
      { 0x85 ,  0xb7 ,  0x58 ,  0xd2 }, // slave 47
      { 0x0b ,  0x5b ,  0x92 ,  0x00 }, // slave 48
      { 0x54 ,  0x98 ,  0x13 ,  0x71 }, // slave 49
      { 0xdc ,  0x27 ,  0xd1 ,  0x2e }, // slave 50
      { 0xb6 ,  0x96 ,  0xbb ,  0xf0 }, // slave 51
      { 0x58 ,  0xb2 ,  0x8d ,  0x8b }, // slave 52
      { 0xb0 ,  0x42 ,  0x3a ,  0x86 }, // slave 53
      { 0x17 ,  0x20 ,  0xce ,  0x65 }, // slave 54
      { 0xa6 ,  0x51 ,  0x68 ,  0xb6 }, // slave 55
      { 0x5c ,  0xd7 ,  0xe5 ,  0x75 }, // slave 56
      { 0x25 ,  0xc4 ,  0x5a ,  0xf8 }, // slave 57
      { 0x78 ,  0x8e ,  0xea ,  0x34 }, // slave 58
      { 0x2a ,  0x4d ,  0x98 ,  0xf8 }, // slave 59
      { 0xe9 ,  0x10 ,  0x66 ,  0xe4 }, // slave 60
      { 0xba ,  0xd0 ,  0x09 ,  0xc0 }, // slave 61
      { 0xec ,  0x0a ,  0x5d ,  0x3b }, // slave 62
      { 0xc2 ,  0xae ,  0x51 ,  0xcd }, // slave 63
      { 0x1d ,  0x5a ,  0x3c ,  0xd3 }, // slave 64
      { 0x37 ,  0x6e ,  0x09 ,  0xd1 }, // slave 65
      { 0xd5 ,  0xd6 ,  0x1a ,  0xab }, // slave 66
      { 0x27 ,  0x57 ,  0xe9 ,  0x9b }, // slave 67
      { 0xc2 ,  0xdd ,  0x83 ,  0xfb }, // slave 68
      { 0xfb ,  0x29 ,  0x63 ,  0x33 }, // slave 69
      { 0x10 ,  0x04 ,  0x36 ,  0x86 }, // slave 70
      { 0xbf ,  0x9a ,  0x9b ,  0xbb }, // slave 71
      { 0xeb ,  0x15 ,  0xdf ,  0xf3 }, // slave 72
      { 0x6f ,  0xac ,  0x1b ,  0x98 }, // slave 73
      { 0xec ,  0x2d ,  0xa9 ,  0x58 }, // slave 74
      { 0x96 ,  0x86 ,  0x94 ,  0x7a }, // slave 75
      { 0xfd ,  0x81 ,  0xcc ,  0x2b }, // slave 76
      { 0x30 ,  0xb2 ,  0x17 ,  0x46 }, // slave 77
      { 0x8a ,  0xbe ,  0x03 ,  0xd3 }, // slave 78
      { 0x29 ,  0xe7 ,  0x92 ,  0xe5 }, // slave 79
      { 0x26 ,  0x82 ,  0xee ,  0xad }, // slave 80
      { 0x21 ,  0xe5 ,  0xf8 ,  0x2e }, // slave 81
      { 0x61 ,  0x77 ,  0xeb ,  0xe1 }, // slave 82
      { 0xd3 ,  0x96 ,  0xe8 ,  0xcb }, // slave 83
      { 0xe8 ,  0x87 ,  0x1b ,  0x1a }, // slave 84
      { 0xc4 ,  0x08 ,  0x5f ,  0x2e }, // slave 85
      { 0x7d ,  0xf3 ,  0x79 ,  0xed }, // slave 86
      { 0x09 ,  0x9e ,  0x6b ,  0x06 }, // slave 87
      { 0xff ,  0xc0 ,  0x61 ,  0xfe }, // slave 88
      { 0xdf ,  0x4c ,  0x78 ,  0xc4 }, // slave 89
      { 0xff ,  0xd9 ,  0x09 ,  0xd0 }, // slave 90
      { 0xfc ,  0x99 ,  0xa7 ,  0x9b }, // slave 91
      { 0x7b ,  0xb2 ,  0x9e ,  0x69 }, // slave 92
      { 0xd2 ,  0xf1 ,  0x32 ,  0x40 }, // slave 93
      { 0xf1 ,  0x10 ,  0x53 ,  0x67 }, // slave 94
      { 0x67 ,  0xd4 ,  0x67 ,  0x56 }, // slave 95
      { 0x72 ,  0xee ,  0x4e ,  0x21 }, // slave 96
      { 0xb8 ,  0x97 ,  0x98 ,  0xa6 }, // slave 97
      { 0x12 ,  0xce ,  0x20 ,  0x75 }, // slave 98
      { 0x09 ,  0x4c ,  0x80 ,  0x59 }, // slave 99
      { 0xe7 ,  0xa9 ,  0x14 ,  0x79 }, // slave 100
      { 0x58 ,  0xfb ,  0x91 ,  0xa0 }, // slave 101
      { 0xf6 ,  0x96 ,  0xb0 ,  0xc8 }, // slave 102
      { 0xef ,  0xf6 ,  0x57 ,  0x76 }, // slave 103
      { 0xb7 ,  0xab ,  0x15 ,  0xed }, // slave 104
      { 0xc3 ,  0x93 ,  0x7b ,  0x94 }, // slave 105
      { 0xd1 ,  0x56 ,  0xcb ,  0xdb }, // slave 106
      { 0xb7 ,  0xc0 ,  0x0d ,  0x21 }, // slave 107
      { 0xda ,  0xbd ,  0x5f ,  0x33 }, // slave 108
      { 0xda ,  0x0d ,  0x68 ,  0x5c }, // slave 109
      { 0x82 ,  0x45 ,  0xf8 ,  0x83 }, // slave 110
      { 0x89 ,  0x51 ,  0xda ,  0xa3 }, // slave 111
      { 0x7c ,  0x22 ,  0x94 ,  0x01 }, // slave 112
      { 0x97 ,  0xf7 ,  0x15 ,  0x47 }, // slave 113
      { 0x47 ,  0x82 ,  0xf9 ,  0x7a }, // slave 114
      { 0x33 ,  0xe8 ,  0x27 ,  0xb6 }, // slave 115
      { 0xe4 ,  0xdc ,  0x03 ,  0xe8 }, // slave 116
      { 0xb0 ,  0xfd ,  0x28 ,  0xc2 }, // slave 117
      { 0x2a ,  0x4c ,  0x33 ,  0x41 }, // slave 118
      { 0x40 ,  0x96 ,  0xdc ,  0xf2 }, // slave 119
      { 0xeb ,  0x99 ,  0x43 ,  0xf7 }, // slave 120
      { 0xac ,  0xce ,  0x0e ,  0x3f }, // slave 121
      { 0x99 ,  0xcd ,  0x26 ,  0x65 }, // slave 122
      { 0x59 ,  0xea ,  0x5f ,  0xb4 }, // slave 123
      { 0x23 ,  0xa2 ,  0x87 ,  0x57 }, // slave 124
      { 0x66 ,  0xbb ,  0x26 ,  0xb8 }, // slave 125
      { 0x71 ,  0x23 ,  0x58 ,  0x2a }, // slave 126
      { 0x46 ,  0xd1 ,  0x68 ,  0x6c }, // slave 127
      { 0xbe ,  0x58 ,  0x18 ,  0xd7 }, // slave 128
      { 0x36 ,  0x86 ,  0x4a ,  0xcd }, // slave 129
      { 0xf8 ,  0x0c ,  0xb5 ,  0x80 }, // slave 130
      { 0x9d ,  0x01 ,  0xff ,  0x05 }, // slave 131
      { 0x4f ,  0xd6 ,  0xcd ,  0x4b }, // slave 132
      { 0x00 ,  0x10 ,  0xa6 ,  0x85 }, // slave 133
      { 0xc8 ,  0xad ,  0x45 ,  0x77 }, // slave 134
      { 0x78 ,  0x82 ,  0x66 ,  0xa2 }, // slave 135
      { 0xd4 ,  0x75 ,  0x6c ,  0x37 }, // slave 136
      { 0xdd ,  0x50 ,  0x8e ,  0x2c }, // slave 137
      { 0x23 ,  0x4d ,  0x77 ,  0xe5 }, // slave 138
      { 0x98 ,  0x8b ,  0x6f ,  0xee }, // slave 139
      { 0x71 ,  0x2d ,  0x74 ,  0xc3 }, // slave 140
      { 0xf5 ,  0x1f ,  0x11 ,  0x5b }, // slave 141
      { 0x12 ,  0xbd ,  0x0d ,  0xcf }, // slave 142
      { 0x56 ,  0x1f ,  0xa3 ,  0x90 }, // slave 143
      { 0x6b ,  0xf0 ,  0xc4 ,  0x74 }, // slave 144
      { 0x96 ,  0x2e ,  0x18 ,  0x80 }, // slave 145
      { 0x49 ,  0x1f ,  0x72 ,  0x24 }, // slave 146
      { 0xd8 ,  0x82 ,  0x06 ,  0x1d }, // slave 147
      { 0xd7 ,  0x89 ,  0x6e ,  0xcd }, // slave 148
      { 0xa3 ,  0x60 ,  0xe7 ,  0x1e }, // slave 149
      { 0x62 ,  0xcd ,  0x84 ,  0x9c }, // slave 150
      { 0x6c ,  0x5f ,  0xb8 ,  0xf2 }, // slave 151
      { 0x22 ,  0x14 ,  0xd5 ,  0x9a }, // slave 152
      { 0x33 ,  0xec ,  0xe8 ,  0x89 }, // slave 153
      { 0xbc ,  0x8f ,  0xe1 ,  0x49 }, // slave 154
      { 0xb3 ,  0xf4 ,  0x01 ,  0xb7 }, // slave 155
      { 0xd3 ,  0x67 ,  0x4a ,  0x9e }, // slave 156
      { 0x95 ,  0x28 ,  0xd1 ,  0xc0 }, // slave 157
      { 0xb1 ,  0x31 ,  0xe2 ,  0xc9 }, // slave 158
      { 0x66 ,  0x24 ,  0xc5 ,  0xca }, // slave 159
      { 0xa5 ,  0xe2 ,  0xc7 ,  0x2a }, // slave 160
      { 0xca ,  0x44 ,  0x4c ,  0xfe }, // slave 161
      { 0x42 ,  0x43 ,  0xfe ,  0xee }, // slave 162
      { 0x7a ,  0xac ,  0xdc ,  0x1e }, // slave 163
      { 0x88 ,  0x85 ,  0x9f ,  0x1b }, // slave 164
      { 0x47 ,  0x72 ,  0xb5 ,  0x36 }, // slave 165
      { 0x78 ,  0xf7 ,  0xc4 ,  0xce }, // slave 166
      { 0x55 ,  0x28 ,  0x47 ,  0x65 }, // slave 167
      { 0x54 ,  0xc3 ,  0x53 ,  0x8e }, // slave 168
      { 0xc0 ,  0x7c ,  0x16 ,  0xe8 }, // slave 169
      { 0x9c ,  0x5c ,  0xd6 ,  0x9d }, // slave 170
      { 0x53 ,  0x53 ,  0xe3 ,  0xbf }, // slave 171
      { 0x66 ,  0x36 ,  0x43 ,  0x67 }, // slave 172
      { 0xe6 ,  0xbc ,  0x7a ,  0x4a }, // slave 173
      { 0xec ,  0x2f ,  0xec ,  0x9b }, // slave 174
      { 0x4f ,  0x00 ,  0xbb ,  0x1c }, // slave 175
      { 0x6b ,  0xef ,  0x1d ,  0xad }, // slave 176
      { 0x25 ,  0x89 ,  0xd2 ,  0xb7 }, // slave 177
      { 0x52 ,  0x56 ,  0xaa ,  0xdb }, // slave 178
      { 0x18 ,  0x9b ,  0x97 ,  0x97 }, // slave 179
      { 0x06 ,  0xf6 ,  0x20 ,  0xaa }, // slave 180
      { 0xc0 ,  0xca ,  0xd3 ,  0x32 }, // slave 181
      { 0x12 ,  0xa9 ,  0xb5 ,  0xd7 }, // slave 182
      { 0x4e ,  0x82 ,  0x1e ,  0xf9 }, // slave 183
      { 0xfd ,  0x9e ,  0xcb ,  0x3c }, // slave 184
      { 0x88 ,  0xc7 ,  0xd7 ,  0x39 }, // slave 185
      { 0xed ,  0x9e ,  0x0c ,  0x14 }, // slave 186
      { 0x5d ,  0x5b ,  0x45 ,  0xee }, // slave 187
      { 0x03 ,  0x3d ,  0x7d ,  0xd0 }, // slave 188
      { 0xd4 ,  0x98 ,  0x50 ,  0x55 }, // slave 189
      { 0x23 ,  0x6f ,  0x67 ,  0x70 }, // slave 190
      { 0xf0 ,  0x69 ,  0x04 ,  0x29 }, // slave 191
      { 0xe1 ,  0xf1 ,  0x9c ,  0x0f }, // slave 192
      { 0x9e ,  0xae ,  0x4c ,  0xa8 }, // slave 193
      { 0x5b ,  0x22 ,  0x20 ,  0xf1 }, // slave 194
      { 0xb6 ,  0x39 ,  0x89 ,  0xfa }, // slave 195
      { 0xaa ,  0xcb ,  0xdc ,  0xb0 }, // slave 196
      { 0x82 ,  0x82 ,  0x57 ,  0x42 }, // slave 197
      { 0x58 ,  0x13 ,  0x42 ,  0x17 }, // slave 198
      { 0x1d ,  0xde ,  0x96 ,  0xe2 }, // slave 199
      { 0x9d ,  0x34 ,  0xda ,  0x27 }, // slave 200
      { 0xb3 ,  0xe4 ,  0xfd ,  0x8b }, // slave 201
      { 0xb2 ,  0x6d ,  0xb7 ,  0x4d }, // slave 202
      { 0xf4 ,  0x3f ,  0xf2 ,  0xb4 }, // slave 203
      { 0x21 ,  0x61 ,  0x31 ,  0xc0 }, // slave 204
      { 0xb7 ,  0x77 ,  0x9f ,  0x02 }, // slave 205
      { 0xc0 ,  0x6c ,  0xe3 ,  0x70 }, // slave 206
      { 0x64 ,  0x5e ,  0x2d ,  0xaf }, // slave 207
      { 0xb0 ,  0x50 ,  0x98 ,  0x2a }, // slave 208
      { 0x6f ,  0xd6 ,  0x56 ,  0x86 }, // slave 209
      { 0xee ,  0xd0 ,  0xe4 ,  0x81 }, // slave 210
      { 0x6e ,  0x04 ,  0x01 ,  0x60 }, // slave 211
      { 0xc6 ,  0xea ,  0xfc ,  0xe1 }, // slave 212
      { 0x33 ,  0x05 ,  0x8f ,  0xed }, // slave 213
      { 0x9b ,  0xa1 ,  0x9c ,  0x8b }, // slave 214
      { 0x3e ,  0xff ,  0x6d ,  0x8c }, // slave 215
      { 0x38 ,  0xfe ,  0x12 ,  0xb2 }, // slave 216
      { 0xb6 ,  0x02 ,  0xfa ,  0x68 }, // slave 217
      { 0x6f ,  0xb8 ,  0x63 ,  0x54 }, // slave 218
      { 0x5e ,  0x89 ,  0xaa ,  0x65 }, // slave 219
      { 0x03 ,  0x7c ,  0xc0 ,  0x07 }, // slave 220
      { 0x8e ,  0x13 ,  0x83 ,  0x33 }, // slave 221
      { 0xfd ,  0x6a ,  0x3a ,  0xb9 }, // slave 222
      { 0x5a ,  0x21 ,  0x9c ,  0xca }, // slave 223
      { 0xdb ,  0xcb ,  0xdf ,  0xbf }, // slave 224
      { 0xdd ,  0x40 ,  0xab ,  0x48 }, // slave 225
      { 0xb5 ,  0x30 ,  0xb1 ,  0xe9 }, // slave 226
      { 0x21 ,  0x2b ,  0xbf ,  0xe7 }, // slave 227
      { 0x42 ,  0x62 ,  0x59 ,  0x00 }, // slave 228
      { 0xbe ,  0x1c ,  0x9c ,  0x65 }, // slave 229
      { 0xb4 ,  0x18 ,  0xe8 ,  0x78 }, // slave 230
      { 0xdc ,  0x21 ,  0x15 ,  0x1a }, // slave 231
      { 0xfb ,  0xf1 ,  0xd2 ,  0xdb }, // slave 232
      { 0xe4 ,  0x1a ,  0x88 ,  0x7d }, // slave 233
      { 0x53 ,  0xc0 ,  0xa0 ,  0xa3 }, // slave 234
      { 0x4d ,  0xbf ,  0x47 ,  0x20 }, // slave 235
      { 0x54 ,  0xe1 ,  0x1b ,  0x64 }, // slave 236
      { 0x2e ,  0x3b ,  0x46 ,  0xd2 }, // slave 237
      { 0xab ,  0x87 ,  0x73 ,  0x8f }, // slave 238
      { 0x52 ,  0x03 ,  0x6e ,  0xda }, // slave 239
      { 0x4c ,  0xf0 ,  0x06 ,  0xd6 }, // slave 240
      { 0xce ,  0xe6 ,  0x2a ,  0xa2 }, // slave 241
      { 0xd7 ,  0xf3 ,  0xa3 ,  0x6c }, // slave 242
      { 0xa5 ,  0x44 ,  0xb9 ,  0xca }, // slave 243
      { 0x42 ,  0x06 ,  0xf4 ,  0xbc }, // slave 244
      { 0xef ,  0xc1 ,  0xe2 ,  0xd2 }, // slave 245
      { 0xcb ,  0x6e ,  0x99 ,  0x3a }, // slave 246
      { 0x56 ,  0x50 ,  0x73 ,  0x03 }, // slave 247
      { 0xf5 ,  0x69 ,  0x2f ,  0x16 }, // slave 248
      { 0xd5 ,  0x49 ,  0x3d ,  0x8d }, // slave 249
      { 0x9b ,  0xb2 ,  0xee ,  0x54 }, // slave 250
      { 0x6a ,  0xc2 ,  0x91 ,  0x03 }, // slave 251
      { 0x61 ,  0x66 ,  0x2f ,  0x36 }, // slave 252
      { 0x6b ,  0x15 ,  0x57 ,  0x58 }, // slave 253
      { 0xf2 ,  0x30 ,  0xc5 ,  0x09 }, // slave 254
      { 0x34 ,  0x97 ,  0x6c ,  0xca }, // slave 255
      { 0x11 ,  0x87 ,  0xba ,  0x1a }, // slave 256
      { 0xbb ,  0x3f ,  0x2a ,  0xe3 }, // slave 257
      { 0x85 ,  0xfb ,  0x3c ,  0xbd }, // slave 258
      { 0x12 ,  0x6b ,  0x25 ,  0xca }, // slave 259
      { 0x78 ,  0x9f ,  0x69 ,  0x9b }, // slave 260
      { 0x0c ,  0x3b ,  0xe6 ,  0xa4 }, // slave 261
      { 0x6c ,  0x1b ,  0xd2 ,  0xcf }, // slave 262
      { 0xa0 ,  0xae ,  0xfa ,  0x1d }, // slave 263
      { 0xda ,  0xdc ,  0xeb ,  0xc6 }, // slave 264
      { 0xd9 ,  0x90 ,  0xcc ,  0x87 }, // slave 265
      { 0xa2 ,  0xd6 ,  0x03 ,  0xba }, // slave 266
      { 0xc3 ,  0x25 ,  0x59 ,  0x10 }, // slave 267
      { 0xe6 ,  0xbc ,  0x72 ,  0x59 }, // slave 268
      { 0x04 ,  0xb7 ,  0x7f ,  0x69 }, // slave 269
      { 0xf3 ,  0xd9 ,  0x6d ,  0x6d }, // slave 270
      { 0x99 ,  0x02 ,  0xf4 ,  0x0e }, // slave 271
      { 0x8e ,  0x17 ,  0x6a ,  0x99 }, // slave 272
      { 0xff ,  0x63 ,  0xe1 ,  0xb3 }, // slave 273
      { 0x63 ,  0x6f ,  0x40 ,  0x5b }, // slave 274
      { 0x0f ,  0x13 ,  0x3c ,  0xa7 }, // slave 275
      { 0x4a ,  0x3a ,  0x20 ,  0x86 }, // slave 276
      { 0x34 ,  0x23 ,  0xd7 ,  0xb2 }, // slave 277
      { 0x5a ,  0x3f ,  0xe5 ,  0xb9 }, // slave 278
      { 0x92 ,  0x35 ,  0xad ,  0x33 }, // slave 279
      { 0x31 ,  0xf1 ,  0xb4 ,  0x69 }, // slave 280
      { 0xe1 ,  0x3f ,  0xf1 ,  0xa6 }, // slave 281
      { 0x31 ,  0x95 ,  0xd6 ,  0xbe }, // slave 282
      { 0x7d ,  0x62 ,  0x1f ,  0x71 }, // slave 283
      { 0x46 ,  0x71 ,  0x9e ,  0xe2 }, // slave 284
      { 0x21 ,  0x82 ,  0x8f ,  0xba }, // slave 285
      { 0x2a ,  0x67 ,  0x5c ,  0xcf }, // slave 286
      { 0xfb ,  0xa3 ,  0xcb ,  0xfc }, // slave 287
      { 0xd2 ,  0xda ,  0x75 ,  0x09 }, // slave 288
      { 0x7d ,  0x62 ,  0x65 ,  0xaf }, // slave 289
      { 0xce ,  0x49 ,  0x57 ,  0x41 }, // slave 290
      { 0xd9 ,  0x59 ,  0x88 ,  0xe7 }, // slave 291
      { 0xe2 ,  0x7c ,  0x3d ,  0x52 }, // slave 292
      { 0x98 ,  0x4e ,  0xc2 ,  0xe6 }, // slave 293
      { 0x8c ,  0x39 ,  0xe6 ,  0x13 }, // slave 294
      { 0x97 ,  0x14 ,  0xb1 ,  0xcd }, // slave 295
      { 0x00 ,  0x38 ,  0xa6 ,  0xd8 }, // slave 296
      { 0x23 ,  0x1d ,  0x36 ,  0x43 }, // slave 297
      { 0xd7 ,  0xc6 ,  0x88 ,  0xce }, // slave 298
      { 0x24 ,  0xae ,  0xc5 ,  0x73 }, // slave 299
   };

}
#endif
