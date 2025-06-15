// Authors Andrew Talviste, Dema Gorkun, Zak Smith
// Assignment: Assignment 2 P2P Nodes

#include "sysio.h"
#include "ser.h"
#include "serf.h"

#include "phys_cc1350.h"
#include "plug_null.h"
#include "tcv.h"

// Message types
#define MSG_DISCOVERY_REQUEST       (0)
#define MSG_DISCOVERY_RESPONSE      (1)
#define MSG_CREATE_RECORD           (2)
#define MSG_DELETE_RECORD           (3)
#define MSG_RETRIEVE_RECORD         (4)
#define MSG_RESPONSE                (5)

// Response status codes
// Dunno why they are hex but spec calls for it
#define STATUS_SUCCESS           (0x01) // operation has been performed successfully
#define STATUS_DB_FULL           (0x02) // record can’t be added to the DB as it is full
#define STATUS_DELETE_FAIL       (0x03) // delete operation fails as DB is already empty
#define STATUS_RETRIEVE_FAIL     (0x04) // retrieve operation as DB entry is empty (no record stored)

#define MAX_PACKET_SIZE 	      (250) // Bytes
#define MAX_DATABASE_SIZE 	       (40) // Maximum 40 Nodes in the DB
#define MAX_RECORD_SIZE 	       (20) // Maximum 20 byte record
#define DEFAULT_NETWORK_ID          (0) // Always 0 in this assignment

// Discovery Request Message:
// Sent by the node to discover and obtain
// the list of reachable neighbours that
// belong to the node group.
typedef struct disc_msg {
    sint groupID;                       // The sender node groupID
    byte type;                          // Always set to ZERO (MSG_DISCOVERY_REQUEST)
    byte requestNum;                    // Randomly generated every time message is sent
    byte senderID;                      // Originator nodeID for this message
    byte receiverID;                    // Always set to ZERO
} __attribute__((packed)) disc_msg_t;

// Discovery Response Message:
// Sent byt he node upon receiving
// a discovery request from another node
typedef struct disc_resp {
    sint groupID;                       // The sender node groupID
    byte type;                          // Always set to ONE (MSG_DISCOVERY_RESPONSE)
    byte requestNum;                    // should be equal to the request number recieved in the disc
    byte senderID;                      // Originator nodeID for this message
    byte receiverID;                    // ID of the node that originally sent the discovery request
} __attribute__((packed)) disc_resp_t;

// Create Record Message:
// Sent when a sender node wishes to create
// and store a record in another node
typedef struct create_msg {
    sint groupID;                       // The sender node groupID
    byte type;                          // Always set to TWO (MSG_CREATE_RECORD)
    byte requestNum;                    // Randomly generated every time message is sent
    byte senderID;                      // Originator nodeID for this message
    byte receiverID;                    // ID of the destination node
    char record[MAX_RECORD_SIZE];       // The string that the sender wishes to send
} __attribute__((packed)) create_msg;

// Delete Record Message:
// sent when a sender node wishes to delete
// a record stored in another node
typedef struct delete_record_msg {
    sint groupID;                       // The sender node groupID
    byte type;                          // Always set to THREE (MSG_DELETE_RECORD)
    byte RequestNum;                    // Randomly generated every time message is sent
    byte senderID;                      // The originator node ID for this message
    byte receiverID;                    // The ID of the receiver node
    char recordIndex;                   // The record index to be deleted at destination
    byte padding;                       // Not used (or is it???)
} __attribute__((packed)) delete_msg_t;

// Retrieve Record Message:
// sent when a sender node wishes to retrieve
//  a record stored in another node
typedef struct retrieve_msg {
    sint groupID;                       // The sender node groupID
    byte type;                          // Always set to FOUR (MSG_RETRIEVE_RECORD)
    byte requestNum;                    // Randomly generated every time message is sent
    byte senderID;                      // The originator node ID for this message
    byte receiverID;                    // The ID of the receiver node
    byte recordIndex;                   // The record index to be retrieved from destination
    byte padding;                       // Not used (or is it???)
} __attribute__((packed)) retrieve_msg_t;

// Retrieve Response Message:
// Formed and sent by a node after receiving a request
// (Create Record msg, Delete Record msg or Retrieve Record msg)
// and after performing the corresponding action
typedef struct response_msg {
    sint groupID;                       // The sender node groupID
    byte type;                          // Always set to FIVE (MSG_RESPONSE)
    byte requestNum;                    // The request number of the original request message
    byte senderID;                      // The originator nodeID for this message
    byte receiverID;                    // The ID of the node that sent the corresponding
    byte status;                        // Indicates result of operation success/failure and type of failure
    byte padding;                       // Not used (or is it???)
    char record[MAX_RECORD_SIZE];       // Only filled if request was a retrieve_msg_t and index has stored data
} __attribute__((packed)) response_msg_t;

typedef struct packet {

} __attribute__((packed)) packet_t;

struct Node {
	sint groupID;
	byte nodeID;
};

struct DB_Entry {
	struct Node *aNode;
	int timeStamp;
	char record[MAX_RECORD_SIZE];
};

/**************************************************/
/* Global Variables for this particular node      */
int databaseSize = 0;									// current DB size
struct DB_Entry nodeDatabase[MAX_DATABASE_SIZE];        // the DB that holds other node information
struct Node thisNode = {0, 0};		  				    // The data for this particular node
char menu_choice = ' ';                                 // user selection in UART
/**************************************************/


fsm root {
	state PRINT_MENU:
		ser_outf(PRINT_MENU,
			"\r\nGroup %d Device #%c (%d/%d records)\r\n"
			"(G)roup ID\r\n"
			"(N)ew device ID\r\n"
			"(C)reate record on neighbor\r\n"
			"(D)elete record on neighbor\r\n"
			"(R)etrieve record from neighbor\r\n"
			"(S)how local records\r\n"
			"R(e)set local storage\r\n\r\n"
			"Selection: ",
			thisNode.groupID, thisNode.nodeID, databaseSize, MAX_DATABASE_SIZE
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
			nodeDatabase[i].aNode->groupID = 0;
			nodeDatabase[i].aNode->nodeID = 0;
			nodeDatabase[i].timeStamp = 0;

			for (int j = 0; j < MAX_RECORD_SIZE; j++){
				nodeDatabase[i].record[j] = '\0';
			}
		}

		databaseSize = 0;

		proceed PRINT_MENU;

}
