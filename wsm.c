/*
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author:  Oancea Aurelian <aurelian@locknet.ro>                       |
   +----------------------------------------------------------------------+
*/
/* $Id: wsm.c,v 1.11 2005/10/09 20:38:17 aurelian Exp $ */

#ifdef PHP_WIN32
#include "zend_config.w32.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "php_wsm.h"
#include <windows.h>

/* {{{ Windows Return codes translated */ 
static char* get_last_error()
{
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    return lpMsgBuf;
}
/* }}} */

static LPQUERY_SERVICE_CONFIG WINAPI query_service_config(SC_HANDLE hService)
{
    LPQUERY_SERVICE_CONFIG lpqscBuf;
    DWORD dwBytesNeeded;
    lpqscBuf = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, 4096);
    if (QueryServiceConfig(hService, lpqscBuf, 4096, &dwBytesNeeded)) {
        return lpqscBuf;
    } else {
        return NULL;
    }
}

/* {{{ WSM Classes */
PHP_WSM_API zend_class_entry *wsm_service_class_entry;
PHP_WSM_API zend_class_entry *wsm_exception_class_entry;
PHP_WSM_API zend_class_entry *wsm_runtimeexception_class_entry;
/* }}} */

static zend_object_handlers wsm_service_object_handlers;

/* {{{ WSM_Exception methods
 * These are used for all WSM exceptions, none of them have any methods other than inherited ones
 */
