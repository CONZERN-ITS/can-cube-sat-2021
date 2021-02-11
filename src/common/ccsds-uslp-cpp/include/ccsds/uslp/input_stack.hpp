#ifndef CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_
#define CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_

#include <map>
#include <memory>
#include <algorithm>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>

#include <ccsds/uslp/common/ids_io.hpp>


namespace ccsds { namespace uslp {


class input_stack_event_handler
{
public:
	input_stack_event_handler() = default;
	virtual ~input_stack_event_handler() = default;

	virtual void on_map_sdu_event(gmapid_t mapid, const event_accepted_map_sdu & event) = 0;
};


class input_stack
{
public:
	input_stack();

	void set_event_handler(input_stack_event_handler * event_handler);

	void push_frame(const uint8_t * frame_data, size_t frame_data_size);

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
	void _register_events(T * sink);

	void _event_callback(const gmapid_t & mapid, const event & evt);
	void _event_callback(const gvcid_t & gvcid, const event & evt);
	void _event_callback(const mcid_t & mcid, const event & evt);
	void _event_callback(const std::string & pchannel_id, const event & evt);

	input_stack_event_handler * _event_handler;

	std::map<gmapid_t, std::unique_ptr<map_acceptor>> _maps;
	std::map<gvcid_t, std::unique_ptr<vchannel_acceptor>> _virtuals;
	std::map<mcid_t, std::unique_ptr<mchannel_acceptor>> _masters;
	std::unique_ptr<pchannel_acceptor> _pchannel;
};


template<typename T>
//typename std::enable_if<std::is_base_of<map_sink, T>::value, void>::type
// Просто чертова шаблонная магия. Удалось обойтись без enable_if!
void input_stack::_register_events(T * sink)
{
	auto channel_id = sink->channel_id;
	auto callback = [channel_id, this](const event & evt) { this->_event_callback(channel_id, evt); };
	sink->set_event_callback(std::move(callback));
}


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
	itt->second->add_map_accceptor(retval);
	_maps.emplace(mapid, std::move(map));
	_register_events(retval);
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
	itt->second->add_vchannel_acceptor(retval);
	_virtuals.emplace(gvcid, std::move(vchannel));
	_register_events(retval);
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
	_pchannel->add_mchannel_acceptor(retval);
	_masters.emplace(mcid, std::move(mchannel));
	_register_events(retval);
	return retval;
}


template<typename T, typename... ARGS>
T * input_stack::create_pchannel(std::string name, ARGS && ...args)
{
	if (_pchannel)
		throw std::logic_error("unable to create second pchannel sink in input stack");

	auto * retval = new T(std::move(name), std::forward<ARGS>(args)...);
	_pchannel.reset(retval);
	_register_events(retval);
	return retval;
}


}}


#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_ */
