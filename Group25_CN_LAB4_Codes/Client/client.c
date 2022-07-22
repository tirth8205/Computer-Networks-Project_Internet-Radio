/* CSE342 Computer Networks */
/* Client Code
 Team: Krunal Savaj(AU1940271), Krina Khakhariya(AU1940208), Arti Singhal (AU1940181), Axay Ghoghari(AU1940269), Tirth Kanani(AU1920144)
 */

//For compiling use these steps:  
//Step 1 : gcc `pkg-config gtk+-2.0 --cflags` client.c -o client `pkg-config gtk+-2.0 --libs`
//Step 2 : ./client 127.0.0.1

/* Following are the different header files added for socket programming */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <net/if.h>
#include <gtk/gtk.h>

/* Frequently used variables are defined as follows in the code */
#define Server_Port 5432
#define BUFFER_MAX_SIZE 256

#define multi_AddressEs "239.192.1.10"
#define BUFFER_SIZE_SMALL 256
#define BUFFER_SIZE 4096


//Here structure for station info is defined as follows
typedef struct station_information_t 
{
	//uint8_t type;   // = 13;
	uint8_t station_number;
	uint8_t station_name_size;
	char station_name[BUFFER_MAX_SIZE];
	uint32_t multicast_address;
	uint16_t data_port;
	uint16_t info_port;
	uint32_t bit_rate;
} station_information;

//Here structure for station info request is defined as follows
typedef struct station_information_request_t {
	uint8_t type;   // = 1;
} station_information_request;

//Init Function for station_information_request_t
station_information_request initStationInfoRequest(station_information_request *si)
{
	si->type=1;
	return *si;
}

//Here structure for site info is defined as follows
typedef struct site_information_t 
{ 
	uint8_t type;// = 10;
	uint8_t site_name_size;
	char site_name[BUFFER_MAX_SIZE];
	uint8_t site_desc_size;
	char site_desc[BUFFER_MAX_SIZE];
	uint8_t station_count;
	station_information station_list[BUFFER_MAX_SIZE];
} site_information;

//Init Function for station_information
site_information initSiteInfo(site_information *siR)
{
	siR->type=10;	
	return *siR;
}

// station not found structure is defined as follows
typedef struct station_Missing_t
{
	uint8_t type;  // = 11;
	uint8_t station_number;
} station_Missing;

//Here structure for station not found is defined as follows
station_Missing initStationNotFound(station_Missing *snnf)
{ 
	snnf->type=11;
	return *snnf;
}

//Here structure for song info is defined as follows
typedef struct song_information_t 
{
	uint8_t type;   // = 12;
	uint8_t song_name_size;
	char song_name[BUFFER_MAX_SIZE];
	uint16_t remaining_time_in_sec;
	uint8_t next_song_name_size;
	char next_song_name[BUFFER_MAX_SIZE];
} song_information;

//Init Function for song_information
song_information initSongInfo(song_information *sI)
{
	sI->type=12;
	return *sI;
}

// Variables are declared as follows
int type_Change = 0;
int Total_No_Stations;
station_information stations[16];

int cur_VLC_Pid = 0;
int station_Now = 0;
int count = 0;

int force_Close = 0;
int argC;

void Receive_songs(void*);
pthread_t recv_Songs_PID;

// Assiging current current_status to 'run'
char current_status = 'r';


// Function to start the Receive_songs thread to receive multicast messages (streaming songs)
void run_Radio(char* argv[])
{
    pthread_create(&recv_Songs_PID, NULL, &Receive_songs, argv);

    // The pthread_join() function blocks the calling thread until the thread with identifier equal to the 'recv_Songs_PID' i.e. first argument of the funtion terminates.
    pthread_join(recv_Songs_PID, NULL);
}

// To clean the temporary files following is the function for that
void clean_Files()
{
    system("rm tempSong*");
}

// To kill the current running process is the function for that
void* close_function(void* args)
{
    char pid_C[10];  
    char cmnd[256];
    
    // clearing the 'cmnd' and 'pid_C' variables
    memset(cmnd, '\0', 256);
    memset(pid_C, '\0', 10);
    
    strcpy(cmnd, "kill ");
    
    sprintf(pid_C, "%d", cur_VLC_Pid);

    // Appending the pid_C after the "kill " string in the cmnd variable
    memcpy(&cmnd[strlen("kill ")], pid_C, strlen(pid_C));
    
    system(cmnd);
    return NULL;
   
}