function_entry wsm_exception_methods[] = {
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ wsm_get_exception_methods */
PHP_WSM_API function_entry *wsm_get_exception_methods()
{
    return wsm_exception_methods;
}
/* }}} */

/* {{{ Module entry */
zend_module_entry wsm_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_WSM_EXTNAME,
    NULL,               /* function list, wsm_functions if we will add any function here */
    PHP_MINIT(wsm),     /* minit */
    PHP_MSHUTDOWN(wsm), /* mshutdown */
    NULL,               /* rinit */
    NULL,               /* rshutdown */
    PHP_MINFO(wsm),     /* minfo */
#if ZEND_MODULE_API_NO >= 20010901
    PHP_WSM_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_WSM
ZEND_GET_MODULE(wsm)
#endif

/* {{{ Our object definition */
typedef struct _wsm_service_object {
    zend_object     zo;
    char            *service_name; /* service name */
    char            *machine;      /* machine to open sch on it, NULL is current computer */
    DWORD           start_type;    /* service start-up type. */
    SC_HANDLE       hService;
} wsm_service_object;
/* }}} */

/* {{{ wsm_service_object_free_storage */
static void wsm_service_object_free_storage(void *object TSRMLS_DC)
{
    wsm_service_object *wsm = (wsm_service_object *)object;
    zend_hash_destroy(wsm->zo.properties);
    FREE_HASHTABLE(wsm->zo.properties);
    if (wsm->service_name) {
        efree(wsm->service_name);
    }
    if (wsm->machine) {
        efree(wsm->machine);
    }

    if (wsm->hService) {
        CloseServiceHandle(wsm->hService);
    }

    efree(object);

}
/* }}} */

/* {{{ wsm_service_object_create */
static zend_object_value wsm_service_object_create(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    zval *tmp;
    wsm_service_object *wsm;
    
    wsm = emalloc(sizeof(wsm_service_object));
    memset(wsm, 0, sizeof(wsm_service_object));
    wsm->zo.ce = ce; 
    
    ALLOC_HASHTABLE(wsm->zo.properties);
    zend_hash_init(wsm->zo.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_copy(wsm->zo.properties, &ce->default_properties, (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
    retval.handle = zend_objects_store_put(wsm, NULL, (zend_objects_free_object_storage_t)wsm_service_object_free_storage, NULL TSRMLS_CC);
    retval.handlers = &wsm_service_object_handlers;
    return retval;
}
/* }}} */

/* {{{ declare method parameters 
    TBD: -> for setStartType(int starttype)
*/
static
ZEND_BEGIN_ARG_INFO(arginfo_wsm_service___construct, 0)
    ZEND_ARG_INFO(0, service_name)  /* parameter name */
    ZEND_ARG_INFO(0, machine)       /* the computer to open the sch on it */
ZEND_END_ARG_INFO();

static
ZEND_BEGIN_ARG_INFO(arginfo_wsm_service_create, 0)
    ZEND_ARG_INFO(0, service_name)  /* service name */
    ZEND_ARG_INFO(0, display_name)  /* display name, optional */
    ZEND_ARG_INFO(0, start_type)    /* start type */
    ZEND_ARG_INFO(0, path)          /* path */
ZEND_END_ARG_INFO();
/* }}} */

/* {{{ WSM_Service methods */
function_entry wsm_service_methods[] = {
    PHP_ME(WSM_Service, __construct, arginfo_wsm_service___construct, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, start,           0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, stop,            0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, create,          arginfo_wsm_service_create, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(WSM_Service, delete,          0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, status,          0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, getBinaryPath,   0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, getDependencies, 0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, getStartName,    0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, change,          0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, setStartType,    0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, getStartType,    0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, getDisplayName,  0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, getServiceName,  0, ZEND_ACC_PUBLIC)
    PHP_ME(WSM_Service, getMachineName,  0, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ WSM_Service::__construct(string $service_name, [string $machine_name]) throws WSM_RuntimeException
    Creates a new WSM_Service object */
PHP_METHOD(WSM_Service, __construct)
{
    char *service_name;
    int service_len;
    char *machine = NULL;
    int machine_len;
    wsm_service_object *wsm;
    SC_HANDLE schSCManager;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &service_name, &service_len, &machine, &machine_len) == FAILURE) {
        RETURN_NULL();
    }
    wsm = (wsm_service_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    if (wsm==NULL) {
        php_error(E_ERROR, "WSM_Service [ %i ] : object failed.", __LINE__);
    }
    // setup the service name
    wsm->service_name = estrndup(service_name, strlen(service_name));
    // setup the machine name
    if (machine != NULL) {
        wsm->machine = estrndup(machine, strlen(machine));
    } else {
        wsm->machine = NULL;
    }
    wsm->start_type = SERVICE_NO_CHANGE;
    schSCManager = OpenSCManager(
            wsm->machine,  // machine
            NULL,     // ServicesActive database
            SC_MANAGER_ALL_ACCESS);  // full access rights
    if (NULL == schSCManager) {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
    }
    wsm->hService = OpenService(
        schSCManager, 
        wsm->service_name, 
        SERVICE_STOP| SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_INTERROGATE | DELETE);
    if (NULL == wsm->hService) {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
    }
    CloseServiceHandle(schSCManager);
}
/* }}} */

/* {{{ proto WSM_Service WSM_Service::create(
                                string $service_name, 
                                string $path, 
                                string $param, 
                                long $start_type, 
                                string $display_name) throws WSM_RuntimeException
 creates a Win32 Service. You must call this method in a static way.*/
PHP_METHOD(WSM_Service, create)
{
    char *service;
    int service_len;
    char *path;
    int path_len;
    char *param;
    int param_len;
    long start_type;
    char *display;
    int display_len;

    char *_path;
    SC_HANDLE schSCManager, schService;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|sls", 
            &service, &service_len, &path, &path_len, &param, &param_len, &start_type, &display, &display_len) == FAILURE) {
        RETURN_NULL();
    }

    schSCManager = OpenSCManager(
            NULL,                   // machine
            NULL,                   // ServicesActive database
            SC_MANAGER_ALL_ACCESS); // full access rights
    if (NULL == schSCManager) {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
    }

    if (strchr(path, ' ')) {
	    spprintf(&_path, 0, "\"%s\" %s", path, param);
    } else {
		spprintf(&_path, 0, "%s %s", path, param);
    }

    schService = CreateService( 
        schSCManager,              // SCManager database 
        TEXT(service),             // name of service 
        display,                   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        start_type,                // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        _path,                     // path to service's binary + param passed
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 
 
    if (schService == NULL) 
    {
        efree(_path);
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
    } else {
        CloseServiceHandle(schService); 
        RETURN_TRUE;
    }

}
/* }}} */

/* {{{ proto bool WSM_Service::delete() throws WSM_RuntimeException
    removes the service from SCM.
*/
PHP_METHOD(WSM_Service, delete)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    if (! DeleteService(wsm->hService) ) {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error()); 
    } else {
        RETURN_TRUE;
    }

}
/* }}} */

/* {{{ proto bool WSM_Service::start() 
    sends a start signal */
PHP_METHOD(WSM_Service, start)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    StartService(wsm->hService, 0, NULL);
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool WSM_Service::stop() 
    sends a stop signal */
PHP_METHOD(WSM_Service, stop)
{
    SERVICE_STATUS status;
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    ControlService(wsm->hService, SERVICE_CONTROL_STOP, &status );
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool WSM_Service::status() 
    it gets the status of a service */
PHP_METHOD(WSM_Service, status)
{
    SERVICE_STATUS status;
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    ControlService(wsm->hService, SERVICE_CONTROL_INTERROGATE, &status);
    RETURN_LONG(status.dwCurrentState);
}
/* }}} */

/* {{{ proto string WSM_Service::getBinaryPath() 
    it gets the binary path
*/
PHP_METHOD(WSM_Service, getBinaryPath)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    LPQUERY_SERVICE_CONFIG lpqscBuf= query_service_config(wsm->hService);
    if (lpqscBuf != NULL) {
        RETURN_STRING(lpqscBuf->lpBinaryPathName, 1);
	} else {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
	}
}
/* }}} */

/* {{{ proto string WSM_Service::getDependencies() throws WSM_RuntimeException
    it gets the dependencies 
*/
PHP_METHOD(WSM_Service, getDependencies)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    LPQUERY_SERVICE_CONFIG lpqscBuf= query_service_config(wsm->hService);
    if (lpqscBuf != NULL) {
        RETURN_STRING(lpqscBuf->lpDependencies, 1);
	} else {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
	}
}
/* }}} */

/* {{{ proto string WSM_Service::getStartName() throws WSM_RuntimeException
    it gets the start name
*/
PHP_METHOD(WSM_Service, getStartName)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    LPQUERY_SERVICE_CONFIG lpqscBuf = query_service_config(wsm->hService);
    if (lpqscBuf != NULL) {
        RETURN_STRING(lpqscBuf->lpServiceStartName, 1);
	} else {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
	}
}
/* }}} */

/* {{{ proto int WSM_Service::getStartType() throws WSM_RuntimeException
    it gets the start type */
PHP_METHOD(WSM_Service, getStartType)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    LPQUERY_SERVICE_CONFIG lpqscBuf = query_service_config(wsm->hService);
    if (lpqscBuf != NULL) {
        RETURN_LONG(lpqscBuf->dwStartType);
	} else {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
	}
}
/* }}} */

