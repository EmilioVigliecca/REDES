/**********************************************************************
 * file:  sr_router.c
 *
 * Descripción:
 *
 * Este archivo contiene todas las funciones que interactúan directamente
 * con la tabla de enrutamiento, así como el método de entrada principal
 * para el enrutamiento.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Inicializa el subsistema de enrutamiento
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    assert(sr);

    /* Inicializa la caché y el hilo de limpieza de la caché */
    sr_arpcache_init(&(sr->cache));

    /* Inicializa los atributos del hilo */
    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    /* Hilo para gestionar el timeout del caché ARP */
    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);

} /* -- sr_init -- */

/* Envía un paquete ICMP de error */
void sr_send_icmp_error_packet(uint8_t type,
                              uint8_t code,
                              struct sr_instance *sr,
                              uint32_t ipDst,
                              uint8_t *ipPacket)
{

  /* COLOQUE AQUÍ SU CÓDIGO*/

} /* -- sr_send_icmp_error_packet -- */

void sr_handle_ip_packet(struct sr_instance *sr,
        uint8_t *packet /* lent */,
        unsigned int len,
        uint8_t *srcAddr,
        uint8_t *destAddr,
        char *interface /* lent */,
        sr_ethernet_hdr_t *eHdr) {

  /* 
  * COLOQUE ASÍ SU CÓDIGO
  * SUGERENCIAS: 
  * - Obtener el cabezal IP y direcciones 
  * - Verificar si el paquete es para una de mis interfaces o si hay una coincidencia en mi tabla de enrutamiento 
  * - Si no es para una de mis interfaces y no hay coincidencia en la tabla de enrutamiento, enviar ICMP net unreachable
  * - Sino, si es para mí, verificar si es un paquete ICMP echo request y responder con un echo reply 
  * - Sino, verificar TTL, ARP y reenviar si corresponde (puede necesitar una solicitud ARP y esperar la respuesta)
  * - No olvide imprimir los mensajes de depuración
  */

      //Cabezal:
      unsigned int eth_hdr_len = sizeof(sr_ethernet_hdr_t);
      sr_ip_hdr_t *ip_hdr = (sr_ip_hdr_t *)(packet + eth_hdr_len);

      unsigned int ip_hdr_len = ip_hdr->ip_hl * 4;
      unsigned int ip_pkt_len = ntohs(ip_hdr->ip_len);
      unsigned int total_pkt_len = eth_hdr_len + ip_pkt_len;
      
      // Verificar si el paquete es para una de mis interfaces
      struct sr_if *coincide = sr_get_interface_given_ip(sr, ip_hdr->ip_dst);
      if(coincide){
        printf("Paquete IP destinado al router: %s.\n", coincide->name);
        //verificar si es un paquete ICMP 
        if (ip_hdr->ip_p == ip_protocol_icmp){
          
            // Obtener encabezado ICMP 
            sr_icmp_hdr_t *icmp_hdr = (sr_icmp_hdr_t *)((uint8_t *)ip_hdr + ip_hdr_len);
            unsigned int icmp_data_len = ip_pkt_len - ip_hdr_len;

            //verificar si es un echo request
            if (icmp_hdr->icmp_type == 8 && icmp_hdr->icmp_code == 0) {
              printf("Se recibió un ICMP Echo Request.\n");
              
              //responder con un echo reply
              
              //Armar el paquete
              uint8_t *pkt_reply = (uint8_t *)malloc(total_pkt_len);
              memcpy(pkt_reply, packet, total_pkt_len);
              
              sr_ethernet_hdr_t *eth_reply_hdr = (sr_ethernet_hdr_t *)pkt_reply;
              sr_ip_hdr_t *ip_reply_hdr = (sr_ip_hdr_t *)(pkt_reply + eth_hdr_len);
              sr_icmp_hdr_t *icmp_reply_hdr = (sr_icmp_hdr_t *)((uint8_t *)ip_reply_hdr + ip_hdr_len);
              
              // Encabezado Ethernet (Invertir MACs)
              memcpy(eth_reply_hdr->ether_dhost, eHdr->ether_shost, ETHER_ADDR_LEN);
              memcpy(eth_reply_hdr->ether_shost, coincide->addr, ETHER_ADDR_LEN);
              
              // Encabezado IP (Invertir IPs, TTL y Checksum)
              uint32_t temp_ip = ip_reply_hdr->ip_src;
              ip_reply_hdr->ip_src = ip_reply_hdr->ip_dst;
              ip_reply_hdr->ip_dst = temp_ip;


              ip_reply_hdr->ip_ttl = 64; // TTL default, no sé si elegir este
              ip_reply_hdr->ip_sum = 0; 
              ip_reply_hdr->ip_sum = ip_cksum(ip_reply_hdr, ip_hdr_len);

              // Encabezado ICMP (Cambiar Tipo/Código y Checksum)
              icmp_reply_hdr->icmp_type = 0; // Tipo 0 (Reply)
              icmp_reply_hdr->icmp_code = 0;
                
              icmp_reply_hdr->icmp_sum = 0; 
              icmp_reply_hdr->icmp_sum = icmp_cksum(icmp_reply_hdr, icmp_data_len);
              
              // Enviar
              print_hdrs(pkt_reply, total_pkt_len);
              sr_send_packet(sr, pkt_reply, total_pkt_len, interface);
              free(pkt_reply);
            } else {
              //Otros tipos de ICMP
              printf("Paquete ICMP recibido, pero no un Echo Request. Descartar.\n");
            }
        
        }  else{
        }
      }
}