// To receive the station list information and displaying it following is the function for that
void Station_list_receive(char * argv[])
{
    int s_T; 
    struct sockaddr_in s_addr;
    char* SERVER_address;
    SERVER_address = argv[1];
    
    // Creating TCP socket
    if ((s_T = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("receiver: socket");
        exit(1);
    }

    // Initializing address data structure and attaching it to port
    bzero((char *) &s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(SERVER_address);;
    s_addr.sin_port = htons(Server_Port);
    
    // connecting the socket with server
    
    for(int st=-1;st<0;)
    	st = connect(s_T, (struct sockaddr *) &s_addr, sizeof(s_addr)); 
    
    
    char buffer[BUFFER_SIZE_SMALL];
    int r_Bytes = BUFFER_SIZE_SMALL;
    
    send(s_T, "name.txt\n", 9, 0);
    
    uint32_t No_Stations = 0;

    read(s_T, &No_Stations, sizeof(uint32_t));
    No_Stations = ntohl(No_Stations);
    Total_No_Stations = No_Stations;
    
    //Counting Total number of stations
    printf("No. of Stations : %d\n", No_Stations);
    if(No_Stations > 16)
    {
        printf("Too many stations!!!\n");
        exit(0);
    }
    
    int i=0;
    station_information* sI = malloc(sizeof(station_information));
    
    // Printing the station information
    for(i=0;i<No_Stations;i++)
    {
        read(s_T, sI, sizeof(station_information));
	printf("---- STATION INFO %d ------\n", i);
        printf("Station No. %hu\n", sI->station_number);
	printf("Station multicast Port : %d\n\n", ntohs(sI->data_port));
        printf("Station Name : %s\n", sI->station_name);
        
        memcpy(&stations[i], sI, sizeof(station_information));
    }

    close(s_T);
}

// Function to create a UDP packet and receiving multicast messages (songs) streaming on the user specified station
void Receive_songs(void* args)
{
    printf("\nReceiving Songs \n");
    char** argv = (char**)args;
    
    int s;                  
    struct sockaddr_in sin; 
    char *if_Name;          
    struct ifreq ifr;       
    char buffer[BUFFER_SIZE];
    int length;

    // Multicast Related Information is as follows
    char *multi_Addr;               
    struct ip_mreq multicast_req;       
    struct sockaddr_in muticast_saddr; 
    socklen_t muticast_saddr_len;
    
    if(argC == 3) {
        if_Name = argv[2];
    }
    else
        if_Name = "wlan0";
    
    
    // UDP socket for multicating messages
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("receiver: socket");
        exit(1);
    }
    
    // Setting and assigning unique port for different stations
    int multicast_port;

    if(station_Now > Total_No_Stations)
    {
	// setting default first station when the station number is not valid
        printf("No such station exists. Reverting to default station\n");
        multicast_port = 2400;
        station_Now = 0;

    } else {
	
	// setting the required station port if the station is valid
        multicast_port = ntohs(stations[station_Now].data_port);
    }
    
    // build address data structure 
    memset((char *)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(multicast_port);
    
    
    // Use the interface specified 
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name , if_Name, sizeof(if_Name)-1);
    
    if ((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&ifr, sizeof(ifr))) < 0)
    {
        perror("receiver: setsockopt() error");
        close(s);
        exit(1);
    }
    
    // bind the socket 
    if ((bind(s, (struct sockaddr *) &sin, sizeof(sin))) < 0) 
    {
        perror("receiver: bind()");
        exit(1);
    }
    
    /* Multicast specific code follows */
    
    // build IGMP join message structure 
   // joining the multicast group where all the stations reside on different ports
    multicast_req.imr_multiaddr.s_addr = inet_addr(multi_AddressEs);
    multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
    
    // send multicast join message 
    if ((setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &multicast_req, sizeof(multicast_req))) < 0) 
    {
        perror("mcast join receive: setsockopt()");
        exit(1);
    }
    
    // receiving multicast messages 
    FILE* fp;
    int counter = 0;
    count=0;

    printf("\nLets Hear the song!\n\n");

    song_information* song_Information = (song_information*) malloc(sizeof(song_information));
    char* temp_Songs[2];
    temp_Songs[0] = "tempSong1.mp3";
    temp_Songs[1] = "tempSong2.mp3";
    
    int type_Change = 0;
    int current=0;
    char* tempSong = temp_Songs[current];

    for(;;)
    {
        if(type_Change) 
	{
            tempSong = temp_Songs[1 ^ current];
        }
        
        // Reset sender structure
        memset(&muticast_saddr, 0, sizeof(muticast_saddr));
        muticast_saddr_len = sizeof(muticast_saddr);
        
        // Clear buffer and receive 
        memset(buffer, 0, sizeof(buffer));
        if ((length = recvfrom(s, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&muticast_saddr, &muticast_saddr_len)) < 0) 
	{
            perror("receiver: recvfrom()");
            exit(1);
        } else {
            count = count + length;
       
            if(count >= 387000) 
	    {
         	// Detaching the thread 
		printf("\nSongs streaming!\n");
                pthread_detach(pthread_self());
                pthread_exit(NULL);
                
            }
        }
        
        // Song information is being displayed
        uint8_t temp_type;
        if(length == sizeof(song_information)) 
	{
            printf("length = %d. Checking if song_Information...\n", length);
            memcpy(song_Information, buffer, length);

            printf("type of Song info = %hu\n", song_Information->type);
            temp_type = (uint8_t)song_Information->type;
            
	    // type = 12 means song info structure
            if(temp_type == 12)
	    {
		// printing current song and next song name
                printf("Current Song : %s\n", song_Information->song_name);
                printf("Next Song : %s\n", song_Information->next_song_name);
                continue;
            }
            else {
                printf("Not here!\n");
	    }
        }
        
        if(counter++ == 10)
	{
            cur_VLC_Pid = fork();
            if(cur_VLC_Pid == 0)
	    {
		// execlp replaces the calling process image with a new process image. This has the effect of running a new program with 			the process ID of the calling process
                execlp("/usr/bin/cvlc", "cvlc", tempSong, (char*) NULL);

		// first arg :  /usr/bin/cvlc = path to new process image
		// arg 2 to NULL : list of arguments passed to new process image
		// second arg: cvlc = name of the executable file for the new process image
		
            }
        }
        
        fp = fopen(tempSong, "ab+");
        fwrite(buffer, length, 1, fp);
        fclose(fp);
        
        // Add code to send multicast leave request 
        if(force_Close == 1)
            break;
    }
    
    close(s);
    force_Close = 0;
    fp = fopen(tempSong, "wb");
    fclose(fp);
}



