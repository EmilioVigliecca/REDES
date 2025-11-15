/*-----------------------------------------------------------------------------
 * File:  sr_rip.c
 * Date:  Mon Sep 22 23:15:59 GMT-3 2025 
 * Authors: Santiago Freire
 * Contact: sfreire@fing.edu.uy
 *
 * Description:
 *
 * Data structures and methods for handling RIP protocol
 *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "sr_router.h"
#include "sr_rt.h"
#include "sr_rip.h"

#define SPLIT_HORIZON_POISONED_REVERSE_ENABLED 1
#define TRIGGERED_UPDATE_ENABLED 1
/*1 habilitado, 0 deshabilitado*/
/*Pide en la letra que tengas un booleano para
Apagar y prender esto*/

#include "sr_utils.h"

#define RIP_MAX_ENTRIES 25

static pthread_mutex_t rip_metadata_lock = PTHREAD_MUTEX_INITIALIZER;

/* Dirección MAC de multicast para los paquetes RIP */
uint8_t rip_multicast_mac[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x09};

/* Función de validación de paquetes RIP */
int sr_rip_validate_packet(sr_rip_packet_t* packet, unsigned int len) {
    if (len < sizeof(sr_rip_packet_t)) {
        return 0;
    }

    if (packet->command != RIP_COMMAND_REQUEST && packet->command != RIP_COMMAND_RESPONSE) {
        return 0;
    }

    if (packet->version != RIP_VERSION) {
        return 0;
    }

    if (packet->zero != 0) {
        return 0;
    }

    unsigned int expected_len = sizeof(struct sr_rip_packet_t) +
                               ((len - sizeof(struct sr_rip_packet_t)) / sizeof(struct sr_rip_entry_t)) *
                               sizeof(struct sr_rip_entry_t);

    if (len != expected_len) {
        return 0;
    }

    return 1;
}

/*ESTA FUNCIÓN LA AGREGUÉ PARA SIMPLIFICAR SR_RIP_UPDATE_ROUTE que es la LONGA FUNCIÓN*/
static struct sr_rt* sr_rt_find_exact(struct sr_rt* rt_table, uint32_t dest, uint32_t mask)
{
    struct sr_rt* rt = rt_table;
    while (rt)
    {
        if (rt->dest.s_addr == dest && rt->mask.s_addr == mask)
        {
            return rt;
        }
        rt = rt->next;
    }
    return NULL;
}

