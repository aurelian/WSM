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
   | Author: Oancea Aurelian <aurelian@locknet.ro>                        |
   +----------------------------------------------------------------------+
*/
/* $Id: php_wsm.h,v 1.8 2005/08/25 10:09:46 aurelian Exp $ */

#ifndef PHP_WSM_H
#define PHP_WSM_H 1

#include "php.h"

extern zend_module_entry wsm_module_entry;
#define phpext_wsm_ptr &wsm_module_entry


#ifdef PHP_WIN32
#    if defined (PHP_EXPORTS)
#        define PHP_WSM_API __declspec(dllexport)
#    elif defined(COMPILE_DL_WSM)
#        define PHP_WSM_API __declspec(dllimport)
#    else
#        define PHP_WSM_API
#    endif
#else
#define PHP_WSM_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_WSM_VERSION "0.9.1"
#define PHP_WSM_EXTNAME "wsm"

PHP_MINIT_FUNCTION(wsm);
PHP_MINFO_FUNCTION(wsm);
PHP_MSHUTDOWN_FUNCTION(wsm);

PHP_METHOD(WSM_Service, __construct);
PHP_METHOD(WSM_Service, start);
PHP_METHOD(WSM_Service, stop);
PHP_METHOD(WSM_Service, create);
PHP_METHOD(WSM_Service, delete);
PHP_METHOD(WSM_Service, status);
PHP_METHOD(WSM_Service, getBinaryPath);
PHP_METHOD(WSM_Service, getDependencies);
PHP_METHOD(WSM_Service, getStartName);
PHP_METHOD(WSM_Service, change);
PHP_METHOD(WSM_Service, setStartType);
PHP_METHOD(WSM_Service, getStartType);
PHP_METHOD(WSM_Service, getDisplayName);
PHP_METHOD(WSM_Service, getServiceName);
PHP_METHOD(WSM_Service, getMachineName);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
