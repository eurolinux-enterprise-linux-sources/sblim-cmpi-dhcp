/// ============================================================================
/// Copyright © 2007, International Business Machines
///
/// THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
/// ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
/// CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
///
/// You can obtain a current copy of the Eclipse Public License from
/// http://www.opensource.org/licenses/eclipse-1.0.php
///
/// Authors:             Ashoka Rao S <ashoka.rao (at) in.ibm.com>
///                      Riyashmon Haneefa <riyashh1 (at) in.ibm.com>
/// ============================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/** Include the required CMPI data types, function headers, and macros */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

/** The include for common MetaCluster settings */
#include "sblim-dhcp.h"

/** Include the abstract resource access functions and abstracted _RESOURCES and _RESOURCE data types. */
#include "Linux_DHCPHost_Resource.h"

#ifndef CMPI_VER_100
#define Linux_DHCPHost_ModifyInstance Linux_DHCPHost_SetInstance
#endif

/// ----------------------------------------------------------------------------
/// COMMON GLOBAL VARIABLES
/// ----------------------------------------------------------------------------

/** Handle to the CIM broker. Initialized when the provider lib is loaded. */
static const CMPIBroker *_BROKER;
/** === For indication purposes === */
static int _enabled;
static int  _numSubscriptions;
/** =============================== */

/// ============================================================================
/// CMPI INSTANCE PROVIDER FUNCTION TABLE
/// ============================================================================

/// ----------------------------------------------------------------------------
/// Info for the class supported by the instance provider
/// ----------------------------------------------------------------------------

/**** CUSTOMIZE FOR EACH PROVIDER ***/
/** NULL terminated list of key properties of this class. */
static const char * _KEYNAMES[] = {"InstanceID", NULL};

/// ----------------------------------------------------------------------------
/// EnumInstanceNames()
/// Return a list of all the instances names (return their object paths only).
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_EnumInstanceNames(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            const CMPIResult * results,          /** [out] Results of this operation. */
            const CMPIObjectPath * reference)    /** [in] Contains target namespace and classname. */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance * instance = NULL;
    CMPIObjectPath * op = NULL;
    _RESOURCES * resources = NULL;
    _RESOURCE * resource = NULL;
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};

    const char * namespace = CMGetCharsPtr( CMGetNameSpace( reference, &status ), NULL );
    int found = 0;

    if ( !Host_isEnumerateInstanceNamesSupported() ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_SUPPORTED, _("This function is not supported") );
        goto exit;
    }

    /** Get a handle to the list of system resources. */
    ra_status = Linux_DHCPHost_getResources( &resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get list of system resources"), ra_status );
        free_ra_status(ra_status);
        goto exit;
    }

    /** Enumerate thru the list of system resources and return a CMPIInstance for each. */
    ra_status = Linux_DHCPHost_getNextResource( resources, &resource );
    while ( ra_status.rc == RA_RC_OK && resource ) {
        /** Create a new CMPIObjectPath to store this resource. */
        op = CMNewObjectPath( _BROKER, namespace, _CLASSNAME, &status );
        if ( CMIsNullObject( op ) ) { 
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Creation of CMPIObjectPath failed") );
            goto clean_on_error; 
        }

        /** Create a new CMPIInstance to store this resource. */
        instance = CMNewInstance( _BROKER, op, &status );
        if ( CMIsNullObject( instance ) ) {
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Creation of CMPIInstance failed"));
            goto clean_on_error; 
        }

        /** Set the instance property values from the resource data. */
        ra_status = Linux_DHCPHost_setInstanceFromResource( resource, instance, _BROKER );
        if ( ra_status.rc != RA_RC_OK ) {
            build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to set property values from resource data"), ra_status );
            goto clean_on_error; 
        }

        /** Free the resource data. */
        ra_status = Linux_DHCPHost_freeResource( resource );
        if ( ra_status.rc != RA_RC_OK ) {
            build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free resource data"), ra_status );
            goto clean_on_error; 
        }
        
        /** Return the CMPIObjectPath for this instance. */
        CMPIObjectPath * objectpath = CMGetObjectPath( instance, &status );
        if ( (status.rc != CMPI_RC_OK) || CMIsNullObject(objectpath) ) {
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get CMPIObjectPath from CMPIInstance") );
            goto clean_on_error; 
        }

        CMSetNameSpace( objectpath, namespace ); /** Note - CMGetObjectPath() does not preserve the namespace! */

        CMReturnObjectPath( results, objectpath );
        found++;
        ra_status = Linux_DHCPHost_getNextResource( resources, &resource );
    }

    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get resource data"), ra_status );
        goto clean_on_error;
    }

    /** Free list of system resources */
    ra_status = Linux_DHCPHost_freeResources( resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free list of system resources"), ra_status );
        goto clean_on_error;
    }

    CMReturnDone( results );
    goto exit;

