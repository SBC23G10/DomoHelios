#ifndef __SOLAR_SYS__
#define __SOLAR_SYS__

#include <cmath>
#include <iostream>
#include <vector>
#include <ctime>
#include <unistd.h>
#include "vec.hpp"

template <typename T>
class Referenced_source
{
	public:
		Referenced_source(std::atomic<T>*);
		void set_src(std::atomic<T>*);
		T get_value();
		/*virtual */
		void set_value(T);
	private:
		std::atomic<T> *src;
};

template <typename T>
Referenced_source<T>::Referenced_source(std::atomic<T> *src):
	src(src)
{}
template <typename T>
void Referenced_source<T>::set_src(std::atomic<T> *src)
{
	this->src = src;
}
template <typename T>
T Referenced_source<T>::get_value()
{
	return src->load();
}
template <typename T>
void Referenced_source<T>::set_value(T value)
{
	this->src->store(value);
}

/* --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * */

template <typename T>
class Servo : public Referenced_source<T>
{
	public:
		Servo(std::atomic<T>*, std::atomic<T>*, const T, const T, const bool, const bool);
		void inc_offset(T);
		bool reverse_stat();
		std::string info();
	private:
		Referenced_source<T> target;
		const T axis_base;
		const T axis_top;
		const T axis_half;
		const T range;
		const bool _full_360;
		bool reverse;
		bool set_reverse;
};

template <typename T>
Servo<T>::Servo(std::atomic<T> *src, std::atomic<T> *target, const T axis_base, const T axis_top, const bool reverse, const bool set_reverse):
	Referenced_source<T>(src), target(target),
	axis_base(axis_base), axis_top(axis_top),
	axis_half((T)(axis_base + axis_top) / 2), 
	range(round(axis_top - axis_base)),
	_full_360((axis_top - axis_base) >= 359.9f),
	reverse(reverse),
	set_reverse(set_reverse)
{}

// Permite disparar de una unica medicion el angulo objetivo
// sin llamar por cada ciclo a inc_offset
template <typename T>
void Servo<T>::inc_offset(T inc)
{
	T val = this->get_value();
	T target;
	// TODO split into two inc_offset methods one for 360, one for the rest
	if(!_full_360) {
		/*if(inc < 0.00f)
			target = val - (val - axis_base) * -inc;
		else
			target = val + (axis_top - val) * inc;*/
		if(inc < 0.00f)
			target = val - axis_top * -inc;
		else
			target = val + axis_top * inc;
		if(target < 0.00f)
			target = 0.00f;
		else if(target > 180.00f)
			target = 179.99f;

		if(set_reverse && ((val < axis_half && target > axis_half)
				|| (val > axis_half && target < axis_half))) {
			reverse = !reverse;
			printf("Inversion logica: %s\n", reverse ? "true":"false");
		}
	} else {
		target = val + 180 * inc;
		if(target < 0.00f)
			target += 360;
		else if(target > 360.00f)
			target -= 360;

		if(target == 360.0f)
			target = 0;
	}

	this->target.set_value(target);
	printf("[Angulo objetivo actualizado de %.2f a %.2f]\n", val, target);
}

template <typename T>
bool Servo<T>::reverse_stat()
{
	return reverse;
}

template <typename T>
std::string Servo<T>::info()
{
	return std::string(
				"Servo-"
				+ std::to_string((int)range)
				+ ": "
				   "current = "
				+ std::to_string(this->get_value())
				+  " target = "
				+ std::to_string(target.get_value())
				+  " (base_axis = "
				+ std::to_string(axis_base)
				+ " top_axis = "
				+ std::to_string(axis_top)
				+ ")"
				);
}

/* --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * */

template <typename T>
class LDR : public Referenced_source<T>
{
	public:
		LDR(std::atomic<T>*, Vec3<T>);
		Vec3<T> get_loc();
		
		std::string info();

	private:
		Vec3<T> loc;
};