/* {{{ proto WSM_Service::setStartupType() 
    it sets the start_type */
PHP_METHOD(WSM_Service, setStartType)
{
    long start_type;
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &start_type) == FAILURE) {
        RETURN_NULL();
    }
    wsm->start_type = start_type;
}
/* }}} */

/* {{{ proto bool WSM_Service::change() throws WSM_RuntimeException
    it changes the service */
PHP_METHOD(WSM_Service, change)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

    // Make the changes. 
    if (! ChangeServiceConfig( 
            wsm->hService,     // handle of service 
            SERVICE_NO_CHANGE, // service type: no change 
            wsm->start_type,   // change service start type 
            SERVICE_NO_CHANGE, // error control: no change 
            NULL,              // binary path: no change 
            NULL,              // load order group: no change 
            NULL,              // tag ID: no change 
            NULL,              // dependencies: no change 
            NULL,              // account name: no change 
            NULL,              // password: no change 
            NULL) )            // display name: no change
    {
        zend_throw_exception_ex(wsm_runtimeexception_class_entry, 0 TSRMLS_CC, get_last_error());
    } else {
        RETURN_TRUE;
    }
}
/* }}} */

/* {{{ proto string WSM_Service::getDisplayName() */
PHP_METHOD(WSM_Service, getDisplayName)
{
    char szNameBuff[256];
	DWORD lenNameBuff = 256;
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    GetServiceDisplayName(wsm->hService, wsm->service_name, szNameBuff, &lenNameBuff);
    RETURN_STRING(szNameBuff, 1);
}
/* }}} */

