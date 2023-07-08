/*
    DMRBot - ChatGPT Voice Bot for DMR
    Copyright (C) 2023 Nuno Silva

    Based on code from:
    reflector_connectors	https://github.com/nostar/reflector_connectors
    MMDVM_CM	https://github.com/juribeparada/MMDVM_CM

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h> 
#include <string.h> 
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/ioctl.h>

#define AMBE_ENCODE_GAIN -15
#define AMBE_DECODE_GAIN 10
#define BUFSIZE 2048
#define TIMEOUT 60
//#define DEBUG

#define SWAP(n) (((n) << 24) | (((n) & 0xff00) << 8) | (((n) >> 8) & 0xff00) | ((n) >> 24))

#define K(I) roundConstants[I]
static const uint32_t roundConstants[64] = {
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
	0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
	0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
	0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
	0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
	0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
	0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
	0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
	0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL,
};

const unsigned char POLY[] = {64U, 56U, 14U, 1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};

const unsigned char EXP_TABLE[] = {
    0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x20U, 0x40U, 0x80U, 0x1DU, 0x3AU, 0x74U, 0xE8U, 0xCDU, 0x87U, 0x13U, 0x26U,
    0x4CU, 0x98U, 0x2DU, 0x5AU, 0xB4U, 0x75U, 0xEAU, 0xC9U, 0x8FU, 0x03U, 0x06U, 0x0CU, 0x18U, 0x30U, 0x60U, 0xC0U,
    0x9DU, 0x27U, 0x4EU, 0x9CU, 0x25U, 0x4AU, 0x94U, 0x35U, 0x6AU, 0xD4U, 0xB5U, 0x77U, 0xEEU, 0xC1U, 0x9FU, 0x23U,
    0x46U, 0x8CU, 0x05U, 0x0AU, 0x14U, 0x28U, 0x50U, 0xA0U, 0x5DU, 0xBAU, 0x69U, 0xD2U, 0xB9U, 0x6FU, 0xDEU, 0xA1U,
    0x5FU, 0xBEU, 0x61U, 0xC2U, 0x99U, 0x2FU, 0x5EU, 0xBCU, 0x65U, 0xCAU, 0x89U, 0x0FU, 0x1EU, 0x3CU, 0x78U, 0xF0U,
    0xFDU, 0xE7U, 0xD3U, 0xBBU, 0x6BU, 0xD6U, 0xB1U, 0x7FU, 0xFEU, 0xE1U, 0xDFU, 0xA3U, 0x5BU, 0xB6U, 0x71U, 0xE2U,
    0xD9U, 0xAFU, 0x43U, 0x86U, 0x11U, 0x22U, 0x44U, 0x88U, 0x0DU, 0x1AU, 0x34U, 0x68U, 0xD0U, 0xBDU, 0x67U, 0xCEU,
    0x81U, 0x1FU, 0x3EU, 0x7CU, 0xF8U, 0xEDU, 0xC7U, 0x93U, 0x3BU, 0x76U, 0xECU, 0xC5U, 0x97U, 0x33U, 0x66U, 0xCCU,
    0x85U, 0x17U, 0x2EU, 0x5CU, 0xB8U, 0x6DU, 0xDAU, 0xA9U, 0x4FU, 0x9EU, 0x21U, 0x42U, 0x84U, 0x15U, 0x2AU, 0x54U,
    0xA8U, 0x4DU, 0x9AU, 0x29U, 0x52U, 0xA4U, 0x55U, 0xAAU, 0x49U, 0x92U, 0x39U, 0x72U, 0xE4U, 0xD5U, 0xB7U, 0x73U,
    0xE6U, 0xD1U, 0xBFU, 0x63U, 0xC6U, 0x91U, 0x3FU, 0x7EU, 0xFCU, 0xE5U, 0xD7U, 0xB3U, 0x7BU, 0xF6U, 0xF1U, 0xFFU,
    0xE3U, 0xDBU, 0xABU, 0x4BU, 0x96U, 0x31U, 0x62U, 0xC4U, 0x95U, 0x37U, 0x6EU, 0xDCU, 0xA5U, 0x57U, 0xAEU, 0x41U,
    0x82U, 0x19U, 0x32U, 0x64U, 0xC8U, 0x8DU, 0x07U, 0x0EU, 0x1CU, 0x38U, 0x70U, 0xE0U, 0xDDU, 0xA7U, 0x53U, 0xA6U,
    0x51U, 0xA2U, 0x59U, 0xB2U, 0x79U, 0xF2U, 0xF9U, 0xEFU, 0xC3U, 0x9BU, 0x2BU, 0x56U, 0xACU, 0x45U, 0x8AU, 0x09U,
    0x12U, 0x24U, 0x48U, 0x90U, 0x3DU, 0x7AU, 0xF4U, 0xF5U, 0xF7U, 0xF3U, 0xFBU, 0xEBU, 0xCBU, 0x8BU, 0x0BU, 0x16U,
    0x2CU, 0x58U, 0xB0U, 0x7DU, 0xFAU, 0xE9U, 0xCFU, 0x83U, 0x1BU, 0x36U, 0x6CU, 0xD8U, 0xADU, 0x47U, 0x8EU, 0x01U,
    0x02U, 0x04U, 0x08U, 0x10U, 0x20U, 0x40U, 0x80U, 0x1DU, 0x3AU, 0x74U, 0xE8U, 0xCDU, 0x87U, 0x13U, 0x26U, 0x4CU,
    0x98U, 0x2DU, 0x5AU, 0xB4U, 0x75U, 0xEAU, 0xC9U, 0x8FU, 0x03U, 0x06U, 0x0CU, 0x18U, 0x30U, 0x60U, 0xC0U, 0x9DU,
    0x27U, 0x4EU, 0x9CU, 0x25U, 0x4AU, 0x94U, 0x35U, 0x6AU, 0xD4U, 0xB5U, 0x77U, 0xEEU, 0xC1U, 0x9FU, 0x23U, 0x46U,
    0x8CU, 0x05U, 0x0AU, 0x14U, 0x28U, 0x50U, 0xA0U, 0x5DU, 0xBAU, 0x69U, 0xD2U, 0xB9U, 0x6FU, 0xDEU, 0xA1U, 0x5FU,
    0xBEU, 0x61U, 0xC2U, 0x99U, 0x2FU, 0x5EU, 0xBCU, 0x65U, 0xCAU, 0x89U, 0x0FU, 0x1EU, 0x3CU, 0x78U, 0xF0U, 0xFDU,
    0xE7U, 0xD3U, 0xBBU, 0x6BU, 0xD6U, 0xB1U, 0x7FU, 0xFEU, 0xE1U, 0xDFU, 0xA3U, 0x5BU, 0xB6U, 0x71U, 0xE2U, 0xD9U,
    0xAFU, 0x43U, 0x86U, 0x11U, 0x22U, 0x44U, 0x88U, 0x0DU, 0x1AU, 0x34U, 0x68U, 0xD0U, 0xBDU, 0x67U, 0xCEU, 0x81U,
    0x1FU, 0x3EU, 0x7CU, 0xF8U, 0xEDU, 0xC7U, 0x93U, 0x3BU, 0x76U, 0xECU, 0xC5U, 0x97U, 0x33U, 0x66U, 0xCCU, 0x85U,
    0x17U, 0x2EU, 0x5CU, 0xB8U, 0x6DU, 0xDAU, 0xA9U, 0x4FU, 0x9EU, 0x21U, 0x42U, 0x84U, 0x15U, 0x2AU, 0x54U, 0xA8U,
    0x4DU, 0x9AU, 0x29U, 0x52U, 0xA4U, 0x55U, 0xAAU, 0x49U, 0x92U, 0x39U, 0x72U, 0xE4U, 0xD5U, 0xB7U, 0x73U, 0xE6U,
    0xD1U, 0xBFU, 0x63U, 0xC6U, 0x91U, 0x3FU, 0x7EU, 0xFCU, 0xE5U, 0xD7U, 0xB3U, 0x7BU, 0xF6U, 0xF1U, 0xFFU, 0xE3U,
    0xDBU, 0xABU, 0x4BU, 0x96U, 0x31U, 0x62U, 0xC4U, 0x95U, 0x37U, 0x6EU, 0xDCU, 0xA5U, 0x57U, 0xAEU, 0x41U, 0x82U,
    0x19U, 0x32U, 0x64U, 0xC8U, 0x8DU, 0x07U, 0x0EU, 0x1CU, 0x38U, 0x70U, 0xE0U, 0xDDU, 0xA7U, 0x53U, 0xA6U, 0x51U,
    0xA2U, 0x59U, 0xB2U, 0x79U, 0xF2U, 0xF9U, 0xEFU, 0xC3U, 0x9BU, 0x2BU, 0x56U, 0xACU, 0x45U, 0x8AU, 0x09U, 0x12U,
    0x24U, 0x48U, 0x90U, 0x3DU, 0x7AU, 0xF4U, 0xF5U, 0xF7U, 0xF3U, 0xFBU, 0xEBU, 0xCBU, 0x8BU, 0x0BU, 0x16U, 0x2CU,
    0x58U, 0xB0U, 0x7DU, 0xFAU, 0xE9U, 0xCFU, 0x83U, 0x1BU, 0x36U, 0x6CU, 0xD8U, 0xADU, 0x47U, 0x8EU, 0x01U, 0x00U
};

const unsigned char LOG_TABLE[] = {
    0x00U, 0x00U, 0x01U, 0x19U, 0x02U, 0x32U, 0x1AU, 0xC6U, 0x03U, 0xDFU, 0x33U, 0xEEU, 0x1BU, 0x68U, 0xC7U, 0x4BU,
    0x04U, 0x64U, 0xE0U, 0x0EU, 0x34U, 0x8DU, 0xEFU, 0x81U, 0x1CU, 0xC1U, 0x69U, 0xF8U, 0xC8U, 0x08U, 0x4CU, 0x71U,
    0x05U, 0x8AU, 0x65U, 0x2FU, 0xE1U, 0x24U, 0x0FU, 0x21U, 0x35U, 0x93U, 0x8EU, 0xDAU, 0xF0U, 0x12U, 0x82U, 0x45U,
    0x1DU, 0xB5U, 0xC2U, 0x7DU, 0x6AU, 0x27U, 0xF9U, 0xB9U, 0xC9U, 0x9AU, 0x09U, 0x78U, 0x4DU, 0xE4U, 0x72U, 0xA6U,
    0x06U, 0xBFU, 0x8BU, 0x62U, 0x66U, 0xDDU, 0x30U, 0xFDU, 0xE2U, 0x98U, 0x25U, 0xB3U, 0x10U, 0x91U, 0x22U, 0x88U,
    0x36U, 0xD0U, 0x94U, 0xCEU, 0x8FU, 0x96U, 0xDBU, 0xBDU, 0xF1U, 0xD2U, 0x13U, 0x5CU, 0x83U, 0x38U, 0x46U, 0x40U,
    0x1EU, 0x42U, 0xB6U, 0xA3U, 0xC3U, 0x48U, 0x7EU, 0x6EU, 0x6BU, 0x3AU, 0x28U, 0x54U, 0xFAU, 0x85U, 0xBAU, 0x3DU,
    0xCAU, 0x5EU, 0x9BU, 0x9FU, 0x0AU, 0x15U, 0x79U, 0x2BU, 0x4EU, 0xD4U, 0xE5U, 0xACU, 0x73U, 0xF3U, 0xA7U, 0x57U,
    0x07U, 0x70U, 0xC0U, 0xF7U, 0x8CU, 0x80U, 0x63U, 0x0DU, 0x67U, 0x4AU, 0xDEU, 0xEDU, 0x31U, 0xC5U, 0xFEU, 0x18U,
    0xE3U, 0xA5U, 0x99U, 0x77U, 0x26U, 0xB8U, 0xB4U, 0x7CU, 0x11U, 0x44U, 0x92U, 0xD9U, 0x23U, 0x20U, 0x89U, 0x2EU,
    0x37U, 0x3FU, 0xD1U, 0x5BU, 0x95U, 0xBCU, 0xCFU, 0xCDU, 0x90U, 0x87U, 0x97U, 0xB2U, 0xDCU, 0xFCU, 0xBEU, 0x61U,
    0xF2U, 0x56U, 0xD3U, 0xABU, 0x14U, 0x2AU, 0x5DU, 0x9EU, 0x84U, 0x3CU, 0x39U, 0x53U, 0x47U, 0x6DU, 0x41U, 0xA2U,
    0x1FU, 0x2DU, 0x43U, 0xD8U, 0xB7U, 0x7BU, 0xA4U, 0x76U, 0xC4U, 0x17U, 0x49U, 0xECU, 0x7FU, 0x0CU, 0x6FU, 0xF6U,
    0x6CU, 0xA1U, 0x3BU, 0x52U, 0x29U, 0x9DU, 0x55U, 0xAAU, 0xFBU, 0x60U, 0x86U, 0xB1U, 0xBBU, 0xCCU, 0x3EU, 0x5AU,
    0xCBU, 0x59U, 0x5FU, 0xB0U, 0x9CU, 0xA9U, 0xA0U, 0x51U, 0x0BU, 0xF5U, 0x16U, 0xEBU, 0x7AU, 0x75U, 0x2CU, 0xD7U,
    0x4FU, 0xAEU, 0xD5U, 0xE9U, 0xE6U, 0xE7U, 0xADU, 0xE8U, 0x74U, 0xD6U, 0xF4U, 0xEAU, 0xA8U, 0x50U, 0x58U, 0xAFU
};

const unsigned int ENCODING_TABLE_2087[] =
{0x0000U, 0xB08EU, 0xE093U, 0x501DU, 0x70A9U, 0xC027U, 0x903AU, 0x20B4U, 0x60DCU, 0xD052U, 0x804FU, 0x30C1U,
    0x1075U, 0xA0FBU, 0xF0E6U, 0x4068U, 0x7036U, 0xC0B8U, 0x90A5U, 0x202BU, 0x009FU, 0xB011U, 0xE00CU, 0x5082U,
    0x10EAU, 0xA064U, 0xF079U, 0x40F7U, 0x6043U, 0xD0CDU, 0x80D0U, 0x305EU, 0xD06CU, 0x60E2U, 0x30FFU, 0x8071U,
    0xA0C5U, 0x104BU, 0x4056U, 0xF0D8U, 0xB0B0U, 0x003EU, 0x5023U, 0xE0ADU, 0xC019U, 0x7097U, 0x208AU, 0x9004U,
    0xA05AU, 0x10D4U, 0x40C9U, 0xF047U, 0xD0F3U, 0x607DU, 0x3060U, 0x80EEU, 0xC086U, 0x7008U, 0x2015U, 0x909BU,
    0xB02FU, 0x00A1U, 0x50BCU, 0xE032U, 0x90D9U, 0x2057U, 0x704AU, 0xC0C4U, 0xE070U, 0x50FEU, 0x00E3U, 0xB06DU,
    0xF005U, 0x408BU, 0x1096U, 0xA018U, 0x80ACU, 0x3022U, 0x603FU, 0xD0B1U, 0xE0EFU, 0x5061U, 0x007CU, 0xB0F2U,
    0x9046U, 0x20C8U, 0x70D5U, 0xC05BU, 0x8033U, 0x30BDU, 0x60A0U, 0xD02EU, 0xF09AU, 0x4014U, 0x1009U, 0xA087U,
    0x40B5U, 0xF03BU, 0xA026U, 0x10A8U, 0x301CU, 0x8092U, 0xD08FU, 0x6001U, 0x2069U, 0x90E7U, 0xC0FAU, 0x7074U,
    0x50C0U, 0xE04EU, 0xB053U, 0x00DDU, 0x3083U, 0x800DU, 0xD010U, 0x609EU, 0x402AU, 0xF0A4U, 0xA0B9U, 0x1037U,
    0x505FU, 0xE0D1U, 0xB0CCU, 0x0042U, 0x20F6U, 0x9078U, 0xC065U, 0x70EBU, 0xA03DU, 0x10B3U, 0x40AEU, 0xF020U,
    0xD094U, 0x601AU, 0x3007U, 0x8089U, 0xC0E1U, 0x706FU, 0x2072U, 0x90FCU, 0xB048U, 0x00C6U, 0x50DBU, 0xE055U,
    0xD00BU, 0x6085U, 0x3098U, 0x8016U, 0xA0A2U, 0x102CU, 0x4031U, 0xF0BFU, 0xB0D7U, 0x0059U, 0x5044U, 0xE0CAU,
    0xC07EU, 0x70F0U, 0x20EDU, 0x9063U, 0x7051U, 0xC0DFU, 0x90C2U, 0x204CU, 0x00F8U, 0xB076U, 0xE06BU, 0x50E5U,
    0x108DU, 0xA003U, 0xF01EU, 0x4090U, 0x6024U, 0xD0AAU, 0x80B7U, 0x3039U, 0x0067U, 0xB0E9U, 0xE0F4U, 0x507AU,
    0x70CEU, 0xC040U, 0x905DU, 0x20D3U, 0x60BBU, 0xD035U, 0x8028U, 0x30A6U, 0x1012U, 0xA09CU, 0xF081U, 0x400FU,
    0x30E4U, 0x806AU, 0xD077U, 0x60F9U, 0x404DU, 0xF0C3U, 0xA0DEU, 0x1050U, 0x5038U, 0xE0B6U, 0xB0ABU, 0x0025U,
    0x2091U, 0x901FU, 0xC002U, 0x708CU, 0x40D2U, 0xF05CU, 0xA041U, 0x10CFU, 0x307BU, 0x80F5U, 0xD0E8U, 0x6066U,
    0x200EU, 0x9080U, 0xC09DU, 0x7013U, 0x50A7U, 0xE029U, 0xB034U, 0x00BAU, 0xE088U, 0x5006U, 0x001BU, 0xB095U,
    0x9021U, 0x20AFU, 0x70B2U, 0xC03CU, 0x8054U, 0x30DAU, 0x60C7U, 0xD049U, 0xF0FDU, 0x4073U, 0x106EU, 0xA0E0U,
    0x90BEU, 0x2030U, 0x702DU, 0xC0A3U, 0xE017U, 0x5099U, 0x0084U, 0xB00AU, 0xF062U, 0x40ECU, 0x10F1U, 0xA07FU,
    0x80CBU, 0x3045U, 0x6058U, 0xD0D6U
 };

const uint32_t ENCODING_TABLE_1676[] =
	{0x0000U, 0x0273U, 0x04E5U, 0x0696U, 0x09C9U, 0x0BBAU, 0x0D2CU, 0x0F5FU, 0x11E2U, 0x1391U, 0x1507U, 0x1774U,
	 0x182BU, 0x1A58U, 0x1CCEU, 0x1EBDU, 0x21B7U, 0x23C4U, 0x2552U, 0x2721U, 0x287EU, 0x2A0DU, 0x2C9BU, 0x2EE8U,
	 0x3055U, 0x3226U, 0x34B0U, 0x36C3U, 0x399CU, 0x3BEFU, 0x3D79U, 0x3F0AU, 0x411EU, 0x436DU, 0x45FBU, 0x4788U,
	 0x48D7U, 0x4AA4U, 0x4C32U, 0x4E41U, 0x50FCU, 0x528FU, 0x5419U, 0x566AU, 0x5935U, 0x5B46U, 0x5DD0U, 0x5FA3U,
	 0x60A9U, 0x62DAU, 0x644CU, 0x663FU, 0x6960U, 0x6B13U, 0x6D85U, 0x6FF6U, 0x714BU, 0x7338U, 0x75AEU, 0x77DDU,
	 0x7882U, 0x7AF1U, 0x7C67U, 0x7E14U, 0x804FU, 0x823CU, 0x84AAU, 0x86D9U, 0x8986U, 0x8BF5U, 0x8D63U, 0x8F10U,
	 0x91ADU, 0x93DEU, 0x9548U, 0x973BU, 0x9864U, 0x9A17U, 0x9C81U, 0x9EF2U, 0xA1F8U, 0xA38BU, 0xA51DU, 0xA76EU,
	 0xA831U, 0xAA42U, 0xACD4U, 0xAEA7U, 0xB01AU, 0xB269U, 0xB4FFU, 0xB68CU, 0xB9D3U, 0xBBA0U, 0xBD36U, 0xBF45U,
	 0xC151U, 0xC322U, 0xC5B4U, 0xC7C7U, 0xC898U, 0xCAEBU, 0xCC7DU, 0xCE0EU, 0xD0B3U, 0xD2C0U, 0xD456U, 0xD625U,
	 0xD97AU, 0xDB09U, 0xDD9FU, 0xDFECU, 0xE0E6U, 0xE295U, 0xE403U, 0xE670U, 0xE92FU, 0xEB5CU, 0xEDCAU, 0xEFB9U,
	 0xF104U, 0xF377U, 0xF5E1U, 0xF792U, 0xF8CDU, 0xFABEU, 0xFC28U, 0xFE5BU
};

#define F2(A,B,C) ( ( A & B ) | ( C & ( A | B ) ) )
#define F1(E,F,G) ( G ^ ( E & ( F ^ G ) ) )

struct sockaddr_in 	host1;
struct sockaddr_in 	host2;
int 				udp1;
int 				udp2;
fd_set 				udpset; 
uint8_t 			buf[BUFSIZE];
char 				callsign[10U];
int					dmrid;
uint32_t			sha256_state[8U];
uint32_t			sha256_total[2];
uint32_t			sha256_buffer[32U];
uint32_t			sha256_buflen;
bool 				bptc_rawData[196];
bool 				bptc_deInterData[196];
bool 				emb_raw[128U];
bool 				emb_data[72U];
int					tx_srcid;
int					tx_tgid;
int					host1_tg;
char				*host1_pw;


static const unsigned char fillbuf[64] = { 0x80, 0 };

#define DISCONNECTED	0
#define CONNECTING		1
#define DMR_AUTH		2
#define DMR_CONF		3
#define DMR_OPTS		4
#define CONNECTED_RW	5
#define CONNECTED_RO	6

// frame type
#define DMRMMDVM_FRAMETYPE_VOICE     0
#define DMRMMDVM_FRAMETYPE_VOICESYNC 1
#define DMRMMDVM_FRAMETYPE_DATASYNC  2

// slot type
#define MMDVM_SLOTTYPE_HEADER        1
#define MMDVM_SLOTTYPE_TERMINATOR    2

void byteToBitsBE(unsigned char byte, bool* bits)
{
	bits[0U] = (byte & 0x80U) == 0x80U;
	bits[1U] = (byte & 0x40U) == 0x40U;
	bits[2U] = (byte & 0x20U) == 0x20U;
	bits[3U] = (byte & 0x10U) == 0x10U;
	bits[4U] = (byte & 0x08U) == 0x08U;
	bits[5U] = (byte & 0x04U) == 0x04U;
	bits[6U] = (byte & 0x02U) == 0x02U;
	bits[7U] = (byte & 0x01U) == 0x01U;
}

void bitsToByteBE(bool* bits, unsigned char* byte)
{
	*byte  = bits[0U] ? 0x80U : 0x00U;
	*byte |= bits[1U] ? 0x40U : 0x00U;
	*byte |= bits[2U] ? 0x20U : 0x00U;
	*byte |= bits[3U] ? 0x10U : 0x00U;
	*byte |= bits[4U] ? 0x08U : 0x00U;
	*byte |= bits[5U] ? 0x04U : 0x00U;
	*byte |= bits[6U] ? 0x02U : 0x00U;
	*byte |= bits[7U] ? 0x01U : 0x00U;
}

int max(int x, int y) 
{ 
    if (x > y) 
        return x; 
    else
        return y; 
}

static inline void set_uint32(unsigned char* cp, uint32_t v)
{
	memcpy(cp, &v, sizeof v);
}

void process_signal(int sig)
{
	static uint32_t c1 = 0;
	static uint32_t c2 = 0;
	static int cnt = 0;
	uint8_t b[20];
	uint8_t out[55];
	if(sig == SIGINT){
		fprintf(stderr, "\n\nShutting down link\n");
		b[0] = 'R';
		b[1] = 'P';
		b[2] = 'T';
		b[3] = 'C';
		b[4] = 'L';
		b[5] = (dmrid >> 24) & 0xff;
		b[6] = (dmrid >> 16) & 0xff;
		b[7] = (dmrid >> 8) & 0xff;
		b[8] = (dmrid >> 0) & 0xff;
		sendto(udp1, b, 9, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG
		fprintf(stderr, "SEND DMR: ");
		for(int i = 0; i < 9; ++i)
			fprintf(stderr, "%02x ", b[i]);
		fprintf(stderr, "\n");
#endif
		close(udp1);
		close(udp2);
		exit(EXIT_SUCCESS);
	}
	if(sig == SIGALRM){
		++cnt;
		char tag[] = { 'R','P','T','P','I','N','G' };
		memcpy(b, tag, 7);
		b[7] = (dmrid >> 24) & 0xff;
		b[8] = (dmrid >> 16) & 0xff;
		b[9] = (dmrid >> 8) & 0xff;
		b[10] = (dmrid >> 0) & 0xff;
		sendto(udp1, b, 11, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG
		fprintf(stderr, "SEND DMR: ");
		for(int i = 0; i < 11; ++i)
			fprintf(stderr, "%02x ", b[i]);
		fprintf(stderr, "\n");
#endif
		alarm(5);
	}
}

void hamming_encode15113_2(bool* d)
{
    d[11] = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
    d[12] = d[1] ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[8] ^ d[9];
    d[13] = d[2] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[9] ^ d[10];
    d[14] = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7] ^ d[10];
}

void hamming_encode1393(bool* d)
{
    d[9]  = d[0] ^ d[1] ^ d[3] ^ d[5] ^ d[6];
    d[10] = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7];
    d[11] = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
    d[12] = d[0] ^ d[2] ^ d[4] ^ d[5] ^ d[8];
}

void bptc_encode(const unsigned char* in, unsigned char* out)
{
	//Extract
    bool bData[96U];
	byteToBitsBE(in[0U],  bData + 0U);
	byteToBitsBE(in[1U],  bData + 8U);
	byteToBitsBE(in[2U],  bData + 16U);
	byteToBitsBE(in[3U],  bData + 24U);
	byteToBitsBE(in[4U],  bData + 32U);
	byteToBitsBE(in[5U],  bData + 40U);
	byteToBitsBE(in[6U],  bData + 48U);
	byteToBitsBE(in[7U],  bData + 56U);
	byteToBitsBE(in[8U],  bData + 64U);
	byteToBitsBE(in[9U],  bData + 72U);
	byteToBitsBE(in[10U], bData + 80U);
	byteToBitsBE(in[11U], bData + 88U);
    
    for (unsigned int i = 0U; i < 196U; i++)
        bptc_deInterData[i] = false;
    
    unsigned int pos = 0U;
    for (unsigned int a = 4U; a <= 11U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 16U; a <= 26U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 31U; a <= 41U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 46U; a <= 56U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 61U; a <= 71U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 76U; a <= 86U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 91U; a <= 101U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 106U; a <= 116U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
    
    for (unsigned int a = 121U; a <= 131U; a++, pos++)
        bptc_deInterData[a] = bData[pos];
        
    //Error check
    for (unsigned int r = 0U; r < 9U; r++) {
        unsigned int pos = (r * 15U) + 1U;
        hamming_encode15113_2(bptc_deInterData + pos);
    }
    
    bool col[13U];
    for (unsigned int c = 0U; c < 15U; c++) {
        unsigned int pos = c + 1U;
        for (unsigned int a = 0U; a < 13U; a++) {
            col[a] = bptc_deInterData[pos];
            pos = pos + 15U;
        }
        
        hamming_encode1393(col);
        
        pos = c + 1U;
        for (unsigned int a = 0U; a < 13U; a++) {
            bptc_deInterData[pos] = col[a];
            pos = pos + 15U;
        }
    }
    //Interleave
    for (unsigned int i = 0U; i < 196U; i++)
        bptc_rawData[i] = false;
    
    for (unsigned int a = 0U; a < 196U; a++)	{
        unsigned int interleaveSequence = (a * 181U) % 196U;
        bptc_rawData[interleaveSequence] = bptc_deInterData[a];
    }
    //Extract
    bitsToByteBE(bptc_rawData + 0U,  &out[0U]);
	bitsToByteBE(bptc_rawData + 8U,  &out[1U]);
	bitsToByteBE(bptc_rawData + 16U, &out[2U]);
	bitsToByteBE(bptc_rawData + 24U, &out[3U]);
	bitsToByteBE(bptc_rawData + 32U, &out[4U]);
	bitsToByteBE(bptc_rawData + 40U, &out[5U]);
	bitsToByteBE(bptc_rawData + 48U, &out[6U]);
	bitsToByteBE(bptc_rawData + 56U, &out[7U]);
	bitsToByteBE(bptc_rawData + 64U, &out[8U]);
	bitsToByteBE(bptc_rawData + 72U, &out[9U]);
	bitsToByteBE(bptc_rawData + 80U, &out[10U]);
	bitsToByteBE(bptc_rawData + 88U, &out[11U]);
    
    unsigned char byte;
	bitsToByteBE(bptc_rawData + 96U, &byte);
    out[12U] = (out[12U] & 0x3FU) | ((byte >> 0) & 0xC0U);
    out[20U] = (out[20U] & 0xFCU) | ((byte >> 4) & 0x03U);
    
	bitsToByteBE(bptc_rawData + 100U,  &out[21U]);
	bitsToByteBE(bptc_rawData + 108U,  &out[22U]);
	bitsToByteBE(bptc_rawData + 116U,  &out[23U]);
	bitsToByteBE(bptc_rawData + 124U,  &out[24U]);
	bitsToByteBE(bptc_rawData + 132U,  &out[25U]);
	bitsToByteBE(bptc_rawData + 140U,  &out[26U]);
	bitsToByteBE(bptc_rawData + 148U,  &out[27U]);
	bitsToByteBE(bptc_rawData + 156U,  &out[28U]);
	bitsToByteBE(bptc_rawData + 164U,  &out[29U]);
	bitsToByteBE(bptc_rawData + 172U,  &out[30U]);
	bitsToByteBE(bptc_rawData + 180U,  &out[31U]);
	bitsToByteBE(bptc_rawData + 188U,  &out[32U]);
}

unsigned char rs129_gmult(unsigned char a, unsigned char b)
{
    if (a == 0U || b == 0U)
        return 0U;
    
    unsigned int i = LOG_TABLE[a];
    unsigned int j = LOG_TABLE[b];
    
    return EXP_TABLE[i + j];
}

void generate_header()
{
	uint8_t sync_ms_data[]     = { 0x0D,0x5D,0x7F,0x77,0xFD,0x75,0x70 };
	uint8_t payload[33];

	memset(payload, 0, sizeof(payload));

	uint8_t lc[12];
	{
		memset(lc, 0, sizeof(lc));
		//DESTID/TGID
		lc[3] = buf[8];		
		lc[4] = buf[9];
		lc[5] = buf[10];
		//SRCID
		lc[6] = buf[5];
		lc[7] = buf[6];
		lc[8] = buf[7];

		uint8_t parity[4];
		
		//RS129 Encode begin
		for (unsigned int i = 0U; i < 3U + 1U; i++)
        parity[i] = 0x00U;
    
		for (unsigned int i = 0U; i < 9; i++) {
			unsigned char dbyte = lc[i] ^ parity[3U - 1U];
        
			for (int j = 3U - 1; j > 0; j--)
				parity[j] = parity[j - 1] ^ rs129_gmult(POLY[j], dbyte);
        
			parity[0] = rs129_gmult(POLY[0], dbyte);
		}
		//RS129 Encode end
		
		lc[9]  = parity[2] ^ 0x96;
		lc[10] = parity[1] ^ 0x96;
		lc[11] = parity[0] ^ 0x96;
	}
	
	memcpy(payload+13, sync_ms_data, sizeof(sync_ms_data));
	{
		uint8_t slottype[3];
		memset(slottype, 0, sizeof(slottype));
		slottype[0]  = (1 << 4) & 0xF0;
		slottype[0] |= (1  << 0) & 0x0FU;
		//Golay2087 encoding
		slottype[1U] = ENCODING_TABLE_2087[slottype[0]] & 0xFFU;
		slottype[2U] = ENCODING_TABLE_2087[slottype[0]] >> 8;
		payload[12U] = (payload[12U] & 0xC0U) | ((slottype[0U] >> 2) & 0x3FU);
		payload[13U] = (payload[13U] & 0x0FU) | ((slottype[0U] << 6) & 0xC0U) | ((slottype[1U] >> 2) & 0x30U);
		payload[19U] = (payload[19U] & 0xF0U) | ((slottype[1U] >> 2) & 0x0FU);
		payload[20U] = (payload[20U] & 0x03U) | ((slottype[1U] << 6) & 0xC0U) | ((slottype[2U] >> 2) & 0x3CU);

	}
	bptc_encode(lc, payload);
	memcpy(buf + 20, payload, 33);
}

void sha256_process_block(const unsigned char* buffer, unsigned int len)
{
	const uint32_t* words = (uint32_t*)buffer;
	unsigned int nwords = len / sizeof(uint32_t);
	const uint32_t* endp = words + nwords;
	uint32_t x[16];
	uint32_t a = sha256_state[0];
	uint32_t b = sha256_state[1];
	uint32_t c = sha256_state[2];
	uint32_t d = sha256_state[3];
	uint32_t e = sha256_state[4];
	uint32_t f = sha256_state[5];
	uint32_t g = sha256_state[6];
	uint32_t h = sha256_state[7];

	sha256_total[0] += len;
	
	if (sha256_total[0] < len)
		++sha256_total[1];

	#define rol(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
	#define S0(x) (rol(x,25)^rol(x,14)^(x>>3))
	#define S1(x) (rol(x,15)^rol(x,13)^(x>>10))
	#define SS0(x) (rol(x,30)^rol(x,19)^rol(x,10))
	#define SS1(x) (rol(x,26)^rol(x,21)^rol(x,7))

	#define M(I) (tm = S1(x[(I-2)&0x0f]) + x[(I-7)&0x0f] + S0(x[(I-15)&0x0f]) + x[I&0x0f], x[I&0x0f] = tm)

	#define R(A,B,C,D,E,F,G,H,K,M)  do { t0 = SS0(A) + F2(A,B,C);			\
					     t1 = H + SS1(E) + F1(E,F,G) + K + M;	\
					     D += t1;  H = t0 + t1;			\
					} while(0)

	while (words < endp) {
		uint32_t tm;
		uint32_t t0, t1;

		for (unsigned int t = 0U; t < 16U; t++) {
			x[t] = SWAP(*words);
			words++;
		}

		R( a, b, c, d, e, f, g, h, K( 0), x[ 0] );
		R( h, a, b, c, d, e, f, g, K( 1), x[ 1] );
		R( g, h, a, b, c, d, e, f, K( 2), x[ 2] );
		R( f, g, h, a, b, c, d, e, K( 3), x[ 3] );
		R( e, f, g, h, a, b, c, d, K( 4), x[ 4] );
		R( d, e, f, g, h, a, b, c, K( 5), x[ 5] );
		R( c, d, e, f, g, h, a, b, K( 6), x[ 6] );
		R( b, c, d, e, f, g, h, a, K( 7), x[ 7] );
		R( a, b, c, d, e, f, g, h, K( 8), x[ 8] );
		R( h, a, b, c, d, e, f, g, K( 9), x[ 9] );
		R( g, h, a, b, c, d, e, f, K(10), x[10] );
		R( f, g, h, a, b, c, d, e, K(11), x[11] );
		R( e, f, g, h, a, b, c, d, K(12), x[12] );
		R( d, e, f, g, h, a, b, c, K(13), x[13] );
		R( c, d, e, f, g, h, a, b, K(14), x[14] );
		R( b, c, d, e, f, g, h, a, K(15), x[15] );
		R( a, b, c, d, e, f, g, h, K(16), M(16) );
		R( h, a, b, c, d, e, f, g, K(17), M(17) );
		R( g, h, a, b, c, d, e, f, K(18), M(18) );
		R( f, g, h, a, b, c, d, e, K(19), M(19) );
		R( e, f, g, h, a, b, c, d, K(20), M(20) );
		R( d, e, f, g, h, a, b, c, K(21), M(21) );
		R( c, d, e, f, g, h, a, b, K(22), M(22) );
		R( b, c, d, e, f, g, h, a, K(23), M(23) );
		R( a, b, c, d, e, f, g, h, K(24), M(24) );
		R( h, a, b, c, d, e, f, g, K(25), M(25) );
		R( g, h, a, b, c, d, e, f, K(26), M(26) );
		R( f, g, h, a, b, c, d, e, K(27), M(27) );
		R( e, f, g, h, a, b, c, d, K(28), M(28) );
		R( d, e, f, g, h, a, b, c, K(29), M(29) );
		R( c, d, e, f, g, h, a, b, K(30), M(30) );
		R( b, c, d, e, f, g, h, a, K(31), M(31) );
		R( a, b, c, d, e, f, g, h, K(32), M(32) );
		R( h, a, b, c, d, e, f, g, K(33), M(33) );
		R( g, h, a, b, c, d, e, f, K(34), M(34) );
		R( f, g, h, a, b, c, d, e, K(35), M(35) );
		R( e, f, g, h, a, b, c, d, K(36), M(36) );
		R( d, e, f, g, h, a, b, c, K(37), M(37) );
		R( c, d, e, f, g, h, a, b, K(38), M(38) );
		R( b, c, d, e, f, g, h, a, K(39), M(39) );
		R( a, b, c, d, e, f, g, h, K(40), M(40) );
		R( h, a, b, c, d, e, f, g, K(41), M(41) );
		R( g, h, a, b, c, d, e, f, K(42), M(42) );
		R( f, g, h, a, b, c, d, e, K(43), M(43) );
		R( e, f, g, h, a, b, c, d, K(44), M(44) );
		R( d, e, f, g, h, a, b, c, K(45), M(45) );
		R( c, d, e, f, g, h, a, b, K(46), M(46) );
		R( b, c, d, e, f, g, h, a, K(47), M(47) );
		R( a, b, c, d, e, f, g, h, K(48), M(48) );
		R( h, a, b, c, d, e, f, g, K(49), M(49) );
		R( g, h, a, b, c, d, e, f, K(50), M(50) );
		R( f, g, h, a, b, c, d, e, K(51), M(51) );
		R( e, f, g, h, a, b, c, d, K(52), M(52) );
		R( d, e, f, g, h, a, b, c, K(53), M(53) );
		R( c, d, e, f, g, h, a, b, K(54), M(54) );
		R( b, c, d, e, f, g, h, a, K(55), M(55) );
		R( a, b, c, d, e, f, g, h, K(56), M(56) );
		R( h, a, b, c, d, e, f, g, K(57), M(57) );
		R( g, h, a, b, c, d, e, f, K(58), M(58) );
		R( f, g, h, a, b, c, d, e, K(59), M(59) );
		R( e, f, g, h, a, b, c, d, K(60), M(60) );
		R( d, e, f, g, h, a, b, c, K(61), M(61) );
		R( c, d, e, f, g, h, a, b, K(62), M(62) );
		R( b, c, d, e, f, g, h, a, K(63), M(63) );

		a = sha256_state[0] += a;
		b = sha256_state[1] += b;
		c = sha256_state[2] += c;
		d = sha256_state[3] += d;
		e = sha256_state[4] += e;
		f = sha256_state[5] += f;
		g = sha256_state[6] += g;
		h = sha256_state[7] += h;
	}
}

void sha256_generate(char *in, int len, char *out)
{
	unsigned int bytes, size;
	
	sha256_state[0] = 0x6a09e667UL;
	sha256_state[1] = 0xbb67ae85UL;
	sha256_state[2] = 0x3c6ef372UL;
	sha256_state[3] = 0xa54ff53aUL;
	sha256_state[4] = 0x510e527fUL;
	sha256_state[5] = 0x9b05688cUL;
	sha256_state[6] = 0x1f83d9abUL;
	sha256_state[7] = 0x5be0cd19UL;

	sha256_total[0] = sha256_total[1] = 0;
	sha256_buflen   = 0;
	
	if (sha256_buflen != 0U) {
		unsigned int left_over = sha256_buflen;
		unsigned int add = 128U - left_over > len ? len : 128U - left_over;

		memcpy(&((char*)sha256_buffer)[left_over], in, add);
		sha256_buflen += add;

		if (sha256_buflen > 64U) {
			sha256_process_block((unsigned char*)sha256_buffer, sha256_buflen & ~63U);

			sha256_buflen &= 63U;

			memcpy(sha256_buffer, &((char*)sha256_buffer)[(left_over + add) & ~63U], sha256_buflen);
		}

		in += add;
		len -= add;
	}

	if (len >= 64U) {
		sha256_process_block(in, len & ~63U);
		in += (len & ~63U);
		len &= 63U;
	}
	if (len > 0U) {
		unsigned int left_over = sha256_buflen;

		memcpy(&((char*)sha256_buffer)[left_over], in, len);
		left_over += len;

		if (left_over >= 64U) {
			sha256_process_block((unsigned char*)sha256_buffer, 64U);
			left_over -= 64U;
			memcpy(sha256_buffer, &sha256_buffer[16], left_over);
		}

		sha256_buflen = left_over;
	}
	
	bytes = sha256_buflen;
	size = (bytes < 56) ? 64 / 4 : 64 * 2 / 4;
	
	sha256_total[0] += bytes;
	
	if (sha256_total[0] < bytes)
		++sha256_total[1];
	
	set_uint32((unsigned char*)&sha256_buffer[size - 2], SWAP((sha256_total[1] << 3) | (sha256_total[0] >> 29)));
	set_uint32((unsigned char*)&sha256_buffer[size - 1], SWAP(sha256_total[0] << 3));

	memcpy(&((char*)sha256_buffer)[bytes], fillbuf, (size - 2) * 4 - bytes);

	sha256_process_block((unsigned char*)sha256_buffer, size * 4);
		
	for (unsigned int i = 0U; i < 8U; i++)
		set_uint32(out + i * sizeof(sha256_state[0]), SWAP(sha256_state[i]));
}

void lc_get_data(uint8_t *bytes)
{
	bool pf, r;
	uint8_t fid, options;
	pf = (bytes[0U] & 0x80U) == 0x80U;
	r  = (bytes[0U] & 0x40U) == 0x40U;
	fid = bytes[1U];
	options = bytes[2U];

	bytes[0U] = 0;

	if (pf)
		bytes[0U] |= 0x80U;

	if (r)
		bytes[0U] |= 0x40U;

	bytes[1U] = fid;
	bytes[2U] = options;
	bytes[3U] = tx_tgid >> 16;
	bytes[4U] = tx_tgid >> 8;
	bytes[5U] = tx_tgid >> 0;
	bytes[6U] = tx_srcid >> 16;
	bytes[7U] = tx_srcid >> 8;
	bytes[8U] = tx_srcid >> 0;
}

void lc_get_data_bits(bool* bits)
{
	uint8_t bytes[9U];
	memset(bytes, 0, 9);
	lc_get_data(bytes);

	byteToBitsBE(bytes[0U], bits + 0U);
	byteToBitsBE(bytes[1U], bits + 8U);
	byteToBitsBE(bytes[2U], bits + 16U);
	byteToBitsBE(bytes[3U], bits + 24U);
	byteToBitsBE(bytes[4U], bits + 32U);
	byteToBitsBE(bytes[5U], bits + 40U);
	byteToBitsBE(bytes[6U], bits + 48U);
	byteToBitsBE(bytes[7U], bits + 56U);
	byteToBitsBE(bytes[8U], bits + 64U);
}

void encode16114(bool* d)
{
	d[11] = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
	d[12] = d[1] ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[8] ^ d[9];
	d[13] = d[2] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[9] ^ d[10];
	d[14] = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7] ^ d[10];
	d[15] = d[0] ^ d[2] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[10];
}

void encode_qr1676(uint8_t* data)
{
	uint32_t value = (data[0U] >> 1) & 0x7FU;
	uint32_t cksum = ENCODING_TABLE_1676[value];

	data[0U] = cksum >> 8;
	data[1U] = cksum & 0xFFU;
}

void encode_embedded_data()
{
	uint32_t crc;
	unsigned short total = 0U;
	
	lc_get_data_bits(emb_data);
	
	for (unsigned int i = 0U; i < 72U; i += 8U) {
		unsigned char c;
		bitsToByteBE(emb_data + i, &c);
		total += c;
	}

	total %= 31U;
	crc = total;

	bool data[128U];
	memset(data, 0x00U, 128U * sizeof(bool));

	data[106U] = (crc & 0x01U) == 0x01U;
	data[90U]  = (crc & 0x02U) == 0x02U;
	data[74U]  = (crc & 0x04U) == 0x04U;
	data[58U]  = (crc & 0x08U) == 0x08U;
	data[42U]  = (crc & 0x10U) == 0x10U;

	uint32_t b = 0U;
	for (uint32_t a = 0U; a < 11U; a++, b++)
		data[a] = emb_data[b];
		
	for (uint32_t a = 16U; a < 27U; a++, b++)
		data[a] = emb_data[b];
		
	for (uint32_t a = 32U; a < 42U; a++, b++)
		data[a] = emb_data[b];
		
	for (uint32_t a = 48U; a < 58U; a++, b++)
		data[a] = emb_data[b];
		
	for (uint32_t a = 64U; a < 74U; a++, b++)
		data[a] = emb_data[b];
		
	for (uint32_t a = 80U; a < 90U; a++, b++)
		data[a] = emb_data[b];
		
	for (uint32_t a = 96U; a < 106U; a++, b++)
		data[a] = emb_data[b];
		
	for (uint32_t a = 0U; a < 112U; a += 16U)
		encode16114(data + a);
		
	for (uint32_t a = 0U; a < 16U; a++)
		data[a + 112U] = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U];

	b = 0U;
	for (uint32_t a = 0U; a < 128U; a++) {
		emb_raw[a] = data[b];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}
}

uint8_t get_embedded_data(uint8_t* data, uint8_t n)
{
	if (n >= 1U && n < 5U) {
		n--;

		bool bits[40U];
		memset(bits, 0x00U, 40U * sizeof(bool));
		memcpy(bits + 4U, emb_raw + n * 32U, 32U * sizeof(bool));

		uint8_t bytes[5U];
		bitsToByteBE(bits + 0U,  &bytes[0U]);
		bitsToByteBE(bits + 8U,  &bytes[1U]);
		bitsToByteBE(bits + 16U, &bytes[2U]);
		bitsToByteBE(bits + 24U, &bytes[3U]);
		bitsToByteBE(bits + 32U, &bytes[4U]);

		data[14U] = (data[14U] & 0xF0U) | (bytes[0U] & 0x0FU);
		data[15U] = bytes[1U];
		data[16U] = bytes[2U];
		data[17U] = bytes[3U];
		data[18U] = (data[18U] & 0x0FU) | (bytes[4U] & 0xF0U);

		switch (n) {
		case 0U:
			return 1U;
		case 3U:
			return 2U;
		default:
			return 3U;
		}
	} else {
		data[14U] &= 0xF0U;
		data[15U]  = 0x00U;
		data[16U]  = 0x00U;
		data[17U]  = 0x00U;
		data[18U] &= 0x0FU;

		return 0U;
	}
}

void get_emb_data(uint8_t* data, uint8_t lcss)
{
	uint8_t DMREMB[2U];
	uint8_t m_colorcode = 1;
	DMREMB[0U]  = (m_colorcode << 4) & 0xF0U;
	//DMREMB[0U] |= m_PI ? 0x08U : 0x00U;
	DMREMB[0U] |= (lcss << 1) & 0x06U;
	DMREMB[1U]  = 0x00U;

	encode_qr1676(DMREMB);

	data[13U] = (data[13U] & 0xF0U) | ((DMREMB[0U] >> 4U) & 0x0FU);
	data[14U] = (data[14U] & 0x0FU) | ((DMREMB[0U] << 4U) & 0xF0U);
	data[18U] = (data[18U] & 0xF0U) | ((DMREMB[1U] >> 4U) & 0x0FU);
	data[19U] = (data[19U] & 0x0FU) | ((DMREMB[1U] << 4U) & 0xF0U);
}

int process_connect(int connect_status, char *buf)
{
	char in[100];
	char out[400];
	int len = 0;
	char latitude[20U], longitude[20U];
	memset(in, 0, 100);
	memset(out, 0, 400);
	
	switch(connect_status){
	case CONNECTING:
		connect_status = DMR_AUTH;
		memcpy(in, &buf[6], 4);
		memcpy(out, "RPTK", 4);
		out[4] = (dmrid >> 24) & 0xff;
		out[5] = (dmrid >> 16) & 0xff;
		out[6] = (dmrid >> 8) & 0xff;
		out[7] = (dmrid >> 0) & 0xff;
		memcpy(&in[4], host1_pw, strlen(host1_pw));
		sha256_generate(in, strlen(host1_pw) + sizeof(uint32_t), &out[8]);
		len = 40;
		fprintf(stderr, "Sending auth to DMR...\n");
		break;
	case DMR_AUTH:
		connect_status = DMR_CONF;
		memcpy(out, "RPTC", 4);
		out[4] = (dmrid >> 24) & 0xff;
		out[5] = (dmrid >> 16) & 0xff;
		out[6] = (dmrid >> 8) & 0xff;
		out[7] = (dmrid >> 0) & 0xff;
		sprintf(latitude, "%08f", 0.0f);
		sprintf(longitude, "%09f", 0.0f);
		sprintf(&out[8], "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s", callsign,
				435000000, 435000000, 1, 1, latitude, longitude, 0, "Nowhere","Portugal", '4', "www.google.com", "20210101", "MMDVM"); // 302 bytes
		len = 302;
		fprintf(stderr, "Sending conf to DMR...\n");
		break;
	case DMR_CONF:
		connect_status = CONNECTED_RW;
		memcpy(buf, "DMRD", 4);
		buf[4] = 0x00;
		buf[5] = (((dmrid>99999999)?dmrid/100:dmrid) >> 16) & 0xff;
		buf[6] = (((dmrid>99999999)?dmrid/100:dmrid) >> 8) & 0xff;
		buf[7] = (((dmrid>99999999)?dmrid/100:dmrid) >> 0) & 0xff;
		buf[8] = (host1_tg >> 16) & 0xff;
		buf[9] = (host1_tg >> 8) & 0xff;
		buf[10] = (host1_tg >> 0) & 0xff;
		buf[11] = (dmrid >> 24) & 0xff;
		buf[12] = (dmrid >> 16) & 0xff;
		buf[13] = (dmrid >> 8) & 0xff;
		buf[14] = (dmrid >> 0) & 0xff;
		buf[15] = 0xa1;
		buf[16] = 0xb6;
		buf[17] = 0x01;
		buf[18] = 0x00;
		buf[19] = 0x00;
		generate_header();
		memcpy(out, buf, 55);
		len = 55;
		fprintf(stderr, "Connected to DMR\n");
		break;
	}
	sendto(udp1, out, len, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG
	fprintf(stderr, "SEND DMR: ");
	for(int i = 0; i < len; ++i)
		fprintf(stderr, "%02x ", (uint8_t)out[i]);
	fprintf(stderr, "\n");
#endif
	return connect_status;
}

typedef struct wav_header_t {
  // RIFF Header
  char riff_header[4]; // Contains "RIFF"
  int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
  char wave_header[4]; // Contains "WAVE"
  // Format Header
  char fmt_header[4]; // Contains "fmt " (includes trailing space)
  int fmt_chunk_size; // Should be 16 for PCM
  short audio_format; // Should be 1 for PCM. 3 for IEEE Float
  short num_channels; // Number of channels
  int sample_rate; // Sample rate (hz)
  int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
  short sample_alignment; // Number of bytes per sample. num_channels * Bytes Per Sample
  short bit_depth; // Number of bits per sample
  // Data
  char data_header[4]; // Contains "data"
  int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
} wav_header;

int main(int argc, char **argv)
{
	struct 	sockaddr_in rx;
	struct 	hostent *hp;
	struct  timeval tv;
	char *	host1_url;
	char *	host2_url;
	int 	host1_port;
	int 	host2_port;
	int 	rxlen;
	int 	r;
	int 	udprx,maxudp;
	socklen_t l = sizeof(host1);
	time_t pong_time1;
	int64_t rx_streamid = -1;
	uint32_t tx_streamid = 0;
	time_t rx_endt = 0;
	FILE *rx_ambefile = NULL;
	FILE *rx_wavefile = NULL;
	FILE *tx_wavefile = NULL;
	wav_header rx_wavheader;
	int rx_ambefcnt = 0;
	int tx_ambefcnt = 0;
	uint8_t tx_ambefr[3][9];
	int64_t trgus = 0;
	int rx_srcid = 0;
	bool txpending = false;
	
	if(argc != 5){
		fprintf(stderr, "Usage: dmrbot [CALLSIGN] [DMRID] [DMRHost1IP:PORT:TG:PW] [AMBEServerIP:PORT]\n");
		return 0;
	}
	else{
		memset(callsign, ' ', 10);
		memcpy(callsign, argv[1], strlen(argv[1]));
		dmrid = atoi(argv[2]);
		host1_url = strtok(argv[3], ":");
		host1_port = atoi(strtok(NULL, ":"));
		host1_tg = atoi(strtok(NULL, ":"));
		host1_pw = strtok(NULL, ":");
		host2_url = strtok(argv[4], ":");
		host2_port = atoi(strtok(NULL, ":"));
		printf("DMR: %s:%d\n", host1_url, host1_port);
		printf("AMBEServer: %s:%d\n", host2_url, host2_port);
	}
	
	signal(SIGINT, process_signal); 						//Handle CTRL-C gracefully
	signal(SIGALRM, process_signal); 						//Ping timer
	
  //change stdout/stderr to line buffering
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);
	
  //change working directory to the executable directory
  char exepath[256+1] = {0};
  if (readlink("/proc/self/exe", exepath, sizeof(exepath)-1) == -1) {
    fprintf(stderr, "failed to get executable path\n");
    return 1;
  }
  char *exelastslash = strrchr(exepath, '/');
  if (exelastslash == NULL) {
    fprintf(stderr, "failed to extract executable directory\n");
    return 1;
  }
  *(exelastslash+1) = '\0';
  if (chdir(exepath) == -1) {
    fprintf(stderr, "failed to change working directory\n");
    return 1;
  }
	
	//seed random generator
	srand(time(NULL));
	
	if ((udp1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}
	
	if ((udp2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}
	
	maxudp = max(udp1, udp2) + 1;
	memset((char *)&host1, 0, sizeof(host1));
	host1.sin_family = AF_INET;
	host1.sin_port = htons(host1_port);
	memset((char *)&host2, 0, sizeof(host2));
	host2.sin_family = AF_INET;
	host2.sin_port = htons(host2_port);

	hp = gethostbyname(host1_url);
	if (!hp) {
		fprintf(stderr, "could not resolve %s\n", host1_url);
		return 0;
	}
	memcpy((void *)&host1.sin_addr, hp->h_addr_list[0], hp->h_length);

	hp = gethostbyname(host2_url);
	if (!hp) {
		fprintf(stderr, "could not resolve %s\n", host2_url);
		return 0;
	}
	memcpy((void *)&host2.sin_addr, hp->h_addr_list[0], hp->h_length);

	int host1_connect_status = DISCONNECTED;
	
	alarm(5);
	
	while (1) {
		if(host1_connect_status == DISCONNECTED){
			host1_connect_status = CONNECTING;
			pong_time1 = time(NULL);
			buf[0] = 'R';
			buf[1] = 'P';
			buf[2] = 'T';
			buf[3] = 'L';
			buf[4] = (dmrid >> 24) & 0xff;
			buf[5] = (dmrid >> 16) & 0xff;
			buf[6] = (dmrid >> 8) & 0xff;
			buf[7] = (dmrid >> 0) & 0xff;
			sendto(udp1, buf, 8, 0, (const struct sockaddr *)&host1, sizeof(host1));
			fprintf(stderr, "Connecting to %s...\n", host1_url);
#ifdef DEBUG
			fprintf(stderr, "SEND DMR: ");
			for(int i = 0; i < 8; ++i)
				fprintf(stderr, "%02x ", buf[i]);
			fprintf(stderr, "\n");
#endif
		}
		FD_ZERO(&udpset);
		FD_SET(udp1, &udpset);
		FD_SET(udp2, &udpset);
		tv.tv_sec = 0;
		tv.tv_usec = 5*1000;
		r = select(maxudp, &udpset, NULL, NULL, &tv);
		//fprintf(stderr, "Select returned r == %d\n", r);
		rxlen = 0;
		if(r > 0){
			if(FD_ISSET(udp1, &udpset)) {
				rxlen = recvfrom(udp1, buf, BUFSIZE, 0, (struct sockaddr *)&rx, &l);
				udprx = udp1;
			}
			else if(FD_ISSET(udp2, &udpset)) {
				rxlen = recvfrom(udp2, buf, BUFSIZE, 0, (struct sockaddr *)&rx, &l);
				udprx = udp2;
			}
		}
#ifdef DEBUG
		if(rxlen >= 11){
			if ((udprx == udp1) && (rx.sin_addr.s_addr == host1.sin_addr.s_addr)){
				fprintf(stderr, "RECV DMR: ");
			}
			else if((udprx == udp2) && (rx.sin_addr.s_addr == host2.sin_addr.s_addr)){
				fprintf(stderr, "RECV AMBE: ");
			}
			for(int i = 0; i < rxlen; ++i){
				fprintf(stderr, "%02x ", buf[i]);
			}
			fprintf(stderr, "\n");
		}
#endif
    if( rxlen && (udprx == udp1) && (rx.sin_addr.s_addr == host1.sin_addr.s_addr) ){
      if((host1_connect_status != CONNECTED_RW) && (memcmp(buf, "RPTACK", 6U) == 0)){
        host1_connect_status = process_connect(host1_connect_status, buf);
      }
      else if( (host1_connect_status == CONNECTED_RW) && (memcmp(buf, "MSTPONG", 7U) == 0) ){
        pong_time1 = time(NULL);
      }
      else if( (host1_connect_status == CONNECTED_RW) && (rxlen == 55) && (memcmp(buf, "DMRD", 4U) == 0) ){
        rx_srcid = ((buf[5] << 16) & 0xff0000) | ((buf[6] << 8) & 0xff00) | (buf[7] & 0xff);
        
        uint8_t Slot = (buf[15] & 0x80) >> 7; //0: slot1, 1: slot2
        uint8_t CallType = (buf[15] & 0x40) >> 6; //0: group call, 1: private call
        uint8_t FrameType = (buf[15] & 0x30) >> 4;

        if ( (FrameType == DMRMMDVM_FRAMETYPE_DATASYNC) && (CallType == 0) ) {
          if ((buf[15] & 0x0F) == MMDVM_SLOTTYPE_HEADER) {
            //ignore duplicate header packets with the same stream id
            if ( rx_streamid == *(uint32_t *)(&buf[16]) )
              continue;
            rx_streamid = *(uint32_t *)(&buf[16]);
            
            /*if (rx_ambefile != NULL) fclose(rx_ambefile);
            rx_ambefile = fopen("rx.ambe" , "wb");
            static const uint8_t header[] = {'A','M','B','E'};
            if (rx_ambefile != NULL)
              fwrite(header, 1, sizeof(header), rx_ambefile);*/

            if (rx_wavefile != NULL) {
              rewind(rx_wavefile);
              rx_wavheader.data_bytes = rx_ambefcnt * 320;
              rx_wavheader.wav_size = rx_wavheader.data_bytes + sizeof(rx_wavheader) - 8;
              fwrite(&rx_wavheader, 1, sizeof(rx_wavheader), rx_wavefile);
              fclose(rx_wavefile);
              rx_wavefile = NULL;
            }

            memcpy(rx_wavheader.riff_header, "RIFF", 4);
            memcpy(rx_wavheader.wave_header, "WAVE", 4);
            memcpy(rx_wavheader.fmt_header, "fmt ", 4);
            rx_wavheader.fmt_chunk_size = 16;
            rx_wavheader.audio_format = 1;
            memcpy(rx_wavheader.data_header, "data", 4);
            rx_wavheader.num_channels = 1;
            rx_wavheader.sample_rate = 8000;
            rx_wavheader.bit_depth = 16;
            rx_wavheader.sample_alignment = (rx_wavheader.bit_depth / 8) * rx_wavheader.num_channels;
            rx_wavheader.byte_rate = rx_wavheader.sample_rate * rx_wavheader.sample_alignment;
            rx_wavheader.data_bytes = 0; //filled later
            rx_wavheader.wav_size = rx_wavheader.data_bytes + sizeof(rx_wavheader) - 8;

            rx_wavefile = fopen("rx.wav" , "wb");
            if (rx_wavefile != NULL) {
              fwrite(&rx_wavheader, 1, sizeof(rx_wavheader), rx_wavefile);
            } else {
              fprintf(stderr, "failed to open rx.wav file\n");
            }

            static const uint8_t ambe_gain[] = {0x61,0x00,0x03,0x00,0x4B,AMBE_ENCODE_GAIN,AMBE_DECODE_GAIN};
            sendto(udp2, ambe_gain, sizeof(ambe_gain), 0, (const struct sockaddr *)&host2, sizeof(host2));

            static const uint8_t ambe_ratep[] = {0x61,0x00,0x0D,0x00,0x0A,0x04,0x31,0x07,0x54,0x24,0x00,0x00,0x00,0x00,0x00,0x6F,0x48};
            sendto(udp2, ambe_ratep, sizeof(ambe_ratep), 0, (const struct sockaddr *)&host2, sizeof(host2));
            
            printf("*** RX START (srcid: %d) ***\n", rx_srcid);
            rx_ambefcnt = 0;
            rx_endt = time(NULL)+2; //allow rx end without terminator, after extra timeout
          }
          else if ((buf[15] & 0x0F) == MMDVM_SLOTTYPE_TERMINATOR) {
            rx_streamid = -1;
            rx_endt = time(NULL)+1;
          }
        }

        else if ( ((FrameType == DMRMMDVM_FRAMETYPE_VOICE) || (FrameType == DMRMMDVM_FRAMETYPE_VOICESYNC)) && (CallType == 0) ) {
          uint8_t rx_ambefr[3][9];
          memcpy(&rx_ambefr[0][0], &buf[20], 9);
          memcpy(&rx_ambefr[1][0], &buf[29], 4);
          rx_ambefr[1][4] = (buf[33] & 0xF0) | (buf[39] & 0x0F);
          memcpy(&rx_ambefr[1][5], &buf[40], 4);
          memcpy(&rx_ambefr[2][0], &buf[44], 9);
          
          if (rx_ambefile != NULL) {
            fwrite(rx_ambefr[0], 1, 9, rx_ambefile);
            fwrite(rx_ambefr[1], 1, 9, rx_ambefile);
            fwrite(rx_ambefr[2], 1, 9, rx_ambefile);
          }

          //send ambe frames to ambeserver
          uint8_t ambebuf[4+2+9] = {0x61, 0x00, 2+9, 0x01,  0x01, 72};
          for (int i=0; i < 3; i++) {
            memcpy(&ambebuf[6], rx_ambefr[i], 9);
            sendto(udp2, ambebuf, sizeof(ambebuf), 0, (const struct sockaddr *)&host2, sizeof(host2));
          }
          
          rx_endt = time(NULL)+2; //allow rx end without terminator, after extra timeout
        }
        
      }
    }

    else if( rxlen && (udprx == udp2) && (rx.sin_addr.s_addr == host2.sin_addr.s_addr) ){ //from ambeserver
      if (rxlen == 4+2+320) {
        if (rx_wavefile == NULL) { //if rx file not open, discard packet
#ifdef DEBUG
          fprintf(stderr, "*** discarding pcm packet from ambeserver ***\n");
#endif
          continue;
        }
        for (int i=0; i < 160; i++) //swap byte order for all samples, AMBE3000 uses MSB first
          ((unsigned short *)(&buf[6]))[i] = (((unsigned short *)(&buf[6]))[i] >> 8) | (((unsigned short *)(&buf[6]))[i] << 8);
        fwrite(&buf[6], 1, 320, rx_wavefile);
        rx_ambefcnt++;
      }
      else if (rxlen == 4+2+9) {
        if (tx_wavefile == NULL) { //if tx terminated, discard late packet from ambeserver, not good to tx them after terminator
#ifdef DEBUG
          fprintf(stderr, "*** discarding ambe packet from ambeserver ***\n");
#endif
          continue;
        }
        memcpy(tx_ambefr[tx_ambefcnt % 3], &buf[6], 9);
        if ( (tx_ambefcnt % 3) == 2 ) {
          memset(buf, 0, 55);
          memcpy(buf, "DMRD", 4);
          buf[4] = ((tx_ambefcnt / 3) + 1) % 256;
          tx_srcid = ((dmrid>99999999)?dmrid/100:dmrid);
          buf[5] = (tx_srcid >> 16) & 0xff;
          buf[6] = (tx_srcid >> 8) & 0xff;
          buf[7] = (tx_srcid >> 0) & 0xff;
          tx_tgid = host1_tg;
          buf[8] = (host1_tg >> 16) & 0xff;
          buf[9] = (host1_tg >> 8) & 0xff;
          buf[10] = (host1_tg >> 0) & 0xff;
          buf[11] = (dmrid >> 24) & 0xff;
          buf[12] = (dmrid >> 16) & 0xff;
          buf[13] = (dmrid >> 8) & 0xff;
          buf[14] = (dmrid >> 0) & 0xff;

          buf[15] = 0x80 | (((tx_ambefcnt / 3) % 6) & 0x0F);
          if ((buf[15] & 0x0F) == 0) {
            buf[15] |= (DMRMMDVM_FRAMETYPE_VOICESYNC << 4);
          } else {
            buf[15] |= (DMRMMDVM_FRAMETYPE_VOICE << 4);
          }
          
          *(uint32_t *)(&buf[16]) = tx_streamid;

          memcpy(&buf[20], tx_ambefr[0], 9);
          memcpy(&buf[29], tx_ambefr[1], 4);
          buf[33] = tx_ambefr[1][4] & 0xF0;
          buf[39] = tx_ambefr[1][4] & 0x0F;
          memcpy(&buf[40], &tx_ambefr[1][5], 4);
          memcpy(&buf[44], tx_ambefr[2], 9);

          if ((buf[15] & 0x0F) == 0) {
            static const uint8_t sync_ms_voice[] = { 0x07,0xF7,0xD5,0xDD,0x57,0xDF,0xD0 };
            buf[33] = (buf[33] & 0xF0) | (sync_ms_voice[0] & 0x0F);
            memcpy(&buf[34], &sync_ms_voice[1], 5);
            buf[39] = (sync_ms_voice[6] & 0xF0) | (buf[39] & 0x0F);
            encode_embedded_data();
          } else {
            uint8_t lcss = get_embedded_data(buf+20, buf[15] & 0x0F);
            get_emb_data(buf+20, lcss);
          }

          sendto(udp1, buf, 55, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG
          fprintf(stderr, "SEND DMR: ");
          for(int i = 0; i < 55; ++i)
            fprintf(stderr, "%02x ", buf[i]);
          fprintf(stderr, "\n");
#endif
        }
        tx_ambefcnt++;
      }
    }

    //if ((host1_connect_status == CONNECTED_RW) && !rx_endt) { rx_endt=time(NULL)+5; } //dbg

    if (rx_endt && (time(NULL) > rx_endt)) { //rx end
        if (rx_ambefile != NULL) {
          fclose(rx_ambefile);
          rx_ambefile = NULL;
        }
        if (rx_wavefile != NULL) {
          rewind(rx_wavefile);
          rx_wavheader.data_bytes = rx_ambefcnt * 320;
          rx_wavheader.wav_size = rx_wavheader.data_bytes + sizeof(rx_wavheader) - 8;
          fwrite(&rx_wavheader, 1, sizeof(rx_wavheader), rx_wavefile);
          fclose(rx_wavefile);
          rx_wavefile = NULL;
          printf("*** RX END (ambeframes: %d) ***\n", rx_ambefcnt);
          rx_streamid = -1;
          
          if (txpending) { //if there is a pending tx, ignore current rx
            rx_endt = time(NULL)+1; //wait a bit more before starting the pending tx
            continue;
          }
          if (rx_ambefcnt < 50) { //if we got less than 1 sec. of audio
            rx_endt = 0; //cancel processing this short rx
            continue;
          }
          if (tx_wavefile != NULL) {
            fclose(tx_wavefile); //ensure tx wav file is closed before script tries to write on it
            tx_wavefile = NULL;
          }
          char cmdstr[50];
          sprintf(cmdstr, "python3 dmrbot.py %d", rx_srcid);
          if (system(cmdstr) != 0) {
            //rx_endt = 0; //cancel tx if script fails
            //continue;
            fprintf(stderr, "dmrbot.py returned error, tx unavailable.wav file...\n");
            system("cat unavailable.wav > tx.wav");
          }
          pong_time1 = time(NULL); //prevent timeout due to time spent on system() call
          rx_endt = time(NULL)+1; //wait a bit more before starting tx, allow rx to check if someone else tx
          txpending = true;
          continue;
        }
          
        //tx playback
        if ( txpending && (tx_wavefile == NULL) ) {
          printf("*** TX START ***\n");
          txpending = false;
          tx_ambefcnt = 0;
          trgus = 0;
          tx_streamid = (rand() % 0xffffffff) + 1;
          tx_wavefile = fopen("tx.wav" , "rb");
          if (tx_wavefile == NULL) {
            fprintf(stderr, "failed to open tx.wav file\n");
            rx_endt = 0; //cancel tx
            continue;
          }
          
          wav_header tx_wavheader;
          if ( fread(&tx_wavheader, 1, sizeof(tx_wavheader), tx_wavefile) != sizeof(tx_wavheader) ) {
            fprintf(stderr, "invalid wav file\n");
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0; //cancel tx
            continue;
          }
          if ( (memcmp(tx_wavheader.riff_header, "RIFF", 4U) != 0)
            || (memcmp(tx_wavheader.wave_header, "WAVE", 4U) != 0)
            || (memcmp(tx_wavheader.fmt_header,  "fmt ", 4U) != 0) ) {
            fprintf(stderr, "invalid wav file\n");
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0; //cancel tx
            continue;
          }
          if ( (tx_wavheader.sample_rate != 8000) || (tx_wavheader.bit_depth != 16) || (tx_wavheader.num_channels != 1) ) {
            fprintf(stderr, "wav file must be 8000Hz 16-bit mono\n");
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0; //cancel tx
            continue;
          }
          
          //send header packet
          memset(buf, 0, 55);
          memcpy(buf, "DMRD", 4);
          buf[4] = 0;
          tx_srcid = ((dmrid>99999999)?dmrid/100:dmrid);
          buf[5] = (tx_srcid >> 16) & 0xff;
          buf[6] = (tx_srcid >> 8) & 0xff;
          buf[7] = (tx_srcid >> 0) & 0xff;
          tx_tgid = host1_tg;
          buf[8] = (host1_tg >> 16) & 0xff;
          buf[9] = (host1_tg >> 8) & 0xff;
          buf[10] = (host1_tg >> 0) & 0xff;
          buf[11] = (dmrid >> 24) & 0xff;
          buf[12] = (dmrid >> 16) & 0xff;
          buf[13] = (dmrid >> 8) & 0xff;
          buf[14] = (dmrid >> 0) & 0xff;
          buf[15] = 0x80 | (DMRMMDVM_FRAMETYPE_DATASYNC << 4) | MMDVM_SLOTTYPE_HEADER;
          *(uint32_t *)(&buf[16]) = tx_streamid;
          generate_header();
          sendto(udp1, buf, 55, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG
          fprintf(stderr, "SEND DMR: ");
          for(int i = 0; i < 55; ++i)
            fprintf(stderr, "%02x ", buf[i]);
          fprintf(stderr, "\n");
#endif
            
          static const uint8_t ambe_gain[] = {0x61,0x00,0x03,0x00,0x4B,AMBE_ENCODE_GAIN,AMBE_DECODE_GAIN};
          sendto(udp2, ambe_gain, sizeof(ambe_gain), 0, (const struct sockaddr *)&host2, sizeof(host2));

          static const uint8_t ambe_ratep[] = {0x61,0x00,0x0D,0x00,0x0A,0x04,0x31,0x07,0x54,0x24,0x00,0x00,0x00,0x00,0x00,0x6F,0x48};
          sendto(udp2, ambe_ratep, sizeof(ambe_ratep), 0, (const struct sockaddr *)&host2, sizeof(host2));
        }
        
        if (tx_wavefile != NULL) {
          //ensure the code below only runs once every 20ms
          struct timespec nanos;
          clock_gettime(CLOCK_MONOTONIC, &nanos);
          int64_t nowus = (int64_t)nanos.tv_sec * 1000000 + nanos.tv_nsec / 1000;
          if (abs(trgus - nowus) > 1000000)
            trgus = nowus;
          if (nowus < trgus)
            continue;
          trgus += 20000;
          //printf("%lld\n", nowus/1000);
          
          uint8_t ambebuf[4+2+320] = {0x61, 0x01, 0x42, 0x02,  0x00, 160};
          if ( fread(&ambebuf[6], 1, 320, tx_wavefile) == 320 ) {
            for (int i=0; i < 160; i++) //swap byte order for all samples, AMBE3000 uses MSB first
              ((unsigned short *)(&ambebuf[6]))[i] = (((unsigned short *)(&ambebuf[6]))[i] >> 8) | (((unsigned short *)(&ambebuf[6]))[i] << 8);
            sendto(udp2, ambebuf, sizeof(ambebuf), 0, (const struct sockaddr *)&host2, sizeof(host2));
          } else {
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0;
            printf("*** TX END ***\n");

            //send terminator packet
            memset(buf, 0, 55);
            memcpy(buf, "DMRD", 4);
            buf[4] = ((tx_ambefcnt / 3) + 1) % 256;
            tx_srcid = ((dmrid>99999999)?dmrid/100:dmrid);
            buf[5] = (tx_srcid >> 16) & 0xff;
            buf[6] = (tx_srcid >> 8) & 0xff;
            buf[7] = (tx_srcid >> 0) & 0xff;
            tx_tgid = host1_tg;
            buf[8] = (host1_tg >> 16) & 0xff;
            buf[9] = (host1_tg >> 8) & 0xff;
            buf[10] = (host1_tg >> 0) & 0xff;
            buf[11] = (dmrid >> 24) & 0xff;
            buf[12] = (dmrid >> 16) & 0xff;
            buf[13] = (dmrid >> 8) & 0xff;
            buf[14] = (dmrid >> 0) & 0xff;
            buf[15] = 0x80 | (DMRMMDVM_FRAMETYPE_DATASYNC << 4) | MMDVM_SLOTTYPE_TERMINATOR;
            *(uint32_t *)(&buf[16]) = tx_streamid;
            generate_header();
            sendto(udp1, buf, 55, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG
            fprintf(stderr, "SEND DMR: ");
            for(int i = 0; i < 55; ++i)
              fprintf(stderr, "%02x ", buf[i]);
            fprintf(stderr, "\n");
#endif
          }
        }
        
    }
    
    if (time(NULL)-pong_time1 > TIMEOUT) {
      host1_connect_status = DISCONNECTED;
      fprintf(stderr, "DMR connection timed out, retrying connection...\n");
    }
  }
}
