/*
 * thd_cdev.h: thermal cooling class interface
 *
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * Author Name <Srinivas.Pandruvada@linux.intel.com>
 *
 */

#ifndef THD_CDEV_H
#define THD_CDEV_H

#include "thd_common.h"
#include "thd_sys_fs.h"
#include "thd_preference.h"
#include <time.h>

class cthd_cdev
{

protected:
	int index;

	int curr_state;
	int max_state;
	int min_state;
	csys_fs cdev_sysfs;
	unsigned int trip_point;
	std::string type_str;
	unsigned int sensor_mask;
	time_t last_op_time;
	int curr_pow;
	int base_pow_state;
	int inc_dec_val;
	bool auto_down_adjust;

private:
	unsigned int int_2_pow(int pow)
	{
		int i;
		int _pow = 1;
		for(i = 0; i < pow; ++i)
			_pow = _pow * 2;
		return _pow;
	}

public:
	cthd_cdev(unsigned int _index, std::string control_path): index(_index),
	cdev_sysfs(control_path.c_str()), trip_point(0), max_state(0), min_state(0),
	curr_state(0), sensor_mask(0), last_op_time(0), curr_pow(0), base_pow_state
	(0), inc_dec_val(1), auto_down_adjust(false){}

	virtual int thd_cdev_set_state(int set_point, int temperature, int state, int
	arg)
	{
		thd_log_debug(">>thd_cdev_set_state index:%d state:%d\n", index, state);
		curr_state = get_curr_state();
		if(curr_state < min_state)
			curr_state = min_state;
		max_state = get_max_state();
		thd_log_debug("thd_cdev_set_%d:curr state %d max state %d\n", index,
	curr_state, max_state);
		if(state)
		{
			if(curr_state < max_state)
			{
				int state = curr_state + inc_dec_val;
				time_t tm;
				time(&tm);
				if(inc_dec_val == 1 && (tm - last_op_time) < thd_poll_interval + 1)
				{
					if(curr_pow == 0)
						base_pow_state = curr_state;
					++curr_pow;
					state = base_pow_state + int_2_pow(curr_pow);
					thd_log_debug("consecutive call, increment exponentially state %d\n",
	state);
					if(state >= max_state)
					{
						state = max_state;
						curr_pow = 0;
						curr_state = max_state;
						last_op_time = tm;
						//						thd_log_debug("Reached Max State: so return\n");
						//						return THD_ERROR;
					}
				}
				else
					curr_pow = 0;
				last_op_time = tm;
				sensor_mask |= arg;
				if(state > max_state)
					state = max_state;
				thd_log_debug("op->device:%s %d\n", type_str.c_str(), state);
				set_curr_state(state, arg);
			}
		}
		else
		{
			curr_pow = 0;
			if(curr_state > 0 && auto_down_adjust == false)
			{
				int state = curr_state - inc_dec_val;
				sensor_mask &= ~arg;
				if(sensor_mask != 0)
				{
					thd_log_debug(
	"skip to reduce current state as this is controlled by two sensor actions and one is still on %x\n", sensor_mask);
				}
				else
				{
					if(state < min_state)
						state = min_state;
					thd_log_debug("op->device:%s %d\n", type_str.c_str(), state);
					set_curr_state(state, arg);
				}
			}
			else
			{
				set_curr_state(min_state, arg);
			}

		}
		thd_log_info("Set : %d, %d, %d, %d, %d\n", set_point, temperature, index,
	get_curr_state(), max_state);

		thd_log_debug("<<thd_cdev_set_state %d\n", state);

		return THD_SUCCESS;
	};

	virtual int thd_cdev_get_index()
	{
		return index;
	}

	virtual int init()
	{
		return 0;
	};
	virtual int control_begin()
	{
		return 0;
	};
	virtual int control_end()
	{
		return 0;
	};
	virtual void set_curr_state(int state, int arg){}

	virtual int get_curr_state()
	{
		return curr_state;
	}

	virtual int get_max_state()
	{
		return 0;
	};
	virtual int update()
	{
		return 0;
	};
	virtual void set_inc_dec_value(int value)
	{
		inc_dec_val = value;
	}
	virtual void set_down_adjust_control(bool value)
	{
		auto_down_adjust = value;
	}
	bool in_min_state()
	{
		if(get_curr_state() <= min_state)
			return true;
		return false;
	}

	bool in_max_state()
	{
		if(get_curr_state() >= get_max_state())
			return true;
		return false;
	}

};

#endif