template <typename T>
LDR<T>::LDR(std::atomic<T> *src, Vec3<T> loc):
	Referenced_source<T>(src), loc(loc)
{}

template <typename T>
Vec3<T> LDR<T>::get_loc()
{
	return loc;
}

template <typename T>
std::string LDR<T>::info()
{
	return std::string(
				"LDR: "
				  "Value = "
				+ std::to_string(this->get_value())
				+ " lux "
				  "Loc = "
				+ Vec3<T>::to_string(loc)
				);
}

/* --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * */

template <typename T>
class Panel : public Referenced_source<T>
{
	public:
		Panel(std::atomic<T>*, T, T);
		T get_height();
		T get_width();
		
		std::string info();

	private:
		T width, height;
};

template <typename T>
Panel<T>::Panel(std::atomic<T> *src, T w, T h):
	Referenced_source<T>(src),
	width(w), height(h)
{}

template <typename T>
T Panel<T>::get_height()
{
	return height;
}
template <typename T>
T Panel<T>::get_width()
{
	return width;
}

template <typename T>
std::string Panel<T>::info()
{
	return std::string(
				"Panel: "
				  "Value = "
				+ std::to_string(this->get_value())
				+ " A "
				  "Width = "
				+ std::to_string(width)
				+ " Height = "
				+ std::to_string(height)
				);
}

/* --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * --------------------------------------------------------------------------
 * */


template <typename T>
class Virtual_Solar_sys
{
	public:
		Virtual_Solar_sys(std::vector<LDR<T>>, std::vector<Servo<T>>, Panel<T>, T);
		void operator()()
		{
			task();
		}
		std::string info();

	private:
		std::vector<LDR<T>> ldr;
		std::vector<Servo<T>> servo;
		Panel<T> panel;
		std::vector<T> x_i_factor, x_f_factor;
		std::vector<T> y_i_factor, y_f_factor;
		T x_i, x_f;
		T y_i, y_f;
		const T max_analog_value;

		void task();
		void initial_calc_imaginary_ldr_factor();
		void enhance_focus();
		bool ldr_balance_stat();
		void recalc_imaginary_ldr_value();
};

template <typename T>
Virtual_Solar_sys<T>::Virtual_Solar_sys(
		std::vector<LDR<T>> ldr,
		std::vector<Servo<T>> servo, Panel<T> panel, T max_analog_value):
	ldr(std::vector<LDR<T>>(ldr)),
	servo(std::vector<Servo<T>>(servo)),
	panel(panel),
	max_analog_value(max_analog_value)
{
	x_i_factor = std::vector<T>();
	x_f_factor = std::vector<T>();
	y_i_factor = std::vector<T>();
	y_f_factor = std::vector<T>();

	initial_calc_imaginary_ldr_factor();
}

template <typename T>
void Virtual_Solar_sys<T>::initial_calc_imaginary_ldr_factor()
{
	T x = panel.get_width(), y = panel.get_height();
	T x_h = (T) x / 2, y_h = (T) y / 2;
	T x_i_dist = 0.0f, x_f_dist = 0.0f;
	T y_i_dist = 0.0f, y_f_dist = 0.0f;
	Vec3<T> v;

	for(auto l : ldr) {
		v = l.get_loc();
		x_i_dist += Vec3<T>::get_mod(v, Vec3<T>(0, y_h, 0));
		x_f_dist += Vec3<T>::get_mod(v, Vec3<T>(x, y_h, 0));
		y_i_dist += Vec3<T>::get_mod(v, Vec3<T>(x_h, 0, 0));
		y_f_dist += Vec3<T>::get_mod(v, Vec3<T>(x_h, y, 0));
	}

	for(auto l : ldr) {
		v = l.get_loc();
		x_i_factor.push_back(1 - (T)Vec3<T>::get_mod(v, Vec3<T>(0, y_h, 0)) / x_i_dist);
		x_f_factor.push_back(1 - (T)Vec3<T>::get_mod(v, Vec3<T>(x, y_h, 0)) / x_f_dist);
		y_i_factor.push_back(1 - (T)Vec3<T>::get_mod(v, Vec3<T>(x_h, 0, 0)) / y_i_dist);
		y_f_factor.push_back(1 - (T)Vec3<T>::get_mod(v, Vec3<T>(x_h, y, 0)) / y_f_dist);
	}
}

