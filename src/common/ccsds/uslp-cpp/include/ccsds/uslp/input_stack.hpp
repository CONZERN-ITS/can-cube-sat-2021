#ifndef CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_
#define CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_

#include <map>
#include <memory>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>

#include <ccsds/uslp/common/ids_io.hpp>


namespace ccsds { namespace uslp {


class input_stack_event
{
public:
	enum class kind
	{
		SDU_ARRIVED,
		SDU_REJECTED

	};
};


class input_stack
{
public:
	input_stack();

	template<typename T, typename... ARGS>
	T * create_map(gmapid_t mapid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_vchannel(gvcid_t gvcid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_mchannel(mcid_t mcid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_pchannel(std::string name, ARGS && ...args);

private:
	template<typename T>
	void register_events(T * sink);

	std::map<gmapid_t, std::unique_ptr<map_sink>> _maps;
	std::map<gvcid_t, std::unique_ptr<vchannel_sink>> _virtuals;
	std::map<mcid_t, std::unique_ptr<mchannel_sink>> _masters;
	std::unique_ptr<pchannel_sink> _pchannel;
};


template<typename T, typename... ARGS>
T * input_stack::create_map(gmapid_t mapid, ARGS && ...args)
{
	auto itt = _virtuals.find(mapid.gvcid());
	if (itt == _virtuals.end())
	{
		std::stringstream error;
		error << "unable to find virtual channel sink for map source " << mapid;
		throw std::logic_error(error.str());
	}

	std::unique_ptr<T> map(new T(
			std::move(mapid), std::forward<ARGS>(args)...
	));

	auto * retval = map.get();
	itt->second->add_map_sink(retval);
	_maps.emplace(mapid, std::move(map));
	return retval;
}


template<typename T, typename... ARGS>
T * input_stack::create_vchannel(gvcid_t gvcid, ARGS && ...args)
{
	auto itt = _masters.find(gvcid.mcid());
	if (itt == _masters.end())
	{
		std::stringstream error;
		error << "unable to find mchannel sink for vchannel " << gvcid;
		throw std::logic_error(error.str());
	}

	std::unique_ptr<T> vchannel(new T(
			std::move(gvcid), std::forward<ARGS>(args)...
	));

	auto * retval = vchannel.get();
	itt->second->add_vchannel_sink(retval);
	_virtuals.emplace(gvcid, std::move(vchannel));
	return retval;
}


template<typename T, typename... ARGS>
T * input_stack::create_mchannel(mcid_t mcid, ARGS && ...args)
{
	if (!_pchannel)
		throw std::logic_error("pchannel sink should be created before mchannel sinks in input stack");

	std::unique_ptr<T> mchannel(new T(
			std::move(mcid), std::forward<ARGS>(args)...
	));

	auto * retval = mchannel.get();
	_pchannel->add_mchannel_sink(retval);
	_masters.emplace(mcid, std::move(mchannel));
	return retval;
}


template<typename T, typename... ARGS>
T * input_stack::create_pchannel(std::string name, ARGS && ...args)
{
	if (_pchannel)
		throw std::logic_error("unable to create second pchannel sink in input stack");

	auto * retval = new T(std::move(name), std::forward<ARGS>(args)...);
	_pchannel.reset(retval);
	return retval;
}


}}


#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_ */
