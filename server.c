//id: 206123689
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include "threadpool.h"
#define BUFREAD 4000                           // for read the first line of the request
#define BUFDNAME 500                           // for name of files insdie diractory
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT" // extract the date
#define QUEUESIZE 5
#define NUMOFWORDS 3
#define RESBUF 100 //for read and print the reaponse from the server
#define GET "GET"
#define HTTP "HTTP/1.1 "
#define BADREQUEST "400 Bad Request\r\n"
#define NOTSUPPORTED "501 Not supported\r\n"
#define FORBIDDEN "403 Forbidden\r\n"
#define NOTFOUND "404 Not Found\r\n"
#define FOUND "302 Found\r\n"
#define OK "200 ok\r\n"
#define INTERNALSERVER "500 Internal Server Error\r\n"
#define SERVER "Server: webserver/1.0\r\n"
#define TYPE "Content-Type: "
#define LEN "Content-Length: "
#define LASTMOD "Last-Modified: "
#define CONNECTION "Connection: close\r\n\r\n"
#define FOUR "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H4>400 Bad request</H4>\nBad Request.\n</BODY></HTML>\n"
#define FOUROFOUR "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H4>404 Not Found</H4>\nFile not found.\n</BODY></HTML>\n"
#define FIVEOONE "<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\n<BODY><H4>501 Not supported</H4>\nMethod is not supported.\n</BODY></HTML>\n"
#define THREEOTWO "<HTML><HEAD><TITLE>302 Found</TITLE></HEAD>\n<BODY><H4>302 Found</H4>\nDirectories must end with a slash.\n</BODY></HTML>\n"
#define FIVE00 "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H4>500 Internal Server Error</H4>\nSome server side error.\n</BODY></HTML>\n"
#define FOUROTHREE "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H4>403 Forbidden</H4>\nAccess denied.\n</BODY></HTML>\n"
#define DATE "Date: "
#define ERRHTMLTYPE "text/html"
#define LOCATION "Location: "
#define INDEX "index.html"
void usageServerError()
{
    printf("Usage: server <port> <pool-size> <max-number-of-request>\n");
    exit(-1);
}
char *get_mime_type(char *name) // type of file
{
    char *ext = strrchr(name, '.');
    if (!ext)
        return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)
        return "image/gif";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".au") == 0)
        return "audio/basic";
    if (strcmp(ext, ".wav") == 0)
        return "audio/wav";
    if (strcmp(ext, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0)
        return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0)
        return "audio/mpeg";
    return NULL;
}