int sr_rip_update_route(struct sr_instance* sr,
                        const struct sr_rip_entry_t* rte,
                        uint32_t src_ip,
                        const char* in_ifname)
{
    /*
    ESTE ES EL COMENTARIO QUE PUSIERON ELLOS COMO GUÍA PARA LA FUNCIÓN, YA ESTABA
     * Procesa una entrada RIP recibida por una interfaz.
     *

     *  - Si la métrica anunciada es >= 16:
     *      - Si ya existe una ruta coincidente aprendida desde el mismo vecino, marca la ruta
     *        como inválida, pone métrica a INFINITY y fija el tiempo de garbage collection.
     *      - Si no, ignora el anuncio de infinito.
     *  - Calcula la nueva métrica sumando el coste del enlace de la interfaz; si resulta >=16,
     *    descarta la actualización.
     *  - Si la ruta no existe, inserta una nueva entrada en la tabla de enrutamiento.
     *  - Si la entrada existe pero está inválida, la revive actualizando métrica, gateway,
     *    learned_from, interfaz y timestamps.
     *  - Si la entrada fue aprendida del mismo vecino:
     *      - Actualiza métrica/gateway/timestamps si cambian; si no, solo refresca el timestamp.
     *  - Si la entrada viene de otro origen:
     *      - Reemplaza la ruta si la nueva métrica es mejor.
     *      - Si la métrica es igual y el next-hop coincide, refresca la entrada.
     *      - En caso contrario (peor métrica o diferente camino), ignora la actualización.
     *  - Actualiza campos relevantes: metric, gw, route_tag, learned_from, interface,
     *    last_updated, valid y garbage_collection_time según corresponda.
     *
     * Valores de retorno:
     *  - -1: entrada inválida o fallo al obtener la interfaz.
     *  -  1: la tabla de rutas fue modificada (inserción/actualización/eliminación).
     *  -  0: no se realizaron cambios.
     *
     */

    /*ES EN ESTA FUNCIÓN (UPDATE) QUE SE APLICA EL VECTOR DE DISTANCIA, 
    ESTO HACE FUNCIONAR TODAS LAS TABLAS*/

    time_t now = time(NULL);

    /* 1 Obtener detalles de la entrada anunciada */
    uint32_t dest_ip = rte->ip;
    uint32_t dest_mask = rte->mask;
    uint32_t announced_metric = ntohl(rte->metric);
    uint16_t new_route_tag = rte->route_tag; /* Guardamos el route tag 
    como se va a usar en send_response */
    uint32_t new_gateway_ip = src_ip;

    /* 2 Obtener la interfaz de entrada y su costo */
    struct sr_if* in_iface = sr_get_interface(sr, in_ifname);
    if (!in_iface) {
        printf("RIP: Error al obtener la interfaz %s. Descartando entrada.\n", in_ifname);
        return -1; /* Fallo al obtener la interfaz */
    }
    /* "El costo corresponde al atributo cost, no debe asumir que el costo siempre es 1" (de la letra) */
    uint8_t if_cost = in_iface->cost ? in_iface->cost : 1;

    /* 3 Buscar si ya tenemos una ruta *exacta* para este destino/máscara */
    struct sr_rt* existing_route = sr_rt_find_exact(sr->routing_table, dest_ip, dest_mask);
    
    /* Una ruta es "dinámica" si fue aprendida de un vecino (learned_from != 0) */
    int is_dynamic = (existing_route && existing_route->learned_from != 0);

    
     /* REQUISITO 1: Si la métrica anunciada es >= 16 (INFINITO)
     */
    if (announced_metric >= INFINITY)
    {
        /* Si existe una ruta coincidente aprendida *desde el mismo vecino*... */
        if (is_dynamic && existing_route->learned_from == new_gateway_ip)
        {
            /* ...marca la ruta como inválida (si no lo estaba ya) */
            if (existing_route->valid) {
                printf("RIP: Marcando ruta como inválida (vecino anunció INFINITO): %s/%s\n",
                      inet_ntoa(existing_route->dest), inet_ntoa(existing_route->mask));
                existing_route->metric = INFINITY;
                existing_route->valid = 0; /* Inválida = 0*/
                existing_route->garbage_collection_time = now; /* Fija tiempo de G.C. */
                return 1; /* La tabla fue modificada */
            }
        }
        /* Si no, ignora el anuncio de infinito (vino de otro vecino, o no existía) */
        return 0; /* No se realizaron cambios */
    }

    /*
     * REQUISITO 2: Calcula la nueva métrica sumando el coste del enlace
     */
    uint32_t new_metric = announced_metric + if_cost;
    
    /* Si resulta >= 16, descarta la actualización. */
    if (new_metric >= INFINITY) {
        /*Osea no normalizamos a 16, simplemente la descartamos */
        return 0; /* No se realizaron cambios */
    }

    /*
     * REQUISITO 3: Si la ruta no existe
     */
    if (!existing_route)
    {
        printf("RIP: Insertando NUEVA ruta: %s/%s via %s (métrica %d) on %s\n",
              inet_ntoa(*(struct in_addr*)&dest_ip),
              inet_ntoa(*(struct in_addr*)&dest_mask),
              inet_ntoa(*(struct in_addr*)&new_gateway_ip),
              new_metric,
              in_ifname);
              
        /* Inserta una nueva entrada en la tabla de enrutamiento */
        sr_add_rt_entry(sr,
                        *(struct in_addr*)&dest_ip,
                        *(struct in_addr*)&new_gateway_ip,
                        *(struct in_addr*)&dest_mask,
                        in_ifname,
                        (uint8_t)new_metric,
                        new_route_tag,
                        new_gateway_ip,  /* learned_from */
                        now,             /* last_updated */
                        1,               /* valid */
                        0);              /* garbage_collection_time */
        return 1; /* La tabla fue modificada */
    }
    
    /* --- La ruta SÍ existe --- */

    /* No debemos actualizar rutas conectadas directamente (estáticas) */
    if (existing_route->learned_from == 0) {
        /*Comenté este print porque es muy GEDE y no puedo ver nada, 
        total ya veo que funciona xd, aunque es raro que la máscara y
        la ruta coinciden
        
        */
        /*printf("RIP: Ignorando anuncio para red conectada directamente: %s/%s\n",
              inet_ntoa(existing_route->dest), inet_ntoa(existing_route->mask));*/
        return 0; /* No se realizan cambios */
    }

    /*
     * REQUISITO 4: Si la entrada existe pero está inválida
     */
    if (existing_route->valid == 0)
    {
        printf("RIP: Reviviendo ruta inválida: %s/%s via %s (métrica %d) on %s\n",
              inet_ntoa(existing_route->dest),
              inet_ntoa(existing_route->mask),
              inet_ntoa(*(struct in_addr*)&new_gateway_ip),
              new_metric,
              in_ifname);

        /*La revive actualizando métrica, gateway, learned_from, etc. */
        existing_route->metric = (uint8_t)new_metric;
        existing_route->gw.s_addr = new_gateway_ip;
        existing_route->route_tag = new_route_tag;
        existing_route->learned_from = new_gateway_ip;
        strncpy(existing_route->interface, in_ifname, sr_IFACE_NAMELEN);
        existing_route->last_updated = now;
        existing_route->valid = 1; /* La revive */
        existing_route->garbage_collection_time = 0; /* Detiene G.C. */
        return 1; /* La tabla fue modificada */
    }

    /*
     * REQUISITO 5: Si la entrada fue aprendida del mismo vecino
     */
    
    if (existing_route->learned_from == new_gateway_ip)
    {
        int changed = 0;
        /* Actualiza métrica/gateway/timestamps si cambian */
        if (existing_route->metric != (uint8_t)new_metric) {
            existing_route->metric = (uint8_t)new_metric;
            changed = 1;
        }
        if (existing_route->gw.s_addr != new_gateway_ip) {
            existing_route->gw.s_addr = new_gateway_ip;
            changed = 1;
        }
        if (existing_route->route_tag != new_route_tag) {
            existing_route->route_tag = new_route_tag;
            changed = 1;
        }
        /* (No dice esto en los comentarios, pero si la métrica/gw cambian, la interfaz también debería) */
        if (strcmp(existing_route->interface, in_ifname) != 0) {
            strncpy(existing_route->interface, in_ifname, sr_IFACE_NAMELEN);
            changed = 1;
        }
        
        /* Y si no, solo refresca el timestamp */
        existing_route->last_updated = now;
        
        if (changed) {
             printf("RIP: Actualizando ruta (mismo vecino): %s/%s\n",
                   inet_ntoa(existing_route->dest), inet_ntoa(existing_route->mask));
        }
        
        return changed; /* 1 si cambió, 0 si solo se refrescó */
    }

    /*
     * REQUISITO 6: Si la entrada viene de otro origen (otro vecino)
     */
    if (existing_route->learned_from != new_gateway_ip)
    {
        /* Reemplaza la ruta si la nueva métrica es mejor */
        if (new_metric < existing_route->metric)
        {
            printf("RIP: Reemplazando ruta (mejor métrica de nuevo vecino): %s/%s\n",
                  inet_ntoa(existing_route->dest), inet_ntoa(existing_route->mask));
                  
            existing_route->metric = (uint8_t)new_metric;
            existing_route->gw.s_addr = new_gateway_ip;
            existing_route->route_tag = new_route_tag;
            existing_route->learned_from = new_gateway_ip;
            strncpy(existing_route->interface, in_ifname, sr_IFACE_NAMELEN);
            existing_route->last_updated = now;
            /* (Ya era válida, no se toca 'valid' ni 'garbage_collection_time') */
            return 1; /* La tabla fue modificada */
        }
        
        /* Si la métrica es igual y el next-hop coincide, refresca la entrada. */
        /* (Caso borde,'learned_from' es diferente pero 'gw' es igual) */
        if (new_metric == existing_route->metric && existing_route->gw.s_addr == new_gateway_ip)
        {
            existing_route->last_updated = now;
            return 0; /* No se realizaron cambios, igual actualizas last updated pq chequeaste*/
        }

        /*En caso contrario (peor métrica o diferente camino), ignora la actualización */
        return 0; /* No se realizaron cambios */
    }

    /*Nunca debería entrar acá igual con todos los ifs, pero para que compile bien y eso*/
    return 0;
}


