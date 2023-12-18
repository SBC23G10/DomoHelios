#ifndef __MISC__
#define __MISC__

#include "vec.hpp"
#include "solar_sys.hpp"

Vec3<float> rand_vec3(int min, int max)
{
	min *= 1000;
	max *= 1000;
	return Vec3<float>(
			(float)(rand() % (max - min + 1) + min) / 1000,
			(float)(rand() % (max - min + 1) + min) / 1000,
			(float)(rand() % (max - min + 1) + min) / 1000
			);
}

void set_virtual_luminance(Vec3<float> source, float luminosity,
		float tipical_dist,
		std::vector<LDR<float>>& ldr)
{
	float dispersion_factor = 0.001;

	for(auto l : ldr) {
		l.set_value(
				(1.0f - dispersion_factor)
				* luminosity 
				* (float) tipical_dist / Vec3<float>::get_mod(l.get_loc(), source));
	}
}

#endif /* !__MISC__ */
