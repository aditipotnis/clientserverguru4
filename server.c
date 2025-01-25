#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "database.h"
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void list(Database *db) {
    printf("%-20s %-12s %-23s %-50s\n", "Handle", "| Followers", "| Last Modified", "| Comment");
    printf("-------------------- ------------ ----------------------- --------------------------------------------------\n");

    for (int i = 0; i < db->size; i++) {
        Record *record = db_index(db, i);

        // Truncate handle to 20 characters if necessary
        char handle[21];
        strncpy(handle, record->handle, 20);
        handle[20] = '\0';

        // Truncate comment to 50 characters if necessary
        char comment[51];
        strncpy(comment, record->comment, 50);
        comment[50] = '\0';

	time_t epocTime = record->dateLastModified;
    	struct tm *timeinfo = localtime(&epocTime);
    	char buffer[80];
    	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);

        printf("%-20s |%-12lu|%-23s|%-50s\n", handle, record->followerCount, buffer, comment);
    }
    printf("-------------------- ------------ ----------------------- --------------------------------------------------\n");
}


int validate_handle(char const *handle){

	//Check if the first character is @
	if (handle[0] != '@') {
		printf("Oops! your handle must start with @\n");
		return -1;
	}

	//Check if the handle contains a comma
	if (strchr(handle, ',') != NULL){
		printf("commas in your handle? really??\n");
		return -1;
	}

	//Check if handle exceeds designated length
	if (strlen(handle) > 30) {
		 printf("handle way too long\n");
       		 return -1;
        }

	return 0;
}

int validate_comment(char const *comment){

	//Check if comments contain commas
	if (strchr(comment, ',') != NULL){
                printf("oops, comments can't have commas\n");
                return -1;
        }

	//Check if the string exceeds length
	if (strlen(comment) > 50) {
		printf("comment way too long\n");
		return -1;
	}

	return 0;
}

void add_update(Database *db, char* handle, int followers, char *comment){

	Record *record;
	record = db_lookup(db, handle);	
	bool record_exists = true;

	//Create record if record does not exist
	if (record == NULL) {
		record_exists = false;
        	record = malloc(sizeof(Record));
		strncpy(record->handle, handle, sizeof(record->handle));
        	record->handle[sizeof(record->handle) - 1] = '\0';
        }	

	//Assign appropriate fields
	strncpy(record->comment, comment, sizeof(record->comment));
	record->comment[sizeof(record->comment) - 1] = '\0';
	
	record->followerCount = followers;
	record->dateLastModified = time(NULL);
	
	//Append record to database if it does not exist
	if (!record_exists){
		db_append(db, record);
		free(record);
	}
}

void error(const char *msg){
	perror(msg);
	exit(1);
}

void send_prompt(int sockfd, const char *prompt) {
    int n = write(sockfd, prompt, strlen(prompt));
    if (n < 0) {
        error("Error on writing");
    }
}

