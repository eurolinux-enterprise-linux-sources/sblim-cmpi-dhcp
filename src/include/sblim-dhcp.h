/*
 * sblim-dhcp.h
 *
 * © Copyright IBM Corp. 2007
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author: Wolfgang Taphorn <taphorn (at) de.ibm.com>
 * Contributors : Ashoka Rao.S <ashoka.rao (at) in.ibm.com>
 *                Riyashmon Haneefa <riyashh1 (at) in.ibm.com>
 *
 *
 */

#ifndef SBLIM_DHCP_H
#define SBLIM_DHCP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cmpimacs.h>


#define INITSCRIPT          "initscript"
#define DHCPPATH            "dhcpd_path"
#define SCRIPTDIR           "scriptdir"

#define DEFAULT_DHCPCONF    "/etc/dhcpd.conf"
#define DEFAULT_INITSCRIPT  "/etc/rc.d/init.d/dhcpd"
#define DEFAULT_SCRIPTDIR   "/usr/local/share/sblim-cmpi-dhcp/"

// definition of the location for the smt_dhcp_ra_support.conf file. This 
// location is hard coded during build processing. If the build does not define
// a location we will use this hard coded default.
#ifndef PROVIDER_CONFFILE
    #define PROVIDER_CONFFILE        "/etc/smt_dhcp_ra_support.conf"
#endif


// gettext support
#ifdef ENABLE_NLS
    #include <libintl.h>
    #include <locale.h>
    #define _(String) gettext(String)
    #define gettext_nohup(String) (String)
    #define N_(String) gettext_nohup(String)
#else
    #define _(String) String
    #define N_(String) String
    #define setlocale(a,b)
    #define bindtextdomain(a,b)
    #define textdomain(a)
#endif

#include <errno.h>
extern int errno;

//Constants for the registered profile
#define PROFILE_REGISTERED_ORGANIZATION        1 /* other */
#define PROFILE_OTHER_REGISTERED_ORGANIZATION  "IBM"
#define PROFILE_REGISTERED_NAME                "DHCP Server Profile"
#define PROFILE_REGISTERED_VERSION             "0.1.1"
#define PROFILE_ADVERTISE_TYPE                 3 /* SLP */

#define ERROR_MSG_TRY_OUT _("TRY OUT")

typedef struct {
    int    rc;
    int    messageID;
    char * messageTxt;
} _RA_STATUS;

#define RA_MESSAGE_PREFIX                      "WBEM-SMT"
#define RA_MESSAGE_GENERICID                   "0001"
#define RA_RC_OK                               0
#define RA_RC_FAILED                           1

//-------------

#define DHCP_CONF_FILE_NOT_FOUND                0
#define DHCP_HASH_FILE_NOT_FOUND                1
#define FAILED_TO_GET_SYSTEM_RESOURCE           2
#define DYNAMIC_MEMORY_ALLOCATION_FAILED        3
#define ENTITY_NOT_FOUND                        4
#define OBJECT_PATH_IS_NULL                     5
#define FAILED_TO_FETCH_KEY_ELEMENT_DATA        6
#define CMPI_INSTANCE_NAME_IS_NULL              7
#define INSTANCE_ID_IS_NULL                     8
#define NAME_FIELD_NOT_SPECIFIED_OR_NOT_PROPER  9
#define FAILED_CREATING_A_NODE                  10
#define PARENTID_NOT_SPECIFIED_OR_NOT_PROPER    11
#define INVALID_INSTANCE_ID                     12
#define INVALID_INSTANCE_NAME                   13
#define INVALID_INSTANCE_VALUE			14
#define VALUE_NOT_SPECIFIED_OR_NOT_PROPER  	15
#define INSTANCE_NOT_FOUND			16
#define CANNOT_SET_PROPERTY_FILTER		17
#define ENTITY_ALREADY_EXISTS			18

static void setRaStatus(_RA_STATUS* status, int rc, int msgID, char* msgTxt ) {
    if(status == NULL)
	return;

    status->rc = rc;
    status->messageID = msgID;
    status->messageTxt = strdup(msgTxt);
}

/*char* raErr[] = {      "Failed to get the system resource",
                        "dhcpd.conf file not found",
                        "dhcpd.hash file not found"
                };
*/
//-------------


static void free_ra_status(_RA_STATUS ra_status) {
    if (ra_status.messageTxt) {
        free(ra_status.messageTxt);
    }
}


static void build_cmpi_error_msg(const CMPIBroker* broker, CMPIStatus* status, int return_code, char* cmpi_message) {
    int count = strlen(RA_MESSAGE_PREFIX) + strlen(RA_MESSAGE_GENERICID);
    char* message = NULL; 
  
    if(cmpi_message)
	count += strlen(cmpi_message);

    message = (char *)malloc(count + 6);
    if(message == NULL)
	return;

    sprintf( message, "%s%s: %s", RA_MESSAGE_PREFIX, RA_MESSAGE_GENERICID, cmpi_message );

    CMSetStatusWithChars( broker, status, return_code, message );
    free( message );

}


static void build_ra_error_msg(const CMPIBroker* broker, CMPIStatus* status, int return_code, char* provider_message, _RA_STATUS ra_status) {
    char* message = NULL;
    int count = strlen(RA_MESSAGE_PREFIX);

    if(provider_message)
	count += strlen(provider_message);

    if(ra_status.messageTxt)
	count += strlen(ra_status.messageTxt);

    message = (char *)malloc(count + 15);
    if(message == NULL)
	return;

    sprintf( message, "%s%d: %s - %s", RA_MESSAGE_PREFIX, ra_status.messageID, provider_message, ra_status.messageTxt );
    CMSetStatusWithChars( broker, status, return_code, message );
    free( message );
}

#endif //SBLIM_DHCP_H