void sr_handle_rip_packet(struct sr_instance* sr,
                          const uint8_t* packet,
                          unsigned int pkt_len,
                          unsigned int ip_off,
                          unsigned int rip_off,
                          unsigned int rip_len,
                          const char* in_ifname)
{
    /* 1 Validar paquete RIP */

    /* 2 Si es un RIP_COMMAND_REQUEST, enviar respuesta por la interfaz donde llegó, se sugiere usar función auxiliar sr_rip_send_response */

    /* 3 Si no es un REQUEST, entonces es un RIP_COMMAND_RESPONSE. En caso que no sea un REQUEST o RESPONSE no pasa la validación. */
    
    /* 4 Procesar entries en el paquete de RESPONSE que llegó, se sugiere usar función auxiliar sr_rip_update_route */

    /* 5 Si hubo un cambio en la tabla, generar triggered update e imprimir tabla */

    sr_rip_packet_t* rip_packet = (struct sr_rip_packet_t*)(packet + rip_off);
    
    /* *Obtenemos la cabecera IP para saber quién envió el paquete
     * Esto es crucial tanto para responder a un REQUEST (unicast)
     * como para saber el 'next hop' en un RESPONSE.
    */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*)(packet + ip_off);
    uint32_t src_ip = ip_hdr->ip_src;

    /* Obtenemos la estructura de la interfaz por la que llegó */
    struct sr_if* in_iface = sr_get_interface(sr, in_ifname);
    if (!in_iface) {
        printf("RIP: Paquete recibido en interfaz desconocida %s. Descartando.\n", in_ifname);
        return;
    }

    /* 1 Validar paquete RIP */
    if (!sr_rip_validate_packet(rip_packet, rip_len)) {
        printf("RIP: Paquete RIP inválido recibido. Descartando.\n");
        return;
    }

    /* Procesar según si es REQUEST o RESPONSE */
    if (rip_packet->command == RIP_COMMAND_REQUEST)
    {
        /* * 2 Si es un RIP_COMMAND_REQUEST, enviar respuesta
         * por la interfaz donde llegó */
        printf("-> RIP: Recibió REQUEST en interfaz %s de %s. Enviando respuesta.\n",
              in_ifname, inet_ntoa(*(struct in_addr*)&src_ip));

        /* Usamos la función auxiliar para enviar la respuesta a la IP de origen, como sugiere arriba */
        sr_rip_send_response(sr, in_iface, src_ip);
    }
    else if (rip_packet->command == RIP_COMMAND_RESPONSE)
    {
        /* * 3 Si es un RIP_COMMAND_RESPONSE, procesar las entradas
         * Y si es otra cosa no pasa la validación */
        printf("-> RIP: Recibió RESPONSE en interfaz %s de %s. Procesando entradas.\n",
              in_ifname, inet_ntoa(*(struct in_addr*)&src_ip));

        int cambios = 0;
        int num_entries = (rip_len - sizeof(sr_rip_packet_t)) / sizeof(sr_rip_entry_t);

        /* * Bloqueo la tabla de enrutamiento para evitar que cambie mientras la estás revisando, 
        Y estas viendo si cambia o no
         */
        pthread_mutex_lock(&rip_metadata_lock);

        for (int i = 0; i < num_entries; i++)
        {
            struct sr_rip_entry_t* entry = &rip_packet->entries[i];
            
            /* * 4 Usamos la función auxiliar como dice arriba para procesar cada entrada de ruta.
             * El 'neighbor_ip' (src_ip) es el router que nos anunció esta ruta.
             */
            int cambios = sr_rip_update_route(sr, entry, src_ip, in_ifname);
            /* Marcamos que la tabla cambió*/
            
        }
        
        pthread_mutex_unlock(&rip_metadata_lock);

        /* * 5 Si hubo un cambio en la tabla, generar triggered update 
         * e imprimir la tabla.
         */
        if (cambios)
        {
            printf("-> RIP: Tabla de enrutamiento cambió. \n");

            /* * Un "triggered update" es una respuesta a todos los vecinos avisando del cambio
             * Envia un RESPONSE a la dirección multicast RIP_IP
             * Osea a todas las interfaces
             */
            if(TRIGGERED_UPDATE_ENABLED){
                printf("TRIGGERED UPDATE ACTIVADO\n");
                struct sr_if* if_walker = sr->if_list;
                while (if_walker)
                {
                    /* * La función sr_rip_send_response debe implementar la lógica de "split horizon"
                    Que es lo de que un router nunca debe anunciar una ruta de vuelta 
                    por la misma interfaz por la que la aprendió porque si hace eso 
                    empieza a loopear hasta infinito
                    
                    */
                    sr_rip_send_response(sr, if_walker, htonl(RIP_IP));
                    if_walker = if_walker->next;
                }    
            }
            
            printf("\n-> RIP: Imprimiendo tabla de enrutamiento luego de procesar:\n");
            print_routing_table(sr);
        }
    }
}