/* {{{ proto string WSM_Service::getServiceName() 
    it gets the service name */
PHP_METHOD(WSM_Service, getServiceName)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_STRING(wsm->service_name, 1);
}
/* }}} */

/* {{{ WSM_Service::getMachineName() it gets the machine name 
When the machine name value was not passed in the constructor, 
we return the current computer name insteard of NULL
*/
PHP_METHOD(WSM_Service, getMachineName)
{
    wsm_service_object *wsm = (wsm_service_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

    if (!wsm->machine) {
        // 32768 is the buffer size.
        TCHAR  infoBuf[32768];
        DWORD  bufCharCount = 32768;
        bufCharCount = 32768;
        GetComputerName( infoBuf, &bufCharCount );
        RETURN_STRING(infoBuf, 1);
    } else {
        RETURN_STRING(wsm->machine, 1);
    }
}
/* }}} */

/* {{{ PHP_MINIT */
PHP_MINIT_FUNCTION(wsm)
{
    zend_class_entry ce;
    /* class WSM_Service */
    INIT_CLASS_ENTRY(ce, "WSM_Service", wsm_service_methods);
    ce.create_object = wsm_service_object_create;
    wsm_service_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
    memcpy(&wsm_service_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

    /* class WSM_Exception extends Exception */
    INIT_CLASS_ENTRY(ce, "WSM_Exception", wsm_exception_methods);
    wsm_exception_class_entry = zend_register_internal_class_ex(
        &ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);

    /* class WSM_RuntimeException extends WSM_Exception */
    INIT_CLASS_ENTRY(ce, "WSM_RuntimeException", wsm_exception_methods);
    wsm_runtimeexception_class_entry = zend_register_internal_class_ex(
        &ce, wsm_exception_class_entry, NULL TSRMLS_CC);

    /* {{{ Constants */
    /* Automatic/Manual/Disabled */
    REGISTER_LONG_CONSTANT("SERVICE_AUTO_START",   SERVICE_AUTO_START,   CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_DEMAND_START", SERVICE_DEMAND_START, CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_DISABLED",     SERVICE_DISABLED,     CONST_CS|CONST_PERSISTENT);
    /* Service Status */
    REGISTER_LONG_CONSTANT("SERVICE_CONTINUE_PENDING", SERVICE_CONTINUE_PENDING, CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_PAUSE_PENDING",    SERVICE_PAUSE_PENDING,    CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_PAUSED",           SERVICE_PAUSED,           CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_RUNNING",          SERVICE_RUNNING,          CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_START_PENDING",    SERVICE_START_PENDING,    CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_STOP_PENDING",     SERVICE_STOP_PENDING,     CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SERVICE_STOPPED",          SERVICE_STOPPED,          CONST_CS|CONST_PERSISTENT);

    REGISTER_STRING_CONSTANT("WSM_VERSION",   PHP_WSM_VERSION,   CONST_CS|CONST_PERSISTENT);
    /* }}} */
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN */
PHP_MSHUTDOWN_FUNCTION(wsm)
{
    return SUCCESS;
}
/* }}} */

/* {{{ php info table */
PHP_MINFO_FUNCTION(wsm)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "Windows Service Management", "enabled");
    php_info_print_table_row(2, "Version", PHP_WSM_VERSION);
    php_info_print_table_row(2, "Build", __DATE__" - "__TIME__);
    php_info_print_table_end();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
