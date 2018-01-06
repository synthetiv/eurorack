/**
 *
 * Copyright (c) 2013-2017 Pascal Gauthier.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 */

#include <stdarg.h>
#include <bitset>

#include "dexed_audio_processor.h"

#include "Dexed.h"
#include "synth.h"
#include "freqlut.h"
#include "sin.h"
#include "exp2.h"
#include "env.h"
#include "pitchenv.h"
#include "aligned_buf.h"
#include "fm_op_kernel.h"

EngineMkI engineMkI;

//==============================================================================
DexedAudioProcessor::DexedAudioProcessor() {
#ifdef DEBUG
    Logger *tmp = Logger::getCurrentLogger();
    if ( tmp == NULL ) {
        Logger::setCurrentLogger(FileLogger::createDateStampedLogger("Dexed", "DebugSession-", "log", "DexedAudioProcessor Created"));
    }
    TRACE("Hi");
#endif

    curShape = -1;

    controllers.defaults();
    noteStartDelay_ = 0;
        
    TRACE("controler %s", controllers.opSwitch);
        
    memset(&voiceStatus, 0, sizeof(VoiceStatus));
}

DexedAudioProcessor::~DexedAudioProcessor() {
    TRACE("Bye");
}

const unsigned char pgm32[] = {
7, 64, 45, 99, 45, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 0, 0, 7, 
10, 64, 49, 99, 46, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 2, 0, 7, 
13, 64, 49, 99, 46, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 0, 0, 7, 
15, 64, 49, 99, 44, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 2, 0, 7, 
25, 64, 49, 99, 50, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 0, 0, 7, 
70, 40, 49, 99, 99, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 1, 0, 7, 
99, 99, 99, 99, 50, 50, 50, 50, 31, 7, 1, 35, 0, 0, 0, 1, 0, 3, 24, 83, 97, 
121, 32, 65, 103, 97, 105, 110, 46, 1, 1, 1, 1, 1, 1};