void sr_rip_send_response(struct sr_instance* sr, struct sr_if* interface, uint32_t ipDst) {
    /*ESTOS SON LOS COMENTARIOS QUE YA ESTABAN:*/   
    /* Reservar buffer para paquete completo con cabecera Ethernet */
    
    /* Construir cabecera Ethernet */
    
    /* Construir cabecera IP */
        /* RIP usa TTL=1 */
    
    /* Construir cabecera UDP */
    
    /* Construir paquete RIP con las entradas de la tabla */
        /* Armar encabezado RIP de la respuesta */
        /* Recorrer toda la tabla de enrutamiento  */
        /* Considerar split horizon con poisoned reverse y rutas expiradas por timeout cuando corresponda */
        /* Normalizar métrica a rango RIP (1..INFINITY) */

        /* Armar la entrada RIP:
           - family=2 (IPv4)
           - route_tag desde la ruta
           - ip/mask toman los valores de la tabla
           - next_hop: siempre 0.0.0.0 */

    /* Calcular longitudes del paquete */
    
    /* Calcular checksums */
    
    /* Enviar paquete */


    /* 1 Reservar buffer para paquete completo (tamaño MÁXIMO) */
    /* Asumimos un máximo de 25 entradas (RIP_MAX_ENTRIES) */
    unsigned int eth_len = sizeof(sr_ethernet_hdr_t);
    unsigned int ip_len = sizeof(sr_ip_hdr_t);
    unsigned int udp_len = sizeof(sr_udp_hdr_t);
    unsigned int rip_max_payload_len = sizeof(sr_rip_packet_t) + (RIP_MAX_ENTRIES * sizeof(sr_rip_entry_t));
    
    unsigned int max_buf_len = eth_len + ip_len + udp_len + rip_max_payload_len;
    uint8_t* packet = (uint8_t*)calloc(1, max_buf_len);
    if (!packet) {
        printf("ERROR: Falla al reservar memoria para paquete RIP de respuesta.\n");
        return;
    }

    /* Punteros a las cabeceras */
    sr_ethernet_hdr_t* eth_hdr = (sr_ethernet_hdr_t*)packet;
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*)(packet + eth_len);
    sr_udp_hdr_t* udp_hdr = (sr_udp_hdr_t*)(packet + eth_len + ip_len);
    sr_rip_packet_t* rip_packet = (sr_rip_packet_t*)(packet + eth_len + ip_len + udp_len);

    /* 2 Construir paquete RIP (el payload) */
    /*Armar encabezado RIP de la respuesta */
    rip_packet->command = RIP_COMMAND_RESPONSE;
    rip_packet->version = RIP_VERSION;
    rip_packet->zero = 0;

    /*Recorrer toda la tabla de enrutamiento */
    pthread_mutex_lock(&rip_metadata_lock); /* Proteger la tabla */

    struct sr_rt* rt_walker = sr->routing_table;
    int num_routes_sent = 0;

    while (rt_walker && num_routes_sent < RIP_MAX_ENTRIES)
    {
        struct sr_rip_entry_t* entry = &rip_packet->entries[num_routes_sent];

        /* Armar la entrada RIP */
        entry->family_identifier = htons(RIP_VERSION); /* 2 = IPv4 */
        entry->route_tag = rt_walker->route_tag; /* "dejarlo con valor cero" (letra) (pero lo propagamos si lo aprendimos) */
        entry->ip = rt_walker->dest.s_addr;
        entry->mask = rt_walker->mask.s_addr;
        entry->next_hop = 0x00000000; /* "El next-hop debe ser 0.0.0.0" (letra) */

        /* Normalizar métrica y aplicar Split Horizon */
        uint32_t metric_to_send = rt_walker->metric;

        /* Acá normalizas métrica a rango RIP (1..INFINITY) */
        if (metric_to_send > INFINITY) {
            metric_to_send = INFINITY;
        }

        /*
         * Lógica de Split Horizon con Reversa Envenenada:
         * Si la ruta fue aprendida dinámicamente (learned_from != 0)
         * Y la interfaz por la que la aprendimos (rt_walker->interface)
         * es la MISMA que por la que vamos a enviar (interface->name)
         * -> Anunciamos la ruta con métrica INFINITO (16).
         */
        int is_dynamic_route = (rt_walker->learned_from != 0);
        int learned_on_this_if = (strcmp(rt_walker->interface, interface->name) == 0);

        if (SPLIT_HORIZON_POISONED_REVERSE_ENABLED && is_dynamic_route && learned_on_this_if)
        {
            metric_to_send = INFINITY;
        }
        
        entry->metric = htonl(metric_to_send);

        num_routes_sent++;
        rt_walker = rt_walker->next;
    }
    
    pthread_mutex_unlock(&rip_metadata_lock); /* Liberar la tabla */

    /* 3 Calcular longitudes FINALES del paquete */
    unsigned int actual_rip_len = sizeof(sr_rip_packet_t) + (num_routes_sent * sizeof(sr_rip_entry_t));
    unsigned int actual_ip_payload_len = udp_len + actual_rip_len;
    unsigned int actual_total_len = eth_len + ip_len + actual_ip_payload_len;

    /* 4 Construir cabecera UDP */
    udp_hdr->src_port = htons(RIP_PORT);
    udp_hdr->dst_port = htons(RIP_PORT); /* Siempre 520, incluso en respuestas unicast */
    udp_hdr->length = htons(actual_ip_payload_len);
    udp_hdr->checksum = 0; /* Se calcula al final */

    /* 5 Construir cabecera IP */
    ip_hdr->ip_v = 4;
    ip_hdr->ip_hl = 5; /* 20 bytes */
    ip_hdr->ip_tos = 0;
    ip_hdr->ip_len = htons(ip_len + actual_ip_payload_len);
    ip_hdr->ip_id = 0; 
    ip_hdr->ip_off = htons(IP_DF); /* No Fragmentar */
    ip_hdr->ip_ttl = 1; /* "Todos los mensajes RIP enviados deben tener... TTL fijado en 1." (PDF) */
    ip_hdr->ip_p = ip_protocol_udp;
    ip_hdr->ip_src = interface->ip; /* IP de la interfaz de SALIDA */
    ip_hdr->ip_dst = ipDst;         /* IP de destino (parámetro) */
    ip_hdr->ip_sum = 0; /* Se calcula al final */

    /* 6 Construir cabecera Ethernet */
    /* MAC Origen (siempre la de nuestra interfaz de salida) */
    memcpy(eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
    eth_hdr->ether_type = htons(ethertype_ip);

    /* MAC Destino (depende de ipDst) */
    uint8_t dest_mac[ETHER_ADDR_LEN];
    int found_mac = 0;

    if (ipDst == htonl(RIP_IP))
    {
        /* Es Multicast: Usamos la MAC multicast de RIP */
        memcpy(dest_mac, rip_multicast_mac, ETHER_ADDR_LEN);
        found_mac = 1;
    }
    else
    {
        /* Es Unicast (respuesta a un REQUEST): Buscamos en caché ARP */
        struct sr_arpentry* arp_entry = sr_arpcache_lookup(&(sr->cache), ipDst);
        if (arp_entry)
        {
            memcpy(dest_mac, arp_entry->mac, ETHER_ADDR_LEN);
            found_mac = 1;
            free(arp_entry);
        }
        else
        {
            /* * No se encontró la MAC. Idealmente, encolaríamos el paquete y enviaríamos 
             * un ARP Request. Pero para RIP, asumimos que si respondemos a un request,
             * acabamos de recibir un paquete de él, por lo que su MAC *debería* estar
             * en la caché. Si no está, descartamos.
             */
            printf("RIP: ERROR! No hay entrada ARP para respuesta unicast a %s. Descartando.\n",
                  inet_ntoa(*(struct in_addr*)&ipDst));
            free(packet);
            return;
        }
    }
    memcpy(eth_hdr->ether_dhost, dest_mac, ETHER_ADDR_LEN);

    /* 7 Calcular checksums */

    ip_hdr->ip_sum = ip_cksum(ip_hdr, ip_len);
    
    /* Checksum UDP (incluye pseudo-cabecera) */
    /* Asumimos que la función udp_cksum (de sr_utils.c) maneja la lógica */
    /* de la pseudo-cabecera internamente, recibiendo el ip_hdr y el udp_hdr. */
    udp_hdr->checksum = udp_cksum(ip_hdr, udp_hdr, (const uint8_t*)rip_packet);

    /* 8 Enviar paquete */
    printf("-> RIP: Enviando RESPUESTA por %s (hacia %s, %d rutas)\n",
          interface->name, inet_ntoa(*(struct in_addr*)&ipDst), num_routes_sent);
    
    sr_send_packet(sr, packet, actual_total_len, interface->name);

    /* 9 Liberar buffer */
    free(packet);
}

