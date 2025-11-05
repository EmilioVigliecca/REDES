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

#include "sr_utils.h"

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

int sr_rip_update_route(struct sr_instance* sr,
                        const struct sr_rip_entry_t* rte,
                        uint32_t src_ip,
                        const char* in_ifname)
{
    /*
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
    sr_rip_packet_t* rip_packet = (struct sr_rip_packet_t*)(packet + rip_off);

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
    struct sr_if* in_iface = sr_get_if(sr, in_ifname);
    if (!in_iface) {
        Debug("RIP: Paquete recibido en interfaz desconocida %s. Descartando.\n", in_ifname);
        return;
    }

    /* 1 Validar paquete RIP */
    if (!sr_rip_validate_packet(rip_packet, rip_len)) {
        Debug("RIP: Paquete RIP inválido recibido. Descartando.\n");
        return;
    }

    /* Procesar según si es REQUEST o RESPONSE */
    if (rip_packet->command == RIP_COMMAND_REQUEST)
    {
        /* * 2 Si es un RIP_COMMAND_REQUEST, enviar respuesta
         * por la interfaz donde llegó */
        Debug("-> RIP: Recibió REQUEST en interfaz %s de %s. Enviando respuesta.\n",
              in_ifname, inet_ntoa(*(struct in_addr*)&src_ip));

        /* Usamos la función auxiliar para enviar la respuesta a la IP de origen, como sugiere arriba */
        sr_rip_send_response(sr, in_iface, src_ip);
    }
    else if (rip_packet->command == RIP_COMMAND_RESPONSE)
    {
        /* * 3 Si es un RIP_COMMAND_RESPONSE, procesar las entradas
         * Y si es otra cosa no pasa la validación */
        Debug("-> RIP: Recibió RESPONSE en interfaz %s de %s. Procesando entradas.\n",
              in_ifname, inet_ntoa(*(struct in_addr*)&src_ip));

        int cambios = 0;
        int num_entries = (rip_len - sizeof(sr_rip_packet_t)) / sizeof(sr_rip_entry_t);

        /* * Bloque la tabla de enrutamiento para evitar que cambie mientras la estás revisando, 
        Y estas viendo si cambia o no
         */
        pthread_mutex_lock(&rip_metadata_lock);

        for (int i = 0; i < num_entries; i++)
        {
            struct sr_rip_entry_t* entry = &rip_packet->entries[i];
            
            /* * 4 Usamos la función auxiliar como dice arriba para procesar cada entrada de ruta.
             * El 'neighbor_ip' (src_ip) es el router que nos anunció esta ruta.
             */
            int resultado = sr_rip_update_route(sr, entry, src_ip, in_ifname);
            
            if (resultado == 1) {
                cambios = 1; /* Marcamos que la tabla cambió*/
            }
        }
        
        pthread_mutex_unlock(&rip_metadata_lock);

        /* * 5 Si hubo un cambio en la tabla, generar triggered update 
         * e imprimir la tabla.
         */
        if (cambios)
        {
            Debug("-> RIP: Tabla de enrutamiento cambió. Enviando triggered update.\n");

            /* * Un "triggered update" es una respuesta a todos los vecinos avisando del cambio
             * Envia un RESPONSE a la dirección multicast RIP_IP
             * Osea a todas las interfaces
             */
            struct sr_if* if_walker = sr->if_list;
            while (if_walker)
            {
                /* * La función sr_rip_send_response debe implementar la lógica de "split horizon"
                 Que es lo de que 
                 Un router nunca debe anunciar una ruta de vuelta 
                 por la misma interfaz por la que la aprendió
                 Pq si hace eso empieza a loopear hasta infinito
                
                */
                sr_rip_send_response(sr, if_walker, htonl(RIP_IP));
                if_walker = if_walker->next;
            }

            Debug("\n-> RIP: Imprimiendo tabla de enrutamiento luego de procesar:\n");
            print_routing_table(sr);
        }
    }
}

void sr_rip_send_response(struct sr_instance* sr, struct sr_if* interface, uint32_t ipDst) {
    
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
        Debug("-> RIP: Adding the directly connected network [%s, ", inet_ntoa(network));
        Debug("%s] to the routing table\n", inet_ntoa(mask));
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
    Debug("\n-> RIP: Printing the forwarding table\n");
    print_routing_table(sr);
    /************************************************************************************/

    /* 
        Espera inicial de RIP_ADVERT_INTERVAL_SEC antes del primer envío.
        A continuación entra en un bucle infinito que, cada RIP_ADVERT_INTERVAL_SEC segundos,
        recorre la lista de interfaces (sr->if_list) y envía una respuesta RIP por cada una,
        utilizando la dirección de multicast definida (RIP_IP).
        Esto implementa el envío periódico de rutas (anuncios no solicitados) en RIPv2.
    */
    return NULL;
}

/* Chequea las rutas y marca las que expiran por timeout */
void* sr_rip_timeout_manager(void* arg) {
    struct sr_instance* sr = arg;
    
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
    return NULL;
}

/* Chequea las rutas marcadas como garbage collection y las elimina si expira el timer */
void* sr_rip_garbage_collection_manager(void* arg) {
    struct sr_instance* sr = arg;
    /*
        - Bucle infinito que espera 1 segundo entre comprobaciones.
        - Recorre la tabla de enrutamiento y elimina aquellas rutas que:
            * estén marcadas como inválidas (valid == 0) y
            * lleven más tiempo en garbage collection que RIP_GARBAGE_COLLECTION_SEC
              (current_time >= garbage_collection_time + RIP_GARBAGE_COLLECTION_SEC).
        - Si se detectan eliminaciones, se imprime la tabla.
        - Se debe usar el mutex rip_metadata_lock para proteger el acceso concurrente
          a la tabla de enrutamiento.
    */
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

