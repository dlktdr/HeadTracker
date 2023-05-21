/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2023 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// NRF52 Based CPU
#define CPU_NRF
#define CPU_NRF_52840

// Perepherial Channels Used, Make sure no dupilcates here
// and can't be used by Zephyr
// Cannot use GPIOTE interrupt as I override the interrupt handler in PPMIN

#define PPMOUT_PPICH 0
#define SERIALIN1_PPICH 1
#define SERIALIN2_PPICH 2
#define SERIALOUT_PPICH 3
#define PPMIN_PPICH1 4
#define PPMIN_PPICH2 5
// 6 Used
// 7 Enable always gets flipped off
// 8+ ??

#define SERIAL_UARTE_CH 1

// GPIOTE
// Known good GPIOTE 0,1,2,3,4,5,6,7
#define SERIALOUT0_GPIOTE 4
#define SERIALOUT1_GPIOTE 5
#define SERIALIN0_GPIOTE 0
#define SERIALIN1_GPIOTE 1
#define SERIALIN2_GPIOTE 2
#define PPMIN_GPIOTE 6
#define PPMOUT_GPIOTE 7

#define PPMOUT_TIMER_CH 3
#define PPMIN_TIMER_CH 4

#define PPMIN_TMRCOMP_CH 0
#define PPMOUT_TMRCOMP_CH 0