// Action listener for station 1
void clicked_Station1(GtkWidget *widget, gpointer data, char * argv[]) 
{
	g_print("Station 1 tuning!!\n");
	close_function(NULL);
        force_Close = 1;
        clean_Files();
        int station=1;
        station_Now = station - 1;
        current_status = 'c';
	//gtk_window_close(widget);
	
}



// Action listener for station 2
void clicked_Station2(GtkWidget *widget, gpointer data, char * argv[]) 
{
	g_print("Station 2 tuning!!\n");
	close_function(NULL);
        force_Close = 1;
        clean_Files();
        int station=2;
        station_Now = station - 1;
        current_status = 'c';
}

// Action listener for station 3
void clicked_Station3(GtkWidget *widget, gpointer data, char * argv[]) 
{
	g_print("Station 3 tuning!!\n");
	close_function(NULL);
        force_Close = 1;
        clean_Files();
        int station=3;
        station_Now = station - 1;
        current_status = 'c';
}



// Action listener for station 4
void clicked_Station4(GtkWidget *widget, gpointer data, char * argv[]) 
{
	g_print("Station 4 tuning!!\n");
	close_function(NULL);
        force_Close = 1;
        clean_Files();
        int station=4;
        station_Now = station - 1;
        current_status = 'c';
}

// Action listener for Running the Radio on the current station
void clicked_but(GtkWidget *widget, gpointer data, char * argv[]) 
{
    printf("Running\n");
    run_Radio(argv);
    current_status = 'r';
}



// Action listener for Pausing the Radio
void clicked_but1(GtkWidget *widget, gpointer data) 
{
    printf("\n\nPausing\n");
    close_function(NULL);
    //pthread_cancel(recv_Songs_PID);
    force_Close = 1;
    //clean_Files();
    current_status = 'p';
}



// Action listener for Stopping the radio
void clicked_but3(GtkWidget *widget, gpointer data) 
{
    g_print("\n\nExiting! \n\n");
    close_function(NULL);
    force_Close = 1;
    clean_Files();
    exit(0);
}



