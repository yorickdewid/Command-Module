Command Module
=

The command module is designed to be a control daemon for Windows workstations. It runs in the background blocking for network input. When a client connects a connection is established and the user as to indentify himself. Because there is only one administrative user (who is able to login over multiple connections) the login credentials are hardcoded. 

The daemon is FTP compatible and any FTP client can connect to the module. The commander automatically mounts all available disks on the remote workstaion.

## Commands
The command module supports most default FTP commands and optionally:

| Command          | Description                                                 |
| ---------------- | ----------------------------------------------------------- |
| FEAT             | Show available features                                     |
| KILL             | Stop the daemon                                             |
| PKILL [program]  | Kill a program on the remote workstation                    |
| INFO             | Information about the current session                       |
| CLSE             | Close the connection                                        |
| LOFF             | Logout the current session                                  |
| MSGB [message]   | Show message on remote workstation                          |
| EXEC [program]   | Execute a program on th remote workstation                  |
| HEXEC [program]  | Execute a program on th remote workstation in the backgrond |
| LDSK             | List al disk stations                                       |
| BLOCK            | Disable user input                                          |
| UNBLOCK          | Enable use input                                            |
