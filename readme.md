a.  Jullian Arta Yapeter

b.  USC ID: YAPETER, USC #: 2576119893

c.  In this assignment, I implemented a client-server system for
student login and course information querying. It employs TCP and UDP
protocols for communication between hosts.

d.  My code files are comprised only of the ones described in the project
    description:

    serverM.c:  Implements Main server functionality, liasing between the
                client and the servers C/CS/EE.
    serverC.c:  Implements credentials server functionality, authenticating
                clients against encrypted username-password pairs.
    serverCS.c: Implements the CS department server functionality, receiving
                and responding to queries about CS courses information.
    serverEE.c: Implements the EE department server functionality, receiving
                and responding to queries about EE courses information.
    client.c:   Implements the client program, allowing users to input credentials
                and subsequently make queries about CS and EE courses.

e.  The messages exchanged are all strings.
client requests...

- authentication request: "username"_"password"
- course query request: "coursecode"_"category"

responses to client...

- authentication response: "2" for success, "1" for wrong password, "0" for wrong username
- course query response: string of answer if found, "None" if course not found, "NoneCategory" if category not found

f.  There are, rarely, times when starting the client the first time around causes
    an exception in the Main Server's "accept" routine. Simply restarting both
    the client and the Main Server (after exiting the terminal) resolves the
    problem.

g.  A large portion of the socket declaration and initialization program was
    taken/inspired by Beej's Guide to Network Programming. Especially the sections
    on TCP and UDP server & client examples.
