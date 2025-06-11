// Author Andrew Talviste
// Student ID: 1698001
// Class: CMPT 464
// Assignment: Lab #6

#include "sysio.h"
#include "ser.h"
#include "serf.h"

#define MAX_PACKET_SIZE 	(250) // Bytes
#define MAX_DATABASE_SIZE 	 (40) // Maximum 40 Nodes in the DB
#define MAX_RECORD_SIZE 	 (20) // Maximum 20 byte record
#define DEFAULT_NETWORK_ID    (0) // Always 0 in this assignment

struct Node {
	char groupID[2];
	char nodeID;
};

struct DB_Entry {
	struct Node *aNode;
	int timeStamp;
	char record[MAX_RECORD_SIZE];
};


int databaseSize = 0;									// current DB size
struct DB_Entry nodeDatabase[MAX_DATABASE_SIZE];       // the DB that holds other node information

struct Node thisNode = {"00", ' '};		  						// The data for this particular node
//thisNode.groupID[0] = '0';  // 
//thisNode.groupID[1] = '1';  // Set to '1' for now (need to confirm with instructor)
//thisNode.nodeID     = '1';  // Set to '1' for now (need to confirm with instructor)

char menu_choice = ' ';

fsm root {
	state PRINT_MENU:
		ser_outf(PRINT_MENU, 
			"\r\nGroup %c%c Device #%c (%d/%d records)\r\n"
			"(G)roup ID\r\n"
			"(N)ew device ID\r\n"
			"(C)reate record on neighbor\r\n"
			"(D)elete record on neighbor\r\n"
			"(R)etrieve record from neighbor\r\n"
			"(S)how local records\r\n"
			"R(e)set local storage\r\n\r\n"
			"Selection: ",
			thisNode.groupID[0], thisNode.groupID[1], thisNode.nodeID, databaseSize, MAX_DATABASE_SIZE
			);		
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
				ser_out(PRINT_MENU,"Enter destination node ID and record Index to be retrieved: ");
				
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
			ser_outf(SHOW_LOCAL_RECORDS, "Index\t Time Stamp\t owner ID\t Record Data\r\n");
			
			if ( (databaseSize >= 0) && (databaseSize <= MAX_DATABASE_SIZE) ) {
				for (int index = 0; index < databaseSize; index++){
					if (nodeDatabase[index].aNode != NULL){  // It shouldn't be invalid, but why not be safe
						int curNodeID = nodeDatabase[index].aNode->nodeID;
						int timeStamp = nodeDatabase[index].timeStamp;

						char *record = nodeDatabase[index].record;

						ser_outf(SHOW_LOCAL_RECORDS, "%d, %d, %d, %s\n\r",
												index,
												nodeDatabase[index].aNode->nodeID - '0',
												nodeDatabase[index].timeStamp,
											    nodeDatabase[index].record
												);
					}
				}
			}

		proceed PRINT_MENU;

	state RESET_LOCAL_RECORDS:

		for (int i = 0; i < databaseSize; i++){
			// delete stuff.
			nodeDatabase[i].aNode->groupID[0] = ' ';
			nodeDatabase[i].aNode->groupID[1] = ' ';
			nodeDatabase[i].aNode->nodeID = '0';
			nodeDatabase[i].timeStamp = 0;
			
			for (int j = 0; j < MAX_RECORD_SIZE; j++){
				nodeDatabase[i].record[j] = '\0';
			}
		}

		databaseSize = 0;

		proceed PRINT_MENU;

}
