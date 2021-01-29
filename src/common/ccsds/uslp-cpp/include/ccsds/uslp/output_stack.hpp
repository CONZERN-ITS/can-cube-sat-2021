#ifndef CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_OUTPUT_STACK_HPP_
#define CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_OUTPUT_STACK_HPP_

#include <map>
#include <memory>
#include <cassert>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>

#include <ccsds/uslp/common/ids_io.hpp>


namespace ccsds { namespace uslp {


class output_stack
{
public:
	output_stack();

	bool peek_frame();
	bool peek_frame(pchannel_frame_params_t & frame_params);
	void pop_frame(uint8_t * frame_buffer, size_t frame_buffer_size);

	template<typename T, typename... ARGS>
	T * create_map(gmapid_t mapid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_vchannel(gvcid_t gvcid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_mchannel(mcid_t mcid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_pchannel(std::string name, ARGS && ...args);

private:
	std::map<gmapid_t, std::unique_ptr<map_source>> _maps;
	std::map<gvcid_t, std::unique_ptr<vchannel_source>> _virtuals;
	std::map<mcid_t, std::unique_ptr<mchannel_source>> _masters;
	std::unique_ptr<pchannel_source> _pchannel;
};


template<typename T, typename... ARGS>
T * output_stack::create_map(gmapid_t mapid, ARGS && ...args)
{
	auto itt = _virtuals.find(mapid.gvcid());
	if (itt == _virtuals.end())
	{
		std::stringstream error;
		error << "unable to find virtual channel source for map source " << mapid;
		throw std::logic_error(error.str());
	}

	std::unique_ptr<T> map(new T(
			std::move(mapid), std::forward<ARGS>(args)...
	));

	auto * retval = map.get();
	itt->second->add_map_source(retval);
	_maps.emplace(mapid, std::move(map));
	return retval;
}


template<typename T, typename... ARGS>
T * output_stack::create_vchannel(gvcid_t gvcid, ARGS && ...args)
{
	auto itt = _masters.find(gvcid.mcid());
	if (itt == _masters.end())
	{
		std::stringstream error;
		error << "unable to find mchannel source for vchannel " << gvcid;
		throw std::logic_error(error.str());
	}

	std::unique_ptr<T> vchannel(new T(
			std::move(gvcid), std::forward<ARGS>(args)...
	));

	auto * retval = vchannel.get();
	itt->second->add_vchannel_source(retval);
	_virtuals.emplace(gvcid, std::move(vchannel));
	return retval;
}


template<typename T, typename... ARGS>
T * output_stack::create_mchannel(mcid_t mcid, ARGS && ...args)
{
	if (!_pchannel)
		throw std::logic_error("pchannel source should be created before mchannel sources in output stack");

	std::unique_ptr<T> mchannel(new T(
			std::move(mcid), std::forward<ARGS>(args)...
	));

	auto * retval = mchannel.get();
	_pchannel->add_mchannel_source(retval);
	_masters.emplace(mcid, std::move(mchannel));
	return retval;
}


template<typename T, typename... ARGS>
T * output_stack::create_pchannel(std::string name, ARGS && ...args)
{
	if (_pchannel)
		throw std::logic_error("unable to create second pchannel in output stack");

	auto * retval = new T(std::move(name), std::forward<ARGS>(args)...);
	_pchannel.reset(retval);
	return retval;
}


}}

#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_OUTPUT_STACK_HPP_ */
