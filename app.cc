// Author Andrew Talviste
// Student ID: 1698001
// Class: CMPT 464
// Assignment: Lab #6

#include "sysio.h"
#include "ser.h"
#include "serf.h"

char menu_choice = ' ';

fsm root {
	state PRINT_MENU:
		ser_out(PRINT_MENU, 
			"\r\nGroup [node group ID] Device #[node id] ([number of stored records]/[maximum number of records] records)\r\n"
			"(G)roup ID\r\n"
			"(N)ew device ID\r\n"
			"(C)reate record on neighbor\r\n"
			"(D)elete record on neighbor\r\n"
			"(R)etrieve record from neighbor\r\n"
			"(S)how local records\r\n"
			"R(e)set local storage\r\n\r\n"
			"Selection: ");		
		proceed MENU_SELECT;

	state MENU_SELECT:
		ser_inf(MENU_SELECT, "%c", &menu_choice);
		proceed PARSE_CHOICE;

	state PARSE_CHOICE:
		switch(menu_choice){
			case 'G': case 'g': // (G)roup ID
				ser_out(PRINT_MENU,"Enter new group ID: ");

				proceed UPDATE_GROUP_ID;
			case 'N': case 'n': // (N)ode ID
				ser_out(PRINT_MENU,"Enter new node ID: ");

				proceed UPDATE_NODE_ID;	
			
			case 'F': case 'f': // (F)ind Protocol
				// Do this first
				// And do the receiver side first
				proceed FIND_NEIGHBOURS;
			
			case 'C': case 'c': // (C)reate Protocol
				ser_out(PRINT_MENU,"Enter new node ID: ");

				proceed CREATE_NEIGHBOUR_RECORD;
			
			case 'D': case 'd': // (D)elete Protocol
				ser_out(PRINT_MENU,"Enter new node ID: ");

				proceed DELETE_NEIGHBOUR_RECORD;
			
			case 'R': case 'r': // (R)etrieve Protocol
				ser_out(PRINT_MENU,"Enter destination node ID and record Index to be retreived: ");
				
				proceed RETREIVE_RECORD;
			
			case 'S': case 's': // (S)how Local Records

				proceed SHOW_LOCAL_RECORDS;
		    
		    case 'E': case 'e': // R(e)set Local Records

		    	proceed RESET_LOCAL_RECORDS;

		    default:

		    	proceed PRINT_MENU;

		}

	state UPDATE_GROUP_ID:

		proceed PRINT_MENU;

	state UPDATE_NODE_ID:

		proceed PRINT_MENU;

	state FIND_NEIGHBOURS:

		proceed PRINT_MENU;

	state CREATE_NEIGHBOUR_RECORD:

		proceed PRINT_MENU;

	state DELETE_NEIGHBOUR_RECORD:

		proceed PRINT_MENU;
	
	state RETREIVE_RECORD:

		proceed PRINT_MENU;

	state SHOW_LOCAL_RECORDS:

		proceed PRINT_MENU;

	state RESET_LOCAL_RECORDS:

		proceed PRINT_MENU;

}
