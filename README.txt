README
Name:Dor Iuran


Server Http

Description 

Program files:

server.c - Parse the string, bulding conenction to the server(welcome socket) ,waiting for clients(suppurting threads) ,print the response to the client.
threadpool.c - the threads creator, all function are generic and will work as a outside libary.

Function:(server.c)

usageError- Error defult message to the screen.
get_mime_type - finding the coorect type for the server , null if didnt find.
connetToServer- bulding the server welcome socket for coneection
error-returning the error by type.
checkPermissions - checking the path if all the dircatory's has X premissions and the file if he got Read premissions
checkExsist - Checking if path exsist inside the server.
readFromFile - Reading all data from file and wrting all to the client.
makeDir - Bulding diractory file's in a table as requsted in the template.
handleClient - main function for handling all the requset's ,it will deiced which fx to call.
main - Creating the welcome socket , creating the thredpool , callacting all clients requset's.


Function:(threadpool.c)
create_threadpool- Creating the pool by value that the main libary provides.
dispatch - Sender of the threds .reponsible of mange the threads.
do_work - the fucnion that need to be done by the main libary.

destroy_threadpool - making sure every thread finish his work and close the opioin for more requset to enter, at the end freeing all memory of the outside libary.








	
---