int connetToServer(int port)
{
    int welcome_socket = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr cil_addr;
    socklen_t clilen;
    welcome_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (welcome_socket < 0)
    {
        perror("Socket \n");
        return -1;
    }
    serv_addr.sin_family = PF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(welcome_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Bind \n");
        return -1;
    }
    if (listen(welcome_socket, QUEUESIZE) < -1)
    {
        perror("Listen \n");
        return -1;
    }
    return welcome_socket;
}
// for all err return's
void error(int new_sockfd, int err, char *path)
{
    int len = 0;
    //Extract date
    time_t now;
    char timebuf[128];
    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));

    //Choose which of the error is requsted
    char *errMe = "";
    char *errTitle = "";
    if (err == 400)
    {
        errMe = FOUR;
        errTitle = BADREQUEST;
    }
    else if (err == 501)
    {
        errMe = FIVEOONE;
        errTitle = NOTSUPPORTED;
    }
    else if (err == 404)
    {
        errMe = FOUROFOUR;
        errTitle = NOTFOUND;
    }
    else if (err == 302)
    {
        errMe = THREEOTWO;
        errTitle = FOUND;
        len += strlen(LOCATION);
        len += strlen(path) + 3; //CHECK HOW MANY NEED TO ADD
    }
    else if (err == 403)
    {
        errMe = FOUROTHREE;
        errTitle = FORBIDDEN;
    }
    else if (err == 500)
    {
        errMe = FIVE00;
        errTitle = INTERNALSERVER;
    }
    ////////////////////////////////////////
    //len for creating  the response
    len += strlen(HTTP) + strlen(errTitle) + strlen(SERVER) + strlen(timebuf) + strlen(DATE) + strlen(TYPE) + strlen(ERRHTMLTYPE) + strlen(LEN) + strlen(CONNECTION) + strlen(errMe) + 6; //add 1 ;

    int size_Me = strlen(errMe); //4 for /n was +4

    //cast int to string
    char temp[10];
    sprintf(temp, "%d", size_Me);

    len += strlen(temp);
    char *to_return = (char *)malloc((sizeof(char) * (len + 1)));
    if (to_return == NULL)
    {
        perror("malloc()");
        return;
    }
    memset(to_return, 0, (len + 1));

    //buliding the response
    strcat(to_return, HTTP);
    strcat(to_return, errTitle);
    strcat(to_return, SERVER);
    strcat(to_return, DATE);
    strcat(to_return, timebuf);
    strcat(to_return, "\r\n");
    if (err == 302)
    {
        strcat(to_return, LOCATION);
        strcat(to_return, path);
        strcat(to_return, "/");
        strcat(to_return, "\r\n");
    }
    strcat(to_return, TYPE);
    strcat(to_return, ERRHTMLTYPE);
    strcat(to_return, "\r\n");
    strcat(to_return, LEN);
    strcat(to_return, temp);
    strcat(to_return, "\r\n");
    strcat(to_return, CONNECTION);
    strcat(to_return, errMe);

    ////////////////////////////
    //write to socket
    int ws = write(new_sockfd, to_return, (len + 1));
    if (ws == -1)
    {
        free(to_return);
        perror("write()\n");
        close(new_sockfd);
        return;
    }
    close(new_sockfd);
    free(to_return);
}

char *checkPermissions(int new_sockfd, char *path)
{
    struct stat fs = {0};
    int sum = 0;
    char *temp_last_path;
    char *path_token;
    int flag_singal_char = 0;
    if (strcmp(path, "/") == 0)
    {
        temp_last_path = malloc(sizeof(char) + 1);
        memset(temp_last_path, 0, sizeof(char) + 1);
        strcpy(temp_last_path, path);
    }
    else if (strcmp(path, "/index.html") == 0)
    {
        return NULL;
    }
    else
    {
        path_token = strtok(path, "/");
        if (path_token == NULL)
        {
            error(new_sockfd, 500, NULL);
            free(path);
            return NULL;
        }

        temp_last_path = malloc((strlen(path_token) + 2) * sizeof(char));
        memset(temp_last_path, 0, (strlen(path_token) + 2));
        while (path_token != NULL)
        {
            strcat(temp_last_path, path_token);
            if (strchr(path_token, '.') != NULL)
            {
                break;
            }
            strcat(temp_last_path, "/");
            if (stat(temp_last_path, &fs) == -1)
            {
                error(new_sockfd, 500, NULL);
                free(path);
                free(temp_last_path);
                return NULL;
            }
            if ((S_IXOTH & fs.st_mode) == 0)
            {
                ////////////////////////check//////////////
                error(new_sockfd, 403, NULL);
                free(path);
                free(temp_last_path);
                return NULL;
                //other doesnt have execute premision
            }

            path_token = strtok(NULL, "/");
            if (path_token == NULL)
            {
                //end of path
                break;
            }
            sum = strlen(temp_last_path) + strlen(path_token) + 2;
            temp_last_path = (char *)realloc(temp_last_path, sum);
        }
    }

    if (stat(temp_last_path, &fs) == -1)
    {
        error(new_sockfd, 500, NULL);
        free(path);
        free(temp_last_path);
        return NULL;
    }
    if ((S_IROTH & fs.st_mode) == 0)
    {
        //last token doenst have read premision
        error(new_sockfd, 403, NULL);
        free(path);           //added now
        free(temp_last_path); //added now
        return NULL;
    }
    return temp_last_path;
}

