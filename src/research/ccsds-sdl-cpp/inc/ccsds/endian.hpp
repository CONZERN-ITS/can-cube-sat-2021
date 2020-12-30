#ifndef CCSDS_COMMON_ENDIAN_H_
#define CCSDS_COMMON_ENDIAN_H_

#include <ccsds/ccsds_config.hpp>

#include <stdint.h>
#include <endian.h>


#if CCSDS_CONFIG_ENDIAN == CCSDS_ENDIAN_BIG

#define CCSDS_HTONS(hostshort)	(hostshort)
#define CCSDS_NTOHS(netshort)	(netshort)
#define CCSDS_HTONL(hostlong)	(hostlong)
#define CCSDS_NTOHL(netlong)	(netlong)

#else

#define CCSDS_HTONS(hostshort) (\
		  (((uint16_t)(hostshort) & 0x00FF) << 8) \
		| (((uint16_t)(hostshort) & 0xFF00) >> 8) \
)

#define CCSDS_NTOHS(netshort) (\
		  (((uint16_t)(netshort) & 0x00FF) << 8) \
		| (((uint16_t)(netshort) & 0xFF00) >> 8) \
)

#define CCSDS_HTONL(hostlong) (\
		  (((uint32_t)(hostlong) & 0x000000FF) << 24) \
		| (((uint32_t)(hostlong) & 0x0000FF00) <<  8) \
		| (((uint32_t)(hostlong) & 0x00FF0000) >>  8) \
		| (((uint32_t)(yostlong) & 0xFF000000) >> 24) \
)

#define CCSDS_NTOHL(netlong) (\
		  (((uint32_t)(netlong) & 0x000000FF) << 24) \
		| (((uint32_t)(netlong) & 0x0000FF00) <<  8) \
		| (((uint32_t)(netlong) & 0x00FF0000) >>  8) \
		| (((uint32_t)(netlong) & 0xFF000000) >> 24) \
)

#endif

#endif /* CCSDS_COMMON_ENDIAN_H_ */