clean_on_error:
    free_ra_status(ra_status);
    ra_status = Linux_DHCPHost_freeResource( resource );
    ra_status = Linux_DHCPHost_freeResources( resources );

exit:

    return status;
}

/// ----------------------------------------------------------------------------
/// EnumInstances()
/// Return a list of all the instances (return all the instance data).
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_EnumInstances(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            const CMPIResult * results,          /** [out] Results of this operation. */
            const CMPIObjectPath * reference,    /** [in] Contains target namespace and classname. */
            const char ** properties)            /** [in] List of desired properties (NULL=all). */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance * instance = NULL;
    CMPIObjectPath * op = NULL;
    _RESOURCES * resources = NULL;
    _RESOURCE * resource = NULL;
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};
    const char * namespace = CMGetCharsPtr( CMGetNameSpace( reference, NULL ), NULL );
    int found = 0;

    if ( !Host_isEnumerateInstancesSupported() ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_SUPPORTED, _("This function is not supported") );
        goto exit;
    }

    /** Get a handle to the list of system resources. */
    ra_status = Linux_DHCPHost_getResources( &resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get list of system resources"), ra_status );
        free_ra_status(ra_status);
        goto exit;
    }

    /** Enumerate thru the list of system resources and return a CMPIInstance for each. */
    ra_status = Linux_DHCPHost_getNextResource( resources, &resource );
    while ( ra_status.rc == RA_RC_OK && resource ) {
        /** Create a new CMPIObjectPath to store this resource. */
        op = CMNewObjectPath( _BROKER, namespace, _CLASSNAME, &status );
        if( CMIsNullObject(op) ) { 
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Creation of CMPIObjectPath failed") );
            goto clean_on_error; 
        }

        /** Create a new CMPIInstance to store this resource. */
        instance = CMNewInstance( _BROKER, op, &status );
        if ( CMIsNullObject( instance ) ) {
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Creation of CMPIInstance failed") );
            goto clean_on_error; 
        }

        /** Setup a filter to only return the desired properties. */
        status = CMSetPropertyFilter( instance, properties, _KEYNAMES );
        if ( status.rc != CMPI_RC_OK ) {
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to set property filter") );
            goto clean_on_error; 
        }

        /** Set the instance property values from the resource data. */
        ra_status = Linux_DHCPHost_setInstanceFromResource( resource, instance, _BROKER );
        if ( ra_status.rc != RA_RC_OK ) {
            build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to set property values from resource data"), ra_status );
            goto clean_on_error; 
        }

        /** Free the resource data. */
        ra_status = Linux_DHCPHost_freeResource( resource );
        if ( ra_status.rc != RA_RC_OK ) {
            build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free resource data"), ra_status );
            goto clean_on_error; 
        }
        
        /** Return the CMPIInstance for this instance. */
        CMReturnInstance(results, instance);
        found++;
        ra_status = Linux_DHCPHost_getNextResource( resources, &resource );
    }

    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get resource data"), ra_status );
        goto clean_on_error;
    }

    /** Free list of system resources */
    ra_status = Linux_DHCPHost_freeResources( resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free list of system resources"), ra_status );
        goto clean_on_error;
    }

    CMReturnDone( results );
    goto exit;