int makeRegRes(int new_sockfd, char *temp_last_path, int totalRed, int dirContent)
{
    //if dir content is 0 it's file else 1 need to return dir content

    int len = 0;
    //Extract date
    time_t now;
    char timebuf[128];
    char lastModifed[128];
    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
    //////////////////////////

    //if dir content is 0 it's index.htmel else 1 need to return dir content
    char *typeFile = "";
    if (dirContent == 0)
    {
        typeFile = get_mime_type(temp_last_path);
    }
    else
    {
        typeFile = ERRHTMLTYPE;
    }

    struct stat fs = {0};
    if (stat(temp_last_path, &fs) == -1)
    {
        error(new_sockfd, 500, NULL);
        perror("Stat \n");
        free(temp_last_path);
        return -1;
    }
    time_t mod = fs.st_mtime;

    strftime(lastModifed, sizeof(lastModifed), RFC1123FMT, gmtime(&mod));

    ////////////////////////////////////////
    //len for creating  the response
    len += strlen(HTTP) + strlen(OK) + strlen(SERVER) + strlen(DATE) + strlen(timebuf) + strlen(LEN) + strlen(LASTMOD) + strlen(lastModifed) + strlen(CONNECTION) + 6;
    if (typeFile != NULL)
    {
        len += strlen(TYPE) + strlen(typeFile) + 2;
    }
    int size_Me = totalRed;
    //cast int to string
    char temp[10];
    sprintf(temp, "%d", size_Me);

    len += strlen(temp);
    char *to_return = (char *)malloc((sizeof(char) * (len + 1)));
    if (to_return == NULL)
    {
        error(new_sockfd, 500, NULL);
        free(to_return);
        return -1;
    }
    memset(to_return, 0, (len + 1));

    //buliding the response
    strcat(to_return, HTTP);
    strcat(to_return, OK);
    strcat(to_return, SERVER);
    strcat(to_return, DATE);
    strcat(to_return, timebuf);
    strcat(to_return, "\r\n");
    if (typeFile != NULL)
    {
        strcat(to_return, TYPE);
        strcat(to_return, typeFile);
        strcat(to_return, "\r\n");
    }
    strcat(to_return, LEN);
    strcat(to_return, temp);
    strcat(to_return, "\r\n");
    strcat(to_return, LASTMOD);
    strcat(to_return, lastModifed);
    strcat(to_return, "\r\n");
    strcat(to_return, CONNECTION);
    ////////////////////////////
    //write to socket
    int ws = write(new_sockfd, to_return, len);
    if (ws == -1)
    {
        error(new_sockfd, 500, NULL);
        free(to_return);
        return -1;
    }
    free(to_return);
    return 1;
}

int checkExsist(int new_sockfd, char *temp_last_path)
{
    struct dirent *dir;
    int bool_f = 0;
    DIR *d = opendir(temp_last_path);
    if (d == NULL)
    {
        error(new_sockfd, 500, NULL);
        //FAILD TO OPEN FOLDER
        free(temp_last_path);
        return -1;
    }
    while ((dir = readdir(d)) != NULL)
    {
        if (strcmp((dir->d_name), INDEX) == 0)
        {
            //found it
            bool_f = 1;
            break;
        }
    }
    if (closedir(d) == -1)
    {
        //FAILD TO close FOLDER
        error(new_sockfd, 500, NULL);
        free(temp_last_path);
        return -1;
    }
    return bool_f;
}
//open file and after read writing back to client
void readFromFile(int new_sockfd, char *tempPr, int file_size)
{
    int fd = open(tempPr, O_RDONLY, 0666);
    if (fd == -1)
    {
        error(new_sockfd, 500, NULL);
        free(tempPr);
        return;
    }
    int rc = 0;
    int totalRec = 0;
    unsigned char *response = malloc(sizeof(char) * (file_size));
    memset(response, 0, (file_size));
    rc = read(fd, response, file_size);
    if (rc < 0)
    {
        error(new_sockfd, 500, NULL);
        free(tempPr);
        free(response);
        return;
    }
    else
    {
        int ws = write(new_sockfd, response, file_size);
        if (ws == -1)
        {
            error(new_sockfd, 500, NULL);
            free(response);
            free(tempPr);
            return;
        }
    }

    free(response);
    free(tempPr);
    if (close(fd) == -1)
    {
        error(new_sockfd, 500, NULL);

        return;
    }
    return;
}

unsigned char *makeDir(int new_sockfd, char *tempPr)
{
    int len = 0;
    int len_of_row = 0;
    int size_re = 0;
    off_t file_size = 0;
    char temp[10];
    char *start = "<HTML>\r\n<HEAD><TITLE>Index of ";
    char *second = "</TITLE></HEAD>\n\n";
    char *body = "<BODY>\n<H4>Index of ";
    char *body_s = "</H4>\n\n";
    char *next = "<table CELLSPACING=8>\n";
    char *title = "<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n"; //was \n\n\n\n\n
    char *tr_s = "<tr>\n";
    char *tr_e = "</tr>\n";
    char *td_s = "<td>";
    char *td_e = "</td>";
    char *row = "<A HREF=";
    char *end_row = "</A>";
    char *table = "</table>\n\n<HR>\n\n<ADDRESS>webserver/1.0</ADDRESS>\n\n</BODY></HTML>\n";
    //len is without the lines of the table;
    len += strlen(start) + strlen(tempPr) + strlen(second) + strlen(body) + strlen(tempPr) + strlen(body_s) + strlen(next) + strlen(title) + strlen(table);
    len_of_row += strlen(tr_s) + strlen(tr_e) + (3 * strlen(td_s)) + (3 * strlen(td_e)) + strlen(row) + strlen(end_row);
    //need to add entity name and the size of file .

    char *to_return = (char *)malloc((sizeof(char) * (len)) + 2);
    memset(to_return, 0, ((sizeof(char) * (len)) + 2));
    if (to_return == NULL)
    {
        error(new_sockfd, 500, NULL);
        free(to_return);
        return NULL;
    }
    struct dirent *dir;
    int bool_f = 0;

    DIR *d = opendir(tempPr);
    if (d == NULL)
    {
        //FAILD TO OPEN FOLDER
        error(new_sockfd, 500, NULL);
        free(tempPr);
        free(to_return);
        return NULL;
    }

    struct stat fs = {0};
    if (stat(tempPr, &fs) == -1)
    {
        error(new_sockfd, 500, NULL);
        free(tempPr);
        free(to_return);
        return NULL;
    }

    strcat(to_return, start);
    strcat(to_return, tempPr);
    strcat(to_return, second);
    strcat(to_return, body);
    strcat(to_return, tempPr);
    strcat(to_return, body_s);
    strcat(to_return, next);
    strcat(to_return, title);
    //how many lines

    char *cat_path = (char *)malloc(strlen(tempPr) + BUFDNAME);
    memset(cat_path, 0, strlen(tempPr) + BUFDNAME);
    int diractory = 0;
    while ((dir = readdir(d)) != NULL)
    {
        diractory = 0;
        memset(cat_path, 0, strlen(tempPr) + BUFDNAME);
        strcat(cat_path, tempPr);
        strcat(cat_path, dir->d_name);

        struct stat fs = {0};

        if (stat(cat_path, &fs) == -1)
        {
            continue;
        }

        time_t mod;
        char lastModifed[128];

        mod = fs.st_mtime;
        strftime(lastModifed, sizeof(lastModifed), RFC1123FMT, gmtime(&mod));
        if (!(S_ISDIR(fs.st_mode)))
        {
            //this is not a dir
            file_size = fs.st_size;
            sprintf(temp, "%ld", file_size);
            size_re += strlen(temp);
        }
        else
        {
            diractory = 1;
            size_re += 1;
        }
        size_re += len + len_of_row + (strlen(dir->d_name) * 2) + strlen(lastModifed) + 7;
        to_return = realloc(to_return, size_re);
        strcat(to_return, tr_s);
        strcat(to_return, td_s);
        strcat(to_return, row);
        strcat(to_return, "\"");
        strcat(to_return, dir->d_name);
        if (diractory == 1)
        {
            strcat(to_return, "/");
        }
        strcat(to_return, "\">");
        strcat(to_return, dir->d_name);
        strcat(to_return, end_row);
        strcat(to_return, td_e);
        strcat(to_return, td_s);
        strcat(to_return, lastModifed);
        strcat(to_return, td_e);
        strcat(to_return, "\n");
        strcat(to_return, td_s);
        if (!(S_ISDIR(fs.st_mode)))
        {
            //this is not a dir
            if (file_size != 0)
            {
                strcat(to_return, temp);
            }
        }
        strcat(to_return, td_e);
        strcat(to_return, "\n");
        strcat(to_return, tr_e);
        strcat(to_return, "\n\n");
    }

    strcat(to_return, table);

    if (closedir(d) == -1)
    {
        //FAILD TO close FOLDER
        error(new_sockfd, 500, NULL);
        free(tempPr);
        free(to_return);
        free(cat_path);
        return NULL;
    }
    free(cat_path);
    return to_return;
}

//retrun null if fail-sys call
int handleClient(void *sockfd)
{
    int new_sockfd = *((int *)sockfd);
    char buf_read[BUFREAD];
    char *response = "";
    if (read(new_sockfd, buf_read, BUFREAD) < 0)
    {
        error(new_sockfd, 404, NULL);
        return -1;
    };
    char *line = strtok(buf_read, "\n\r");
    if (line == NULL)
    {
        error(new_sockfd, 500, NULL);
        close(new_sockfd);
        return -1;
    }
    int flag_501 = 0;
    char *check_token = "";
    check_token = strtok(line, " ");
    if (strcmp(check_token, GET) != 0)
    {
        //need to be after 400 err
        flag_501 = 1;
    }
    int counter = 1;
    char *path;
    char *protocol;
    while (check_token != NULL)
    {
        check_token = strtok(NULL, " ");
        if (check_token == NULL)
        {
            break;
        }

        if (counter == 1)
        {
            if (strlen(check_token) > 1)
            {
                path = (char *)malloc((sizeof(char) * strlen(check_token)) + 1);
                memset(path, 0, (strlen(check_token) + 1));
                strcpy(path, (check_token + 1));
            }
            else
            {
                path = (char *)malloc(sizeof(char) + 1);
                memset(path, 0, (sizeof(char) + 1));
                strcpy(path, check_token);
            }
        }
        if (counter == 2)
        {
            protocol = (char *)malloc((sizeof(char) * strlen(check_token) + 1));
            memset(protocol, 0, (strlen(check_token) + 1));
            strcpy(protocol, check_token);
        }

        counter++;
    }

    if (counter == 3)
    {

        if (strcmp(protocol, "HTTP/1.1") != 0 && strcmp(protocol, "HTTP/1.0") != 0)
        {
            error(new_sockfd, 400, NULL);
            free(path);
            free(protocol);
            return -1;
        }
        free(protocol);
    }

    if (counter != NUMOFWORDS)
    {
        error(new_sockfd, 400, NULL);
        free(path);
        return -1;
    }
    if (flag_501 == 1)
    {
        free(path);
        error(new_sockfd, 501, NULL);
        return -1;
    }
    if (access(path, F_OK) != 0)
    {
        error(new_sockfd, 404, NULL);

        free(path);
        return -1;
    }

    struct stat fs;
    if (stat(path, &fs) == -1)
    {
        error(new_sockfd, 404, NULL);
        free(path);
        return -1;
    }
    //if the path is dir
    if (S_ISDIR(fs.st_mode))
    {
        char *temp_last_path;
        if (path[strlen(path) - 1] != '/')
        {
            error(new_sockfd, 302, path);
            free(path);
            return -1;
        }

        else if (path[strlen(path) - 1] == '/')
        {
            temp_last_path = checkPermissions(new_sockfd, path);
            //no more need the path
            free(path);
            //if null their no permissions
            if (temp_last_path != NULL)
            {
                if (checkExsist(new_sockfd, temp_last_path) == 1)
                {
                    temp_last_path = realloc(temp_last_path, (strlen(temp_last_path) + strlen(INDEX) + 2));
                    strcat(temp_last_path, INDEX);

                    char *tempPr = checkPermissions(new_sockfd, temp_last_path);
                    //no more need the temp last
                    free(temp_last_path);

                    if (tempPr != NULL)
                    {
                        if (stat(tempPr, &fs) == -1)
                        {
                            error(new_sockfd, 500, NULL);
                            free(tempPr);
                            return -1;
                        }

                        int file_size = fs.st_size;
                        int check_writeen = makeRegRes(new_sockfd, tempPr, file_size, 0);
                        readFromFile(new_sockfd, tempPr, file_size);

                        close(new_sockfd);
                        return 1;
                    }
                }
                else
                {
                    /// Need to bulid a index. html
                    if (strcmp(temp_last_path, "/") == 0)
                    {

                        temp_last_path = realloc(temp_last_path, strlen(temp_last_path) + 2);
                        memset(temp_last_path, 0, strlen(temp_last_path) + 2);
                        strcat(temp_last_path, "./");
                    }
                    //Already cheked the read pri to dir
                    char *read_dir = makeDir(new_sockfd, temp_last_path);
                    int total_len = strlen(read_dir);
                    int check = makeRegRes(new_sockfd, temp_last_path, total_len, 1);
                    free(temp_last_path);
                    int ws = write(new_sockfd, read_dir, total_len);
                    if (ws == -1)
                    {
                        error(new_sockfd, 500, NULL);
                        return -1;
                    }
                    close(new_sockfd);
                    free(read_dir);
                    return 1;
                }
            }
            else
            {
                // no primision to path

                error(new_sockfd, 403, NULL);

                return -1;
            }
        }
    }

    char *temp_last_path;
    temp_last_path = checkPermissions(new_sockfd, path);
    if (temp_last_path == NULL)
    {
        //their no primision to the path
        free(path);
        return -1;
    }
    if (!(S_ISREG(fs.st_mode)))
    {
        // not regular
        error(new_sockfd, 403, NULL);
        free(path);
        return 1;
    }
    else
    {
        //return the file himself
        //got pri already
        int file_size = fs.st_size;
        int check_writeen = makeRegRes(new_sockfd, temp_last_path, file_size, 0);
        readFromFile(new_sockfd, temp_last_path, file_size);
        close(new_sockfd);
        free(path);
        return 1;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        usageServerError();
    }
    int port = atoi(argv[1]);
    int pool_size = atoi(argv[2]);
    int max_num_of_req = atoi(argv[3]);
    //valid check for input of runing the server .
    if (port == 0 || port < 0 || pool_size == 0 || pool_size < 0 || max_num_of_req == 0 || max_num_of_req < 0)
    {
        usageServerError();
        exit(EXIT_FAILURE);
    }

    int welcome_socket = connetToServer(port);
    if (welcome_socket == -1)
    {

        exit(EXIT_FAILURE);
    }
    //create threadpool
    threadpool *thread_pool = create_threadpool(pool_size);
    int *new_sockfd = (int *)malloc(sizeof(int) * max_num_of_req);
    if (new_sockfd == NULL)
    {
        perror("Malloc \n");
        destroy_threadpool(thread_pool);
        free(new_sockfd);
        exit(EXIT_FAILURE);
    }
    int t = 0;
    for (; t < max_num_of_req; t++)
    {
        new_sockfd[t] = accept(welcome_socket, NULL, NULL);
        if (new_sockfd < 0)
        {
            perror("accept \n");
            free(new_sockfd);
            destroy_threadpool(thread_pool);
            exit(EXIT_FAILURE);
        }
        dispatch(thread_pool, handleClient, &(new_sockfd[t]));
    }
    destroy_threadpool(thread_pool);
    free(new_sockfd);
    close(welcome_socket);
}
