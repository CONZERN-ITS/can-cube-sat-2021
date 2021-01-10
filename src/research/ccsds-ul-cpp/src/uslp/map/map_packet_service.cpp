#include <ccsds/uslp/map/map_packet_service.hpp>

#include <cassert>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/map/detail/tfdf_header.hpp>
#include <ccsds/uslp/map/detail/epp_header.hpp>


namespace ccsds { namespace uslp {

map_packet_service::map_packet_service(gmap_id_t map_id_)
	: map_service(map_id_), _chunk_reader(_data_queue)
{
	// С порога ставим себе размер зоны, чтобы в нее хоть заголовок влез
	tfdf_size(detail::tfdf_header::full_size);
}


void map_packet_service::tfdf_size(uint16_t value)
{
	// Проверка на то, что мы можем столько
	if (value < detail::tfdf_header::full_size)
		throw einval_exception("map_packet service requires no less than 4 bytes for tfdf zone size");

	this->map_service::tfdf_size(value);
}


bool map_packet_service::peek_tfdf()
{
	// Тут оптимизации не будет :c
	tfdf_params dummy;
	return peek_tfdf(dummy);
}


bool map_packet_service::peek_tfdf(tfdf_params & params)
{
	// Смотрим какой объем нам нужно занять
	const size_t tfdz_size = _tfdz_size();

	// Первый вариант - нам вообще нечего отправить так-то
	if (_data_queue.empty())
		return false;

	// Окей, хотябы один пакет у нас есть
	const auto & front_elem = _data_queue.front();

	// Второй вариант - у нас есть сколько-то там данных, которых точно хватит чтобы набить ими фрейм
	size_t readable;
	std::tie(std::ignore, readable) = _chunk_reader.peek_all();
	if (readable >= tfdz_size)
	{
		// Блин, а вот тут хз. Мы заблокируем канал?
		// Вообще говоря да. Только если не ожидается насильного флаша после этого пакета
		params.channel_lock
	}

	// Второй вариант - У нас есть очередной кусок пакета, который больше чем фрейм
	// И поэтому мы его отправим

	std::tie(std::ignore, packet_size) = _chunk_reader.peek_chunk();
	if (packet_size >= tfdz_size)
	{
		// Полюбому отправляем
		params.channel_lock = packet_size > tfdz_size; // лочим канал только если пакет больше
		params.qos = front_elem.qos;
		return true;
	}

	// Третий вариант - у нас есть очередной кусок пакета, но он меньше  чем фрейм
	// Тут мы либо ждем пока придет следующий кусок
	// Либо отправляем прямо сейчас. Отправлять прямо сейчас мы можем либо по флагу, который пришел с пакетом
	// Либо, если у следующего пакета другой QOS. Тут полюбому придется отправлять
	if (
		front_elem.flush_immidiately
		||
		(_data_queue.size() > 1 && _data_queue[1].qos != _data_queue[0].qos)
	)
	{
		params.channel_lock = false;
		params.qos = front_elem.qos;
		return true;
	}

	// Имхо тут стоит добавить таймаут на ожидание и отправлять очередной кусок пакета
	// даже если его не хватает чтобы набить фрейм
	return false;
}


void map_packet_service::pop_tfdf(uint8_t * tfdf_buffer)
{

}


uint16_t map_packet_service::_tfdz_size() const
{
	return map_service::tfdf_size() - detail::tfdf_header::full_size; // У нас всегда полный заголовок
}


bool map_packet_service::_will_lock_channel() const
{
	if (_data_queue.empty())
		return false; // Нет данных - нет блокировки

	// Если остаток от текущего пакета больше чем влезает в фрейм - тогда мы точно заблокируемся
	if (_chunk_reader.peek_chunk() > _tfdz_size())
		return true;

	// Если остаток меньше
}




}


}}