clean_on_error:
    free_ra_status(ra_status);
    ra_status = Linux_DHCPHost_freeResource( resource );
    ra_status = Linux_DHCPHost_freeResources( resources );

exit:

    return status;
}

/// ----------------------------------------------------------------------------
/// GetInstance()
/// Return the instance data for the specified instance only.
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_GetInstance(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            const CMPIResult * results,          /** [out] Results of this operation. */
            const CMPIObjectPath * reference,    /** [in] Contains the target namespace, classname and object path. */
            const char ** properties)            /** [in] List of desired properties (NULL=all). */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIInstance * instance = NULL;
    CMPIObjectPath * op = NULL;
    _RESOURCES * resources = NULL;
    _RESOURCE * resource = NULL;
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};
    const char * namespace = CMGetCharsPtr(CMGetNameSpace(reference, NULL), NULL);

    if ( !Host_isGetSupported() ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_SUPPORTED, _("This function is not supported") );
        goto exit;
    }

    /** Get a handle to the list of system resources. */
    ra_status = Linux_DHCPHost_getResources( &resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get list of system resources"), ra_status );
        free_ra_status(ra_status);
        goto exit;
    }

    /** Get the target resource. */
    ra_status = Linux_DHCPHost_getResourceForObjectPath( resources, &resource, reference );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get resource data"), ra_status );
        goto clean_on_error;

    } else if ( !resource ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_FOUND, _("Target instance not found") );
        goto clean_on_error;
    }

    /** Create a new CMPIObjectPath to store this resource. */
    op = CMNewObjectPath( _BROKER, namespace, _CLASSNAME, &status );
    if( CMIsNullObject( op ) ) { 
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Creation of CMPIObjectPath failed")); 
        goto clean_on_error;
    }

    /** Create a new CMPIInstance to store this resource. */
    instance = CMNewInstance( _BROKER, op, &status );
    if( CMIsNullObject( instance ) ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Creation of CMPIInstance failed"));
        goto clean_on_error;
    }

    /** Setup a filter to only return the desired properties. */
    status = CMSetPropertyFilter( instance, properties, _KEYNAMES );
    if ( status.rc != CMPI_RC_OK ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to set property filter") );
        goto clean_on_error;
    }

    /** Set the instance property values from the resource data. */
    ra_status = Linux_DHCPHost_setInstanceFromResource( resource, instance, _BROKER );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to set property values from resource data"), ra_status );
        goto clean_on_error;
    }

    /** Free the resource data. */
    ra_status = Linux_DHCPHost_freeResource( resource );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free resource data"), ra_status );
        goto clean_on_error; 
    }
    
    /** Free list of system resources */
    ra_status = Linux_DHCPHost_freeResources( resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free list of system resources"), ra_status );
        goto clean_on_error;
    }

    /** Return the CMPIInstance for this instance. */
    CMReturnInstance( results, instance );

    CMReturnDone( results );
    goto exit;

clean_on_error:
    free_ra_status(ra_status);
    ra_status = Linux_DHCPHost_freeResource( resource );
    ra_status = Linux_DHCPHost_freeResources( resources );

exit:
    return status;
}

