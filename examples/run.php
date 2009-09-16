<?php
    /* $Id: run.php,v 1.1 2005/08/14 16:45:23 aurelian Exp $ */
    include_once('functions.inc');
    
    echo "WSM Version: " . WSM_VERSION . "\n";

    $s= 'FastWork2-MySQL4';
    $service = new WSM_Service($s);
    
    if ($service->status() == SERVICE_STOPPED) {
        include_once('start.php');
    } else {
        include_once('stop.php');
    }
    
