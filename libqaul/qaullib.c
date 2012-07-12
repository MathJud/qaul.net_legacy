/*
 * qaul.net is free software
 * licensed under GPL (version 3)
 */

#include "qaullib_private.h"

// ------------------------------------------------------------
void Qaullib_Init(const char* resourcePath)
{
	int rc, i;

	// -------------------------------------------------
	// define global variables
	qaul_new_msg = 0;
	ipc_connected = 0;
	qaul_username_set = 0;
	qaul_locale_set = 0;
	qaul_ip_set = 0;
	qaul_gui_pagename_set = 0;
	app_event = 0;
	pickFileCheck = 0;
	qaul_chunksize = 2000000000; // todo: smaller chunk size
	qaul_configured = 0;
	qaul_loading_wait = 1;
	qaul_conf_quit = 0;
	qaul_conf_voip = 0;

	// -------------------------------------------------
	// create buffers for socket communication
	for(i=0; i<MAX_USER_CONNECTIONS; i++)
	{
		userconnections[i].conn.connected = 0;
		// get memory for buffer
	    memset(&userconnections[i].conn.buf, 0, BUFFSIZE + 1);
	}

	for(i=0; i<MAX_FILE_CONNECTIONS; i++)
	{
		fileconnections[i].conn.connected = 0;
		// get memory for buffer
	    memset(&fileconnections[i].conn.buf, 0, BUFFSIZE + 1);
	}

	// -------------------------------------------------

	printf("Qaullib_Init:\n");
	printf("path: %s\n", resourcePath);

	// set webserver path
	strcpy(webPath, resourcePath);
#ifdef WIN32
	strcat(webPath, "\\www");
#else
	strcat(webPath, "/www");
#endif

	// set db path
	strcpy(dbPath, resourcePath);
#ifdef WIN32
	strcat(dbPath, "\\qaullib.db");
#else
	strcat(dbPath, "/qaullib.db");
#endif

	// check if db exists
	int dbExists = Qaullib_FileExists(dbPath);

	// configure sqlite
	// make sure sqlite is in fullmutex mode for multithreading
	if(sqlite3_config(SQLITE_CONFIG_SERIALIZED) != SQLITE_OK) printf("SQLITE_CONFIG_SERIALIZED error\n");
	// open database
	rc = sqlite3_open(dbPath, &db);
	if( rc ){
		//fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}
	// initialize Database
	Qaullib_DbInit();
	// insert filesharing
	if(dbExists == 0)
	{
		Qaullib_FilePopulate();
		Qaullib_DbPopulateConfig();
	}

	// initialize linked list
	Qaullib_UserInit ();

#ifdef WIN32
	// needs to be called before socket()
	WSADATA data;
	WSAStartup(MAKEWORD(2,2), &data);
#endif // WIN32
}

// ------------------------------------------------------------
void Qaullib_Exit(void) //destructor
{
	if(ipc_connected)
	{
		// send exit message to olsrd
		Qaullib_IpcSendCom(0);
		usleep(200000);
		printf("Qaullib exit message sent\n");
		Qaullib_IpcClose();
	}
	else printf("ipc not connected\n");

	// clean up qaullib
	sqlite3_close(db);

#ifdef WIN32
	(void) WSACleanup();
#endif // WIN32
}

// ------------------------------------------------------------
// configuration
// ------------------------------------------------------------
void Qaullib_ConfigStart(void)
{
	qaul_loading_wait = 0;
}

void Qaullib_SetConfQuit(void)
{
	qaul_conf_quit = 1;
}

void Qaullib_SetConfVoIP(void)
{
	qaul_conf_voip = 1;
}

int Qaullib_ExistsLocale(void)
{
	// if username is set return it
	if (qaul_locale_set) return qaul_locale_set;

	// check if username is in Database
	qaul_locale_set = Qaullib_DbGetConfigValue("locale", qaul_locale);
	return qaul_locale_set;
}

const char* Qaullib_GetLocale(void)
{
	// if username is set return it
	if (qaul_locale_set) return qaul_locale;

	// check if username is in Database
	if (Qaullib_DbGetConfigValue("locale", qaul_locale)) return qaul_locale;

	// no username found
	return "";
}

void Qaullib_SetLocale(const char* locale)
{
	Qaullib_DbSetConfigValue("locale", locale);
	strcpy(qaul_locale, locale);
	qaul_locale_set = 1;
}

// ------------------------------------------------------------
// timed functions
// ------------------------------------------------------------
void Qaullib_TimedSocketReceive(void)
{
	// check ipc socket
	Qaullib_IpcReceive();
	// check user & file sockets
	Qaullib_UserCheckSockets();
	Qaullib_FileCheckSockets();
}

int Qaullib_TimedCheckAppEvent(void)
{
	int tmp_event = app_event;
	app_event = 0;
	return tmp_event;
}

void Qaullib_TimedDownload(void)
{
	// download user names
	Qaullib_UserCheckNonames();
	// dowload scheduled files
	Qaullib_FileCheckScheduled();
	// delete users
	Qaullib_User_LL_Clean();
}

// ------------------------------------------------------------
// App Events
// ------------------------------------------------------------
const char* Qaullib_GetAppEventOpenPath(void)
{
	return qaullib_AppEventOpenPath;
}

// ------------------------------------------------------------
int Qaullib_WebserverStart(void)
{
	static const char *options[] = {
	  "document_root", webPath,
	  "listening_ports", CHAT_PORT,
	  "num_threads", "10",
	  NULL
	};
	ctx = mg_start(&Qaullib_WwwEvent_handler, options);
	//mg_set_option(ctx, "dir_list", "no");
	//assert(ctx != NULL);
	if( ctx == NULL )
	{
		fprintf(stderr, "Can't open web server\n");
		//exit(1);
		return 0;
	}

	return 1;
}




// ------------------------------------------------------------
// SQLite functions
// ------------------------------------------------------------
int Qaullib_DbInit(void)
{
	// create msg-table
	if(sqlite3_exec(db, sql_msg_table, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("creating message table failed \n");
	}
	else if(sqlite3_exec(db, sql_msg_index, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("creating message index failed \n");
	}

	// create config-table
	if(sqlite3_exec(db, sql_config_table, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("creating config table failed \n");
	}

	// create user table
	if(sqlite3_exec(db, sql_user_table, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("creating user table failed \n");
	}
	else if(sqlite3_exec(db, sql_user_index, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("creating user index failed \n");
	}

	// create file table
	if(sqlite3_exec(db, sql_file_table, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("creating file table failed \n");
	}
	else if(sqlite3_exec(db, sql_file_index, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("creating file index failed \n");
	}

	return 1;
}

// ------------------------------------------------------------
// protect functionality
const char* Qaullib_ProtectString(const char* unprotected_string)
{
	bstring b = bfromcstr(unprotected_string);
	int i = bfindreplace(b, bfromcstr("\\"), bfromcstr("\\\\"), 0);
	i = bfindreplace(b, bfromcstr("'"), bfromcstr("\'"), 0);
	return bdata(b);
}

const char* Qaullib_UnprotectString(const char* protected_string)
{
	bstring b = bfromcstr(protected_string);
	int i = bfindreplace(b, bfromcstr("\'"), bfromcstr("'"), 0);
	i = bfindreplace(b, bfromcstr("\\\\"), bfromcstr("\\"), 0);
	return bdata(b);
}

// ------------------------------------------------------------
// get and set configuration
void Qaullib_DbSetConfigValue(const char* key, const char* value)
{
	char buffer[10240];
	char *stmt = buffer;
	char *error_exec=NULL;

	// delete old entries (if exist)
	sprintf(stmt, sql_config_delete, key);
	if(sqlite3_exec(db, stmt, NULL, NULL, &error_exec) != SQLITE_OK)
	{
		printf("SQLite error: %s\n",error_exec);
		sqlite3_free(error_exec);
		error_exec=NULL;
	}

	// insert new IP
	sprintf(stmt, sql_config_set,key,value);
	if(sqlite3_exec(db, stmt, NULL, NULL, &error_exec) != SQLITE_OK)
	{
		printf("SQLite error: %s\n",error_exec);
		sqlite3_free(error_exec);
		error_exec=NULL;
	}
}

int Qaullib_DbGetConfigValue(const char* key, char *value)
{
	sqlite3_stmt *ppStmt;
	char buffer[10240];
	char *stmt = buffer;
	int value_found = 0;

	// check if key exists in Database
	sprintf(stmt, sql_config_get, key);
	if( sqlite3_prepare_v2(db, stmt, -1, &ppStmt, NULL) != SQLITE_OK )
	{
		printf("SQLite error: %s\n",sqlite3_errmsg(db));
	}
	else
	{
		// For each row returned
		while (sqlite3_step(ppStmt) == SQLITE_ROW)
		{
		  // For each collumn
		  int jj;
		  for(jj=0; jj < sqlite3_column_count(ppStmt); jj++)
		  {
				if(strcmp(sqlite3_column_name(ppStmt,jj), "value") == 0)
				{
					sprintf(value,"%s",sqlite3_column_text(ppStmt, jj));
					value_found = 1;
					break;
				}
		  }
		}
		sqlite3_finalize(ppStmt);
	}
	return value_found;
}

int Qaullib_DbGetConfigValueInt(const char* key)
{
	sqlite3_stmt *ppStmt;
	char buffer[1024];
	char *stmt = buffer;
	int myvalue = 0;

	sprintf(stmt, sql_config_get, key);
	if( sqlite3_prepare_v2(db, stmt, -1, &ppStmt, NULL) != SQLITE_OK )
	{
		printf("SQLite error: %s\n",sqlite3_errmsg(db));
	}
	else
	{
		// For each row returned
		while (sqlite3_step(ppStmt) == SQLITE_ROW)
		{
		  // For each collumn
		  int jj;
		  for(jj=0; jj < sqlite3_column_count(ppStmt); jj++)
		  {
				if(strcmp(sqlite3_column_name(ppStmt,jj), "value_int") == 0)
				{
					myvalue = sqlite3_column_int(ppStmt, jj);
					return myvalue;
				}
		  }
		}
		sqlite3_finalize(ppStmt);
	}
	return myvalue;
}

// ------------------------------------------------------------
void Qaullib_DbPopulateConfig(void)
{
	char buffer[10240];
	char *stmt = buffer;
	char *error_exec=NULL;
	int i;

	// loop trough entries
	for(i=0; i<MAX_POPULATE_CONFIG; i++)
	{
		// write entry into DB
		sprintf(stmt,
				sql_config_set_all,
				qaul_populate_config[i].key,
				qaul_populate_config[i].type,
				qaul_populate_config[i].value,
				qaul_populate_config[i].value_int
				);

		if(sqlite3_exec(db, stmt, NULL, NULL, &error_exec) != SQLITE_OK)
		{
			printf("SQLite error: %s\n",error_exec);
			sqlite3_free(error_exec);
			error_exec=NULL;
		}
	}
}

// ------------------------------------------------------------
// configure user name
const char* Qaullib_GetUsername(void)
{
	// if username is set return it
	if (qaul_username_set) return qaul_username;

	// check if username is in Database
	if (Qaullib_DbGetConfigValue("username", qaul_username)) return qaul_username;

	// no username found
	return "";
}

int Qaullib_ExistsUsername(void)
{
	// if username is set return it
	if (qaul_username_set) return qaul_username_set;

	// check if username is in Database
	qaul_username_set = Qaullib_DbGetConfigValue("username", qaul_username);
	return qaul_username_set;
}

int Qaullib_SetUsername(const char* name)
{
	Qaullib_DbSetConfigValue("username", name);
	strcpy(qaul_username,name);
	qaul_username_set = 1;
	return 1;
}

// ------------------------------------------------------------
void Qaullib_FilePicked(int check, const char* path)
{
	strncpy(pickFilePath, path, MAX_PATH_LEN);
	memcpy(&pickFilePath[MAX_PATH_LEN],"\0",1);
	pickFileCheck = check;
}

// ------------------------------------------------------------
// configure IP
const char* Qaullib_GetIP(void)
{
	// if IP is set return it
	if (qaul_ip_set) return qaul_ip_str;

	qaul_ip_version = AF_INET;
	qaul_ip_size = sizeof(struct in_addr);

	// check if username is in Database
	if (Qaullib_DbGetConfigValue("ip", qaul_ip_str))
	{
		// create ip bin
		// FIXME: ipv6
		inet_pton(AF_INET, qaul_ip_str, &qaul_ip_addr.v4);

		qaul_ip_set = 1;
		// return string
		return qaul_ip_str;
	}

	// create new IP
	Qaullib_CreateIP(qaul_ip_str);
	// write IP into config
	Qaullib_SetIP(qaul_ip_str);
	// return IP
	return qaul_ip_str;
}

int Qaullib_SetIP(const char* IP)
{
	Qaullib_DbSetConfigValue("ip", IP);
	strcpy(qaul_ip_str, IP);
	// create ip bin
	// FIXME: ipv6
	inet_pton(AF_INET, qaul_ip_str, &qaul_ip_addr.v4);

	qaul_ip_set = 1;
	return 1;
}

void Qaullib_CreateIP(char* IP)
{
	// todo: take network-card-number or processor-number into account
	srand(time(NULL));
	int rand1 = rand()%255;
	int rand2 = rand()%255;
	int rand3 = (rand()%254)+1;

	sprintf(IP,"10.%i.%i.%i",rand1,rand2,rand3);
}

// ------------------------------------------------------------
int Qaullib_GetNetProtocol(void)
{
	int protocol = Qaullib_DbGetConfigValueInt("net.protocol");
	if (protocol > 0)
	{
		return protocol;
	}
	else return 4;
}

int Qaullib_GetNetMask(void)
{
	return Qaullib_DbGetConfigValueInt("net.mask");
}

const char* Qaullib_GetNetGateway(void)
{
	if (Qaullib_DbGetConfigValue("net.gateway", qaul_net_gateway))
	{
		return qaul_net_gateway;
	}
	return "0.0.0.0";
}

const char* Qaullib_GetNetIbss(void)
{
	if (Qaullib_DbGetConfigValue("net.ibss", qaul_net_ibss))
	{
		return qaul_net_ibss;
	}
	return "";
}

int Qaullib_GetNetBssIdSet(void)
{
	return Qaullib_DbGetConfigValueInt("net.bssid_set");
}

const char* Qaullib_GetNetBssId(void)
{
	if (Qaullib_DbGetConfigValue("net.bssid", qaul_net_bssid))
	{
		return qaul_net_bssid;
	}
	return "";
}

// ------------------------------------------------------------
void Qaullib_ConfigurationFinished(void)
{
	qaul_configured = 1;
}