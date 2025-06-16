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
#define STATUS_DB_FULL           (0x02) // record can't be added to the DB as it is full
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

// RF communication
int sfd = -1;  // Session file descriptor for RF

// Discovery globals
#define MAX_NEIGHBORS 25
byte neighbor_list[MAX_NEIGHBORS];
int neighbor_count = 0;
int discovery_round = 0;
byte current_request_num = 0;
int create_target = 0;  // Used to hold neighbors node ID between the two FSM states (will persist across transitions)
int create_request_num = 0; // Used for 3s delay in neighbour ID & DATA states
/**************************************************/

// Helper function to get next request number
byte get_next_request_num() {
    return ++current_request_num;
}

// Basic RF sending function
void send_message(void* msg, int size) {
    address packet;
    
    // Make sure size is even (CC1350 requirement)
    if (size & 1) size++;
    
    packet = tcv_wnp(WNONE, sfd, size + 4); // +4 for network ID and CRC
    if (packet != NULL) {
        packet[0] = DEFAULT_NETWORK_ID; // Network ID = 0
        // Copy message after network ID
        memcpy((char*)(packet + 1), msg, size);
        tcv_endp(packet);
    }
}

// ============================================================================
// DISCOVERY FSM
// ============================================================================

fsm discovery {
    disc_msg_t disco_msg;
    address rpkt;
    disc_resp_t *resp_msg;
    int i;
    bool already_found;
    
    state INIT:
        // Reset neighbor list for fresh discovery
        neighbor_count = 0;
        discovery_round = 0;
        
        ser_out(INIT, "\r\nStarting neighbor discovery...\r\n");
        proceed SEND_DISCOVERY;
        
    state SEND_DISCOVERY:
        // Build discovery request message
        disco_msg.groupID = thisNode.groupID;
        disco_msg.type = MSG_DISCOVERY_REQUEST;  // 0
        disco_msg.requestNum = get_next_request_num();
        disco_msg.senderID = thisNode.nodeID;
        disco_msg.receiverID = 0;  // Broadcast
        
        // Send the discovery request
        send_message(&disco_msg, sizeof(disco_msg));
        
        ser_outf(SEND_DISCOVERY, "Sent discovery request #%d (round %d)\r\n", 
                disco_msg.requestNum, discovery_round + 1);
        
        // Start 3-second timer and wait for responses
        delay(3000, DISCOVERY_TIMEOUT);
        proceed WAIT_RESPONSES;
        
    state WAIT_RESPONSES:
        // Wait for either a response message or timeout
        when(RECEIVE, HANDLE_RESPONSE);
        when(DISCOVERY_TIMEOUT, CHECK_ROUND);
        
    state HANDLE_RESPONSE:
        // Get the received packet
        rpkt = tcv_rnp(HANDLE_RESPONSE, sfd);
        
        if (rpkt == NULL) {
            proceed WAIT_RESPONSES;
        }
        
        // Check if it's a discovery response for us
        resp_msg = (disc_resp_t*)(rpkt + 1);
        
        // Validate this is a discovery response for us
        if (resp_msg->type == MSG_DISCOVERY_RESPONSE &&          // Type 1
            resp_msg->groupID == thisNode.groupID &&             // Same group
            resp_msg->requestNum == disco_msg.requestNum &&      // Our request
            resp_msg->receiverID == thisNode.nodeID) {           // Addressed to us
            
            // Check if we already found this neighbor
            already_found = false;
            for (i = 0; i < neighbor_count; i++) {
                if (neighbor_list[i] == resp_msg->senderID) {
                    already_found = true;
                    break;
                }
            }
            
            // Add to neighbor list if new and we have space
            if (!already_found && neighbor_count < MAX_NEIGHBORS) {
                neighbor_list[neighbor_count] = resp_msg->senderID;
                neighbor_count++;
                
                ser_outf(HANDLE_RESPONSE, "Found neighbor: Node %d\r\n", 
                        resp_msg->senderID);
            }
        }
        
        // Clean up the packet
        tcv_endp(rpkt);
        proceed WAIT_RESPONSES;
        
    state DISCOVERY_TIMEOUT:
        ser_outf(DISCOVERY_TIMEOUT, "Discovery round %d complete\r\n", 
                discovery_round + 1);
        proceed CHECK_ROUND;
        
    state CHECK_ROUND:
        discovery_round++;
        
        if (discovery_round < 2) {
            // Do second round
            ser_out(CHECK_ROUND, "Starting second discovery round...\r\n");
            proceed SEND_DISCOVERY;
        } else {
            // Done with both rounds
            proceed DISPLAY_NEIGHBORS;
        }
        
    state DISPLAY_NEIGHBORS:
        // Display results according to assignment format
        ser_out(DISPLAY_NEIGHBORS, "\r\nNeighbors: ");
        
        if (neighbor_count == 0) {
            ser_out(DISPLAY_NEIGHBORS, "None found");
        } else {
            for (i = 0; i < neighbor_count; i++) {
                ser_outf(DISPLAY_NEIGHBORS, "%d", neighbor_list[i]);
                if (i < neighbor_count - 1) {
                    ser_out(DISPLAY_NEIGHBORS, " ");
                }
            }
        }
        
        ser_out(DISPLAY_NEIGHBORS, "\r\n");
        
        ser_outf(DISPLAY_NEIGHBORS, "Discovery complete. Found %d neighbors.\r\n", 
                neighbor_count);
        
        finish; // End discovery FSM
}

