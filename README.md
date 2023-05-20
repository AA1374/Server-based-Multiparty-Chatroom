# Server-based-Multiparty-Chatroom
A server-based multiparty chatroom with online and offline messages. This chatroom supports multiple commands and two modes of communication. Once clients are connected, the server acts as a proxy between them. This chatroom application supports multiple commands and two modes of communication. Once clients are connected, the server acts as a proxy between them. 

### Commands
Clients can send the following requests to the server:

- `REGISTER <nickname>`: Sent in response to the server prompting the user to supply a nickname for registration or login.

- `WHO`: Can be sent by the client to the server to obtain a tab-separated list of nicknames of all the users in the system (except itself), where the state of each user is also labelled.

- `EXIT`: Can be sent by the client to the server to leave the chatroom.

### Modes of Communication
Clients can communicate in two modes:

- Broadcast: A client can send a message to all other clients by simply typing the message.

- Private: A client can send a message to another client privately (via the server). To do so, the client must type `#<Nickname>: <MSG>`, where `<Nickname>` is the nickname of the destination client and `<MSG>` is the message to be sent privately. If the destination client is offline, the server will store the message in its message box (for example, a text file). When the client comes back online, it can view the message box and read the messages sent by other clients.

#### NOTE: Computer and Communication Networks Project