/// ----------------------------------------------------------------------------
/// ModifyInstance()
/// Save modified instance data for the specified instance.
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_ModifyInstance(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            const CMPIResult * results,          /** [out] Results of this operation. */
            const CMPIObjectPath * reference,    /** [in] Contains the target namespace, classname and object path. */
            const CMPIInstance * newinstance,    /** [in] Contains the new instance data. */
            const char** properties)             /** [in] List of desired properties (NULL=all). */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIStatus indStatus = {CMPI_RC_OK, NULL};
    _RESOURCES * resources = NULL;
    _RESOURCE * resource = NULL;
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};

    /** Parameters for the use of Indication*/
    
    CMPIObjectPath *indop = NULL; 
    CMPIInstance *indinst = NULL; 

    /**=====================================*/
    
    const char * namespace = CMGetCharsPtr( CMGetNameSpace( reference, NULL ), NULL );

    if ( !Host_isModifySupported() ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_SUPPORTED, _("This function is not supported") );
        goto exit;
    }

    /** Get a handle to the list of system resources. */
    ra_status = Linux_DHCPHost_getResources( &resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get list of system resources"), ra_status );
        free_ra_status(ra_status);
        goto exit;
    }
    /** Get the target resource. */
    ra_status = Linux_DHCPHost_getResourceForObjectPath( resources, &resource, reference );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get resource data"), ra_status );
        goto clean_on_error;

    } else if ( !resource ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_FOUND, _("Target instance not found") );
        goto clean_on_error;
    }
    /** Update the target resource data with the new instance property values. */
    ra_status = Linux_DHCPHost_setResourceFromInstance( &resource, newinstance, properties, _BROKER );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to modify resource data"), ra_status );
        goto clean_on_error; 
    }


    /** Create an instance of the Indication and Deliver it to the filter */ 
    /**===================================================================*/

    indop = CMNewObjectPath( _BROKER, namespace, "Linux_DHCPHost_Ind", &indStatus );
       if ( CMIsNullObject( indop ) ) {
            printf("Creation of CMPIObjectPath for indication object failed\n" );
       }

     indinst = CMNewInstance( _BROKER, indop, &indStatus );
     if(indinst == NULL)
     printf("indinst = NULL\n");

    CMSetProperty (indinst, "IndicationIdentifier", (CMPIValue *)"Linux_DHCPHost", CMPI_chars);
    indStatus = CBDeliverIndication(_BROKER, context, (char*)namespace , (CMPIInstance*)indinst);
    if(indStatus.rc != 0)
        printf("Failed to deliver Indication, status = %d\n", indStatus.rc);

   /**========================================================================*/

    /** Free the resource data. */
    ra_status = Linux_DHCPHost_freeResource( resource );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free resource data"), ra_status );
        goto clean_on_error; 
    }
    
    /** Free list of system resources */
    ra_status = Linux_DHCPHost_freeResources( resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free list of system resources"), ra_status );
        goto clean_on_error;
    }
    goto exit;

clean_on_error:
    free_ra_status(ra_status);
    ra_status = Linux_DHCPHost_freeResource( resource );
    ra_status = Linux_DHCPHost_freeResources( resources );

exit:

    return status;
}

/// ----------------------------------------------------------------------------
/// CreateInstance()
/// Create a new instance from the specified instance data.
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_CreateInstance(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            const CMPIResult * results,          /** [out] Results of this operation. */
            const CMPIObjectPath * reference,    /** [in] Contains the target namespace, classname and object path. */
            const CMPIInstance * newinstance)    /** [in] Contains the new instance data. */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIStatus indStatus = {CMPI_RC_OK, NULL};
    _RESOURCES * resources = NULL;
    _RESOURCE * resource = NULL;
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};
    /** Parameters for the use of Indication*/

    CMPIObjectPath *indop = NULL; 
    CMPIInstance *indinst = NULL;

    /**=====================================*/
    
    const char * namespace = CMGetCharsPtr( CMGetNameSpace( reference, NULL ), NULL );
    
    if ( !Host_isCreateSupported() ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_SUPPORTED, _("This function is not supported") );
	goto exit;
    }

    /** WORKAROUND FOR PEGASUS BUG?! reference does not contain object path, only namespace & classname. */
    reference = CMGetObjectPath( newinstance, NULL );

    /** Get a handle to the list of system resources. */
    ra_status = Linux_DHCPHost_getResources( &resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get list of system resources"), ra_status );
        free_ra_status(ra_status);
        goto exit;
    }

    /** Get the target resource. */
    ra_status = Linux_DHCPHost_getResourceForObjectPath( resources, &resource, reference );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get resource data"), ra_status );
        goto clean_on_error;

    } else if ( resource ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_ALREADY_EXISTS, _("Target instance already exists") );
        goto clean_on_error;
    }

    /** Create a new resource with the new instance property values. */
    ra_status = Linux_DHCPHost_createResourceFromInstance( resources, &resource, newinstance, _BROKER );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to create resource data"), ra_status );
        goto clean_on_error;
    }

    /** Return the object path for the newly created instance. */
    CMPIObjectPath * objectpath = CMGetObjectPath( newinstance, NULL );
    ra_status = Linux_DHCPHost_BuildObjectPath(objectpath, (CMPIInstance*)newinstance , (char*)namespace, resource);
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to build object path for the new instance"), ra_status );
        goto clean_on_error;
    }
    
    /** Create an instance of the Indication and Deliver it to the filter */
    /**===================================================================*/

    indop = CMNewObjectPath( _BROKER, namespace, "Linux_DHCPHost_Ind", &indStatus );
       if ( CMIsNullObject( indop ) ) {
            printf("Creation of CMPIObjectPath for indication object failed\n" );
       }

     indinst = CMNewInstance( _BROKER, indop, &indStatus );
     if(indinst == NULL)
     printf("indinst = NULL\n");

    CMSetProperty (indinst, "IndicationIdentifier", (CMPIValue *)"Linux_DHCPHost", CMPI_chars);
    indStatus = CBDeliverIndication(_BROKER, context, (char*)namespace , (CMPIInstance*)indinst);
    if(indStatus.rc != 0)
    	printf("Failed to deliver Indication, status = %d\n", indStatus.rc);

   /**========================================================================*/


    /** Free the resource data. */


    ra_status = Linux_DHCPHost_freeResource( resource );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free resource data"), ra_status );
        goto clean_on_error;
    }
    
    /** Free list of system resources */
    ra_status = Linux_DHCPHost_freeResources( resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free list of system resources"), ra_status );
        goto clean_on_error;
    }

    CMReturnObjectPath( results, objectpath );
    CMReturnDone( results );
    goto exit;