// ============================================================================
// RECEIVER FSM
// ============================================================================

fsm receiver {
    address rpkt;
    disc_msg_t *req_msg;
    disc_resp_t resp_msg;
    
    state LISTEN:
        rpkt = tcv_rnp(LISTEN, sfd);
        
        if (rpkt == NULL) {
            proceed LISTEN;
        }
        
        // Get message and check if it's a discovery request
        req_msg = (disc_msg_t*)(rpkt + 1);
        
        if (req_msg->type == MSG_DISCOVERY_REQUEST) {
            proceed HANDLE_DISCOVERY_REQ;
        } else {
            // Not a discovery request, ignore for now
            tcv_endp(rpkt);
            proceed LISTEN;
        }
        
    state HANDLE_DISCOVERY_REQ:
        // Validate it's for our group and not from ourselves
        if (req_msg->groupID == thisNode.groupID && 
            req_msg->senderID != thisNode.nodeID) {
            
            // Build discovery response
            resp_msg.groupID = thisNode.groupID;
            resp_msg.type = MSG_DISCOVERY_RESPONSE;  // 1
            resp_msg.requestNum = req_msg->requestNum;  // Copy from request
            resp_msg.senderID = thisNode.nodeID;
            resp_msg.receiverID = req_msg->senderID;  // Send back to requester
            
            // Send the response
            send_message(&resp_msg, sizeof(resp_msg));
            
            ser_outf(HANDLE_DISCOVERY_REQ, 
                    "Responded to discovery from Node %d\r\n", 
                    req_msg->senderID);
        }
        
        tcv_endp(rpkt);
        proceed LISTEN;
}

// ============================================================================
// MAIN ROOT FSM
// ============================================================================

