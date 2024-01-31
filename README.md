# Tennis-Scheduler  

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
