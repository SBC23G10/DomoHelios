#ifndef __VEC__
#define __VEC__

#include <cmath>
#include <iostream>

template <typename T>
class Vec3
{
	public:
		Vec3();
		Vec3(const T, const T, const T);

		Vec3 operator+(T const&);
		Vec3 operator*(T const&);

		Vec3 operator+(Vec3<T>const&);
		Vec3 operator-(Vec3<T>const&);
		Vec3 operator*(Vec3<T>const&);
		Vec3 operator/(Vec3<T>const&);

		static T get_mod(Vec3<T> const&);
		static T get_mod(Vec3<T> const&, Vec3<T> const&);
		static void normalize(Vec3<T>&);
		static void set_mod(Vec3<T>&, const T);
		static std::string to_string(Vec3<T>const&);
		static T get_scalar(Vec3<T>const&, char);

	private:
		T x, y, z;
};

template <typename T>
Vec3<T>::Vec3() :
	x(0), y(0), z(0)
{
}

template <typename T>
Vec3<T>::Vec3(T x, T y, T z) :
	x(x), y(y), z(z)
{
}

template <typename T>
Vec3<T> Vec3<T>::operator+(T const &v)
{
	return Vec3(this->x + v, this->y + v, this->z + v);
}

template <typename T>
Vec3<T> Vec3<T>::operator*(T const &v)
{
	return Vec3(this->x * v, this->y * v, this->z * v);
}

template <typename T>
Vec3<T> Vec3<T>::operator+(Vec3<T>const &v)
{
	return Vec3(this->x + v.x, this->y + v.y, this->z + v.z);
}

template <typename T>
Vec3<T> Vec3<T>::operator-(Vec3<T>const &v)
{
	return Vec3(this->x - v.x, this->y - v.y, this->z - v.z);
}

template <typename T>
Vec3<T> Vec3<T>::operator*(Vec3<T>const &v)
{
	return Vec3(this->x * v.x, this->y * v.y, this->z * v.z);
}

template <typename T>
Vec3<T> Vec3<T>::operator/(Vec3<T>const &v)
{
	return Vec3((T)this->x / v.x, (T)this->y / v.y, (T)this->z / v.z);
}

template <typename T>
T Vec3<T>::get_mod(Vec3<T> const &v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

template <typename T>
T Vec3<T>::get_mod(Vec3<T> const &v, Vec3<T> const &w)
{
	Vec3<T> t = v;
	return get_mod(t - w);
}

template <typename T>
void Vec3<T>::normalize(Vec3<T> &v)
{
	v = v * ((T) 1 / get_mod(v));
}

template <typename T>
void Vec3<T>::set_mod(Vec3<T> &v, const T mod)
{
	v = v * ((T) mod / get_mod(v));
}

template <typename T>
std::string Vec3<T>::to_string(Vec3<T> const &v)
{
	return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + ")";
}

template <typename T>
T Vec3<T>::get_scalar(Vec3<T> const& v, char sel)
{
	if(sel == 0)
		return v.x;
	else if(sel == 1)
		return v.y;
	return v.z;
}

#endif /* !__VEC__ */
