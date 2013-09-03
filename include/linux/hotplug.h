/*
 * Copyright (c) 2013, Francisco Franco <franciscofranco.1990@gmail.com>.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_HOTPLUG_H
#define _LINUX_HOTPLUG_H

extern void update_first_level(unsigned int level);
extern void update_suspend_frequency(unsigned int freq);

extern unsigned int get_first_level(void);
extern unsigned int get_suspend_frequency(void);

extern unsigned int report_load_at_max_freq(int cpu);

#endif
