<?php
	
    /**
     * This file is part on WSM php extension samples.
     * It shows how to install/remove a Windows Service using the WSM php extension.
     *
     * wsm.exe is the sample service that we want to install and is the result of
     * exaples\service\service.cpp compilation.
     * 
     * $Id: create_remove_service.php,v 1.2 2005/08/25 10:00:00 aurelian Exp $ 
     */    
    
    function usage($code=0) {
        echo __FILE__ . "\n\n";
        echo "Usage: \n\tphp " . __FILE__ . " delete --> removes the service\n\tphp " . __FILE__ . " create --> install the service\n";
        exit($code);
    }
    
	echo "WSM Version: " . WSM_VERSION . "\n";
	
    if ($argc != 2) {
        usage(-1);
    }
    
    if ($argv[1] == 'delete') {
    	$service_name= 'wsm';
        try {
    	    $service = new WSM_Service($service_name);
            $service->delete();
        } catch (WSM_RuntimeException $wsm_rEx) {
            exit($wsm_rEx->getMessage());
        }
        echo "Service removed...\n";
    } elseif($argv[1] == 'create') {
        /*
        WSM_Service WSM_Service::create(
                        string $service_name, 
                        string $path, 
                        string $param, 
                        long $start_type, 
                        string $display_name)
        */
        try {
            $service = WSM_Service::create(
                                        'wsm', 
                                        dirname(__FILE__) . '\wsm.exe', 
                                        NULL, 
                                        SERVICE_DEMAND_START, 
                                        'Hello WSM Service');
            echo "Service installed...\n";
        } catch (WSM_RuntimeException $wsm_rEx) {
            exit ($wsm_rEx->getMessage());
        }
    } else {
        usage(-1);
    }