clean_on_error:
    free_ra_status(ra_status);
    ra_status = Linux_DHCPHost_freeResource( resource );
    ra_status = Linux_DHCPHost_freeResources( resources );

exit:

    return status;
}

/// ----------------------------------------------------------------------------
/// DeleteInstance()
/// Delete or remove the specified instance from the system.
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_DeleteInstance(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            const CMPIResult * results,          /** [out] Results of this operation. */
            const CMPIObjectPath * reference)  	 /** [in] Contains the target namespace, classname and object path. */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    CMPIStatus indStatus = {CMPI_RC_OK, NULL};
    _RESOURCES * resources = NULL;
    _RESOURCE * resource = NULL;
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};
    /** Parameters for the use of Indication*/

    CMPIObjectPath *indop = NULL;
    CMPIInstance *indinst = NULL;
    const char* namespace = "root/cimv2";
    /**=====================================*/

    if ( !Host_isDeleteSupported() ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_SUPPORTED, _("This function is not supported") );
	goto exit;
    }
    /** Get a handle to the list of system resources. */
    ra_status = Linux_DHCPHost_getResources( &resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get list of system resources"), ra_status );
        free_ra_status(ra_status);
        goto exit;
    }

    /** Get the target resource. */
    ra_status = Linux_DHCPHost_getResourceForObjectPath( resources, &resource, reference );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get resource data"), ra_status );
        goto clean_on_error;

    } else if ( !resource ) {
        build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_NOT_FOUND, _("Target instance not found") );
        goto clean_on_error;
    }

    /** Delete the target resource. */
    ra_status = Linux_DHCPHost_deleteResource( resources, resource, _BROKER );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to delete resource data"), ra_status );
        goto clean_on_error; 
    }

    /** Create an instance of the Indication and Deliver it to the filter */
    /**===================================================================*/

    indop = CMNewObjectPath( _BROKER, namespace, "Linux_DHCPHost_Ind", &indStatus );
        if ( CMIsNullObject( indop ) ) {
            printf("Creation of CMPIObjectPath for indication object failed\n" );
        }

     indinst = CMNewInstance( _BROKER, indop, &indStatus );
     if(indinst == NULL)
     printf("indinst = NULL\n");

    CMSetProperty (indinst, "IndicationIdentifier", (CMPIValue *)"Linux_DHCPHost", CMPI_chars);
    indStatus = CBDeliverIndication(_BROKER, context, (char*)namespace , (CMPIInstance*)indinst);
    if(indStatus.rc != 0)
    	printf("Failed to deliver Indication, status = %d\n", indStatus.rc);

   /**========================================================================*/

    /** Free the resource data. */
    ra_status = Linux_DHCPHost_freeResource( resource );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free resource data"), ra_status );
        goto clean_on_error; 
    }
    
    /** Free list of system resources */
    ra_status = Linux_DHCPHost_freeResources( resources );
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to free list of system resources"), ra_status );
        goto clean_on_error;
    }
    goto exit;