void* sr_rip_send_requests(void* arg) {
    sleep(3); // Esperar a que se inicialice todo
    struct sr_instance* sr = arg;
    struct sr_if* interface = sr->if_list;
    // Se envia un Request RIP por cada interfaz:
        /* Reservar buffer para paquete completo con cabecera Ethernet */
        
        /* Construir cabecera Ethernet */
        
        /* Construir cabecera IP */
            /* RIP usa TTL=1 */
        
        /* Construir cabecera UDP */
        
        /* Construir paquete RIP */

        /* Entrada para solicitar la tabla de ruteo completa (ver RFC) */

        /* Calcular longitudes del paquete */
        
        /* Calcular checksums */
        
        /* Enviar paquete */


        printf("-> RIP: Enviando REQUESTS iniciales...\n");

    // Se envia un Request RIP por cada interfaz:
    while (interface)
    {
        /* 1 Reservar buffer para paquete (Eth + IP + UDP + Encabezado RIP + 1 entrada) */
        unsigned int eth_len = sizeof(sr_ethernet_hdr_t);
        unsigned int ip_len = sizeof(sr_ip_hdr_t);
        unsigned int udp_len = sizeof(sr_udp_hdr_t);
        /* El payload es un encabezado RIP + 1 entrada especial */
        unsigned int rip_payload_len = sizeof(sr_rip_packet_t) + sizeof(sr_rip_entry_t);
        unsigned int total_len = eth_len + ip_len + udp_len + rip_payload_len;

        uint8_t* packet = (uint8_t*)calloc(1, total_len);
        if (!packet) {
            printf("ERROR: Falla al reservar memoria para paquete RIP de solicitud.\n");
            interface = interface->next;
            continue;
        }

        /* Punteros a las cabeceras */
        sr_ethernet_hdr_t* eth_hdr = (sr_ethernet_hdr_t*)packet;
        sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*)(packet + eth_len);
        sr_udp_hdr_t* udp_hdr = (sr_udp_hdr_t*)(packet + eth_len + ip_len);
        sr_rip_packet_t* rip_packet = (sr_rip_packet_t*)(packet + eth_len + ip_len + udp_len);

        /* 2 Construir paquete RIP (REQUEST) */
        rip_packet->command = RIP_COMMAND_REQUEST;
        rip_packet->version = RIP_VERSION;
        rip_packet->zero = 0;

        /* 3 Entrada para solicitar la tabla de ruteo completa (RFC 2453) */
        /* Un REQUEST con 1 entrada, AFI=0 y Métrica=16 pide la tabla completa */
        sr_rip_entry_t* entry = &rip_packet->entries[0];
        entry->family_identifier = htons(0); /* AFI = 0 */
        entry->route_tag = 0;
        entry->ip = 0;
        entry->mask = 0;
        entry->next_hop = 0;
        entry->metric = htonl(INFINITY); /* Métrica = 16 */

        /* 4 Construir cabecera UDP */
        udp_hdr->src_port = htons(RIP_PORT);
        udp_hdr->dst_port = htons(RIP_PORT);
        udp_hdr->length = htons(udp_len + rip_payload_len);
        udp_hdr->checksum = 0;

        /* 5 Construir cabecera IP */
        ip_hdr->ip_v = 4;
        ip_hdr->ip_hl = 5;
        ip_hdr->ip_len = htons(ip_len + udp_len + rip_payload_len);
        ip_hdr->ip_ttl = 1; /* "Todos los mensajes RIP enviados deben tener TTL fijado en 1" (letra) */
        ip_hdr->ip_p = ip_protocol_udp;
        ip_hdr->ip_src = interface->ip;
        ip_hdr->ip_dst = htonl(RIP_IP); /* Los Requests van a la IP multicast */
        ip_hdr->ip_sum = 0;

        /* 6 Construir cabecera Ethernet */
        memcpy(eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
        memcpy(eth_hdr->ether_dhost, rip_multicast_mac, ETHER_ADDR_LEN); /* MAC multicast */
        eth_hdr->ether_type = htons(ethertype_ip);

        /* 7 Calcular checksums */
        ip_hdr->ip_sum = ip_cksum(ip_hdr, ip_len);
        udp_hdr->checksum = udp_cksum(ip_hdr, udp_hdr, (const uint8_t*)rip_packet);
        
        /* 8 Enviar paquete */
        printf("-> RIP: Enviando REQUEST por %s\n", interface->name);
        sr_send_packet(sr, packet, total_len, interface->name);
        
        free(packet);
        interface = interface->next;
    }
        
    return NULL;
}
  


