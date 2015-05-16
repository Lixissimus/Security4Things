#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* save program memory */
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC      nullrdc_driver
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC      nullmac_driver
#undef UIP_CONF_TCP
#define UIP_CONF_TCP           0

#endif /* PROJECT_CONF_H_ */