template <typename T>
void Virtual_Solar_sys<T>::recalc_imaginary_ldr_value()
{
	uint32_t i = 0;
	T value;
	y_f = y_i = x_f = x_i = 0.0f;

	for(i = 0; i < ldr.size(); i++) {
		value = ldr.at(i).get_value();
		x_i += value * x_i_factor.at(i);
		x_f += value * x_f_factor.at(i);
		y_i += value * y_i_factor.at(i);
		y_f += value * y_f_factor.at(i);
	}
}

	template <typename T>
std::string Virtual_Solar_sys<T>::info()
{
	std::string info = "= + Virtual_Solar_sys + =\n";
	uint32_t i;

	info += "=========================\n\n";
	info += "[Panel]:\n\t" + panel.info() + "\n";

	i = 0;
	info += "[LDR]:\n";
	for(auto l : ldr)
		info += "\tEntry " + std::to_string(i++) + " : " + l.info() + "\n";
	
	T x = panel.get_width(), y = panel.get_height();
	T x_h = (T) x / 2, y_h = (T) y / 2;
	info += "[Imaginary LDR]:\n";

	info += ("\tEntry 0 : " "LDR_x_i: Value = " +
			std::to_string(x_i) + " lux " "Loc = ("
			+ std::to_string(0.0f) + "," + std::to_string(y_h) + ")\n");
	info += ("\tEntry 1 : " "LDR_x_f: Value = " +
			std::to_string(x_f) + " lux " "Loc = ("
			+ std::to_string(x) + "," + std::to_string(y_h) + ")\n");
	info += ("\tEntry 2 : " "LDR_y_i: Value = " +
			std::to_string(y_i) + " lux " "Loc = ("
			+ std::to_string(x_h) + "," + std::to_string(0.0f) + ")\n");
	info += ("\tEntry 3 : " "LDR_y_f: Value = " +
			std::to_string(y_f) + " lux " "Loc = ("
			+ std::to_string(x_h) + "," + std::to_string(y) + ")\n");
	
	i = 0;
	info += "[Servo]:\n";
	for(auto s : servo)
		info += "\tEntry " + std::to_string(i++) + " : " + s.info() + "\n";

	return info;
}

template <typename T>
void Virtual_Solar_sys<T>::enhance_focus()
{
	recalc_imaginary_ldr_value();
	
	T x_diff = x_i - x_f;
	T y_diff = y_i - y_f;

	if(fabs(x_diff) >= 175) {
		x_diff = (T)x_diff / (x_i + x_f);
		if(servo.at(1).reverse_stat())
			x_diff = -x_diff;
		printf("Referenced Servo 0 ::info:: [incremento del %.2f%c]", x_diff * 100, '%');
		servo.at(0).inc_offset(x_diff);
	}
	if(fabs(y_diff) >= 175) {
		y_diff = (T)y_diff / (y_i + y_f);
		printf("Referenced Servo 1 ::info:: [incremento del %.2f%c (amortiguado)]", y_diff * 100, '%');
		servo.at(1).inc_offset(y_diff);
	}
}

template <typename T>
bool Virtual_Solar_sys<T>::ldr_balance_stat()
{
	recalc_imaginary_ldr_value();
	return (fabs(x_i - x_f) < 175) && (fabs(y_i - y_f) < 175);
}

template <typename T>
void Virtual_Solar_sys<T>::task()
{
	int usecs = 100000;
	for(;;) {
		// TODO change this sim, implement thread_term call safe exit
		int i;
		for(i = 0; i < 10; i++)
			usleep(usecs);
		enhance_focus();
	}
}

#endif /* !__SOLAR_SYS__ */