/* Periodic advertisement thread */
void* sr_rip_periodic_advertisement(void* arg) {
    
    struct sr_instance* sr = arg;

    sleep(2); // Esperar a que se inicialice todo
    
    // Agregar las rutas directamente conectadas
    /************************************************************************************/
    pthread_mutex_lock(&rip_metadata_lock);
    struct sr_if* int_temp = sr->if_list;
    while(int_temp != NULL)
    {
        struct in_addr ip;
        ip.s_addr = int_temp->ip;
        struct in_addr gw;
        gw.s_addr = 0x00000000;
        struct in_addr mask;
        mask.s_addr =  int_temp->mask;
        struct in_addr network;
        network.s_addr = ip.s_addr & mask.s_addr;
        uint8_t metric = int_temp->cost ? int_temp->cost : 1;

        for (struct sr_rt* it = sr->routing_table; it; it = it->next) {
        if (it->dest.s_addr == network.s_addr && it->mask.s_addr == mask.s_addr)
            sr_del_rt_entry(&sr->routing_table, it);
        }
        printf("-> RIP: Adding the directly connected network [%s, ", inet_ntoa(network));
        printf("%s] to the routing table\n", inet_ntoa(mask));
        sr_add_rt_entry(sr,
                        network,
                        gw,
                        mask,
                        int_temp->name,
                        metric,
                        0,
                        htonl(0),
                        time(NULL),
                        1,
                        0);
        int_temp = int_temp->next;
    }
    
    pthread_mutex_unlock(&rip_metadata_lock);
    printf("\n-> RIP: Printing the forwarding table\n");
    print_routing_table(sr);
    /************************************************************************************/

    /* A PARTIR DE ACÁ VA LO NUEVO, QUE ES CON ESTE COMENTARIO QUE DEJARON
        Espera inicial de RIP_ADVERT_INTERVAL_SEC antes del primer envío.
        A continuación entra en un bucle infinito que, cada RIP_ADVERT_INTERVAL_SEC segundos,
        recorre la lista de interfaces (sr->if_list) y envía una respuesta RIP por cada una,
        utilizando la dirección de multicast definida (RIP_IP).
        Esto implementa el envío periódico de rutas (anuncios no solicitados) en RIPv2.
    */

    /* En la letra dice que 10 segundos para el timer de avisos no solicitados */
    
    sleep(RIP_ADVERT_INTERVAL_SEC); 

    while (1)
    {
        printf("-> RIP: Enviando anuncio periódico no solicitado (multicast)...\n");

        /* Recorre la lista de interfaces (sr->if_list) */
        struct sr_if* if_walker = sr->if_list;
        while (if_walker)
        {
            /* Y envía una respuesta RIP por cada una,
             * utilizando la dirección de multicast definida (RIP_IP)
             */
            sr_rip_send_response(sr, if_walker, htonl(RIP_IP));
            if_walker = if_walker->next;
        }

        /* Cada RIP_ADVERT_INTERVAL_SEC segundos */
        sleep(RIP_ADVERT_INTERVAL_SEC);
    }
    return NULL;
}

