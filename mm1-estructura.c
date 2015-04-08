/* External definitions for single-server queueing system. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "lcgrand.h"  /* Header file for random-number generator. */

#define Q_LIMIT 100  /* Limit on queue length. */
#define BUSY      1  /* Mnemonics for server's being busy */
#define IDLE      0  /* and idle. */

void  initialize(void);
void  timing(void);
void  arrive(void);
void  depart(void);
void  report(void);
void  update_time_avg_stats(void);
float expon(float mean);
float gen_next_interarrive(void);
float gen_next_service_time(void);
float  gen_next_depart(void);
int next(float x, float y);
void encolar(float tiempo_servicio, float tiempo_entrada);
void desencolar(void);

float num_random; /*variable utilizada para almacenar numero random*/
float num_random2;
float sumatoria_tiempo_en_cola;
float sim_time;
float num_custs_delayed;

float num_delays_required;
float total_of_delays;
float area_num_in_q;
float area_server_status;

float queue[100][2];
float time_next_arrive;
float time_next_depart;

int next_event_type;
int server_status; /*--estado del/los servidor/es BUSY|IDLE--*/
int num_in_q; /*--nuemero de clientes en cola--*/
float time_last_event; /*--tiempo del ultimo evento--*/

main()  /* Main function. */
{

    /* Initialize the simulation. */
    initialize();

    /* Run the simulation while more delays are still needed. */
  
    /*Num.clientes atendidos <  Num.clientes requeridos*/
    while (num_custs_delayed < num_delays_required) { 
       
        /* Determine the next event. */
        timing();
        /* Update time-average statistical accumulators. */

        update_time_avg_stats();

        /* Invoke the appropriate event function. */

        switch (next_event_type) {
            case 1:
                printf("Arribo en tiempo: %2f \n", sim_time);
                arrive();
                break;
            case 2:
                printf("Salida en tiempo: %2f \n", sim_time);
                depart();
                break;
        }
        printf("Numero de clientes atendidos: %2f \n", num_custs_delayed);
    }

    /* Invoke the report generator and end the simulation. */
    report(); 

    return 0;
}


void initialize(void)  /* Initialization function. */ /*--inicializacion de las variables de la simulacion--*/
{
    /* Initialize the simulation clock. */

    sumatoria_tiempo_en_cola = 0.0;
    sim_time = 0.0; /*--reloj de la simulacion--*/
    num_delays_required = 100;

    /* Initialize the state variables. */

    server_status   = IDLE; /*--estado del/los servidor/es BUSY|IDLE--*/
    num_in_q        = 0; /*--nuemero de clientes en cola--*/
    time_last_event = 0.0; /*--tiempo del ultimo evento--*/

    /* Initialize the statistical counters. num_custs_delayed, total_of_delays, area_num_in_q, area_server_status */
    num_custs_delayed = 0;
    total_of_delays = 0.0;
    area_num_in_q = 0.0;
    area_server_status = 0.0;

    /* Initialize event list.  Since no customers are present, the departure */
    /* (service completion) event is eliminated from consideration. */
    /*arreglos donde se guarda en el primer elemento el tiempo de la proximo arribo y en el segundo */
    /*elemento el tiempo de la proxima salida --se podria hacer con 2 variables --*/
    time_next_arrive = sim_time + gen_next_interarrive() ; /* Planificacion del primer arribo */
    time_next_depart = FLT_MAX; /* infinito --si tengo multiples servidores deberia tener una lista del mismo # de servidores para la salida */
}


void timing(void)  /* Timing function. */ /*--Se fija el minimo entre el proximo tiempo de entrada y el proximo tiempo de salida--*/
{
    int   i;
    float min_time_next_event = 1.0e+29;

    next_event_type = 0; /* Al final debe quedar en 1 en caso de que el proximo evento sea un arribo, 2: si es la salida de un servicio */

    /* Determine the event type of the next event to occur. */
    /* printf("La proxima entrada es %2f \n",time_next_event[1]);
    printf("La proxima salida es %2f \n",time_next_event[2]);*/

    next_event_type = next(time_next_arrive,time_next_depart);

    /* advance the simulation clock. */
    if(next_event_type == 1){
        sim_time = time_next_arrive;
    }else{
        sim_time = time_next_depart;
    }
}


void arrive(void)  /* Arrival event function. */
{
    float delay;

    /* Schedule next arrival. */

    time_next_arrive = sim_time + gen_next_interarrive();

    /* Check to see whether server is busy. */

    if (server_status == BUSY) {

        /* Server is busy, so increment number of customers in queue. */
        /* Check to see whether an overflow condition exists. */
        /* Guardar el tiempo de arribo de esta entidad para los calculos estadisticos */ 
        /*es para calcular el tiempo que espero en cola*/

        encolar(gen_next_service_time(),sim_time);
    }
    else {

        /* Server libre, tener en  cuenta la entidad que pasa directamente al server para los calculos estadisticos */
        server_status = BUSY;

        /* Schedule a departure (service completion). */

        time_next_depart = sim_time + gen_next_service_time(); /* generar con una funcion (con una distribucion estadistica) el tiempo de servicio */
    }
}


