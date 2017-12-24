/**********************************************************************
*** This is a util.c file used at both client and the server 
*** it has common functions shared between client and server
*** Autor: Shekhar 
*** REferences: Websites like Stackoverflow etc..
**********************************************************************/
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/md5.h>
#include "util.h"
#include <time.h>
#include <sys/stat.h>


/**********************************************************************
*** strToInteger function is to get to know if the str can be converted to an integer
*** if not it provides appropriate message before it exits the program 
*** str: the argumet is the string which is to be tested - if its an integer/not 
**********************************************************************/
int strToInteger(char * str) {
  char * endptr;
  errno = 0;
  long l = strtol(str, & endptr, 0);
  if (errno == ERANGE || * endptr != '\0' || str == endptr) {
    printf("Invalid  Number\n");
    exit(-1);
  }
  // Only needed if sizeof(int) < sizeof(long)
  if (l < INT_MIN || l > INT_MAX) {
    printf("Invalid  Number\n");
    exit(-1);
  }
  return (int) l;
}


/**********************************************************************
*** fileExist function  is to check if the file exists in the current directory or not
*** filename: is the filename which needs to be check if it exists in the cuurrent directory
**********************************************************************/
int fileExist(const char * filename) {
  DIR * dirp;
  struct dirent * dp;
  long len = strlen(filename);
  dirp = opendir(".");
  if ((dirp = opendir(".")) != NULL) {
    while ((dp = readdir(dirp)) != NULL) {
      if (strcmp(dp->d_name, filename) == 0) {
        (void) closedir(dirp);
        return 1;
      }
    }
    (void) closedir(dirp);
    printf("the file name :%s does not exist in current directory\n", filename);
  } else {
    printf("Can not open the current directory\n");
  }
  return 0;
}



/**********************************************************************
*** listFileInCurrentDirectory function  displays all the files in the current directory
*** dosenot take any argumenets
**********************************************************************/
char* listFileInCurrentDirectory(char * path, int * fileExists){
  DIR * dirp;
  struct dirent * dp;fflush(stdout);
  char *listFiles = (char*)malloc(MAXLIMIT);
  memset(listFiles,'\0',sizeof(listFiles));
  dirp = opendir(".");
  if ((dirp = opendir(path)) != NULL) {
    while ((dp = readdir(dirp)) != NULL) {
      if(dp-> d_type != DT_DIR) 
	{
      strcat(listFiles,dp->d_name);
      strcat(listFiles,FILE_DELIMITER);
      *fileExists =1;
	}
    }
    (void) closedir(dirp);
  } else {
    strcat(listFiles,"Can not open the current directory\n");
  }
  return listFiles;  
}

/**********************************************************************
*** encryptDecryptData function encrypts caeser cipher the data with a key (knwon to both server and the client )
*** data : is the data to be encrypted
*** len : is the length of the data which need to be encrpted
**********************************************************************/
void encryptDecryptData(char * data, unsigned int len, char * password, unsigned int passlen)
{
    unsigned int i;
    unsigned int j=0;
    for(i=0;i<len;i++)
    {
	if(j==passlen)j=0;
        data[i] = data[i] ^ password[j];
	j++;
    }
}


/***********************************************************************************
*** getFileNameFromDirectoryString function gets the fileName from the lengthy DIrectory names 
*** ex : /images/fileName.txt this function gets filename fileName.txt
***********************************************************************************/
char* getFileNameFromDirectoryString(char* token)
{
    char* temp;
    char* tokenCopy;
    tokenCopy = (char*)malloc(strlen(token));
    strcpy(tokenCopy, token);

    DEBUG(printf("this is before first substring: %s\n", token));
    temp = strtok(token, "/");
    DEBUG(printf("this is after first substring: %s\n this is the token now : %s\n", temp, token));
    while (temp != NULL) {
        if (temp != NULL) {
            memset(tokenCopy, '\0', strlen(tokenCopy));
            strcpy(tokenCopy, temp);
            DEBUG(printf("\nthei is the string: %s\n", tokenCopy));
        }
        temp = strtok(NULL, "/");
    }
    return tokenCopy;
}

/***********************************************************************************
*** show_dir_content : A recursive function to search a file of interest recursively in a given directory
*** reference : https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
*** path : the the path in which the search  has to be done
*** fileName: the file Name to search
*** fileExixts: is a global flag which will be updated respective to the existence of the file
***********************************************************************************/

