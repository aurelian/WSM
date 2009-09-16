<?php
    /**
     * This file is part on WSM php extension samples.
     * $Id: stop.php,v 1.2 2005/08/25 09:22:47 aurelian Exp $ 
     */
    echo "===> Stopping A Windows Service with WSM.\n";

    if ($service->status() == SERVICE_STOPPED) {
        echo "The requested service is not started.\n";
        exit(-1);
    }
    
    try {
        $service->stop();
    } catch (WSM_RuntimeException $wsm_rEx) {
        exit($wsm_rEx->getMessage());
    }
    
    echo "The $s service is stopping.";
    $i=0;
    while ($service->status() != SERVICE_STOPPED) {
        sleep(2); 
        echo "."; 
        $i++;
        if ($i > 5) {
            exit(status($service->status()));
        }
    }
    
    echo "\nThe $s service was stopped successfully.\n";
    
