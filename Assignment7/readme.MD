CLDP Protocol Project
=====================

Overview
--------
This project implements the Custom Link Discovery Protocol (CLDP) using raw IP sockets on Linux.
CLDP is designed to enable hosts on a local network to discover each other and exchange system information.
The protocol supports three message types:
  - **HELLO**: An announcement message sent periodically by servers.
  - **QUERY**: A request message sent by clients to ask for specific system information.
  - **RESPONSE**: A reply message sent by servers containing the requested information.

Files Included
--------------
1. **cldp_server.c**
   - Implements the server side of CLDP.
   - Periodically sends HELLO messages (every 10 seconds) to announce its presence.
   - Listens for QUERY messages and sends a RESPONSE containing system information based on the query payload.

2. **cldp_client.c**
   - Implements the client side of CLDP.
   - Listens for HELLO messages to discover available servers.
   - Sends QUERY messages to the discovered servers.
   - Receives and displays RESPONSE messages from servers.

3. **cldp_spec.pdf**  
   - A PDF document containing the protocol specification, including details on the packet structure, message types, and header format.

Build Instructions
------------------
*Note: Since raw sockets are used, the application must be run with root privileges or with appropriate capabilities (e.g., CAP_NET_RAW).*

To build the server:
    gcc -o cldp_server cldp_server.c -pthread

To build the client:
    gcc -o cldp_client cldp_client.c

Run Instructions
----------------
Launch the server and client on separate terminals or on different hosts within the same local network.

For the server (example using localhost):
    sudo ./cldp_server 127.0.0.1

For the client (example using localhost):
    sudo ./cldp_client 127.0.0.1 6

The second parameter for the client is optional. It selects the query payload using the following index:
    0: "H"   – Hostname only
    1: "T"   – Timestamp only
    2: "S"   – System Information only
    3: "HT"  – Hostname and Timestamp
    4: "HS"  – Hostname and System Information
    5: "TS"  – Timestamp and System Information
    6: "HTS" – Hostname, Timestamp, and System Information (default)

Assumptions and Limitations
---------------------------
- The project is developed and tested on Linux.
- Applications using raw sockets require elevated privileges (root or CAP_NET_RAW).
- The protocol is designed for local network use and may not handle complex network configurations.
- Error handling is basic; advanced error recovery is not implemented.
- The QUERY payload is interpreted case-insensitively and should contain a combination (without duplicates) of the letters H, T, and S.

Demo Output
-----------
Below is a sample output from the server and client:

**Server Terminal:**
------------------------------------------------------------
prabhash-golla@Prabhash-Golla:~/Desktop/Networks_Lab/Assignment7$ gcc -o server cldp_server.c  
prabhash-golla@Prabhash-Golla:~/Desktop/Networks_Lab/Assignment7$ sudo ./server  
[sudo] password for prabhash-golla:  
Sending HELLO message  
Sending HELLO message  
Sending HELLO message  
Received QUERY from 127.0.0.1, transaction id: 48617  
Sent RESPONSE to 127.0.0.1  
------------------------------------------------------------

**Client Terminal:**
------------------------------------------------------------
prabhash-golla@Prabhash-Golla:~/Desktop/Networks_Lab/Assignment7$ gcc -o client cldp_client.c  
prabhash-golla@Prabhash-Golla:~/Desktop/Networks_Lab/Assignment7$ sudo ./client  
[sudo] password for prabhash-golla:  
Waiting for HELLO messages for 10 seconds...  
Received HELLO from 127.0.0.1  
Sending QUERY to all servers at "127.0.0.1" with Transaction ID 48617 and payload "HTS"  
Waiting for responses...  
______________________________________________________________________________
Received RESPONSE from 127.0.0.1:
Hostname: Prabhash-Golla  
Timestamp: 1743439366.795751  
Sysname: Linux  
Release: 6.11.0-21-generic  
Version: #21~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Mon Feb 24 16:52:15 UTC 2  
Machine: x86_64  
______________________________________________________________________________
------------------------------------------------------------

Contact / Support
-----------------
For questions or issues, please contact the project maintainer.

------------------------------------------------------------