int main_loop(Database * db, int newsockfd) {
	char input[80];
    	char *command;
    	char *handle;
    	char *followers_str;
    	long followers;
	char *extra_input;
	bool changes_saved = true;
	char buffer[1024];
	int n;

	//Print starter prompt
	bzero(buffer, 1023);
	printf("Loaded %d records\nEnter help to see commands\n",db->size);

	while (1) {
		
		bzero(buffer, 1023);
		n = read(newsockfd, buffer, 1023);
		if (n < 0){ error("Error on reading");}

		printf("> ");
		
		bzero(buffer, 1023);
		//Check for EOF or CTRL+D 
		if (fgets(buffer,sizeof(buffer), stdin) == NULL){
			break;
		}
	
		buffer[strcspn(buffer, "\n")] = '\0'; 
		n = write(newsockfd, buffer, strlen(buffer));
		if (n < 0) { error("Error on writing");}
		//Tokenize command handle and follower count
		command = strtok(buffer, " ");
        	handle = strtok(NULL, " ");
        	followers_str = strtok(NULL, " ");
		extra_input = strtok(NULL, " ");
		

		//Check for list command
		if (strcmp(command, "list") == 0) {
			if (handle != NULL){
				printf("list command only takes 1 input\n");
				continue;
			}
            		list(db);
		}

		//Check for help command
		else if (strcmp(command, "help") == 0){
			if (handle != NULL){
				printf("help only takes 1 input\n");
				continue;
			}
			printf("list - tabulates database\nadd HANDLE FOLLOWERS\nupdate HANDLE FOLLOWERS\nsave - saves changes\nexit - exit program\n");
		}
		
		//Check for add or update command
		else if (strcmp(command, "add") == 0 || strcmp(command, "update") == 0) {
			
			//Check if the user has entered the right inputs
			if (handle == NULL || followers_str == NULL) {
                                printf("too short\n");
                                continue;
                        }

			//Check if the user has entered extra inputs
                        else if (extra_input != NULL){
                                printf("too long\n");
                                continue;
                        }

			//Check if record exists for both update and add command
			if (strcmp(command, "update") == 0){
				Record *record_exist;
        			record_exist = db_lookup(db, handle);
				if (record_exist == NULL){
					printf("record does not exist!!\n");
					continue;
				}
			}
			if (strcmp(command, "add") == 0){
                                Record *record_exist;
                                record_exist = db_lookup(db, handle);
                                if (record_exist != NULL){
                                        printf("record already exists!!\n");
					continue;
                                }
                        }


			//If handle isn't validated skip this iteration
			if (validate_handle(handle) != 0){
				continue;
			}

			//Convert string to long for follower count
			char *endptr;
			errno = 0;
                        followers = strtol(followers_str, &endptr, 0);
                        if ((*endptr != '\0' || followers < 0) || (followers == LONG_MIN || followers == LONG_MAX) && errno == ERANGE){
                                printf("follower count has to be a positive integer\n");
                                continue;
                        }

			
			//memset(comment_input, 0, sizeof(comment_input));
			send_prompt(newsockfd, "> Comment: ");
			char comment_input[50];
			n = read(newsockfd, comment_input, sizeof(comment_input));
			if (n < 0){
				error("Error on reading");
			}
			comment_input[strcspn(comment_input, "\n")] = '\0';
		

			//Validate the comment input
			if (validate_comment(comment_input) != 0){
				continue;
			}
			//bzero(comment_input, 49);
			
			//Call add_update to make changes to the database
			add_update(db, handle, followers, comment_input);
			printf("%s operation successful!\n",command);
			changes_saved = false;
			
               }
		else if (strcmp(command, "save") == 0){

			//Modify the changes_saved flag and write to the database.csv file
			changes_saved = true;
			db_write_csv(db, "database.csv");
			printf("Wrote %d records\n",db->size);
		}

		else if (strcmp(command, "exit") == 0){

			//Exit after checking if the changes are saved
			if (!changes_saved){
				printf("You have unsaved changes use exit fr to exit anyways\n> ");
				char exitCode[8];
				fgets(exitCode, 8, stdin);
				int c;
				while ((c = getchar()) != '\n' && c != EOF) {} 
				if (strcmp(exitCode, "exit fr") != 0){
					printf("%s is not a valid command :(\n",exitCode);
					continue;
				}
			}			
			printf("bye! see you later\n");
			break;
		}

		else{
			printf("please enter a valid command\n");
		}
	}
	close(newsockfd);
	//Free the database
	db_free(db);
}

int main(int argc, char *argv[]){
    Database db = db_create();
    db_load_csv(&db, "database.csv");

    if (argc < 2){
	    fprintf(stderr, "Port number not provided\n");
	    exit(1);
    }

    int sockfd, newsockfd, portno, n;
    //char buffer[1024];

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
	    error("Error opening socket.");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){error("Binding failed.");}
    listen(sockfd, 5);
    clilen =sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0){ error("Error on accept");}
	
    close(sockfd);

    return main_loop(&db, newsockfd);    
}
