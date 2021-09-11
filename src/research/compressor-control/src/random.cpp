#include "random.h"

#include <random>



struct the_rgenerator
{
	the_rgenerator(int seed, float mean, float stddev)
		: device(seed), generator(seed)
	{
		generator.param(generator_type::param_type(mean, stddev));
	}


	~the_rgenerator()
	{}


	float operator()()
	{
		return generator(device);
	}


	typedef std::mt19937 device_type;
	typedef std::normal_distribution<float> generator_type;


	device_type device;
	generator_type generator;
};



rgenerator_t * random_generator_ctor(int seed, float mean, float stddev)
{
	auto * retval = new the_rgenerator(seed, mean, stddev);
	return (rgenerator_t*)retval;
}


void random_genetor_dtor(rgenerator_t * generator)
{
	the_rgenerator * self = reinterpret_cast<the_rgenerator*>(generator);
	delete self;
}


float random_generator_get(rgenerator_t * generator)
{
	the_rgenerator * self = reinterpret_cast<the_rgenerator*>(generator);
	return self->operator()();
}