void depart(void)  /* Departure event function. */
{
    int   i;
    float delay;

    /* Check to see whether the queue is empty. */

    if (num_in_q == 0) {

        /* The queue is empty so make the server idle and eliminate the */
        /* departure (service completion) event from consideration. */
        server_status = IDLE; 
    }
    else {

        /* The queue is nonempty, so decrement the number of customers in
           queue. */ 

        /* Compute the delay of the customer who is beginning service and update
           the total delay accumulator. */
        desencolar();     
        /* Increment the number of customers delayed, and schedule departure. */
        num_custs_delayed = num_custs_delayed + 1;
    }
}


void report(void)  /* Report generator function. */
{
    /* Compute and write estimates of desired measures of performance. */
    printf("*** Reporte de Estadisticos ****\n");
    
    /*Average delay in queue*/ 
    printf("Demora Promedio en cola: %2f \n", sumatoria_tiempo_en_cola/num_custs_delayed);

    /*Average number in queue*/ 
    printf("Numero promedio en cola: \n" );

    /*Server utilization*/
    printf("Utilizacion del servidor: \n");
}


void update_time_avg_stats(void)  /* Update area accumulators for time-average statistics. */
{
    float time_since_last_event;

    /* Compute time since last event, and update last-event-time marker. */

    time_since_last_event = sim_time - time_last_event;
    time_last_event       = sim_time;

    /* Update area under number-in-queue function. */

    /* Update area under server-busy indicator function. */
}


float gen_next_interarrive()  
{
    /*
    Descripcion: Cada vez que es invocada la funcion "gen_next_interarrive()"
    me retorna como resultado un interarrivo dependiendo del num_random generado.
    Aclaración: No fue posible realizar un switch por que no trabaja con tipo float.
    */

   num_random = 0.0; /*inicialización de variable*/
   num_random = lcgrand(1); /*generacion de random*/

   /*Casos segun tabla del Ejercicio 3 - Pract. 1*/
   if( 0.0 <= num_random && num_random <= 0.2 )
   {
        return 15.0;
   }
   else if( 0.2 < num_random && num_random <= 0.35)
   {
        return 20.0;
   }
   else if( 0.35 < num_random && num_random <= 0.55 )
   {
        return 25.0;
   }
   else if (0.55 < num_random && num_random <= 0.7)
   {
        return 30.0;
   }
   else if(0.7 < num_random && num_random <= 1)
   {
        return 35.0;
   }
   else if (num_random > 1.0)
   {
        exit(2); 
        printf("Error!, pues las probabilidades van de 0 a 1 \n");
   }
}

float  gen_next_depart()
{
  return 2.0;
}

float gen_next_service_time()
{
   num_random2 = 0.0; /*inicialización de variable*/
   num_random2 = lcgrand(1); /*generacion de random*/

   /*Casos segun tabla del Ejercicio 3 - Pract. 1*/
   if( 0.0 <= num_random2 && num_random2 <= 0.2 )
   {
        return 15.0;
   }
   else if( 0.2 < num_random2 && num_random2 <= 0.55)
   {
        return 20.0;
   }
   else if( 0.55 < num_random2 && num_random2 <= 0.7 )
   {
        return 25.0;
   }
   else if (0.7 < num_random2 && num_random2 <= 1.0)
   {
        return 30.0;
   }
   else if (num_random2 > 1.0)
   {
        exit(2); 
        printf("Error!, pues las probabilidades van de 0 a 1 \n");
   }
}

int next(float x, float y)
{
    if(x<=y)
    {
       return 1;
    }else{
       return 2;
    }
}

void encolar (float tiempo_servicio, float tiempo_entrada)
{
   num_in_q = num_in_q + 1.0; 
   if (num_in_q > Q_LIMIT) {
            exit(2); /* cuando no tengo mas lugar para encolar clientes termina la simulacion */
        }else{
            queue[num_in_q][0]= gen_next_service_time();
            queue[num_in_q][1]= sim_time;
    } 
}

void desencolar (void)
{
    if (num_in_q > 0)
    {
        time_next_depart = sim_time + queue[1] [1];
        sumatoria_tiempo_en_cola += (sim_time - queue[1][2]); /*Sumo el tiempo que estuvo en cola*/
        int i;
        for(i=0; i<num_in_q; i++)
        {
            queue[i][0]= queue[i+1][0];
            queue[i][1]= queue[i+1][1];
        }
        num_in_q = num_in_q -1; 
    }   
}