/* Gestiona la llegada de un paquete ARP*/
void sr_handle_arp_packet(struct sr_instance *sr,
        uint8_t *packet /* lent */,
        unsigned int len,
        uint8_t *srcAddr,
        uint8_t *destAddr,
        char *interface /* lent */,
        sr_ethernet_hdr_t *eHdr) {

  /* Imprimo el cabezal ARP */
  printf("*** -> It is an ARP packet. Print ARP header.\n");
  print_hdr_arp(packet + sizeof(sr_ethernet_hdr_t));

  /* COLOQUE SU CÓDIGO AQUÍ
  
  SUGERENCIAS:
  - Verifique si se trata de un ARP request o ARP reply
  */
    //Obtener el encabezado ARP
    sr_arp_hdr_t *arp_hdr = (sr_arp_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t));
  
    //Extraer y convertir el código de operación (si es request o reply)
    unsigned short op_code = ntohs(arp_hdr->ar_op);

    if (op_code == arp_op_request) {
        printf("Es un ARP Request.\n");
        //- Si es una ARP request, antes de responder verifique si el mensaje consulta 
        //por la dirección MAC asociada a una dirección IP configurada en una interfaz 
        //del propio router
        
        struct sr_if *es_igual = sr_get_interface_given_ip(sr, arp_hdr->ar_tip);

        if(es_igual){
          //La IP pertenece a este router
          //Hay que construir y enviar un ARP Reply
          unsigned int tam_reply = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
          uint8_t *pkt_reply = (uint8_t *)malloc(tam_reply); 

          //Punteros a encabezados del paquete nuevo
          sr_ethernet_hdr_t *eth_reply_hdr = (sr_ethernet_hdr_t *)pkt_reply;
          sr_arp_hdr_t *arp_reply_hdr = (sr_arp_hdr_t *)(pkt_reply + sizeof(sr_ethernet_hdr_t));

          //Encabezado ETHERNET:
          //La MAC de destino es la MAC del host que envió el request
          memcpy(eth_reply_hdr->ether_dhost, arp_hdr->ar_sha, ETHER_ADDR_LEN);
          //La MAC de origen es la MAC de esta interfaz de router
          memcpy(eth_reply_hdr->ether_shost, es_igual->addr, ETHER_ADDR_LEN);
          //El tipo de ARP es un reply
          eth_reply_hdr->ether_type = htons(ethertype_arp);
          
          //Copiar la estructura del header
          arp_reply_hdr->ar_hrd = arp_hdr->ar_hrd; 
          arp_reply_hdr->ar_pro = arp_hdr->ar_pro; 
          arp_reply_hdr->ar_hln = arp_hdr->ar_hln; 
          arp_reply_hdr->ar_pln = arp_hdr->ar_pln;

          arp_reply_hdr->ar_op = htons(arp_op_reply);

          //Hacer lo mismo que antes pero con el arp_reply_hdr
          //pero agregando las direcciones IP
          memcpy(arp_reply_hdr->ar_sha, es_igual->addr, ETHER_ADDR_LEN);
          arp_reply_hdr->ar_sip = es_igual->ip;
          
          memcpy(arp_reply_hdr->ar_tha, arp_hdr->ar_sha, ETHER_ADDR_LEN);
          arp_reply_hdr->ar_tip = arp_hdr->ar_sip;

          //Se envía el paquete
          print_hdrs(pkt_reply, tam_reply); //debbuging
          sr_send_packet(sr, pkt_reply, tam_reply, interface);

          free(pkt_reply);

        } else{
          printf("Request no destinado a este router. Se descarta. \n");
        }
    } 
    else if (op_code == arp_op_reply) {
        printf("Es un ARP Reply.\n");
        //- Si es una ARP reply, agregue el mapeo MAC->IP del emisor a la caché ARP y 
        //envíe los paquetes que hayan estado esperando por el ARP reply   
        // Lógica para insertar en la caché ARP y reenviar paquetes pendientes
        
        
        /* Este método (que dan ellos) hace dos cosas:
        1) Busca si existe alguna cola de paquetes IP esperando por esta MAC,
        si no hay devuelve null.
        2) Inserta el mapeo MAC/IP en la caché*/

        /*
          Tipo sr_arpreq:

          struct sr_arpreq {
              uint32_t ip;
              time_t sent;                // Last time this ARP request was sent. You 
                                            should update this. If the ARP request was 
                                            never sent, will be 0.
              uint32_t times_sent;        // Number of times this request was sent. You 
                                            should update this. 
              struct sr_packet *packets;  //List of pkts waiting on this req to finish 
              struct sr_arpreq *next;
          };

        */

        struct sr_arpreq *pendientes = sr_arpcache_insert(&(sr->cache), 
                                                          arp_hdr->ar_sha, 
                                                          arp_hdr->ar_sip);
        if (pendientes){
          uint8_t *dhost = arp_hdr->ar_sha;
          struct sr_if *if_salida = sr_get_interface(sr, pendientes->packets->iface);
          uint8_t *shost = if_salida->addr;

          sr_arp_reply_send_pending_packets(sr, pendientes, dhost, shost, if_salida);
          
          sr_arpreq_destroy(&(sr->cache), pendientes);
    } 
    else {
        printf("Código de operación ARP desconocido (OpCode: %hu). Paquete descartado.\n", op_code);
    }
  }
}