/* Chequea las rutas y marca las que expiran por timeout */
void* sr_rip_timeout_manager(void* arg) {
    struct sr_instance* sr = arg;
    /*ESTE ES COMENTARIO QUE YA ESTABA:*/
    /*  - Bucle periódico que espera 1 segundo entre comprobaciones.
        - Recorre la tabla de enrutamiento y para cada ruta dinámica (aprendida de un vecino) que no se haya actualizado
        en el intervalo de timeout (RIP_TIMEOUT_SEC), marca la ruta como inválida, fija su métrica a
        INFINITY y anota el tiempo de inicio del proceso de garbage collection.
        - Si se detectan cambios, se desencadena una actualización (triggered update)
        hacia los vecinos y se actualiza/visualiza la tabla de enrutamiento.
        - Imprimir la tabla si hay cambios
        - Se debe usar el mutex rip_metadata_lock para proteger el acceso concurrente
          a la tabla de enrutamiento.
    */

    /* La letra dice 60 segundos para el timer de timeout */
    while(1)
    {
        /* Loop periódico que espera 1 segundo entre comprobaciones */
        sleep(1); 
        
        time_t now = time(NULL);
        int changes_made = 0;

        /* Se debe usar el mutex rip_metadata_lock */
        pthread_mutex_lock(&rip_metadata_lock);

        /* Recorre la tabla de enrutamiento */
        struct sr_rt* rt_walker = sr->routing_table;
        while (rt_walker)
        {
            /* Para cada ruta dinámica pasada por vecino
            que no se haya actualizado y siga válida */
            if (rt_walker->learned_from != 0 && rt_walker->valid)
            {
                /* En el intervalo de timeout (RIP_TIMEOUT_SEC) */
                if ((now - rt_walker->last_updated) >= RIP_TIMEOUT_SEC)
                {
                    printf("RIP: Ruta expirada (timeout): %s/%s via %s\n",
                          inet_ntoa(rt_walker->dest), 
                          inet_ntoa(rt_walker->mask),
                          inet_ntoa(rt_walker->gw));

                    /* Marca la ruta como inválida */
                    rt_walker->valid = 0;
                    /* Fija su métrica a INFINITY */
                    rt_walker->metric = INFINITY;
                    /* Anota el tiempo de inicio del proceso de garbage collection */
                    rt_walker->garbage_collection_time = now;
                    
                    changes_made = 1;
                }
            }
            rt_walker = rt_walker->next;
        }

        pthread_mutex_unlock(&rip_metadata_lock);

        /* Si se detectan cambios, marca triggered update */
        if (changes_made)
        {
            printf("-> RIP: Rutas expiradas. Enviando triggered update...\n");
            
            struct sr_if* if_walker = sr->if_list;
            while (if_walker)
            {
                /* Un triggered update es un RESPONSE multicast por todas las interfaces */
                sr_rip_send_response(sr, if_walker, htonl(RIP_IP));
                if_walker = if_walker->next;
            }

            /* Y se actualiza e imprime la tabla de enrutamiento */
            printf("\n-> RIP: Imprimiendo tabla de rutas (post-timeout):\n");
            print_routing_table(sr);
        }
    }

    return NULL;
}