clean_on_error:
    free_ra_status(ra_status);
    ra_status = Linux_DHCPHost_freeResource( resource );
    ra_status = Linux_DHCPHost_freeResources( resources );

exit:

    return status;
}


/// ----------------------------------------------------------------------------
/// ExecQuery()
/// Return a list of all the instances that satisfy the specified query filter.
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_ExecQuery(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            const CMPIResult * results,          /** [out] Results of this operation. */
            const CMPIObjectPath * reference,    /** [in] Contains the target namespace and classname. */
            const char * language,               /** [in] Name of the query language. */
            const char * query)                  /** [in] Text of the query written in the query language. */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};

    /** EXECQUERY() IS NOT YET SUPPORTED FOR THIS CLASS */
    CMSetStatus( &status, CMPI_RC_ERR_NOT_SUPPORTED );

    CMReturnDone( results );

    return status;
}


/// ----------------------------------------------------------------------------
/// Initialize()
/// Perform any necessary initialization immediately after this provider is
/// first loaded.
/// ----------------------------------------------------------------------------
CMPIStatus Linux_DHCPHost_Initialize(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context)         /** [in] Additional context info, if any. */
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};
    CMPIObjectPath * objectpath;
    CMPIObjectPath * op = NULL;
    CMPIInstance * instance = NULL;
    const char * namespace = "root/cimv2";

/** Create a new CMPIObjectPath to store this resource. */
        op = CMNewObjectPath( _BROKER, namespace, "CIM_IndicationFilter", &status );
        if ( CMIsNullObject( op ) ) {
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Creation of CMPIObjectPath failed") );
            return status;
        }

        CMAddKey (op, "CreationClassName", (CMPIValue *)"CIM_IndicationFilter", CMPI_chars);
        CMAddKey (op, "Name", (CMPIValue *)"HostIndication", CMPI_chars);
        CMAddKey (op, "SystemCreationClassName", (CMPIValue *)"CIM_ComputerSystem", CMPI_chars);
    
       /** Create a new CMPIInstance to store this resource. */

        instance = CMNewInstance( _BROKER, op, &status );
        CMSetProperty(instance, "Name", (CMPIValue *)"HostIndication", CMPI_chars);
        char *v;
        v = "SELECT * FROM Linux_DHCPHost_Ind"; 
        CMSetProperty(instance, "Query", (CMPIValue *)v, CMPI_chars);
        v = "WQL";
        CMSetProperty(instance, "QueryLanguage", (CMPIValue *)v, CMPI_chars);

        objectpath = CBCreateInstance( _BROKER, context, op, instance, &status );

        objectpath = CMGetObjectPath( instance, &status );
        if ( (status.rc != CMPI_RC_OK) || CMIsNullObject(objectpath) ) {
            build_cmpi_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to get CMPIObjectPath from CMPIInstance") );
            return status;
        }

        CMSetNameSpace( objectpath, namespace ); /** Note - CMGetObjectPath() does not preserve the namespace! */

      //printf("Created an instance of HostIndication Filter\n");

    /** Initialize instance provider */
    ra_status = Linux_DHCPHost_InstanceProviderInitialize(&ra_status);
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to initialize instance provider"), ra_status );
        free_ra_status(ra_status);
    }

    return status;
}