/* 
* ***** A partir de aquí no debería tener que modificar nada ****
*/

/* Envía todos los paquetes IP pendientes de una solicitud ARP */
void sr_arp_reply_send_pending_packets(struct sr_instance *sr,
                                        struct sr_arpreq *arpReq,
                                        uint8_t *dhost,
                                        uint8_t *shost,
                                        struct sr_if *iface) {

  struct sr_packet *currPacket = arpReq->packets;
  sr_ethernet_hdr_t *ethHdr;
  uint8_t *copyPacket;

  while (currPacket != NULL) {
     ethHdr = (sr_ethernet_hdr_t *) currPacket->buf;
     memcpy(ethHdr->ether_shost, shost, sizeof(uint8_t) * ETHER_ADDR_LEN);
     memcpy(ethHdr->ether_dhost, dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);

     copyPacket = malloc(sizeof(uint8_t) * currPacket->len);
     memcpy(copyPacket, ethHdr, sizeof(uint8_t) * currPacket->len);

     print_hdrs(copyPacket, currPacket->len);
     sr_send_packet(sr, copyPacket, currPacket->len, iface->name);
     currPacket = currPacket->next;
  }
}

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  assert(sr);
  assert(packet);
  assert(interface);

  printf("*** -> Received packet of length %d \n",len);

  /* Obtengo direcciones MAC origen y destino */
  sr_ethernet_hdr_t *eHdr = (sr_ethernet_hdr_t *) packet;
  uint8_t *destAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint8_t *srcAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(destAddr, eHdr->ether_dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(srcAddr, eHdr->ether_shost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint16_t pktType = ntohs(eHdr->ether_type);

  if (is_packet_valid(packet, len)) {
    if (pktType == ethertype_arp) {
      sr_handle_arp_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    } else if (pktType == ethertype_ip) {
      sr_handle_ip_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    }
  }

}/* end sr_ForwardPacket */