/* Chequea las rutas marcadas como garbage collection y las elimina si expira el timer */
void* sr_rip_garbage_collection_manager(void* arg) {
    struct sr_instance* sr = arg;
    /* COMENTARIO QUE YA ESTABA:
        - Bucle infinito que espera 1 segundo entre comprobaciones.
        - Recorre la tabla de enrutamiento y elimina aquellas rutas que:
            * estén marcadas como inválidas (valid == 0) y
            * lleven más tiempo en garbage collection que RIP_GARBAGE_COLLECTION_SEC
              (current_time >= garbage_collection_time + RIP_GARBAGE_COLLECTION_SEC).
        - Si se detectan eliminaciones, se imprime la tabla.
        - Se debe usar el mutex rip_metadata_lock para proteger el acceso concurrente
          a la tabla de enrutamiento.
    */
    /* En letra dice 40 segundos para el timer de garbage collection */

    /* Bucle infinito que espera 1 segundo entre comprobaciones. */
    while (1)
    {
        sleep(1);

        time_t now = time(NULL);
        int routes_deleted = 0;

        /* Se debe usar el mutex rip_metadata_lock */
        pthread_mutex_lock(&rip_metadata_lock);

        struct sr_rt* rt_walker = sr->routing_table;
        struct sr_rt* rt_next = NULL; 
        
        /* * Iteramos de forma segura, guardando el 'next' antes de 
         * potencialmente eliminar el nodo actual
         */
        while (rt_walker)
        {
            rt_next = rt_walker->next; /* Guardar el siguiente puntero */

            /* Recorre la tabla y elimina aquellas rutas que: 
             estén marcadas como inválidas (osea valid == 0) y 
             hay una comprobación de que el timer se haya iniciado */
            if (rt_walker->valid == 0 && rt_walker->garbage_collection_time != 0)
            {
                /* * lleven más tiempo en garbage collection que RIP_GARBAGE_COLLECTION_SEC */
                if ((now - rt_walker->garbage_collection_time) >= RIP_GARBAGE_COLLECTION_SEC)
                {
                    printf("RIP: Eliminando ruta (garbage collection): %s/%s\n",
                          inet_ntoa(rt_walker->dest), 
                          inet_ntoa(rt_walker->mask));
                    
                    /* sr_del_rt_entry se encarga de liberar la memoria y mantener
                    enlazada la lista */
                    sr_del_rt_entry(&(sr->routing_table), rt_walker);
                    routes_deleted = 1;
                }
            }
            
            rt_walker = rt_next; /* Moverse al siguiente nodo guardado ya */
        }

        pthread_mutex_unlock(&rip_metadata_lock);

        /* Si se detectan eliminaciones, se imprime la tabla */
        if (routes_deleted)
        {
            printf("\n-> RIP: Imprimiendo tabla de rutas (post-garbage-collection):\n");
            print_routing_table(sr);
        }
    }
    
    return NULL;
}


/* Inicialización subsistema RIP */
int sr_rip_init(struct sr_instance* sr) {
    /* Inicializar mutex */
    if(pthread_mutex_init(&sr->rip_subsys.lock, NULL) != 0) {
        printf("RIP: Error initializing mutex\n");
        return -1;
    }

    /* Iniciar hilo avisos periódicos */
    if(pthread_create(&sr->rip_subsys.thread, NULL, sr_rip_periodic_advertisement, sr) != 0) {
        printf("RIP: Error creating advertisement thread\n");
        pthread_mutex_destroy(&sr->rip_subsys.lock);
        return -1;
    }

    /* Iniciar hilo timeouts */
    pthread_t timeout_thread;
    if(pthread_create(&timeout_thread, NULL, sr_rip_timeout_manager, sr) != 0) {
        printf("RIP: Error creating timeout thread\n");
        pthread_cancel(sr->rip_subsys.thread);
        pthread_mutex_destroy(&sr->rip_subsys.lock);
        return -1;
    }

    /* Iniciar hilo garbage collection */
    pthread_t garbage_collection_thread;
    if(pthread_create(&garbage_collection_thread, NULL, sr_rip_garbage_collection_manager, sr) != 0) {
        printf("RIP: Error creating garbage collection thread\n");
        pthread_cancel(sr->rip_subsys.thread);
        pthread_mutex_destroy(&sr->rip_subsys.lock);
        return -1;
    }

    /* Iniciar hilo requests */
    pthread_t requests_thread;
    if(pthread_create(&requests_thread, NULL, sr_rip_send_requests, sr) != 0) {
        printf("RIP: Error creating requests thread\n");
        pthread_cancel(sr->rip_subsys.thread);
        pthread_mutex_destroy(&sr->rip_subsys.lock);
        return -1;
    }

    return 0;
}