const unsigned char pgm33[] = {
72, 76, 10, 32, 99, 85, 80, 0, 49, 0, 0, 0, 0, 0, 0, 2, 84, 0, 10, 0, 7, 
76, 73, 10, 28, 99, 92, 90, 0, 50, 0, 0, 0, 0, 0, 0, 1, 90, 0, 2, 0, 3, 
49, 74, 10, 32, 98, 98, 98, 0, 41, 61, 64, 2, 2, 0, 0, 2, 71, 0, 2, 0, 9, 
55, 15, 10, 47, 99, 92, 92, 0, 56, 91, 0, 3, 0, 0, 0, 1, 99, 0, 2, 0, 14, 
55, 32, 32, 29, 93, 94, 90, 0, 68, 0, 99, 2, 0, 0, 0, 1, 88, 0, 2, 1, 0, 
55, 56, 10, 47, 99, 98, 64, 0, 39, 99, 0, 3, 3, 0, 0, 2, 98, 0, 2, 1, 11, 
98, 98, 98, 98, 53, 49, 50, 50, 14, 7, 1, 33, 35, 16, 0, 0, 0, 1, 12, 76, 65, 
85, 82, 73, 69, 32, 32, 32, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm34[] = {
99, 46, 54, 85, 99, 96, 99, 0, 57, 0, 0, 0, 0, 2, 0, 1, 99, 1, 2, 75, 7, 
99, 46, 34, 91, 99, 95, 99, 0, 0, 0, 0, 0, 0, 3, 0, 0, 78, 1, 3, 49, 14, 
88, 73, 53, 93, 99, 77, 0, 0, 0, 0, 0, 0, 0, 2, 0, 7, 91, 0, 8, 3, 14, 
88, 56, 60, 93, 99, 77, 0, 0, 0, 0, 0, 0, 0, 3, 0, 5, 79, 0, 1, 28, 10, 
88, 56, 60, 93, 99, 77, 0, 0, 0, 0, 0, 0, 0, 3, 0, 6, 83, 0, 1, 98, 4, 
78, 32, 44, 60, 99, 85, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 99, 0, 0, 7, 5, 
86, 93, 99, 99, 0, 52, 0, 93, 17, 7, 1, 35, 0, 0, 0, 1, 0, 3, 0, 66, 101, 
97, 116, 109, 101, 104, 114, 100, 114, 1, 1, 1, 1, 1, 1};

const unsigned char pgm35[] = {
99, 38, 25, 41, 99, 72, 72, 0, 39, 0, 1, 3, 3, 0, 0, 1, 82, 0, 2, 0, 14, 
99, 39, 25, 33, 99, 71, 64, 0, 39, 0, 0, 3, 3, 0, 0, 0, 52, 0, 2, 0, 14, 
99, 76, 99, 47, 99, 88, 96, 0, 39, 0, 0, 3, 3, 0, 0, 0, 92, 0, 1, 0, 14, 
99, 39, 25, 33, 99, 71, 64, 0, 39, 0, 1, 3, 3, 0, 0, 1, 99, 0, 2, 0, 7, 
99, 38, 25, 36, 99, 72, 72, 0, 39, 0, 0, 3, 3, 0, 0, 0, 78, 0, 4, 0, 7, 
99, 76, 99, 47, 99, 88, 96, 0, 39, 0, 0, 3, 3, 0, 0, 0, 0, 0, 1, 0, 7, 
84, 95, 95, 60, 50, 50, 50, 50, 31, 6, 1, 37, 0, 0, 0, 0, 0, 4, 24, 80, 72, 
65, 82, 79, 72, 32, 32, 32, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm36[] = {
99, 48, 1, 24, 94, 79, 0, 0, 60, 54, 0, 1, 0, 7, 0, 3, 99, 0, 15, 0, 8, 
95, 35, 8, 28, 99, 70, 0, 0, 52, 0, 35, 3, 0, 4, 0, 4, 79, 0, 3, 0, 11, 
95, 32, 25, 33, 99, 70, 0, 0, 0, 0, 0, 0, 0, 3, 3, 5, 99, 0, 1, 0, 3, 
85, 50, 2, 10, 99, 70, 0, 0, 58, 0, 37, 0, 0, 3, 0, 0, 79, 0, 9, 0, 1, 
75, 46, 16, 12, 99, 70, 0, 0, 52, 8, 0, 3, 0, 3, 0, 3, 79, 0, 2, 0, 5, 
74, 30, 27, 31, 99, 70, 0, 0, 60, 0, 0, 0, 0, 3, 3, 5, 99, 0, 1, 0, 12, 
94, 67, 95, 60, 50, 50, 50, 50, 2, 6, 1, 34, 33, 0, 0, 1, 4, 1, 12, 67, 104, 
114, 111, 109, 97, 32, 53, 32, 92, 1, 1, 1, 1, 1, 1};

const unsigned char pgm37[] = {
85, 62, 24, 45, 99, 99, 0, 0, 32, 0, 0, 3, 0, 4, 3, 0, 76, 0, 5, 81, 2, 
84, 58, 18, 36, 99, 93, 0, 0, 32, 0, 18, 0, 0, 3, 0, 7, 89, 0, 2, 0, 7, 
87, 45, 27, 45, 99, 99, 0, 0, 32, 99, 0, 3, 0, 2, 0, 4, 99, 0, 1, 0, 0, 
85, 62, 24, 46, 99, 99, 0, 0, 32, 0, 0, 3, 0, 4, 0, 0, 71, 0, 5, 1, 1, 
86, 58, 18, 36, 99, 93, 0, 0, 32, 0, 18, 0, 0, 3, 0, 5, 89, 0, 2, 1, 0, 
88, 45, 27, 45, 99, 99, 0, 0, 32, 99, 0, 3, 0, 2, 0, 4, 99, 0, 1, 1, 0, 
0, 0, 0, 0, 50, 50, 50, 50, 3, 0, 1, 25, 0, 10, 0, 0, 4, 0, 12, 76, 97, 
118, 105, 116, 97, 114, 32, 32, 92, 1, 1, 1, 1, 1, 1};

const unsigned char pgm38[] = {
0, 0, 16, 31, 99, 99, 99, 0, 0, 0, 4, 0, 0, 0, 3, 0, 80, 0, 1, 0, 10, 
70, 0, 16, 21, 91, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 78, 0, 1, 0, 10, 
46, 0, 15, 38, 92, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 1, 0, 10, 
0, 0, 0, 32, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 2, 0, 80, 0, 0, 99, 5, 
50, 0, 9, 18, 96, 99, 99, 0, 24, 0, 5, 0, 0, 0, 0, 0, 82, 0, 0, 99, 7, 
46, 0, 28, 38, 93, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 1, 0, 5, 0, 
99, 99, 99, 99, 50, 50, 50, 50, 3, 3, 1, 23, 0, 15, 0, 1, 0, 1, 12, 83, 108, 
111, 83, 119, 108, 32, 47, 47, 92, 1, 1, 1, 1, 1, 1};

const unsigned char pgm39[] = {
78, 95, 70, 11, 99, 96, 75, 0, 55, 0, 27, 0, 0, 1, 0, 0, 63, 0, 2, 0, 14, 
40, 30, 25, 36, 99, 90, 90, 0, 51, 0, 0, 0, 0, 1, 3, 0, 99, 0, 1, 0, 2, 
62, 95, 70, 12, 99, 96, 79, 0, 48, 0, 30, 0, 0, 1, 0, 0, 68, 0, 2, 0, 3, 
40, 30, 25, 36, 99, 90, 90, 0, 51, 0, 0, 0, 0, 1, 3, 5, 99, 0, 1, 1, 12, 
57, 95, 70, 11, 99, 96, 91, 0, 34, 0, 34, 0, 0, 1, 0, 0, 72, 0, 1, 0, 13, 
40, 30, 25, 36, 99, 90, 90, 0, 51, 0, 0, 0, 0, 1, 3, 4, 99, 0, 1, 0, 1, 
57, 83, 80, 84, 43, 51, 50, 50, 5, 5, 0, 28, 48, 34, 99, 0, 4, 2, 24, 79, 66, 
32, 71, 101, 110, 118, 105, 118, 92, 1, 1, 1, 1, 1, 1};

const unsigned char pgm40[] = {
99, 23, 11, 14, 99, 89, 92, 0, 15, 0, 0, 0, 1, 6, 0, 0, 53, 0, 26, 1, 10, 
66, 14, 11, 28, 94, 89, 90, 0, 15, 0, 0, 0, 1, 6, 0, 1, 72, 0, 3, 0, 10, 
88, 14, 67, 38, 99, 89, 71, 0, 15, 0, 0, 0, 1, 6, 0, 0, 88, 0, 23, 0, 5, 
88, 14, 71, 39, 99, 89, 88, 0, 15, 0, 0, 0, 1, 6, 0, 0, 97, 0, 1, 0, 7, 
98, 96, 27, 42, 99, 28, 82, 0, 15, 0, 14, 0, 1, 0, 0, 0, 96, 0, 2, 127, 14, 
99, 10, 7, 44, 99, 96, 89, 0, 15, 0, 14, 0, 1, 0, 0, 0, 99, 0, 0, 0, 14, 
99, 99, 99, 99, 39, 50, 53, 46, 14, 7, 1, 5, 0, 99, 0, 0, 2, 7, 0, 83, 65, 
87, 32, 69, 77, 32, 85, 80, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm41[] = {
51, 42, 35, 17, 99, 99, 99, 0, 30, 0, 6, 0, 0, 0, 3, 0, 44, 0, 1, 0, 12, 
51, 17, 34, 22, 99, 99, 99, 0, 30, 0, 17, 0, 0, 0, 2, 0, 99, 0, 1, 0, 12, 
42, 38, 20, 30, 99, 99, 99, 0, 32, 0, 0, 0, 0, 0, 0, 2, 99, 0, 1, 0, 12, 
53, 42, 37, 25, 99, 99, 99, 99, 30, 0, 3, 0, 0, 0, 3, 0, 62, 0, 0, 99, 8, 
53, 17, 34, 23, 99, 99, 99, 0, 30, 0, 13, 0, 0, 0, 2, 0, 98, 0, 0, 99, 8, 
42, 38, 20, 31, 99, 99, 99, 0, 32, 0, 0, 0, 0, 0, 0, 2, 99, 0, 0, 99, 8, 
99, 99, 83, 99, 47, 55, 50, 50, 3, 5, 1, 1, 0, 0, 85, 0, 4, 0, 12, 76, 70, 
79, 82, 101, 122, 32, 43, 43, 92, 1, 1, 1, 1, 1, 1};

const unsigned char pgm42[] = {
99, 24, 99, 40, 0, 0, 99, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 7, 
99, 32, 99, 40, 0, 0, 99, 0, 0, 0, 0, 0, 0, 1, 0, 0, 99, 0, 0, 51, 7, 
99, 35, 99, 40, 0, 0, 98, 0, 0, 0, 0, 0, 0, 1, 0, 0, 99, 0, 0, 0, 7, 
99, 39, 99, 40, 0, 0, 99, 0, 0, 0, 0, 0, 0, 1, 0, 0, 99, 0, 1, 0, 7, 
99, 45, 99, 40, 0, 0, 99, 0, 0, 0, 0, 0, 0, 1, 0, 0, 99, 0, 1, 26, 7, 
99, 35, 99, 40, 99, 71, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 99, 0, 1, 13, 7, 
99, 99, 99, 40, 50, 50, 50, 50, 31, 0, 1, 20, 79, 30, 2, 1, 3, 1, 24, 69, 78, 
67, 79, 85, 78, 84, 69, 82, 83, 1, 1, 1, 1, 1, 1};

const unsigned char pgm43[] = {
11, 15, 23, 56, 78, 75, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 1, 2, 37, 14, 
99, 99, 99, 55, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 94, 0, 0, 0, 14, 
11, 15, 23, 57, 78, 75, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 1, 2, 38, 3, 
99, 99, 99, 56, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 94, 0, 0, 0, 0, 
11, 15, 23, 54, 78, 75, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 1, 2, 38, 14, 
99, 99, 99, 53, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 94, 0, 0, 0, 7, 
99, 28, 99, 99, 50, 50, 50, 50, 4, 7, 1, 46, 0, 14, 45, 1, 2, 3, 24, 84, 104, 
117, 110, 100, 101, 114, 32, 32, 51, 1, 1, 1, 1, 1, 1};

const unsigned char pgm44[] = {
0, 85, 13, 43, 50, 99, 0, 0, 14, 0, 0, 0, 0, 6, 0, 0, 83, 0, 4, 0, 9, 
12, 87, 37, 43, 53, 99, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 99, 0, 2, 0, 2, 
25, 87, 34, 43, 58, 99, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 99, 0, 2, 0, 12, 
0, 87, 43, 43, 47, 99, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 95, 0, 2, 0, 7, 
0, 89, 60, 43, 65, 99, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 82, 0, 6, 0, 12, 
79, 21, 60, 43, 99, 80, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 99, 0, 2, 0, 7, 
0, 0, 0, 0, 50, 50, 50, 50, 21, 7, 1, 26, 20, 35, 0, 1, 0, 1, 12, 69, 99, 
104, 111, 69, 99, 104, 111, 32, 51, 1, 1, 1, 1, 1, 1};

const unsigned char pgm45[] = {
53, 7, 34, 44, 99, 84, 30, 0, 49, 2, 0, 3, 0, 0, 0, 1, 53, 0, 8, 0, 10, 
18, 11, 41, 38, 99, 59, 28, 0, 50, 0, 36, 3, 0, 0, 0, 2, 92, 0, 1, 0, 8, 
22, 8, 37, 43, 99, 87, 45, 0, 53, 17, 0, 3, 0, 1, 0, 2, 68, 0, 5, 0, 9, 
32, 17, 27, 43, 99, 74, 64, 0, 42, 0, 0, 0, 0, 0, 1, 1, 96, 0, 1, 0, 5, 
18, 18, 26, 28, 99, 71, 52, 0, 46, 4, 35, 3, 0, 2, 2, 1, 82, 0, 1, 0, 2, 
60, 6, 21, 37, 99, 94, 53, 0, 98, 0, 0, 3, 3, 0, 1, 2, 99, 0, 1, 0, 11, 
99, 99, 99, 99, 50, 50, 50, 50, 8, 7, 0, 42, 0, 0, 0, 1, 0, 3, 12, 45, 65, 
78, 65, 76, 79, 71, 32, 49, 45, 1, 1, 1, 1, 1, 1};

const unsigned char pgm46[] = {
36, 25, 49, 31, 99, 68, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 62, 0, 16, 75, 7, 
37, 22, 22, 50, 99, 22, 15, 0, 0, 0, 0, 0, 0, 0, 3, 3, 99, 0, 5, 0, 13, 
36, 36, 22, 44, 99, 42, 37, 0, 0, 0, 0, 0, 0, 0, 1, 0, 44, 0, 28, 0, 11, 
42, 16, 33, 41, 99, 32, 0, 0, 0, 0, 5, 0, 0, 0, 3, 0, 99, 0, 5, 0, 6, 
25, 24, 22, 27, 99, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 0, 15, 0, 11, 
35, 18, 22, 35, 99, 80, 43, 0, 0, 0, 0, 0, 0, 0, 3, 0, 99, 0, 5, 0, 0, 
99, 99, 99, 99, 50, 50, 50, 50, 4, 4, 1, 39, 0, 6, 0, 0, 4, 1, 19, 80, 46, 
73, 67, 69, 32, 50, 53, 46, 49, 1, 1, 1, 1, 1, 1};

const unsigned char pgm47[] = {
99, 60, 40, 49, 99, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 78, 1, 3, 46, 7, 
99, 60, 35, 49, 99, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 87, 1, 3, 46, 5, 
99, 60, 32, 49, 99, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 83, 1, 3, 16, 7, 
90, 7, 3, 7, 99, 94, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 56, 0, 3, 49, 7, 
99, 49, 38, 28, 99, 94, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 80, 0, 1, 127, 7, 
99, 27, 53, 43, 99, 91, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 1, 0, 7, 
99, 58, 45, 24, 50, 48, 92, 50, 2, 0, 1, 35, 0, 0, 0, 1, 0, 3, 24, 70, 76, 
69, 88, 73, 32, 32, 32, 32, 52, 1, 1, 1, 1, 1, 1};

const unsigned char pgm48[] = {
99, 85, 99, 46, 77, 92, 98, 0, 39, 53, 20, 0, 0, 7, 0, 0, 14, 0, 2, 0, 7, 
99, 90, 22, 55, 80, 93, 85, 0, 48, 99, 16, 2, 0, 7, 0, 0, 74, 0, 1, 0, 7, 
99, 42, 22, 70, 99, 97, 60, 0, 39, 92, 75, 1, 2, 7, 0, 0, 74, 0, 1, 0, 7, 
99, 83, 22, 67, 99, 92, 80, 0, 34, 9, 0, 0, 0, 7, 0, 0, 89, 0, 1, 0, 7, 
99, 68, 39, 99, 99, 99, 99, 97, 0, 0, 65, 0, 0, 7, 0, 0, 82, 0, 1, 0, 7, 
83, 39, 34, 61, 96, 99, 93, 0, 0, 0, 0, 0, 0, 7, 0, 0, 99, 0, 1, 0, 7, 
94, 67, 95, 60, 50, 50, 50, 50, 17, 7, 0, 51, 39, 0, 0, 1, 4, 3, 24, 65, 82, 
80, 32, 50, 54, 48, 48, 32, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm49[] = {
99, 46, 0, 28, 99, 93, 87, 0, 54, 0, 0, 0, 0, 1, 0, 0, 71, 0, 2, 0, 6, 
99, 46, 0, 28, 99, 93, 87, 0, 54, 0, 0, 0, 0, 1, 0, 0, 77, 0, 1, 0, 7, 
99, 46, 0, 28, 99, 93, 87, 0, 54, 0, 0, 0, 0, 1, 0, 1, 84, 0, 1, 0, 5, 
46, 33, 20, 35, 99, 92, 84, 0, 0, 0, 0, 0, 0, 2, 0, 3, 99, 1, 0, 0, 0, 
99, 99, 99, 25, 99, 99, 99, 0, 40, 20, 7, 0, 0, 0, 0, 0, 93, 0, 0, 0, 14, 
50, 99, 99, 30, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 2, 0, 14, 
99, 99, 99, 99, 50, 50, 50, 50, 1, 6, 0, 38, 33, 0, 0, 1, 4, 3, 24, 65, 114, 
112, 43, 66, 117, 122, 122, 82, 115, 1, 1, 1, 1, 1, 1};

const unsigned char pgm50[] = {
78, 95, 70, 29, 99, 96, 75, 0, 55, 0, 75, 0, 0, 1, 2, 0, 99, 0, 2, 0, 7, 
25, 25, 12, 61, 99, 99, 99, 0, 51, 0, 0, 0, 0, 1, 0, 0, 85, 0, 1, 0, 7, 
33, 95, 70, 29, 99, 99, 99, 0, 48, 0, 30, 0, 0, 1, 0, 0, 90, 0, 2, 0, 14, 
50, 25, 12, 70, 99, 90, 97, 0, 51, 0, 0, 0, 0, 1, 0, 5, 99, 0, 1, 1, 7, 
28, 95, 70, 29, 99, 96, 79, 0, 48, 0, 67, 0, 0, 1, 0, 1, 90, 0, 2, 0, 0, 
34, 25, 12, 33, 99, 99, 57, 0, 51, 0, 0, 0, 0, 1, 0, 2, 99, 0, 1, 0, 7, 
10, 5, 6, 6, 50, 50, 50, 50, 22, 5, 5, 37, 42, 38, 36, 0, 4, 1, 24, 84, 82, 
87, 32, 32, 32, 32, 32, 32, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm51[] = {
83, 13, 58, 17, 99, 90, 90, 0, 32, 1, 0, 0, 0, 1, 0, 0, 81, 0, 17, 0, 4, 
82, 12, 31, 16, 99, 88, 88, 0, 37, 1, 0, 0, 0, 1, 0, 0, 59, 0, 7, 0, 4, 
89, 99, 56, 36, 99, 99, 99, 0, 0, 0, 2, 0, 0, 1, 0, 0, 83, 0, 0, 0, 5, 
17, 31, 22, 33, 99, 99, 99, 0, 0, 0, 7, 0, 0, 1, 0, 0, 99, 0, 1, 0, 5, 
78, 14, 11, 25, 99, 82, 87, 47, 12, 0, 0, 0, 0, 1, 0, 0, 65, 0, 17, 0, 10, 
17, 32, 34, 30, 99, 99, 99, 0, 0, 0, 0, 0, 0, 1, 0, 0, 99, 0, 1, 0, 10, 
99, 99, 99, 66, 51, 50, 50, 50, 7, 5, 1, 99, 3, 30, 0, 0, 1, 1, 24, 77, 73, 
82, 73, 68, 79, 82, 32, 49, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm52[] = {
94, 44, 41, 46, 99, 63, 54, 0, 35, 66, 5, 2, 3, 1, 2, 0, 79, 0, 8, 0, 7, 
95, 33, 49, 35, 99, 51, 54, 0, 41, 0, 4, 0, 3, 3, 2, 4, 82, 0, 3, 0, 7, 
94, 23, 64, 53, 99, 51, 54, 0, 71, 48, 7, 0, 1, 2, 2, 4, 71, 0, 2, 0, 7, 
66, 22, 42, 39, 99, 51, 53, 0, 39, 39, 7, 0, 3, 5, 2, 3, 99, 0, 1, 1, 0, 
99, 21, 99, 28, 99, 0, 0, 0, 47, 30, 10, 0, 2, 6, 2, 4, 76, 0, 1, 1, 8, 
99, 21, 99, 28, 99, 0, 0, 0, 10, 30, 10, 0, 2, 6, 2, 5, 99, 0, 1, 1, 13, 
77, 91, 56, 60, 51, 50, 50, 50, 7, 5, 1, 64, 0, 0, 99, 1, 1, 3, 24, 68, 69, 
86, 73, 76, 32, 84, 65, 67, 75, 1, 1, 1, 1, 1, 1};

const unsigned char pgm53[] = {
11, 26, 5, 28, 99, 95, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 75, 1, 2, 35, 7, 
10, 25, 0, 42, 99, 94, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70, 0, 2, 99, 7, 
9, 25, 0, 42, 99, 94, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 79, 1, 1, 78, 7, 
12, 16, 17, 7, 99, 94, 0, 0, 0, 0, 0, 0, 0, 3, 3, 31, 86, 0, 0, 64, 7, 
17, 0, 8, 28, 99, 97, 86, 0, 0, 0, 0, 0, 0, 3, 0, 0, 80, 1, 1, 53, 7, 
26, 11, 26, 23, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 0, 69, 7, 
82, 0, 0, 14, 50, 52, 48, 50, 16, 6, 0, 35, 0, 0, 0, 1, 5, 3, 0, 82, 85, 
77, 66, 76, 69, 32, 32, 32, 49, 1, 1, 1, 1, 1, 1};

const unsigned char pgm54[] = {
99, 16, 14, 16, 50, 34, 99, 0, 39, 0, 0, 0, 0, 0, 0, 0, 68, 0, 16, 0, 4, 
7, 15, 17, 36, 99, 85, 58, 0, 39, 0, 0, 0, 0, 0, 0, 0, 99, 0, 8, 0, 4, 
6, 13, 11, 24, 54, 85, 99, 0, 39, 0, 0, 0, 0, 0, 0, 0, 91, 0, 2, 0, 7, 
24, 15, 17, 39, 99, 85, 32, 0, 39, 0, 0, 0, 0, 0, 0, 0, 99, 0, 2, 0, 7, 
8, 12, 15, 21, 52, 85, 99, 0, 39, 0, 0, 0, 0, 0, 0, 0, 86, 0, 6, 0, 9, 
33, 20, 26, 39, 99, 85, 53, 0, 39, 0, 0, 0, 0, 0, 0, 0, 99, 0, 3, 0, 9, 
99, 99, 99, 99, 50, 50, 50, 50, 5, 4, 1, 12, 12, 5, 0, 0, 4, 2, 12, 67, 65, 
83, 67, 65, 68, 69, 32, 50, 49, 1, 1, 1, 1, 1, 1};

const unsigned char pgm55[] = {
99, 99, 99, 99, 99, 99, 99, 0, 67, 99, 0, 0, 0, 0, 3, 0, 99, 1, 3, 16, 14, 
99, 99, 99, 99, 99, 99, 99, 0, 67, 99, 99, 0, 0, 0, 3, 0, 93, 0, 0, 32, 0, 
99, 99, 99, 68, 99, 99, 99, 0, 43, 99, 99, 0, 0, 0, 3, 0, 99, 1, 3, 12, 1, 
99, 99, 99, 99, 99, 99, 99, 0, 43, 99, 99, 0, 0, 0, 3, 0, 99, 0, 2, 32, 3, 
99, 99, 99, 99, 99, 99, 99, 0, 20, 0, 99, 0, 0, 0, 3, 0, 99, 1, 3, 8, 2, 
83, 99, 99, 99, 99, 99, 99, 0, 20, 0, 99, 0, 0, 0, 3, 0, 90, 0, 6, 77, 0, 
99, 99, 99, 99, 50, 50, 50, 50, 31, 1, 1, 0, 0, 0, 0, 1, 1, 3, 24, 67, 44, 
68, 44, 69, 98, 44, 70, 32, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm56[] = {
24, 88, 46, 37, 60, 99, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 99, 0, 9, 7, 7, 
28, 88, 48, 37, 60, 99, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 99, 0, 7, 9, 7, 
32, 88, 46, 37, 60, 99, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 99, 0, 5, 15, 7, 
36, 89, 46, 37, 60, 99, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 99, 0, 5, 0, 7, 
45, 88, 46, 37, 60, 99, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 99, 0, 2, 11, 7, 
99, 88, 47, 38, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 99, 0, 2, 0, 7, 
99, 99, 99, 99, 50, 50, 50, 50, 31, 0, 1, 99, 0, 0, 0, 1, 3, 3, 12, 84, 117, 
110, 100, 83, 116, 97, 116, 99, 94, 1, 1, 1, 1, 1, 1};

const unsigned char pgm57[] = {
34, 30, 18, 37, 99, 71, 24, 0, 38, 0, 65, 0, 3, 3, 0, 4, 57, 0, 13, 8, 2, 
23, 30, 26, 46, 99, 81, 53, 0, 50, 70, 0, 3, 3, 2, 1, 2, 44, 0, 2, 0, 12, 
30, 38, 51, 45, 99, 72, 28, 0, 43, 61, 20, 3, 0, 2, 1, 4, 57, 0, 1, 0, 3, 
45, 23, 36, 43, 99, 75, 53, 0, 0, 0, 0, 0, 0, 3, 2, 1, 99, 0, 1, 0, 6, 
99, 27, 60, 26, 73, 99, 0, 0, 0, 31, 9, 3, 0, 0, 1, 3, 99, 0, 1, 0, 3, 
99, 21, 37, 42, 99, 99, 91, 0, 0, 0, 0, 0, 0, 4, 1, 2, 99, 0, 1, 0, 12, 
99, 99, 99, 99, 50, 50, 50, 50, 6, 7, 1, 24, 57, 9, 25, 1, 0, 3, 12, 45, 87, 
79, 66, 66, 76, 69, 32, 49, 45, 1, 1, 1, 1, 1, 1};

const unsigned char pgm58[] = {
57, 32, 77, 30, 99, 99, 58, 0, 10, 0, 0, 0, 1, 0, 0, 2, 99, 0, 0, 0, 7, 
11, 30, 25, 28, 99, 99, 0, 0, 10, 0, 0, 0, 1, 0, 0, 2, 92, 0, 5, 0, 7, 
26, 71, 20, 39, 99, 99, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 93, 0, 2, 0, 7, 
10, 99, 20, 41, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 93, 0, 0, 1, 7, 
18, 99, 20, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 90, 0, 2, 0, 7, 
28, 99, 20, 35, 99, 99, 17, 0, 0, 0, 0, 0, 0, 0, 0, 1, 99, 0, 0, 0, 7, 
99, 98, 75, 60, 50, 50, 50, 50, 16, 3, 0, 35, 89, 0, 0, 0, 4, 0, 24, 70, 73, 
76, 84, 69, 82, 45, 83, 87, 80, 1, 1, 1, 1, 1, 1};

const unsigned char pgm59[] = {
20, 10, 28, 24, 94, 83, 64, 0, 42, 54, 0, 1, 0, 0, 0, 3, 64, 0, 3, 0, 7, 
31, 8, 23, 28, 99, 87, 66, 0, 44, 5, 14, 3, 0, 0, 0, 2, 92, 0, 1, 0, 7, 
74, 48, 12, 36, 81, 99, 68, 0, 47, 8, 13, 0, 0, 0, 0, 5, 99, 0, 1, 0, 7, 
24, 42, 13, 10, 99, 92, 66, 0, 45, 9, 12, 0, 0, 0, 0, 0, 61, 0, 2, 0, 7, 
30, 46, 8, 12, 99, 99, 67, 0, 45, 8, 12, 1, 0, 0, 0, 0, 84, 0, 1, 0, 7, 
76, 18, 4, 44, 84, 99, 70, 0, 44, 10, 13, 0, 0, 0, 0, 5, 99, 0, 1, 0, 14, 
94, 67, 95, 60, 50, 50, 50, 50, 2, 6, 1, 56, 68, 0, 63, 1, 3, 1, 24, 83, 65, 
72, 65, 82, 65, 32, 32, 32, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm60[] = {
99, 99, 20, 57, 99, 99, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 59, 0, 17, 99, 7, 
7, 99, 99, 25, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 0, 11, 53, 7, 
35, 99, 99, 29, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 5, 42, 7, 
23, 99, 99, 25, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 7, 71, 7, 
27, 22, 99, 42, 85, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 0, 17, 0, 7, 
40, 99, 99, 36, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 10, 6, 10, 
65, 99, 99, 99, 50, 50, 50, 50, 14, 6, 0, 35, 0, 0, 0, 1, 0, 3, 0, 69, 76, 
69, 67, 84, 82, 79, 78, 32, 49, 1, 1, 1, 1, 1, 1};

const unsigned char pgm61[] = {
99, 0, 0, 0, 99, 99, 99, 0, 15, 0, 0, 0, 1, 7, 0, 0, 76, 0, 1, 0, 0, 
75, 42, 71, 34, 99, 99, 99, 0, 15, 0, 0, 0, 1, 3, 0, 0, 99, 0, 1, 0, 14, 
99, 72, 31, 17, 0, 70, 0, 0, 15, 0, 0, 0, 1, 7, 0, 0, 99, 0, 8, 0, 7, 
99, 99, 36, 35, 99, 99, 0, 0, 15, 0, 0, 0, 1, 3, 0, 0, 99, 0, 1, 0, 7, 
99, 0, 0, 0, 99, 99, 99, 0, 15, 0, 0, 0, 1, 7, 0, 0, 85, 0, 0, 0, 12, 
52, 42, 71, 34, 99, 99, 99, 0, 15, 0, 0, 0, 1, 3, 0, 0, 99, 0, 1, 0, 12, 
99, 99, 99, 99, 50, 50, 50, 50, 23, 7, 1, 27, 40, 16, 0, 0, 0, 2, 24, 66, 65, 
78, 75, 83, 44, 32, 84, 46, 32, 1, 1, 1, 1, 1, 1};

const unsigned char pgm62[] = {
24, 99, 99, 25, 29, 27, 31, 99, 0, 0, 0, 0, 0, 0, 0, 0, 88, 0, 0, 0, 14, 
99, 99, 99, 26, 70, 53, 52, 99, 0, 0, 0, 0, 0, 0, 0, 0, 70, 1, 0, 0, 1, 
13, 99, 99, 23, 77, 77, 79, 0, 0, 0, 0, 0, 0, 0, 0, 0, 77, 0, 6, 0, 7, 
58, 99, 99, 37, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 0, 1, 0, 7, 
99, 99, 99, 23, 77, 77, 79, 99, 0, 0, 0, 0, 0, 0, 0, 0, 69, 0, 6, 24, 7, 
25, 99, 99, 37, 99, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 0, 2, 24, 3, 
99, 99, 99, 99, 50, 50, 50, 50, 8, 6, 1, 9, 3, 7, 0, 0, 0, 1, 12, 83, 108, 
111, 119, 51, 68, 32, 80, 97, 100, 1, 1, 1, 1, 1, 1};

const unsigned char pgm63[] = {
57, 32, 77, 30, 99, 99, 58, 0, 10, 0, 0, 0, 1, 0, 0, 1, 99, 1, 0, 25, 6, 
11, 30, 25, 28, 99, 99, 0, 0, 10, 0, 0, 0, 1, 0, 0, 1, 92, 0, 4, 0, 7, 
26, 71, 20, 39, 99, 99, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 93, 1, 25, 5, 9, 
8, 10, 20, 41, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 93, 0, 7, 0, 7, 
18, 99, 20, 36, 99, 99, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 90, 0, 2, 0, 8, 
28, 99, 20, 35, 99, 99, 17, 0, 0, 0, 0, 0, 0, 0, 1, 1, 99, 0, 6, 0, 7, 
98, 98, 75, 60, 50, 50, 50, 50, 16, 3, 0, 35, 99, 10, 8, 0, 0, 3, 12, 76, 70, 
79, 32, 83, 87, 69, 69, 80, 32, 1, 1, 1, 1, 1, 1};

const unsigned char *pgms[] = {
pgm32, pgm33, pgm34, pgm35, pgm36, pgm37, pgm38, pgm39, 
pgm40, pgm41, pgm42, pgm43, pgm44, pgm45, pgm46, pgm47, 
pgm48, pgm49, pgm50, pgm51, pgm52, pgm53, pgm54, pgm55, 
pgm56, pgm57, pgm58, pgm59, pgm60, pgm61, pgm62, pgm63
};

int32_t audiobuf[N];

void DexedAudioProcessor::set_shape(int i) {
    if (curShape == i) {
        return;
    }
    curShape = i;
    if (i >= NUM_DEXED_PATCHES) { i = NUM_DEXED_PATCHES-1; }
    data = &pgms[i][0];

    reset();
}

const char *DexedAudioProcessor::patchName() {
    return (char *) &data[118];
}

void DexedAudioProcessor::reset() {
    // keyup();
    for (int note = 0; note < MAX_ACTIVE_NOTES; ++note) {
        voices[note].keydown = false;
        voices[note].sustained = false;
    }

//    unpackOpSwitch(data[155]);

    extra_buf_size = 0;
        
    lfo.reset(data + 137);
}

//==============================================================================
void DexedAudioProcessor::init(int sampleRate, int samplesPerBlock) {
    Lfo::init(sampleRate>>1);
    PitchEnv::init(sampleRate>>1);
    Env::init_sr(sampleRate>>1);

    set_shape(0);
}

void DexedAudioProcessor::Render(const uint8_t* sync_buffer, int16_t* channelData, size_t origSamples) {
    unsigned int i;
    unsigned int numSamples = origSamples >> 1;

    if (pitch_ != voices[0].braids_pitch) {
        for(i=0;i < MAX_ACTIVE_NOTES;i++) {
            voices[i].braids_pitch = pitch_;
            if ( voices[i].live )
                voices[i].dx7_note.update(data, pitch_, voices[i].velocity, false);
        }
    }

    if (!gatestate_ && voices[0].keydown) {
        keyup();
        noteStartDelay_ = 50;
    }  
    if (noteStart_ && noteStartDelay_ == 0) {
        keydown();
        noteStart_ = false;
    }

    // todo apply params

    // flush first events
    for (i=0; i < numSamples && i < extra_buf_size; i++) {
        if (noteStartDelay_ > 0) {
            noteStartDelay_--;
        }
        channelData[(i<<1)] = extra_buf[i];
        channelData[(i<<1)+1] = extra_buf[i];
    }
    
    // remaining buffer is still to be processed
    if (extra_buf_size > numSamples) {
        for (unsigned int j = 0; j < extra_buf_size - numSamples; j++) {
            extra_buf[j] = extra_buf[j + numSamples];
        }
        extra_buf_size -= numSamples;
    } else {
        for (; i < numSamples; i += N) {
            
            for (int j = 0; j < N; ++j) {
                audiobuf[j] = 0;
            }
            int32_t lfovalue = lfo.getsample();
            int32_t lfodelay = lfo.getdelay();
            
            for (int note = 0; note < MAX_ACTIVE_NOTES; ++note) {
                if (voices[note].live) {
                    voices[note].dx7_note.compute(&audiobuf[0], lfovalue, lfodelay, &controllers);
                }
            }
            
            int jmax = numSamples - i;
            for (int j = 0; j < N; ++j) {
                int32_t value = audiobuf[j] >> 12;
                if (value > 32767) {
                    value = 32767;
                }
                if (value < -32768) {
                    value = -32768;
                }
                    
                if (j < jmax) {
                    if (noteStartDelay_ > 0) {
                        noteStartDelay_--;
                    }
                    channelData[(i + j) << 1] = value;
                    channelData[((i + j) << 1)+1] = value;
                } else {
                    extra_buf[j - jmax] = value;
                }
            }
        }
        extra_buf_size = i - numSamples;
    }
}

void DexedAudioProcessor::keydown() {
    int note = 0;
    for (int i=0; i<MAX_ACTIVE_NOTES; i++) {
        if (!voices[note].keydown) {
            lfo.keydown();
            voices[note].live = true;
            voices[note].velocity = 100;
            voices[note].sustained = false;
            voices[note].keydown = true;
            voices[note].braids_pitch = pitch_;
            voices[note].dx7_note.init(data, pitch_, 100);

            if ( data[136] )
                voices[note].dx7_note.oscSync();
            break;
        }
        note = (note + 1) % MAX_ACTIVE_NOTES;
    }
}

void DexedAudioProcessor::keyup() {
    int note;
    for (note=0; note<MAX_ACTIVE_NOTES; ++note) {
        if (voices[note].keydown ) {
            voices[note].keydown = false;
            break;
        }
    }
    
    // note not found ?
    if ( note >= MAX_ACTIVE_NOTES ) {
        TRACE("note-off not found???");
        return;
    }
    
    voices[note].dx7_note.keyup();
}

void DexedAudioProcessor::panic() {
    for(int i=0;i<MAX_ACTIVE_NOTES;i++) {
        voices[i].keydown = false;
        voices[i].live = false;
        voices[i].dx7_note.oscSync();
    }
}

void DexedAudioProcessor::unpackOpSwitch(char packOpValue) {
    controllers.opSwitch[5] = (packOpValue & 32) + 48;
    controllers.opSwitch[4] = (packOpValue & 16) + 48;
    controllers.opSwitch[3] = (packOpValue & 8) + 48;
    controllers.opSwitch[2] = (packOpValue & 4) + 48;
    controllers.opSwitch[1] = (packOpValue & 2) + 48;
    controllers.opSwitch[0] = (packOpValue & 1) + 48;
}
