<?php
    /**
     * This file is part on WSM php extension samples.
     * $Id: start.php,v 1.2 2005/08/25 09:23:08 aurelian Exp $ 
     */
     
    echo "===> Starting A Windows Service with WSM.\n";
    if ($service->status() == SERVICE_RUNNING) {
        echo "The requested service has already been started.\n";
        exit(-1);
    }
    try {
        $service->start();
    } catch (WSM_RuntimeException $wsm_rEX) {
        exit($wsm_rEx->getMessage());
    }
    echo "The $s service is starting.";
    $i=0; // do not wait more than 10 seconds.
    while ($service->status() != SERVICE_RUNNING) {
        sleep(2); 
        echo "."; 
        $i++;
        if ( $i>5 ) {
            exit(status($service->status()));
        }
    }
    
    echo "\nThe $s service was started successfully.\n";
    