void show_dir_content(char* path, char* fileName, int* fileExists, char * fileNameArray[])
{
    DIR* d = opendir(path); // open the path
    if (d == NULL)
        return; // if was not able return
    struct dirent* dir; // for the directory entries
    while ((dir = readdir(d)) != NULL) // if we were able to read somehting from the directory
    {
        if (dir->d_type != DT_DIR) // if the type is not directory see if the file we are searching is present
        {
            if (strstr(dir->d_name, fileName)!=NULL) {
		//if(strcmp() // you can compare the files with strcat with .0,.1.2.3               
		DEBUG(printf("FOund THe file %s\n", dir->d_name));
                sprintf(fileNameArray[ *fileExists ], "%s/%s", path, dir->d_name);
	        DEBUG(printf("the file name is %s\n", fileNameArray[ *fileExists ]));
		//fileNameArray++;
                *fileExists += 1;
            }
        }
        else if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) // if it is a directory
        {
            char d_path[255]; // here I am using sprintf which is safer than strcat
            sprintf(d_path, "%s/%s", path, dir->d_name);
            show_dir_content(d_path, fileName, fileExists, fileNameArray); // recall with the new path
        }
    }
    closedir(d); // finally close the directory
}

/***********************************************************************************
*** compare_filename_ext function : compares if the file extension exists in the ws.conf; replies -1 if not found
*** filename : the filename with extension  
*** replyfileType:  will be updated with the appropirate ws.conf filetype for browsers if the file exists
***********************************************************************************/
int compare_filename_ext(const char* filename, char* replyfileType)
{
    const char* dot = strrchr(filename, '.');
    DEBUG(printf("*************************************\nfilename :%s\n", filename));
    DEBUG(printf("dot name :%s\n", dot));
    if (!(!dot || dot == filename)) {
       /* DEBUG(printf("row size:%d\n", (int)(sizeof(_global_Content_Type) / sizeof(_global_Content_Type[0]))));
        for (int i = 0; i < (sizeof(_global_Content_Type) / sizeof(_global_Content_Type[0])), _global_Content_Type[i][0] != NULL; i++) {
            if (strcmp(_global_Content_Type[i][0], dot) == 0) {

                DEBUG(printf("value : %s\n", _global_Content_Type[i][0]));
                strcpy(replyfileType, _global_Content_Type[i][1]);
                return 0;
            }
        }*/
    }
    return -1;
}

unsigned int _log2( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return 1<<ans ;
}

char * fileNameFromDirectoryString(char * fullPath)
{
	if(!(fullPath!=NULL))
	    return NULL;
	char * fileName = (char *) malloc(sizeof(strlen(fullPath)));
	fileName = strrchr(fullPath, '/');
        if(fileName!=NULL)
            return ++fileName;	
  	return NULL;
}

int getFileChunkName(char * fileName, int index)
{
       if(!(fileName!=NULL)) return -1;
       sprintf(fileName,"%s.%d", fileName, index);
       return 0; 
}







#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#define PATH_MAX_STRING_SIZE 256

/* recursive mkdir */
int mkdir_p(const char *dir, const mode_t mode) {
    char tmp[PATH_MAX_STRING_SIZE];
    char *p = NULL;
    struct stat sb;
    size_t len;

    /* copy path */
    strncpy(tmp, dir, sizeof(tmp));
    len = strlen(tmp);
    if (len >= sizeof(tmp)) {
        return -1;
    }

    /* remove trailing slash */
    if(tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    /* recursive mkdir */
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            /* test path */
            if (stat(tmp, &sb) != 0) {
                /* path does not exist - create directory */
                if (mkdir(tmp, mode) < 0) {
                    return -1;
                }
            } else if (!S_ISDIR(sb.st_mode)) {
                /* not a directory */
                return -1;
            }
            *p = '/';
        }
    }
    /* test path */
    if (stat(tmp, &sb) != 0) {
        /* path does not exist - create directory */
        if (mkdir(tmp, mode) < 0) {
            return -1;
        }
    } else if (!S_ISDIR(sb.st_mode)) {
        /* not a directory */
        return -1;
    }
    return 0;
}


void getMD5sum(char * fileName, char * md5string)
{
   if(fileName!=NULL){
	unsigned char digest[16];
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, fileName, strlen(fileName));
	MD5_Final(digest, &context);
	for(int i = 0; i < 16; i++) {
		char buf[16];
		sprintf(buf,"%02x", digest[i]);
		strcat(md5string,buf); 
	    }
	   // printf("\nThe md5 in buffer is : %s\n", md5string); 
   }
}

int checkFileTimeout(char * fileName,  long int timeout)
{
    FILE * fileOpen = NULL;
    if((fileOpen = fopen(fileName, "rb+"))!=NULL){
	    printf("file is present in cache\n	");
	    struct stat stbuf;
	    stat(fileName,&stbuf);
	    long int timeDiff= ((long int)time(NULL)- (long int)stbuf.st_mtime);
	    printf("Last Modifed in (s): %ld", timeDiff);
	    if(timeDiff>timeout)
	    {
	       if(remove(fileName)<0)printf("\nError Deleting the file:%s\n",fileName);
	       return 0;
	    }else
	    {
	       return 1;
	    }
	fclose(fileOpen);
    }else{
    return 0;
    }
}

