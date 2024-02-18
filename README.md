# Tennis-Scheduler  
Please note that some points about Assignment 2 are below in this ReadMe.  
## Assignment 1  

This program implements the following scenario.  
There are 4 Tennis courts and players arrive randomly to play.  

Characteristics of the player:  
Gender: M or F  
Game Preference: S, D, b, B   
S -> Singles (2 players),  
D -> Doubles (4 players),  
b -> first preference S and second preference D  
B -> first preference D and second preference S  
Each game lasts for 'x' minutes where x is:  
Singles Game  
Male: 10 min  
Female: 5 min  
Doubles Game  
Male: 15 min  
Female: 10 min  

A game can consist of both M and F players. In such a case, take game time to be that of a M player.  

## Executing The Scheduler  
### Compile Scheduler.c
```
gcc Scheduler.c -o scheduler
```
### Prepare the inputs  
The inputs should be prepared as a csv file in the following format  
Player-ID Arrival-time Gender Preference  
1 1 M S  
2 1 F S  
3 2 M S  
4 3 M b  

A sample is provided for reference (`input.csv')  

### Execute  
Rung the following command to execute.  
```
./scheduler "<input-file>"
```
Demo example  
```
./scheduler "input.csv"
```
The output.csv that is generated will containg the results in the following form  
Game-start-time Game-end-time Court-Number List-of-player-ids  
1 11 1 1 2  
3 13 2 3 4  

## Logic of the Scheduler
1) The player requests are pushed into two differnet queues based on their first preferences.
2) Games are scheduled to satisy the player's first preference.
3) Once the the games to be scheduled on based of first prefernefces are complete, the scheduler shcedules games on the basis of the second preference.
4) The game start time is the MAX(Game-end-time, Arrival-time of the last player (to play in a court))
5) Courts are assigned to a group of players from the set of 4 courts by choosing the one with the lowest Game-end-time. Game-end-time of an un-assigned court is -1

The Scheduler is multi-threaded.  
```
                  |------------enqueueing thread------------>|  
---main thread--->|----thread to schedule single's games---->|---main thread to perform 2nd priority scheduling-->  
                  |----thread to schedule double's games---->|  
```
# Assignment 2  
For assignment 2, the required source files are `client.c`, `server_schedule.c` and `server_play.c`.  
I have tried to mimic the actual world scenario in the sports complex through these programs for scheduling tennis games. Additinally, I have tried to decouple the working of the servers and the clients as much as I can.  
## Features
### Client
1) The `client.c` loops through the rows of the input csv, and makes connection with the scheduling server (mimics attendant sitting in issuing complex) when the arrival time in the row is equal to the global time.  
2) After this the client non-blockingly waits for a recieve from the server for 30s (one second is one minute) with respect to the global time. After which it times out.
3) If the client gets a shcedule (i.e. the set of players it will play with, start time, end time etc..) it (each client) will initiate a connection (after global time equals the start time recieved) with the playing server (mimics actually entering into the court and playing).
4) After playing, every client prints out when it game was supposed to end and what the global time is at the time of printing. Some synchronisation issues between server_play and client show up here.
5) The chidren are forked, and the game schedule contains a lot of information, more than what is required for this assignment. This is to facilitate the building process for the upcoming assignment.

### Sceduling Server
1) This server uses OpenMP directives for multi-threading.
2) The scheduling logic is similar to the one followed by the Scheduler in Assignment 1.
3) An extra function, `isClientAlive` is added to allow the server to check if a client has timed out or not before incuding this client in a schedule. This is important as the server doesn't maintain any notion of time.
4) This server succesfully mimics an attendant in the sport's complex issue centre, where one asks if one can play tennis (if raquets are available and courts are free).

### Playing Server
1) This server uses OpenMP directives for multi-threading.
2) This server mimics an actual game being played. It recieve requests to play for let's say some `x` mins by the client and sleeps for roughly `x` seconds before replying back to the client. Since the playing server and clients aren't synchronised... the time the clients stopped playing, and the time the should have...don't match...but thisdoesn't cause in inconsitency in the execution.