/// ----------------------------------------------------------------------------
/// Cleanup()
/// Perform any necessary cleanup immediately before this provider is unloaded.
/// ----------------------------------------------------------------------------
static CMPIStatus Linux_DHCPHost_Cleanup(
            CMPIInstanceMI * self,               /** [in] Handle to this provider (i.e. 'self'). */
            const CMPIContext * context,         /** [in] Additional context info, if any. */
            CMPIBoolean terminating)             /** [in] Switch to wether to teminate or not*/ 
{
    CMPIStatus status = {CMPI_RC_OK, NULL};
    _RA_STATUS ra_status = {RA_RC_OK, 0, NULL};
    bool lTerminating = false;

    if (terminating) {
        lTerminating = true;
    }

    /** Cleanup instance provider */
    ra_status = Linux_DHCPHost_InstanceProviderCleanUp(lTerminating);
    if ( ra_status.rc != RA_RC_OK ) {
        build_ra_error_msg ( _BROKER, &status, CMPI_RC_ERR_FAILED, _("Failed to cleanup instance provider"), ra_status );
        free_ra_status(ra_status);
    }


    return status;
}

///===========================================================================
/// Code for supporting Indication 
///===========================================================================

CMPIStatus Linux_DHCPHost_IndicationCleanup(
                                           CMPIIndicationMI * mi,
                                           const CMPIContext * ctx,
                                           CMPIBoolean term) 
{
    //printf("Inside Indication cleanup method\n");
    CMReturn (CMPI_RC_OK);
}

CMPIStatus Linux_DHCPHost_AuthorizeFilter(
                                           CMPIIndicationMI * mi,
                                           const CMPIContext * ctx,
                                           const CMPISelectExp * se,
                                           const char *ns,
                                           const CMPIObjectPath * op,
                                           const char *user)
{   //printf("Inside Authorize Filter\n");
    CMReturn (CMPI_RC_OK);
}

CMPIStatus Linux_DHCPHost_MustPoll(
                                    CMPIIndicationMI * mi,
                                    const CMPIContext * ctx,
                                    const CMPISelectExp * se,
                                    const char *ns, const CMPIObjectPath * op)
{
    //printf("Inside the MustPoll method\n");
    CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus Linux_DHCPHost_ActivateFilter(
                                          CMPIIndicationMI * mi,
                                          const CMPIContext * ctx,
                                          const CMPISelectExp * se,
                                          const char *ns,
                                          const CMPIObjectPath * op,
                                          CMPIBoolean firstActivation)
{
    _numSubscriptions++;

    //printf("Inside Activate Filter\n");
    CMReturn (CMPI_RC_OK);
}

CMPIStatus Linux_DHCPHost_DeActivateFilter(
                                            CMPIIndicationMI * mi,
                                            const CMPIContext * ctx,
                                            const CMPISelectExp * se,
                                            const char *ns,
                                            const CMPIObjectPath * op,
                                            CMPIBoolean lastActivation)
{
    //printf("Deactivate Filter invoked\n");
    _numSubscriptions--;

    if (_numSubscriptions == 0)
    {
        _enabled = 0;
    }

    CMReturn (CMPI_RC_OK);
}


CMPIStatus Linux_DHCPHost_EnableIndications(
                                              CMPIIndicationMI * mi,
                                              const CMPIContext * ctx)
{
    _enabled = 1;
    //printf("Inside Enable Indication\n");
    CMReturn (CMPI_RC_OK);
}

CMPIStatus Linux_DHCPHost_DisableIndications(
                                               CMPIIndicationMI * mi,
                                               const CMPIContext * ctx)
{
    //printf("Disable Indications invoked\n");
    if (_numSubscriptions == 0)
    {
        _enabled = 0;
    }
    CMReturn (CMPI_RC_OK);
}


///============================================================================


/// ============================================================================
/// CMPI INSTANCE PROVIDER FUNCTION TABLE SETUP
/// ============================================================================
CMInstanceMIStub( Linux_DHCPHost_ , Linux_DHCPHostProvider, _BROKER, Linux_DHCPHost_Initialize( &mi, ctx ) );
CMIndicationMIStub( Linux_DHCPHost_ , Linux_DHCPHostProvider, _BROKER, CMNoHook );