// Action listener for displaying station list
void clicked_but2(char* argv[])
{
    // Variables for GUI station list Window is being Initialized
    GtkWidget *Window;
    GtkWidget *Halign;
    GtkWidget *vBox;
    GtkWidget *button;
    GtkWidget *button1;
    GtkWidget *button2;
    GtkWidget *button3;
    GtkWidget *labeL;

    int argc = 1;
    gtk_init(&argc, &argv);
    char o;
    char l0 = 'r';
    int station, f=1;
    
    // Creation of the Station List GUI Window
    Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(Window), "INTERNET RADIO");
    gtk_window_set_default_size(GTK_WINDOW(Window), 230, 150);
    gtk_container_set_border_width(GTK_CONTAINER(Window), 16);
    gtk_window_set_position(GTK_WINDOW(Window), GTK_WIN_POS_CENTER);
    vBox = gtk_vbox_new(TRUE,1);
    gtk_container_add(GTK_CONTAINER(Window), vBox);

    // Header labeL for the Station List Window
    button3 = gtk_frame_new ("");
    labeL = gtk_label_new ("Welcome to Rj KATAK!!!");
  
    gtk_container_add (GTK_CONTAINER (button3), labeL);
    gtk_box_pack_start (GTK_BOX (vBox), button3, TRUE, TRUE, 0);    
    
    //labels to the 3 buttons corresponding to 3 stations are assigned as follows
    button = gtk_button_new_with_label("Bollywood Dhamaka");
    button1 = gtk_button_new_with_label("Hollywood Chill");
    button2 = gtk_button_new_with_label("Romantic");
    button3 = gtk_button_new_with_label("KATAK Special");
    
    // Setting button's length and width
    gtk_widget_set_size_request(button, 80, 40);
    gtk_widget_set_size_request(button1, 80, 40);
    gtk_widget_set_size_request(button2, 80, 40);
    gtk_widget_set_size_request(button3, 80, 40);
    
    // gtk Box pack widgets (buttons) into a GtkBox from start to end
    gtk_box_pack_start(GTK_BOX(vBox), button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vBox), button1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vBox), button2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vBox), button3, TRUE, TRUE, 0);    

    // Buttons to their respective station listeners is being connected as follows
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clicked_Station1), argv);
    g_signal_connect(G_OBJECT(button1), "clicked", G_CALLBACK(clicked_Station2), argv);
    g_signal_connect(G_OBJECT(button2), "clicked", G_CALLBACK(clicked_Station3), argv);
    g_signal_connect(G_OBJECT(button3), "clicked", G_CALLBACK(clicked_Station4), argv);

    g_signal_connect(G_OBJECT(Window), "destroy",G_CALLBACK(gtk_main_quit), G_OBJECT(Window));
    
    // The Window is being displayed as follows
    gtk_widget_show_all(Window);
    gtk_main();
}

// MAIN FUNCTION
int main(int argc, char * argv[])
{
    argC = argc;
   
    // Receiving and printing Station List coming from server
    Station_list_receive(argv);

    // Variables for GUI main Window is being intialised as follows
    GtkWidget *Window;
    GtkWidget *Halign;
    GtkWidget *vBox;
    GtkWidget *button;
    GtkWidget *button1;
    GtkWidget *button2;
    GtkWidget *button3;
    gtk_init(&argc, &argv);

    printf("\nIn Main Function\n");
    char o;
    char l0 = 'r';
    int station;
    int f=1;
    
   
    // Creation of the Station List GUI Window
    Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(Window), "INTERNET RADIO");
    gtk_window_set_default_size(GTK_WINDOW(Window), 230, 150);
    gtk_container_set_border_width(GTK_CONTAINER(Window), 16);
    gtk_window_set_position(GTK_WINDOW(Window), GTK_WIN_POS_CENTER);
    vBox=gtk_vbox_new(TRUE,1);
    gtk_container_add(GTK_CONTAINER(Window), vBox);
    
    //Labels to the 4 buttons corresponding to 4 options given to user is being assigned 
    button=gtk_button_new_with_label("Play radio");
    button1=gtk_button_new_with_label("Pause");
    button2=gtk_button_new_with_label("Change station");
    button3=gtk_button_new_with_label("Stop radio");
    
    // Setting button's length and width
    gtk_widget_set_size_request(button, 80, 40);
    gtk_widget_set_size_request(button1, 80, 40);
    gtk_widget_set_size_request(button2, 80, 40);
    gtk_widget_set_size_request(button3, 80, 40);
    
    // gtk Box pack widgets (buttons) into a GtkBox from start to end
    gtk_box_pack_start(GTK_BOX(vBox), button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vBox), button1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vBox), button2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vBox), button3, TRUE, TRUE, 0);


     // Buttons to their respective station listeners is being connected
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clicked_but), argv);
    g_signal_connect(G_OBJECT(button1), "clicked", G_CALLBACK(clicked_but1), NULL);
    g_signal_connect(G_OBJECT(button2), "clicked", G_CALLBACK(clicked_but2), argv);
    g_signal_connect(G_OBJECT(button3), "clicked", G_CALLBACK(clicked_but3), NULL);

    g_signal_connect(G_OBJECT(Window), "destroy",G_CALLBACK(gtk_main_quit), G_OBJECT(Window));
    
    gtk_widget_show_all(Window);
    
    gtk_main();

    return 0;
}