fsm root {
    sint temp_value;
    
    state INIT:
        // Initialize RF communication
        phys_cc1350(0, MAX_PACKET_SIZE);
        tcv_plug(0, &plug_null);
        sfd = tcv_open(WNONE, 0, 0);
        tcv_control(sfd, PHYSOPT_ON, NULL);
        
        if (sfd < 0) {
            diag("Cannot open RF interface");
            halt();
        }
        
        // Start the receiver FSM
        runfsm receiver;
        
        // Set default node values if not set
        if (thisNode.groupID == 0) thisNode.groupID = 1;
        if (thisNode.nodeID == 0) thisNode.nodeID = 1;
        
        ser_out(INIT, "RF initialized successfully\r\n");
        proceed PRINT_MENU;

	state PRINT_MENU:
		ser_outf(PRINT_MENU,
			"\r\nGroup %d Device #%d (%d/%d records)\r\n"
			"(G)roup ID\r\n"
			"(N)ew device ID\r\n"
			"(F)ind neighbors\r\n"
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
				ser_out(PARSE_CHOICE,"Enter new group ID: ");
				proceed UPDATE_GROUP_ID;
				
			case 'N': case 'n': // (N)ode ID
				ser_out(PARSE_CHOICE,"Enter new node ID: ");
				proceed UPDATE_NODE_ID;

			case 'F': case 'f': // (F)ind Protocol
				runfsm discovery;  // Start the discovery FSM
				proceed PRINT_MENU;

			case 'C': case 'c': // (C)reate Protocol
				ser_out(PARSE_CHOICE,"Enter new node ID: ");
				proceed CREATE_NEIGHBOUR_RECORD_ID;

			case 'D': case 'd': // (D)elete Protocol
				ser_out(PARSE_CHOICE,"Enter new node ID: ");
				proceed DELETE_NEIGHBOUR_RECORD;

			case 'R': case 'r': // (R)etrieve Protocol
				ser_out(PARSE_CHOICE,"Enter destination node ID and record Index to be retrieved: ");
				proceed RETREIVE_RECORD;

			case 'S': case 's': // (S)how Local Records
				proceed SHOW_LOCAL_RECORDS;

		    case 'E': case 'e': // R(e)set Local Records
		    	proceed RESET_LOCAL_RECORDS;

		    default:
		    	proceed PRINT_MENU;
		}

	state UPDATE_GROUP_ID:
		ser_inf(UPDATE_GROUP_ID, "%d", &temp_value);
		thisNode.groupID = temp_value;
		ser_outf(UPDATE_GROUP_ID, "Group ID updated to %d\r\n", thisNode.groupID);
		proceed PRINT_MENU;

	state UPDATE_NODE_ID:
		ser_inf(UPDATE_NODE_ID, "%d", &temp_value);
		thisNode.nodeID = (byte)temp_value;
		ser_outf(UPDATE_NODE_ID, "Node ID updated to %d\r\n", thisNode.nodeID);
		proceed PRINT_MENU;

	state FIND_NEIGHBOURS:
		// This is now handled by the discovery FSM
		proceed PRINT_MENU;

	state CREATE_NEIGHBOUR_RECORD:
        // Declare a new FSM state for reading the nodeID
        int temp;

        ser_inf(CREATE_NEIGHBOUR_RECORD_ID, "%d", &temp); // Block until user enters number & hits enter -> parse it into temp
        create_target = temp; 
        ser_out(CREATE_NEIGHBOUR_RECORD_ID, "Enter record, maximum %d characters: ", MAX_RECORD_SIZE); // Asks for actual record data (curently shows buffer limit)
        proceed CREATE_NEIGHBOUR_RECORD_DATA; 

    state CREATE_NEIGHBOUR_RECORD_DATA: {
        char buffer[MAX_RECORD_SIZE]; 
        ser_inf(CREATE_NEIGHBOUR_RECORD_DATA, "%s", buffer);
        
        create_msg md; 
        md.groupID = thisNode.groupID; // Attaches packet with nodes groupID
        md.type = MSG_CREATE_RECORD; // Type = 2 
        md.requestNum = get_next_request_num(); // Asks helper function for new # for matching
        md.senderID = thisNode.nodeID; // Marks it as coming from own node
        md.receiverID = create_target; // Directs the packet to the nieghbor ID we saved previously
        create_request_num = cm.requestNum;
        send_message(&cm, sizeof(cm));

        // 3s timer to create response timeout
        delay(3000, CREATE_RESPONSE_TIMEOUT); 
        when(RECEIVE, HANDLE_CREATE_RESPONSE); // If packet arrives first we move to handle create response
}
    // Handle repsonse (if received within 3s window)
    state HANDLE_CREATE_RESPONSE: {
        address rpkt = tcv_rnp(HANDLE_CREATE_RESPONSE, sfd); 
        if (!rpkt) {proceed CREATE_NEIGHBOUR_RECORD_DATA;}  // No packet yet
        response_msg_t *resp = (response_msg_t*)(rpkt + 1);

        // Only handle our own reply
        if (resp->type == MSG_RESPONSE // only handle packets that are type 5 and match our request number
            && resp->requestNum == create_request_num) {
            if (resp->status == STATUS_SUCCESS)
                ser_out(HANDLE_CREATE_RESPONSE, "Record created.\r\n");
            else
                ser_outf(HANDLE_CREATE_RESPONSE,
                        "Create failed (0x%02X)\r\n",
                        resp->status);
            tcv_endp(rpkt); // Frees up the packet buffer
            proceed PRINT_MENU;
        }

        // Otherwise drop and keep listening
        tcv_endp(rpkt);
        proceed CREATE_NEIGHBOUR_RECORD_DATA;
    }

    state CREATE_RESPONSE_TIMEOUT:
        ser_out(CREATE_RESPONSE_TIMEOUT,
                "Create record timed out after 3 s\r\n");
        proceed PRINT_MENU;

	state DELETE_NEIGHBOUR_RECORD:
		// TODO: Implement delete record protocol
		proceed PRINT_MENU;

	state RETREIVE_RECORD:
		// TODO: Implement retrieve record protocol
		proceed PRINT_MENU;

	state SHOW_LOCAL_RECORDS:
		ser_outf(SHOW_LOCAL_RECORDS, "Index\tTime Stamp\towner ID\tRecord Data\r\n");

		if (databaseSize > 0) {
			for (int index = 0; index < databaseSize; index++){
				if (nodeDatabase[index].aNode != NULL){  // It shouldn't be invalid, but why not be safe
					ser_outf(SHOW_LOCAL_RECORDS, "%d\t%d\t%d\t%s\r\n",
											index,
											nodeDatabase[index].timeStamp,
											nodeDatabase[index].aNode->nodeID,
										    nodeDatabase[index].record
											);
				}
			}
		} else {
			ser_out(SHOW_LOCAL_RECORDS, "No records stored.\r\n");
		}

		proceed PRINT_MENU;

	state RESET_LOCAL_RECORDS:
		// Clear all database entries safely
		for (int i = 0; i < databaseSize; i++){
			if (nodeDatabase[i].aNode != NULL) {
				ufree(nodeDatabase[i].aNode);
				nodeDatabase[i].aNode = NULL;
			}
			nodeDatabase[i].timeStamp = 0;
			memset(nodeDatabase[i].record, 0, MAX_RECORD_SIZE);
		}
		databaseSize = 0;
		
		ser_out(RESET_LOCAL_RECORDS, "All records cleared.\r\n");
		proceed PRINT_MENU;
}
