#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "elev.h"
#include "io.h"
#include "channels.h"
#include "queue.h"
#include "io_user.h"

int main () {

    // Initialize 'global' program variables
    int current_floor = -1;
    int last_floor = -1;
    int next_floor = -1;	
    int direction = DIRN_UP;		// Which direction to look for new orders

	clock_t start_time;				//Variable to keep track off stoptime
	int time_delay = 3*1000000;		//How long the elev will stop, in this case 3 seconds

    bool queue[N_FLOORS][N_BUTTONS];
    queue_init(queue);

    bool no_orders = true;

    elev_initialize_hardware(direction);

    // INITIALIZE LOOP
    while (1) {
	
	// Checks if currently on floor, if so stop motor and break out of loop
	// If not, elevator moves until it reaches a floor
	last_floor = elev_get_last_floor(last_floor);
	if (last_floor != -1) {
	    elev_set_motor_direction(DIRN_STOP);
	    printf("CHANGING TO IDLE STATE\n");
	    break;
		}
    }

    // PROGRAM LOOP
    while (1) {

	// STOP HANDLER - Stops elevator, clears queue, turns off all lights 
	// and opens door if currently on a floor
	// **********
	if (elev_get_stop_signal()) {
		elev_set_stop_lamp(1);
		if (next_floor == last_floor) {
		    direction *= -1;
		}
		if (!no_orders) {
			elev_stop_button_handler(current_floor);
			queue_init(queue);
			next_floor = -1;
			no_orders = true;
		}
		else if (current_floor != -1) {
			elev_set_door_open_lamp(1);	
		}
		elev_set_stop_lamp(0);
	}	
	
		
	// IDLE STATE - Do not move the elevator while there are no orders.
	// *********
	else if (no_orders) {


	    // Check for new orders for every iteration of the
	    // program loop while the program is in IDLE STATE.
	    // Break out of IDLE STATE once there are any orders.
	    if (queue_update(queue)) {
		
			io_user_set_ordered_lights(queue);
			elev_set_door_open_lamp(0);

			next_floor = queue_get_next_floor(queue, direction, last_floor);

			if (next_floor == last_floor) {
			    direction *= -1;
			} 
			direction = elev_set_direction(direction, next_floor, last_floor);	

			printf("next_floor: %d\n", next_floor);
			printf("last_floor: %d\n", last_floor);
			queue_print(queue);

			no_orders = false;
			printf("CHANGING TO RUNNING STATE\n");
	    }
	}

	// RUNNING STATE - Elevator is running
	// ***********
	else {

	    direction = elev_set_direction(direction, next_floor, last_floor);
	    // Update last_floor for every iteration of the
	    // program loop while the program is in RUNNING STATE.
	    current_floor = elev_get_floor_sensor_signal();

	    elev_set_floor_indicator(last_floor);

	    last_floor = elev_get_last_floor(last_floor);
	

	    // Look for new orders from users
	    // and update which order to exectute if the new order
	    // is better suited to the current direction and floor.
	    // (does not change directon or affect the engine)
	    if (queue_update(queue)) {
			io_user_set_ordered_lights(queue);

			next_floor = queue_get_next_floor(queue, direction, last_floor);
			printf("next_floor: %d\n", next_floor);
			printf("last_floor: %d\n", last_floor);
			queue_print(queue);
	    }

	    // Stop the elevator and open the doors for three
	    // seconds when a destination is reached,
	    // remove finished order from queue and go into IDLE STATE
	    // if the queue now is empty
	    if ((next_floor == last_floor)
		    && (next_floor == current_floor)) {
			
			elev_set_motor_direction(DIRN_STOP);
			io_user_clear_lights_on_floor(last_floor);			
			
			printf("Stopping at floor: %d\n", last_floor);
			
			elev_set_door_open_lamp(1);
			start_time = clock();
			while (clock() < start_time + time_delay) {
				if (queue_update(queue)) {
					io_user_set_ordered_lights(queue);
					io_user_clear_lights_on_floor(last_floor);			
				}
			}

			elev_set_door_open_lamp(0);
			queue_reset(queue, last_floor);

		if (queue_empty(queue)) {
		    printf("CHANGING TO IDLE STATE:\n");
		    printf("next_floor: %d\n", next_floor);
		    printf("last_floor: %d\n", last_floor);
		    queue_print(queue);

		    printf("DONE CHANGING TO IDLE STATE\n");
		    no_orders = true;
		}

		// Look for a new order to execute (with the current
		// motor direction), and start the engine if the queue
		// is not empty and if there exists such a new order.
		else {
		    next_floor = queue_get_next_floor(queue, direction, last_floor);
		    printf("next_floor: %d\n", next_floor);
		    printf("last_floor: %d\n", last_floor);
		    queue_print(queue);

		    if (next_floor != -1) {
			direction = elev_set_direction(direction, next_floor, last_floor);	
		    }
		}
	    }
	    
	    // Change direction, start engine and get new order if
	    // there are no more orders in the same direction as
	    // the elevator is currently moving.
	    if (next_floor == -1) {
			direction *= -1;
			next_floor = queue_get_next_floor(queue, direction, last_floor);
			direction = elev_set_direction(direction, next_floor, last_floor);
		
			printf("******\nCHANGING DIRECTION\n******\n");
			printf("next_floor: %d\n", next_floor);
			printf("last_floor: %d\n", last_floor);
			queue_print(queue);
	    }
	}
	}
    return 0;
}
