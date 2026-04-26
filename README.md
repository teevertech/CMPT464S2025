# P2P Embedded Node Network

A peer-to-peer embedded systems project built in C for CC1350 wireless nodes using PicOS-style finite state machines. The project implements wireless node discovery and defines a message protocol for creating, deleting, and retrieving records between neighbouring embedded devices.

## Authors

- Andrew Talviste
- Dema Gorkun
- Zak Smith

## Project Overview

This project was developed for an embedded systems assignment focused on peer-to-peer communication between wireless nodes. Each device can be assigned a group ID and node ID, discover nearby nodes in the same group, and manage local record storage through a UART-based menu.

The system uses custom packet structures, RF communication, and finite state machines to organize node discovery, message handling, and user interaction.

## Tech Stack

- C
- PicOS-style finite state machines
- CC1350 wireless embedded nodes
- UART serial interface
- RF communication using `phys_cc1350`, `tcv`, and `plug_null`

## Features

- UART-based menu for interacting with the node
- Configurable group ID and node ID
- Wireless neighbour discovery
- Discovery request and response messages
- Two-round discovery process with timeout handling
- Local database structure for storing node records
- Local record display and reset functionality
- Custom message types for future create, delete, retrieve, and response operations

## Message Protocol

The project defines several custom message types:

- `MSG_DISCOVERY_REQUEST`
- `MSG_DISCOVERY_RESPONSE`
- `MSG_CREATE_RECORD`
- `MSG_DELETE_RECORD`
- `MSG_RETRIEVE_RECORD`
- `MSG_RESPONSE`

It also defines response status codes for success, database full errors, delete failures, and retrieve failures.

## Core Systems

### Node Discovery

The discovery system broadcasts a discovery request to nearby nodes in the same group. Nodes that receive a valid discovery request respond with their node ID. The requesting node collects unique responses and displays the discovered neighbours through the UART interface.

### Receiver FSM

A receiver finite state machine continuously listens for incoming RF packets. When a discovery request is received, the node validates the group ID and sender ID before sending a discovery response.

### Root FSM and UART Menu

The root finite state machine initializes RF communication, starts the receiver FSM, and displays a UART menu. The menu allows the user to update group and node IDs, find neighbours, show local records, and reset local storage.

### Local Record Storage

The project includes a local database structure with support for storing node ownership, timestamps, and fixed-size records. The current implementation includes displaying and clearing local records.

## Project Status

Partially completed assignment project.

Implemented:

- RF initialization
- UART menu
- Node/group ID configuration
- Wireless neighbour discovery
- Discovery response handling
- Local record display
- Local record reset
- Message structures for discovery and CRUD operations

Planned but not fully completed:

- Create record protocol
- Delete record protocol
- Retrieve record protocol
- Full response handling for CRUD operations

## What I Learned

This project helped me practice:

- Embedded C programming
- Wireless communication between embedded nodes
- Finite state machine design
- Custom packet/message structures
- UART-based user interaction
- Memory management in constrained environments
- Debugging hardware-oriented software
- Designing simple distributed communication protocols

## How to Run

This project is intended to run on CC1350 embedded hardware using the course-provided PicOS/TI environment.

General setup:

1. Open the project in the configured embedded development environment.
2. Build and flash the program to one or more CC1350 nodes.
3. Open a serial/UART terminal.
4. Set each node's group ID and node ID.
5. Use the menu option `F` to run neighbour discovery.

## Example Menu

```text
Group 1 Device #1 (0/40 records)
(G)roup ID
(N)ew device ID
(F)ind neighbors
(C)reate record on neighbor
(D)elete record on neighbor
(R)etrieve record from neighbor
(S)how local records
R(e)set local storage

Selection:
