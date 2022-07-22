/* CSE342 Computer Networks */
/* Server Code
//Team: Krunal Savaj(AU1940271), Krina Khakhariya(AU1940208), Arti Singhal (AU1940181), Axay Ghoghari(AU1940269), Tirth Kanani(AU1920144)
//Using following commands we can Compile and Run this code

//step 1: gcc server.c
//step 2: ./server 127.0.0.1

/* Following are the different header files added for socket programming */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* Frequently used variables are defined as follows in the code */
#define SERVER_PORT 5432
#define BUF_SIZE 4096

#define MAXIMUM_BUF_SIZE 256
#define MULTICAST_SERVER_IP "239.192.1.10"
#define NUMBER_OF_STATIONS 4
#define BUF_SIZE_SMALL 1024

/* Here structure for station info request defined as follows */
typedef struct station_information_request_t
{
    uint8_t type;
} station_information_request;

/* Init Function for station_information_request_t as follows */

station_information_request initStationInfoRequest(station_information_request *si)
{
    si->type=1;
    return *si;
}

/* Here structure for station info defined as follows */

typedef struct station_information_t
{
    uint8_t station_number;
    uint8_t station_name_size;
    char station_name[MAXIMUM_BUF_SIZE];
    uint32_t multicast_address;
    uint16_t data_port;
    uint16_t info_port;
    uint32_t bit_rate;
} station_information;

/* Structure for site info defined as follows */
typedef struct site_information_t
{
    uint8_t type; // = 10;
    uint8_t site_name_size;
    char site_name[MAXIMUM_BUF_SIZE];
    uint8_t site_desc_size;
    char site_desc[MAXIMUM_BUF_SIZE];
    uint8_t station_count;
    station_information station_list[MAXIMUM_BUF_SIZE];
} site_information;

/* Init Function for station_information */
site_information initSiteInfo(site_information *sir)
{
    sir->type=10;
    return *sir;
}

/* Here structure for station not found defined as follows */
typedef struct station_not_found_t
{
    uint8_t type; // = 11;
    uint8_t station_number;
} station_not_found;

/* Init Function for station_not_found */
station_not_found initStationNotFound(station_not_found *snnf)
{
    snnf->type=11;
    return *snnf;
}

/* Here structure for song info defined as follows */
typedef struct song_information_t
{
    uint8_t type; // = 12;
    uint8_t song_name_size;
    char song_name[MAXIMUM_BUF_SIZE];
    uint16_t remaining_time_in_sec;
    uint8_t next_song_name_size;
    char next_song_name[MAXIMUM_BUF_SIZE];
} song_information;

/* Init Function for song_information */
song_information initSongInfo(song_information *siR)
{
    siR->type = 12;
    return *siR;
}

/* Here structure for station id path is defined as follows */
typedef struct station_ID_path_t
{
    int id;
    char path[BUF_SIZE_SMALL];
    int port;
} station_ID_path;

/* Here array of objects of structures is created as follows */
station_information stations[NUMBER_OF_STATIONS];
station_ID_path stationIDPaths[NUMBER_OF_STATIONS];

/* Here threads for stations is defined as follows */
pthread_t stationThreads[NUMBER_OF_STATIONS];

/* Here variables for storing command line arguments is declared as follows */
char **argV;
int argC;

/* Here sleep variable is defined as follows */
long idealSleepS;





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Here Function for getting the bit rate is defined as follows */
int get_Bit_Rate(char *fName)
{
    int pID = 0;
    char *cmnd1 = "mediainfo ";                                                                 //Fetching all the media properties
    char *reDirect = " | grep 'Overall bit rate                         : ' > information.txt"; //Writing down the output (bit rate) into a file
    char cmnd[256];
    strcpy(cmnd, cmnd1);
    strcat(cmnd, fName);
    strcat(cmnd, reDirect);
    system(cmnd);
    // execl(params[0], params[1], params[2],params[3],NULL);
    wait(NULL);
    FILE *fp = fopen("information.txt", "r");
    char *str = "Overall bit rate                         : ";
    char buffer[256];
    memset(buffer, '\0', sizeof(buffer)); //setting buffer to 0
    char *s;
    fgets(buffer, sizeof(buffer), fp);
    char *k = strstr(buffer, "Kbps\n"); //string representing bit rate
    memset(k, '\0', strlen(k));         //setting string to 0

    s = strstr(buffer, str);
    s = s + strlen(str);

    char buffer1[256];
    memset(buffer1, '\0', 256);

    int i = 0, j = 0;

    while(i<strlen(s)+1){
        if (s[i] == ' ')
            continue;
        buffer1[j++] = s[i];
        i++;
    }

    int brate = atoi(buffer1); //char array to int conversion
    return brate;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Here function for sending structures to stations is defined as follows */
void *start_Station_ListServer(void *arg)
{
    //Structure object initialised
    struct sockaddr_in sin;
    int length;
    int socket1, new_Socket;
    char str[INET_ADDRSTRLEN];
    //Server IP initialised and declared
    char *serverIP;
    if (argC > 1)
        serverIP = argV[1];

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;                  //assigning family
    sin.sin_addr.s_addr = inet_addr(serverIP); //assigning server address
    sin.sin_port = htons(SERVER_PORT);         //assigning server port

    /* setup passive open */
    if ((socket1 = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed!!"); //generating error on failure of socket
        exit(1);                            //program is terminated if socket is not created
    }

    inet_ntop(AF_INET, &(sin.sin_addr), str, INET_ADDRSTRLEN); //converting address from network format to presentation format

    printf("Server is using address %s and port %d.\n", str, SERVER_PORT);
    //Binding server to the address
    if ((bind(socket1, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        perror("Bind failed"); //generating error if bind not done
        exit(1);               //termination on failure of bind
    }
    else
        printf("Binded successfully\n");

    listen(socket1, 5); //call for listen where pending request can be atmost 5
    //Structures infinitely to client is sent from here
    for(;;)
    {
        //The connection request from client is accepted from here
        if ((new_Socket = accept(socket1, (struct sockaddr *)&sin, (unsigned int *)&length)) < 0)
        {
            perror("Accept failed"); //Generating error if accept command fails
            exit(1);                 //terminating on failure of accept call
        }

        printf("Client and server are connected! To send structures is being started from here. \n");
        uint32_t no_of_stat = 3;                            //initialising no of stations
        no_of_stat = htonl(no_of_stat);                     //sending int requires this conversion
        send(new_Socket, &no_of_stat, sizeof(uint32_t), 0); //no of stations(int) is sent

        for (int i = 0; i < NUMBER_OF_STATIONS; i++) //sending the station info for all the stations specified
        {
           send(new_Socket, &stations[i], sizeof(station_information), 0);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Here the function for calculation of bit rate is defined as follows */
void BR_Calculation(char names[][BUF_SIZE_SMALL], int bit_Rate[], int s_Count)
{
    int i=0;
    while(i<s_Count){
        bit_Rate[i] = get_Bit_Rate(names[i]);
        i++;
    }

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Here function fill stations is defined as follows */
void Station_Details()
{
    station_information stat_information1;
    station_ID_path stat_ID_path;

    //Here station1 info into structure variables is intialized from here
    stat_information1.station_number = 1;
    stat_information1.station_name_size = htonl(strlen("Station 1"));
    strcpy(stat_information1.station_name, "Station 1");
    stat_information1.data_port = htons(9100);
    stat_ID_path.port = 9100;
    stat_ID_path.id = 1;
    strcpy(stat_ID_path.path, "./Station no._1/");

    //station1 info and path is copied from here
    memcpy(&stations[0], &stat_information1, sizeof(station_information));
    memcpy(&stationIDPaths[0], &stat_ID_path, sizeof(station_ID_path));

    //Clearing out the station1 info and path
    bzero(&stat_information1, sizeof(station_information));
    bzero(&stat_ID_path, sizeof(station_ID_path));

    //Here station2 info into structure variables is Initialised from here
    stat_information1.station_number = 2;
    stat_information1.station_name_size = htonl(strlen("Station 2"));
    strcpy(stat_information1.station_name, "Station 2");
    stat_information1.data_port = htons(9101);
    stat_ID_path.port = 9101;
    stat_ID_path.id = 2;
    strcpy(stat_ID_path.path, "./Station no._2/");

    //station2 info and path is copied from here
    memcpy(&stations[1], &stat_information1, sizeof(station_information));
    memcpy(&stationIDPaths[1], &stat_ID_path, sizeof(station_ID_path));

    //Clearing out the station2 info and path
    bzero(&stat_information1, sizeof(station_information));
    bzero(&stat_ID_path, sizeof(station_ID_path));

    //Here station3 info into structure variables is initialised from here
    stat_information1.station_number = 3;
    stat_information1.station_name_size = htonl(strlen("Station 3"));
    strcpy(stat_information1.station_name, "Station 3");
    stat_information1.data_port = htons(9102);
    stat_ID_path.port = 9102;
    stat_ID_path.id = 3;
    strcpy(stat_ID_path.path, "./Station no._3/");

    //station3 info and path is copied from here
    memcpy(&stations[2], &stat_information1, sizeof(station_information));
    memcpy(&stationIDPaths[2], &stat_ID_path, sizeof(station_ID_path));

    //Clearing out the station2 info and path
    bzero(&stat_information1, sizeof(station_information));
    bzero(&stat_ID_path, sizeof(station_ID_path));

    //Here station3 info into structure variables is initialised from here
    stat_information1.station_number = 4;
    stat_information1.station_name_size = htonl(strlen("Station 4"));
    strcpy(stat_information1.station_name, "Station 4");
    stat_information1.data_port = htons(9103);
    stat_ID_path.port = 9103;
    stat_ID_path.id = 4;
    strcpy(stat_ID_path.path, "./Station no._4/");

    //station3 info and path is copied from here
    memcpy(&stations[3], &stat_information1, sizeof(station_information));
    memcpy(&stationIDPaths[3], &stat_ID_path, sizeof(station_ID_path));
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Here Function for starting / setting up station is defined as follows
void *start_Station(void *arg)
{
    //Parsing directory and opening songs
    station_ID_path *stat_ID_path = (station_ID_path *)arg;
    DIR *directory1;
    struct dirent *entry;
    int s_Count = 0;
    //path for song file
    printf("Path:-- %s\n", stat_ID_path->path);
    //searching in the directory and then reading all the songs
    if ((directory1 = opendir(stat_ID_path->path)) != NULL)
    {
        //Reading all the files with extension mp3 and then collecting them
        for(;(entry = readdir(directory1)) != NULL;)
        {
            if (strstr(entry->d_name, ".mp3") != NULL)
                ++s_Count; //counter for songs with mp3 extension
        }
        closedir(directory1); //closing directory
    }
    else
    {
        /* could not open file in the directory */
        perror("Could not find file in the directory!");
        return 0;
    }

    char songs[s_Count][BUF_SIZE_SMALL];   //Declaring 2D array for path of the songs and their size
    char s_Names[s_Count][BUF_SIZE_SMALL]; //Declaring 2D array for names of the songs and their size

    FILE *song_Files[s_Count];
    int bit_Rates[s_Count];
    int i=0;
    while(i<s_Count){
        memset(songs[i], '\0', BUF_SIZE_SMALL);
        strcpy(songs[i], stat_ID_path->path); //storing path of the songs
        i++;
    }

    int current = 0;
    if ((directory1 = opendir(stat_ID_path->path)) != NULL) //Checks for station directory is not empty
    {
        for(;(entry = readdir(directory1)) != NULL;)
        {
            if (strstr(entry->d_name, ".mp3") != NULL)
            {
                memcpy(&(songs[current][strlen(stat_ID_path->path)]), entry->d_name, strlen(entry->d_name) + 1);
                strcpy((s_Names[current]), entry->d_name); //stores names of the available songs

                song_Files[current] = fopen(songs[current], "rb"); //Opening the file
                if (song_Files[current] == NULL)
                {
                    perror("No song file present in the directory"); //Displaying error for no song files
                    exit(1);
                }

                current++; //incrementing value to check for all the files in the directory
            }
        }
        closedir(directory1); //closing the current directory
    }

    //Song_Info Structures is being created
    song_information s_Info[s_Count];
    i=0;
    while(i<s_Count){
        bzero(&s_Info[i], sizeof(song_information));  //zeros out the struct for information of songs
        i++;
    }

    i=0;
    while(i<=s_Count){
        initSongInfo(&s_Info[i]); //calling initSongInfo for the number of songs
        printf("song information : %hu p = %p\n", (unsigned short)s_Info[i].type, &s_Info[i].type);
        i++;
    }

   for (int i = 0; i < s_Count; i++)
   {
       //Here size and name of the song for the current song fetched from here
       s_Info[i].song_name_size = (uint8_t)strlen(s_Names[i]) + 1;
       printf("%d", s_Info[i].song_name_size);
       strcpy((s_Info[i].song_name), s_Names[i]);

       //Here size and name of the next song fetched from here
       s_Info[i].next_song_name_size = (uint8_t)strlen(s_Names[(i + 1) % s_Count]) + 1;
       strcpy((s_Info[i].next_song_name), s_Names[(i + 1) % s_Count]);
   }

    //information about songs id displayed by following
    i=0;
    while(i<s_Count){
        printf("%s\n", songs[i]);
        printf("Song information : %hu p = %p\n", (unsigned short)s_Info[i].type, &s_Info[i].type);
        i++;
    }

    int socket1;            //socket variable
    struct sockaddr_in sin; //object that refers to sockaddr structure
    int length;
    char buffer[BUF_SIZE_SMALL];
    char *mcast_addr; /* multicast address */

    struct timespec time_s;
    time_s.tv_sec = 0;
    time_s.tv_nsec = 20000000L;

    //Multicast address initialisation
    mcast_addr = "239.192.1.10";

    //socket creation for UDP multicase
    if ((socket1 = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed!!"); //Error if socket is not created
        exit(1);
    }

    //build address data structure
    memset((char *)&sin, 0, sizeof(sin));        //setting address variable to 0 using memset
    sin.sin_family = AF_INET;                    //assigning family
    sin.sin_addr.s_addr = inet_addr(mcast_addr); //assigning multicast address
    sin.sin_port = htons(stat_ID_path->port);    //assigning port number
    printf("\nStarting station ID : %d!\n\n", stat_ID_path->id);

    /* Send multicast messages */
    memset(buffer, 0, sizeof(buffer));

    int current_song = -1;
  
    for(;;)
    {
        //Songs one by one choosed from here
        current_song = (current_song + 1) % s_Count;
        //Following is the Pointer for song
        FILE *song = song_Files[current_song];
        //In case when song is not found
        if (song == NULL)
            printf("Song not found!!\n");
        //Print statement for song number and song name
        printf("Current Song number = %d Current Song name= %p\n", current_song, song);
        rewind(song); //Setting pointer to the beginning of file

        int size = BUF_SIZE_SMALL;
        int counter = 0;
        //Printing structure which is sent
        printf("Sending Structure : current Song number= %d. Song_Info->type = %hu p = %p\n", current_song, (unsigned short)s_Info[current_song].type, &s_Info[current_song].type);

        if ((length = sendto(socket1, &s_Info[current_song], sizeof(song_information), 0, (struct sockaddr *)&sin, sizeof(sin))) == -1)
        {
            perror("Server sending failed");
            exit(1);
        }
        // calculating sleep time in accordance with bit rate
        float bit_rate = bit_Rates[current_song];
        idealSleepS = ((BUF_SIZE_SMALL * 8) / bit_rate) * 500000000;

        // if sleep time is less than zero, assigning it to the default value specified
        if (idealSleepS < 0)
            idealSleepS = time_s.tv_nsec;

        // if sleep time is greater than zero, assigning it to the idealSleepS
        if (time_s.tv_nsec > idealSleepS)
            time_s.tv_nsec = idealSleepS;

	for(;!(size < BUF_SIZE_SMALL);)
        {
            // Sending the contents of song
            size = fread(buffer, 1, sizeof(buffer), song);

            if ((length = sendto(socket1, buffer, size, 0, (struct sockaddr *)&sin, sizeof(sin))) == -1)
            {
                perror("server: sendto");
                exit(1);
            }
            if (length != size)
            {
                printf("ERROR!!");
                exit(0);
            }

            // Delaying the in between packet time in order to reduce packet loss at the client side
            // Delaying it with the time assigned to idealSleepS
            nanosleep(&time_s, NULL);
            memset(buffer, 0, sizeof(buffer)); //Setting buffer to 0
        }
    }
    //closing the socket
    close(socket1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    //Assigning command line arguments to variables
    argC = argc;
    argV = argv;
    // Stations is initialised
    Station_Details();

    // Starting TCP Server from here
    //pthread for starting TCP connection is declared and created from here
    pthread_t tTCPid;
    pthread_create(&tTCPid, NULL, start_Station_ListServer, NULL);
    //Starting all stations

    int i=0;
    while(i<NUMBER_OF_STATIONS){
        //thread for each station is being created
        pthread_create(&stationThreads[i], NULL, start_Station, &stationIDPaths[i]);
        i++;
    }

    //waits for the thread tTCPid to terminate
    pthread_join(tTCPid, NULL);
    return 0;
}
